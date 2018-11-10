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

bool SelectOperationExecutor::executeDefault() {
	for (std::size_t rowIndex = 0; rowIndex < mTable.numRows(); rowIndex++) {
		mFilterExecutionEngine.execute(rowIndex);

		if (mFilterExecutionEngine.popEvaluation().getValue<bool>()) {
			ExecutorHelpers::addRowToResult(mProjectionExecutionEngines, mReducedProjections.storage, mResult, rowIndex);

			if (mOrderResult) {
				addForOrdering(rowIndex);
			}
		}
	}

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