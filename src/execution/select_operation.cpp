#include "select_operation.h"
#include "../query_expressions/compiler.h"
#include "../helpers.h"

#include <iostream>
#include <algorithm>

SelectOperationExecutor::SelectOperationExecutor(Table& table,
												 QuerySelectOperation* operation,
												 std::vector<std::unique_ptr<ExpressionExecutionEngine>>& projectionExecutionEngines,
												 ExpressionExecutionEngine& filterExecutionEngine,
												 QueryResult& result,
												 bool optimize)
	: mTable(table),
	  mOperation(operation),
	  mProjectionExecutionEngines(projectionExecutionEngines),
	  mFilterExecutionEngine(filterExecutionEngine),
	  mResult(result),
	  mOptimize(optimize) {
	if (optimize) {
		mExecutors.emplace_back(std::bind(&SelectOperationExecutor::executeNoFilter, this));
	}

	mExecutors.emplace_back(std::bind(&SelectOperationExecutor::executeTreeIndexScan, this));

	if (optimize) {
		mExecutors.emplace_back(std::bind(&SelectOperationExecutor::executeFilterLeftIsColumn, this));
		mExecutors.emplace_back(std::bind(&SelectOperationExecutor::executeFilterRightIsColumn, this));
		mExecutors.emplace_back(std::bind(&SelectOperationExecutor::executeFilterBothColumn, this));
	}

	mExecutors.emplace_back(std::bind(&SelectOperationExecutor::executeDefault, this));
}

bool SelectOperationExecutor::hasReducedToOneInstruction() const {
	return this->mFilterExecutionEngine.instructions().size() == 1;
}

void SelectOperationExecutor::addForOrdering(std::size_t rowIndex) {
	mOrderExecutionEngine->execute(rowIndex);
	mOrderingData.push_back(mOrderExecutionEngine->popEvaluation().data);
}

bool SelectOperationExecutor::executeNoFilter() {
	if (hasReducedToOneInstruction() && mReducedProjections.allReduced) {
		auto firstInstruction = this->mFilterExecutionEngine.instructions().front().get();
		if (auto instruction = dynamic_cast<QueryValueExpressionIR*>(firstInstruction)) {
			auto value = instruction->value.getValue<bool>();
			if (value) {
				auto reducedIndex = this->mReducedProjections.indexOfColumn(this->mOperation->order.name);
				if (reducedIndex != -1 || !mOrderResult) {
					std::size_t columnIndex = 0;
					for (auto& column : this->mReducedProjections.columns) {
						auto& columnDefinition = this->mTable.schema().getDefinition(column);
						auto& resultStorage = this->mResult.columns[columnIndex];

						auto handleForType = [&](auto dummy) {
							using Type = decltype(dummy);
							resultStorage.getUnderlyingStorage<Type>() = this->mTable.getColumnValues<Type>(column);

							if (mOrderResult) {
								if ((std::size_t)reducedIndex == columnIndex) {
									for (auto currentValue : resultStorage.getUnderlyingStorage<Type>()) {
										mOrderingData.push_back(QueryValue(currentValue).data);
									}
								}
							}
						};

						handleGenericType(columnDefinition.type(), handleForType);
						columnIndex++;
					}
				} else {
					for (std::size_t rowIndex = 0; rowIndex < this->mTable.numRows(); rowIndex++) {
						ExecutorHelpers::addRowToResult(this->mReducedProjections.storage, this->mResult, rowIndex);
						addForOrdering(rowIndex);
					}
				}
			}

			return true;
		}
	}

	return false;
}



bool SelectOperationExecutor::executeFilterLeftIsColumn() {
	if (hasReducedToOneInstruction() && mReducedProjections.allReduced) {
		auto firstInstruction = this->mFilterExecutionEngine.instructions().front().get();

		return anyGenericType([&](auto dummy) {
			using Type = decltype(dummy);
			if (auto instruction = dynamic_cast<CompareExpressionLeftColumnRightValueKnownTypeIR<Type>*>(firstInstruction)) {
				auto& lhsColumn = *(this->mFilterExecutionEngine.getStorage(instruction->lhs));
				auto& lhsColumnValues = lhsColumn.template getUnderlyingStorage<Type>();

				for (std::size_t rowIndex = 0; rowIndex < lhsColumnValues.size(); rowIndex++) {
					if (QueryExpressionHelpers::compare<Type>(instruction->op, lhsColumnValues[rowIndex], instruction->rhs)) {
						ExecutorHelpers::addRowToResult(this->mReducedProjections.storage, this->mResult, rowIndex);
					}

					if (mOrderResult) {
						addForOrdering(rowIndex);
					}
				}

				return true;
			}

			return false;
		});
	}

	return false;
}

