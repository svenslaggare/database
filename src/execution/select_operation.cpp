#include "select_operation.h"
#include "../query_expressions/compiler.h"
#include "../helpers.h"
#include "../query_expressions/ir_optimizer.h"
#include "index_scanner.h"
#include "virtual_table.h"

#include <iostream>
#include <algorithm>
#include <assert.h>

SelectOperationExecutor::SelectOperationExecutor(DatabaseEngine& databaseEngine,
												 VirtualTableContainer& tableContainer,
												 QuerySelectOperation* operation,
												 std::vector<std::unique_ptr<ExpressionExecutionEngine>>& projectionExecutionEngines,
												 ExpressionExecutionEngine& filterExecutionEngine,
												 QueryResult& result)
	: mDatabaseEngine(databaseEngine),
	  mTableContainer(tableContainer),
	  mTable(tableContainer.getTable(operation->table)),
	  mOperation(operation),
	  mProjectionExecutionEngines(projectionExecutionEngines),
	  mFilterExecutionEngine(filterExecutionEngine),
	  mResult(result),
	  mReducedProjections(operation->table) {
	if (databaseEngine.config().optimizeExecution) {
		mExecutors.emplace_back(std::bind(&SelectOperationExecutor::executeNoFilter, this));
		mExecutors.emplace_back(std::bind(&SelectOperationExecutor::executeFilterLeftIsColumn, this));
		mExecutors.emplace_back(std::bind(&SelectOperationExecutor::executeFilterRightIsColumn, this));
		mExecutors.emplace_back(std::bind(&SelectOperationExecutor::executeFilterBothColumn, this));
	}

	mExecutors.emplace_back(std::bind(&SelectOperationExecutor::executeDefault, this));
}

std::vector<ColumnStorage>& SelectOperationExecutor::getWorkingStorage(const std::string& tableName) {
	auto workingStorageIterator = mWorkingStorage.find(tableName);
	if (workingStorageIterator != mWorkingStorage.end()) {
		return *workingStorageIterator->second;
	}

	auto workingStorage = std::make_unique<std::vector<ColumnStorage>>();
	auto workingStoragePtr = workingStorage.get();
	mWorkingStorage[tableName] = std::move(workingStorage);
	return *workingStoragePtr;
}


bool SelectOperationExecutor::hasReducedToOneInstruction() const {
	return this->mFilterExecutionEngine.instructions().size() == 1;
}

void SelectOperationExecutor::addForOrdering(std::size_t rowIndex) {
	mOrderExecutionEngine->execute(rowIndex);
	for (auto& columnOrdering : mOrderingData) {
		columnOrdering.push_back(mOrderExecutionEngine->popEvaluation().data);
	}

	assert(mOrderExecutionEngine->evaluationStackSize() == 0);
}

