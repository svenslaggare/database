#include "select_operation.h"
#include "../query_expressions/compiler.h"
#include "../helpers.h"
#include "../query_expressions/ir_optimizer.h"
#include "index_scanner.h"
#include "virtual_table.h"

#include <iostream>
#include <algorithm>

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
						auto& columnDefinition = this->mTable.underlying().schema().getDefinition(column);
						auto& resultStorage = this->mResult.columns[columnIndex];

						auto handleForType = [&](auto dummy) {
							using Type = decltype(dummy);
							resultStorage.getUnderlyingStorage<Type>() = this->mReducedProjections.getStorage(column)
								->storage()
								->getUnderlyingStorage<Type>();

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
	TreeIndexScanner treeIndexScanner;
	IndexScanContext context(mTable, mFilterExecutionEngine);

	auto possibleIndexScans = treeIndexScanner.findPossibleIndexScans(context);
	if (!possibleIndexScans.empty()) {
		auto& indexScan = possibleIndexScans[0];
		std::cout << "Using index: " << indexScan.index.column().name() << std::endl;

		treeIndexScanner.execute(context, indexScan, mWorkingStorage);
		mFilterExecutionEngine.makeCompareAlwaysTrue(indexScan.instructionIndex);
	} else {
		return false;
	}

	ExpressionIROptimizer optimizer(mFilterExecutionEngine);
	optimizer.optimize();

	// Change storage to the result of the index scan
	mTable.setStorage(mWorkingStorage);
	return true;
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
	if (!mOperation->order.empty) {
		mOrderResult = true;
		auto orderRootExpression = std::make_unique<QueryColumnReferenceExpression>(mOperation->order.name);
		mOrderExecutionEngine = std::make_unique<ExpressionExecutionEngine>(ExecutorHelpers::compile(
			mTableContainer,
			mOperation->table,
			orderRootExpression.get(),
			mDatabaseEngine.config()));
	}

	if (!mOperation->join.empty) {
		auto joinTable = mOperation->join.joinOnTable;
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
			mOrderExecutionEngine->expressionType(),
			mOrderingData,
			mOperation->order.descending,
			mResult);
	}
}