bool SelectOperationExecutor::executeFilterRightIsColumn() {
	if (hasReducedToOneInstruction() && mReducedProjections.allReduced) {
		auto firstInstruction = this->mFilterExecutionEngine.instructions().front().get();

		return anyGenericType([&](auto dummy) {
			using Type = decltype(dummy);
			if (auto instruction = dynamic_cast<CompareExpressionLeftValueKnownTypeRightColumnIR<Type>*>(firstInstruction)) {
				auto& rhsColumn = *(this->mFilterExecutionEngine.getStorage(instruction->rhs));
				auto& rhsColumnValues = rhsColumn.template getUnderlyingStorage<Type>();

				for (std::size_t rowIndex = 0; rowIndex < rhsColumnValues.size(); rowIndex++) {
					if (QueryExpressionHelpers::compare<Type>(instruction->op, instruction->lhs, rhsColumnValues[rowIndex])) {
						ExecutorHelpers::addRowToResult(this->mReducedProjections.storage, this->mResult, rowIndex);
					}

					if (mOrderResult) {
						addForOrdering(rowIndex);
					}
				}

				return true;
			}

			return false;
		});
	}

	return false;
}

bool SelectOperationExecutor::executeFilterBothColumn() {
	if (hasReducedToOneInstruction() && mReducedProjections.allReduced) {
		auto firstInstruction = this->mFilterExecutionEngine.instructions().front().get();

		if (auto instruction = dynamic_cast<CompareExpressionLeftColumnRightColumnIR*>(firstInstruction)) {
			auto& lhsColumn = *this->mFilterExecutionEngine.getStorage(instruction->lhs);
			auto& rhsColumn = *this->mFilterExecutionEngine.getStorage(instruction->rhs);

			auto handleForType = [&](auto dummy) {
				using Type = decltype(dummy);
				auto& lhsColumnValues = lhsColumn.getUnderlyingStorage<Type>();
				auto& rhsColumnValues = rhsColumn.getUnderlyingStorage<Type>();

				for (std::size_t rowIndex = 0; rowIndex < lhsColumnValues.size(); rowIndex++) {
					if (QueryExpressionHelpers::compare<Type>(instruction->op, lhsColumnValues[rowIndex], rhsColumnValues[rowIndex])) {
						ExecutorHelpers::addRowToResult(this->mReducedProjections.storage, this->mResult, rowIndex);

						if (mOrderResult) {
							addForOrdering(rowIndex);
						}
					}
				}
			};

			handleGenericType(lhsColumn.type(), handleForType);
			return true;
		}
	}

	return false;
}

namespace {
	template<typename T>
	using TreeIndexIterator = typename TreeIndex::UnderlyingStorage<T>::const_iterator;

	template<typename T>
	std::pair<TreeIndexIterator<T>, TreeIndexIterator<T>> findTreeIndexIterators(const TreeIndex::UnderlyingStorage<T>& underlyingIndex,
																				 CompareOperator op,
																				 const T& indexSearchValue) {
		auto startIterator = underlyingIndex.end();
		auto endIterator = underlyingIndex.end();

		switch (op) {
			case CompareOperator::Equal:
				startIterator = underlyingIndex.lower_bound(indexSearchValue);
				endIterator = underlyingIndex.upper_bound(indexSearchValue);

				if (endIterator == underlyingIndex.end()) {
					startIterator = underlyingIndex.end();
					endIterator = underlyingIndex.end();
				}
				break;
			case CompareOperator::NotEqual:
				break;
			case CompareOperator::LessThan: {
				startIterator = underlyingIndex.begin();
				endIterator = underlyingIndex.upper_bound(indexSearchValue);

				if (endIterator != underlyingIndex.end()) {
					--endIterator;
				} else {
					startIterator = underlyingIndex.end();
					endIterator = underlyingIndex.end();
				}
				break;
			}
			case CompareOperator::LessThanOrEqual:
				startIterator = underlyingIndex.begin();
				endIterator = underlyingIndex.upper_bound(indexSearchValue);

				if (endIterator == underlyingIndex.end()) {
					startIterator = underlyingIndex.end();
					endIterator = underlyingIndex.end();
				}
				break;
			case CompareOperator::GreaterThan:
				startIterator = underlyingIndex.upper_bound(indexSearchValue);
				endIterator = underlyingIndex.end();

				if (startIterator == underlyingIndex.end()) {
					startIterator = underlyingIndex.end();
					endIterator = underlyingIndex.end();
				}
				break;
			case CompareOperator::GreaterThanOrEqual:
				startIterator = underlyingIndex.upper_bound(indexSearchValue);
				endIterator = underlyingIndex.end();

				if (startIterator != underlyingIndex.end()) {
					--startIterator;
				} else {
					startIterator = underlyingIndex.end();
					endIterator = underlyingIndex.end();
				}
				break;
		}

		return std::make_pair(startIterator, endIterator);
	}