bool SelectOperationExecutor::executeNoFilter() {
	if (hasReducedToOneInstruction() && mReducedProjections.allReduced) {
		auto firstInstruction = this->mFilterExecutionEngine.instructions().front().get();
		if (auto instruction = dynamic_cast<QueryValueExpressionIR*>(firstInstruction)) {
			auto value = instruction->value.getValue<bool>();
			if (value) {
				std::int64_t reducedIndex = -1;
				if (this->mOperation->order.columns.size() == 1) {
					reducedIndex = this->mReducedProjections.indexOfColumn(this->mOperation->order.columns.front().name);
				}

				if (reducedIndex != -1 || !mOrderResult) {
					std::size_t columnIndex = 0;
					for (auto& column : this->mReducedProjections.columns) {
						auto columnParts = QueryExpressionHelpers::splitColumnName(column, mOperation->table);
						auto& columnDefinition = this->mTableContainer
							.getTable(columnParts.first).underlying()
							.getColumn(columnParts.second);

						auto& resultStorage = this->mResult.columns[columnIndex];

						auto handleForType = [&](auto dummy) {
							using Type = decltype(dummy);
							resultStorage.getUnderlyingStorage<Type>() = this->mReducedProjections.getStorage(column)
								->storage()
								->getUnderlyingStorage<Type>();

							if (mOrderResult) {
								if ((std::size_t)reducedIndex == columnIndex) {
									for (auto currentValue : resultStorage.getUnderlyingStorage<Type>()) {
										mOrderingData[0].push_back(QueryValue(currentValue).data);
									}
								}
							}
						};

						handleGenericType(columnDefinition.type(), handleForType);
						columnIndex++;
					}
				} else {
					for (std::size_t rowIndex = 0; rowIndex < mTable.numRows(); rowIndex++) {
						ExecutorHelpers::addRowToResult(this->mReducedProjections.storage, this->mResult, rowIndex);
						addForOrdering(rowIndex);
					}
				}
			}

			std::cout << "executed: executeNoFilter" << std::endl;
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
				auto& lhsColumn = *(this->mFilterExecutionEngine.columnFromSlot(instruction->lhs)->storage());
				auto& lhsColumnValues = lhsColumn.template getUnderlyingStorage<Type>();

				for (std::size_t rowIndex = 0; rowIndex < lhsColumnValues.size(); rowIndex++) {
					if (QueryExpressionHelpers::compare<Type>(instruction->op, lhsColumnValues[rowIndex], instruction->rhs)) {
						ExecutorHelpers::addRowToResult(this->mReducedProjections.storage, this->mResult, rowIndex);

						if (mOrderResult) {
							addForOrdering(rowIndex);
						}
					}
				}

				std::cout << "executed: executeFilterLeftIsColumn" << std::endl;
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

		auto handleForType = [&](auto dummy) {
			using Type = decltype(dummy);
			if (auto instruction = dynamic_cast<CompareExpressionLeftValueKnownTypeRightColumnIR<Type>*>(firstInstruction)) {
				auto& rhsColumn = *(this->mFilterExecutionEngine.columnFromSlot(instruction->rhs)->storage());
				auto& rhsColumnValues = rhsColumn.template getUnderlyingStorage<Type>();

				for (std::size_t rowIndex = 0; rowIndex < rhsColumnValues.size(); rowIndex++) {
					if (QueryExpressionHelpers::compare<Type>(instruction->op, instruction->lhs, rhsColumnValues[rowIndex])) {
						ExecutorHelpers::addRowToResult(this->mReducedProjections.storage, this->mResult, rowIndex);

						if (mOrderResult) {
							addForOrdering(rowIndex);
						}
					}
				}

				std::cout << "executed: executeFilterRightIsColumn" << std::endl;
				return true;
			}

			return false;
		};

		return anyGenericType(handleForType);
	}

	return false;
}

bool SelectOperationExecutor::executeFilterBothColumn() {
	if (hasReducedToOneInstruction() && mReducedProjections.allReduced) {
		auto firstInstruction = this->mFilterExecutionEngine.instructions().front().get();

		if (auto instruction = dynamic_cast<CompareExpressionLeftColumnRightColumnIR*>(firstInstruction)) {
			auto& lhsColumn = *this->mFilterExecutionEngine.columnFromSlot(instruction->lhs)->storage();
			auto& rhsColumn = *this->mFilterExecutionEngine.columnFromSlot(instruction->rhs)->storage();

			auto handleForType = [&](auto dummy) {
				using Type = decltype(dummy);
				auto& lhsColumnValues = lhsColumn.getUnderlyingStorage<Type>();
				auto& rhsColumnValues = rhsColumn.getUnderlyingStorage<Type>();

				for (std::size_t rowIndex = 0; rowIndex < lhsColumnValues.size(); rowIndex++) {
					auto lhsValue = lhsColumnValues[rowIndex];
					auto rhsValue = rhsColumnValues[rowIndex];

					if (QueryExpressionHelpers::compare<Type>(instruction->op, lhsValue, rhsValue)) {
						ExecutorHelpers::addRowToResult(this->mReducedProjections.storage, this->mResult, rowIndex);

						if (mOrderResult) {
							addForOrdering(rowIndex);
						}
					}
				}
			};

			handleGenericType(lhsColumn.type(), handleForType);
			std::cout << "executed: executeFilterBothColumn" << std::endl;
			return true;
		}
	}

	return false;
}

bool SelectOperationExecutor::tryExecuteTreeIndexScan() {
	// Don't try to use index if we have joined
	if (!mWorkingStorage.empty()) {
		return false;
	}


	auto& workingStorage = getWorkingStorage(mOperation->table);

	auto possibleIndexScans = mTreeIndexScanner.findPossibleIndexScans(mTable, mFilterExecutionEngine);
	if (!possibleIndexScans.empty()) {
		auto& indexScan = possibleIndexScans[0];
		std::cout << "Using index: " << indexScan.index.column().name() << std::endl;

		mTreeIndexScanner.execute(mTable, indexScan, workingStorage);
		mFilterExecutionEngine.makeCompareAlwaysTrue(indexScan.instructionIndex);
	} else {
		return false;
	}

	ExpressionIROptimizer optimizer(mFilterExecutionEngine);
	optimizer.optimize();

	// Change storage to the result of the index scan
	mTable.setStorage(workingStorage);
	return true;
}

void SelectOperationExecutor::joinTables() {
	auto& joinTable = mTableContainer.getTable(mOperation->join.joinOnTable);

	auto createColumnAccessExecution = [&](const std::string& table, const std::string& columnName) {
		auto accessExpression = std::make_unique<QueryColumnReferenceExpression>(columnName);
		return ExecutorHelpers::compile(
			mTableContainer,
			table,
			accessExpression.get(),
			mDatabaseEngine.config());
	};

	auto createWorkingStorageForTable = [&](VirtualTable& table) -> std::vector<ColumnStorage>& {
		auto& schema = table.underlying().schema();
		auto& workingStorage = getWorkingStorage(schema.name());
		for (auto& column : schema.columns()) {
			workingStorage.emplace_back(column);
		}

		return workingStorage;
	};

	auto joinOnExpressionEngine = createColumnAccessExecution(mOperation->join.joinOnTable, mOperation->join.joinOnColumn);
	auto& joinOnTableStorage = createWorkingStorageForTable(joinTable);

	auto joinFromExpressionEngine = createColumnAccessExecution(mOperation->table, mOperation->join.joinFromColumn);
	auto& joinFromTableStorage = createWorkingStorageForTable(mTable);

	auto findJoinIndex = [&](VirtualTable& table, const std::string& columnName) {
		auto fullColumnName = QueryExpressionHelpers::fullColumnName(
			table.underlying().schema().name(),
			columnName);
		TreeIndex* joinIndex = nullptr;

		for (auto& index : table.underlying().indices()) {
			if (index->columnName() == fullColumnName) {
				joinIndex = index.get();
			}
		}

		return joinIndex;
	};

	auto joinFromIndex = findJoinIndex(mTable, mOperation->join.joinFromColumn);
	auto joinOnIndex = findJoinIndex(joinTable, mOperation->join.joinOnColumn);

	auto indexJoin = [&](VirtualTable& nonIndexTable,
					     ExpressionExecutionEngine& nonIndexExecutionEngine,
					     std::vector<ColumnStorage>& nonIndexStorage,
					     VirtualTable& indexTable,
					     TreeIndex& index,
					     std::vector<ColumnStorage>& indexStorage) {
		for (std::size_t nonIndexRowIndex = 0; nonIndexRowIndex < nonIndexTable.numRows(); nonIndexRowIndex++) {
			nonIndexExecutionEngine.execute(nonIndexRowIndex);
			auto joinValue = nonIndexExecutionEngine.popEvaluation();

			PossibleIndexScan indexScan(0, index, CompareOperator::Equal, joinValue);
			mTreeIndexScanner.execute(
				indexTable,
				indexScan,
				[&](std::size_t columnIndex, const ColumnDefinition& columnDefinition) {},
				[&](std::size_t rowIndex, std::size_t columnIndex, const ColumnStorage* columnStorage, bool isFirstColumn) {
					ExecutorHelpers::addColumnToResult(*columnStorage, indexStorage[columnIndex], rowIndex);

					if (isFirstColumn) {
						ExecutorHelpers::copyRow(nonIndexTable, nonIndexStorage, nonIndexRowIndex);
					}
				});
		}
	};

	if (joinFromIndex != nullptr) {
		indexJoin(
			joinTable,
			joinOnExpressionEngine,
			joinOnTableStorage,
			mTable,
			*joinFromIndex,
			joinFromTableStorage);
	} else if (joinOnIndex != nullptr) {
		indexJoin(
			mTable,
			joinFromExpressionEngine,
			joinFromTableStorage,
			joinTable,
			*joinOnIndex,
			joinOnTableStorage);
	} else {
		for (std::size_t joinOnRowIndex = 0; joinOnRowIndex < joinTable.numRows(); joinOnRowIndex++) {
			joinOnExpressionEngine.execute(joinOnRowIndex);
			auto joinOnValue = joinOnExpressionEngine.popEvaluation();

			for (std::size_t joinFromRowIndex = 0; joinFromRowIndex < mTable.numRows(); joinFromRowIndex++) {
				joinFromExpressionEngine.execute(joinFromRowIndex);
				auto joinFromValue = joinFromExpressionEngine.popEvaluation();

				if (joinOnValue == joinFromValue) {
					ExecutorHelpers::copyRow(mTable, joinFromTableStorage, joinFromRowIndex);
					ExecutorHelpers::copyRow(joinTable, joinOnTableStorage, joinOnRowIndex);
				}
			}
		}
	}

	mTable.setStorage(joinFromTableStorage);
	joinTable.setStorage(joinOnTableStorage);
}

void SelectOperationExecutor::prepareOrdering() {
	mOrderResult = true;
	std::vector<std::unique_ptr<QueryExpression>> orderExpressions;
	for (auto& column : mOperation->order.columns) {
		orderExpressions.emplace_back(std::make_unique<QueryColumnReferenceExpression>(column.name));
		mOrderingData.emplace_back();
	}

	auto numReturnValues = orderExpressions.size();
	auto orderRootExpression = std::make_unique<QueryMultipleExpressions>(std::move(orderExpressions));

	mOrderExecutionEngine = std::make_unique<ExpressionExecutionEngine>(ExecutorHelpers::compile(
		mTableContainer,
		mOperation->table,
		orderRootExpression.get(),
		mDatabaseEngine.config(),
		numReturnValues));
}

bool SelectOperationExecutor::executeDefault() {
	ExecutorHelpers::forEachRowFiltered(
		mTable,
		mFilterExecutionEngine,
		[&](std::size_t rowIndex) {
			ExecutorHelpers::addRowToResult(
				mProjectionExecutionEngines,
				mReducedProjections,
				mResult,
				rowIndex);

			if (mOrderResult) {
				addForOrdering(rowIndex);
			}
		});

	return true;
}

void SelectOperationExecutor::execute() {
	if (!mOperation->join.empty) {
		joinTables();
	}

	if (!mOperation->order.empty()) {
		prepareOrdering();
	}

	mReducedProjections.tryReduce(mOperation->projections, mTableContainer);

	tryExecuteTreeIndexScan();

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
			mOrderExecutionEngine->expressionTypes(),
			mOperation->order.columns,
			mOrderingData,
			mResult);
	}
}