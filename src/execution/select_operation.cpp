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
	: table(table),
	  operation(operation),
	  projectionExecutionEngines(projectionExecutionEngines),
	  filterExecutionEngine(filterExecutionEngine),
	  result(result),
	  optimize(optimize) {
	if (optimize) {
		executors.emplace_back([this]() {
			if (this->filterExecutionEngine.instructions().size() == 1 && reducedProjections.allReduced) {
				auto firstInstruction = this->filterExecutionEngine.instructions().front().get();
				if (auto instruction = dynamic_cast<QueryValueExpressionIR*>(firstInstruction)) {
					auto value = instruction->value.getValue<bool>();
					if (value) {
						auto reducedIndex = this->reducedProjections.indexOfColumn(this->operation->order.name);
						if (reducedIndex != -1 || !doOrdering) {
							std::size_t columnIndex = 0;
							for (auto& column : this->reducedProjections.columns) {
								auto& columnDefinition = this->table.schema().getDefinition(column);
								auto& resultStorage = this->result.columns[columnIndex];

								auto handleForType = [&](auto dummy) {
									using Type = decltype(dummy);
									resultStorage.getUnderlyingStorage<Type>() = this->table.getColumnValues<Type>(column);

									if (doOrdering) {
										if ((std::size_t)reducedIndex == columnIndex) {
											for (auto currentValue : resultStorage.getUnderlyingStorage<Type>()) {
												orderingData.push_back(QueryValue(currentValue).data);
											}
										}
									}
								};

								handleGenericType(columnDefinition.type, handleForType);
								columnIndex++;
							}
						} else {
							for (std::size_t rowIndex = 0; rowIndex < this->table.numRows(); rowIndex++) {
								ExecutorHelpers::addRowToResult(this->reducedProjections.storage, this->result, rowIndex);
								addForOrdering(rowIndex);
							}
						}
					}

					return true;
				}
			}

			return false;
		});

		executors.emplace_back([this]() {
			if (this->filterExecutionEngine.instructions().size() == 1 && reducedProjections.allReduced) {
				auto firstInstruction = this->filterExecutionEngine.instructions().front().get();

				return anyGenericType([&](auto dummy) {
					using Type = decltype(dummy);
					if (auto instruction = dynamic_cast<CompareExpressionLeftValueKnownTypeRightColumnIR<Type>*>(firstInstruction)) {
						auto& rhsColumn = *(this->filterExecutionEngine.getStorage(instruction->rhs));
						auto& rhsColumnValues = rhsColumn.template getUnderlyingStorage<Type>();

						for (std::size_t rowIndex = 0; rowIndex < rhsColumnValues.size(); rowIndex++) {
							if (QueryExpressionHelpers::compare<Type>(instruction->op, instruction->lhs, rhsColumnValues[rowIndex])) {
								ExecutorHelpers::addRowToResult(this->reducedProjections.storage, this->result, rowIndex);
							}

							if (doOrdering) {
								addForOrdering(rowIndex);
							}
						}

						return true;
					}

					return false;
				});
			}

			return false;
		});

		executors.emplace_back([this]() {
			if (this->filterExecutionEngine.instructions().size() == 1 && reducedProjections.allReduced) {
				auto firstInstruction = this->filterExecutionEngine.instructions().front().get();

				if (auto instruction = dynamic_cast<CompareExpressionLeftColumnRightColumnIR*>(firstInstruction)) {
					auto& lhsColumn = *this->filterExecutionEngine.getStorage(instruction->lhs);
					auto& rhsColumn = *this->filterExecutionEngine.getStorage(instruction->rhs);

					auto handleForType = [&](auto dummy) {
						using Type = decltype(dummy);
						auto& lhsColumnValues = lhsColumn.getUnderlyingStorage<Type>();
						auto& rhsColumnValues = rhsColumn.getUnderlyingStorage<Type>();

						for (std::size_t rowIndex = 0; rowIndex < lhsColumnValues.size(); rowIndex++) {
							if (QueryExpressionHelpers::compare<Type>(instruction->op, lhsColumnValues[rowIndex], rhsColumnValues[rowIndex])) {
								ExecutorHelpers::addRowToResult(this->reducedProjections.storage, this->result, rowIndex);

								if (doOrdering) {
									addForOrdering(rowIndex);
								}
							}
						}
					};

					handleGenericType(lhsColumn.type, handleForType);
					return true;
				}
			}

			return false;
		});
	}
}

void SelectOperationExecutor::addForOrdering(std::size_t rowIndex) {
	orderExecutionEngine->execute(rowIndex);
	orderingData.push_back(orderExecutionEngine->popEvaluation().data);
}

void SelectOperationExecutor::execute() {
	if (!operation->order.name.empty()) {
		doOrdering = true;
		auto orderRootExpression = std::make_unique<QueryColumnReferenceExpression>(operation->order.name);
		orderExecutionEngine = std::make_unique<ExpressionExecutionEngine>(ExecutorHelpers::compile(table, orderRootExpression.get()));
	}

	reducedProjections.tryReduce(operation->projections, table);

	// First, try the optimized executors
	bool executed = false;
	for (auto& executor : executors) {
		if (executor()) {
			executed = true;
			break;
		}
	}

	// Else fallback to default executor
	if (!executed) {
		for (std::size_t rowIndex = 0; rowIndex < table.numRows(); rowIndex++) {
			filterExecutionEngine.execute(rowIndex);

			if (filterExecutionEngine.popEvaluation().getValue<bool>()) {
				ExecutorHelpers::addRowToResult(projectionExecutionEngines, reducedProjections.storage, result, rowIndex);

				if (doOrdering) {
					addForOrdering(rowIndex);
				}
			}
		}
	}

	if (doOrdering) {
		ExecutorHelpers::orderResult(orderExecutionEngine->expressionType(), orderingData, result);
	}
}