	CompareOperator otherSideCompareOp(CompareOperator op) {
		switch (op) {
			case CompareOperator::Equal:
				return CompareOperator::Equal;
			case CompareOperator::NotEqual:
				return CompareOperator::NotEqual;
			case CompareOperator::LessThan:
				return CompareOperator::GreaterThan;
			case CompareOperator::LessThanOrEqual:
				return CompareOperator::GreaterThanOrEqual;
			case CompareOperator::GreaterThan:
				return CompareOperator::LessThan;
			case CompareOperator::GreaterThanOrEqual:
				return CompareOperator::LessThanOrEqual;
		}
	}

	bool canTreeIndexScan(const TreeIndex& index, const std::string& column, CompareOperator op) {
		return index.column().name() != column || op == CompareOperator::NotEqual;
	}

	template<typename T>
	bool treeIndexScan(const Table& table,
					   ExpressionExecutionEngine& filterExecutionEngine,
					   std::size_t columnSlot,
					   CompareOperator op,
					   T indexSearchValue,
					   std::vector<ColumnStorage>& workingStorage) {
		for (auto& index : table.indices()) {
			if (canTreeIndexScan(*index, filterExecutionEngine.fromSlot(columnSlot), op)) {
				return false;
			}

			std::vector<const ColumnStorage*> columnsStorage;
			for (auto& column : table.schema().columns()) {
				workingStorage.emplace_back(column);
				columnsStorage.push_back(&table.getColumn(column.name()));
			}

			using Type = decltype(indexSearchValue);

			auto& underlyingIndex = index->getUnderlyingStorage<Type>();
			auto iteratorRange = findTreeIndexIterators(underlyingIndex, op, indexSearchValue);

			for (auto it = iteratorRange.first; it != iteratorRange.second; ++it) {
				auto rowIndex = it->second;

				for (std::size_t columnIndex = 0; columnIndex < table.schema().columns().size(); columnIndex++) {
					auto& columnStorage = columnsStorage[columnIndex];
					auto& resultStorage = workingStorage[columnIndex];

					auto handleForType = [&](auto dummy) {
						using Type = decltype(dummy);
						resultStorage.getUnderlyingStorage<Type>().push_back(columnStorage->getUnderlyingStorage<Type>()[rowIndex]);
					};

					handleGenericType(columnStorage->type(), handleForType);
				}
			}

			return true;
		}

		return false;
	}
}

void SelectOperationExecutor::updateSlotsStorage(std::vector<ColumnStorage>& newStorage, ExpressionExecutionEngine& executionEngine) {
	std::vector<ColumnStorage*> workingSlots;
	for (std::size_t slot = 0; slot < executionEngine.numSlots(); slot++) {
		auto column = executionEngine.fromSlot(slot);
		workingSlots.push_back(&newStorage.at(mTable.schema().getDefinition(column).index()));
	}

	executionEngine.setSlotStorage(std::move(workingSlots));
}

void SelectOperationExecutor::updateAllSlotsStorage(std::vector<ColumnStorage>& newStorage) {
	updateSlotsStorage(newStorage, mFilterExecutionEngine);
	for (auto& projection : mProjectionExecutionEngines) {
		updateSlotsStorage(newStorage, *projection);
	}

	if (mOrderExecutionEngine) {
		updateSlotsStorage(newStorage, *mOrderExecutionEngine);
	}
}

void SelectOperationExecutor::executeSequentialScan(std::size_t numRows) {
	for (std::size_t rowIndex = 0; rowIndex < numRows; rowIndex++) {
		mFilterExecutionEngine.execute(rowIndex);

		if (mFilterExecutionEngine.popEvaluation().getValue<bool>()) {
			ExecutorHelpers::addRowToResult(mProjectionExecutionEngines, mReducedProjections.storage, mResult, rowIndex);

			if (mOrderResult) {
				addForOrdering(rowIndex);
			}
		}
	}
}

bool SelectOperationExecutor::executeTreeIndexScan() {
	std::vector<ColumnStorage> workingStorage;

	auto& instructions = mFilterExecutionEngine.instructions();
	std::int64_t indexedInstruction = -1;
	for (std::size_t i = 0; i < instructions.size(); i++) {
		auto instruction = instructions[i].get();
		auto indexed = anyGenericType([&](auto dummy) {
			using Type = decltype(dummy);

			if (auto compareInstruction = dynamic_cast<CompareExpressionLeftColumnRightValueKnownTypeIR<Type>*>(instruction)) {
				return treeIndexScan(
					mTable,
					mFilterExecutionEngine,
					compareInstruction->lhs,
					compareInstruction->op,
					compareInstruction->rhs,
					workingStorage);
			} else if (auto compareInstruction = dynamic_cast<CompareExpressionLeftValueKnownTypeRightColumnIR<Type>*>(instruction)) {
				return treeIndexScan(
					mTable,
					mFilterExecutionEngine,
					compareInstruction->rhs,
					otherSideCompareOp(compareInstruction->op),
					compareInstruction->lhs,
					workingStorage);
			}

			return false;
		});

		if (indexed) {
			indexedInstruction = i;
			break;
		}
	}

	if (indexedInstruction != -1) {
		mFilterExecutionEngine.replaceInstruction(
			(std::size_t)indexedInstruction,
			std::make_unique<QueryValueExpressionIR>(QueryValue(true)));
	} else {
		return false;
	}

	updateAllSlotsStorage(workingStorage);

	mReducedProjections = {};
	mReducedProjections.tryReduce(
		mOperation->projections,
		[&](const std::string& column) {
			return &workingStorage[mTable.schema().getDefinition(column).index()];
		});

	executeSequentialScan(workingStorage[0].size());
	return true;

//	auto treeIndexSearch = [&](std::size_t columnSlot, CompareOperator op, auto indexSearchValue) -> bool {
//		if (mTable.primaryIndex().column().name() != mFilterExecutionEngine.fromSlot(columnSlot) || op == CompareOperator::NotEqual) {
//			return false;
//		}
//
//		using Type = decltype(indexSearchValue);
//
//		auto& underlyingIndex = mTable.primaryIndex().getUnderlyingStorage<Type>();
//		auto iteratorRange = findTreeIndexIterators(underlyingIndex, op, indexSearchValue);
//
//		for (auto it = iteratorRange.first; it != iteratorRange.second; ++it) {
//			auto rowIndex = it->second;
////			std::cout << rowIndex << std::endl;
//			ExecutorHelpers::addRowToResult(
//				mProjectionExecutionEngines,
//				mReducedProjections.storage,
//				mResult,
//				rowIndex);
//
//			if (mOrderResult) {
//				this->addForOrdering(rowIndex);
//			}
//		}
//
//		return true;
//	};
//
//	return anyGenericType([&](auto dummy) {
//		using Type = decltype(dummy);
//
//		if (hasReducedToOneInstruction()) {
//			auto firstInstruction = mFilterExecutionEngine.instructions().front().get();
//
//			if (auto compareInstruction = dynamic_cast<CompareExpressionLeftColumnRightValueKnownTypeIR<Type>*>(firstInstruction)) {
//				return treeIndexSearch(compareInstruction->lhs, compareInstruction->op, compareInstruction->rhs);
//			} else if (auto compareInstruction = dynamic_cast<CompareExpressionLeftValueKnownTypeRightColumnIR<Type>*>(firstInstruction)) {
//				return treeIndexSearch(compareInstruction->rhs, otherSideCompareOp(compareInstruction->op), compareInstruction->lhs);
//			}
//		}
//
//		return false;
//	});
}

bool SelectOperationExecutor::executeDefault() {
	executeSequentialScan(mTable.numRows());
	return true;
}

void SelectOperationExecutor::execute() {
	if (!mOperation->order.name.empty()) {
		mOrderResult = true;
		auto orderRootExpression = std::make_unique<QueryColumnReferenceExpression>(mOperation->order.name);
		mOrderExecutionEngine = std::make_unique<ExpressionExecutionEngine>(ExecutorHelpers::compile(mTable, orderRootExpression.get()));
	}

	mReducedProjections.tryReduce(mOperation->projections, mTable);

	bool executed = false;
	for (auto& executor : mExecutors) {
		if (executor()) {
			executed = true;
			break;
		}
	}

	if (!executed) {
		throw std::runtime_error("Operation not executed.");
	}

	if (mOrderResult) {
		ExecutorHelpers::orderResult(
			mOrderExecutionEngine->expressionType(),
			mOrderingData,
			mOperation->order.descending,
			mResult);
	}
}