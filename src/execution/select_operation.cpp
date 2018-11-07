#include "select_operation.h"
#include "../query_expressions/compiler.h"

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

	executors.emplace_back([this]() {
		if (this->filterExecutionEngine.instructions().size() == 1 && !reducedColumns.columns.empty()) {
			auto firstInstruction = this->filterExecutionEngine.instructions().front().get();
			if (auto instruction = dynamic_cast<QueryValueExpressionIR*>(firstInstruction)) {
				auto value = instruction->value.getValue<bool>();
				if (value) {
					std::size_t columnIndex = 0;
					for (auto& column : this->reducedColumns.columns) {
						auto& columnDefinition = this->table.schema().getDefinition(column);
						auto& resultStorage = this->result.columns[columnIndex];

						auto handleForType = [&](auto dummy) {
							using Type = decltype(dummy);
							resultStorage.getUnderlyingStorage<Type>() = this->table.getColumnValues<Type>(column);
						};

						handleGenericType(columnDefinition.type, handleForType);
						columnIndex++;
					}
				}

				return true;
			}
		}

		return false;
	});

	if (optimize) {
		executors.emplace_back([this]() {
			if (this->filterExecutionEngine.instructions().size() == 1 && !reducedColumns.columns.empty()) {
				auto firstInstruction = this->filterExecutionEngine.instructions().front().get();

				return anyGenericType([&](auto dummy) {
					using Type = decltype(dummy);
					if (auto instruction = dynamic_cast<CompareExpressionLeftValueKnownTypeRightColumnIR<Type>*>(firstInstruction)) {
						auto& rhsColumn = *(this->filterExecutionEngine.getStorage(instruction->rhs));
						auto& rhsColumnValues = rhsColumn.template getUnderlyingStorage<Type>();

						for (std::size_t rowIndex = 0; rowIndex < rhsColumnValues.size(); rowIndex++) {
							if (QueryExpressionHelpers::compare<Type>(instruction->op, instruction->lhs, rhsColumnValues[rowIndex])) {
								ExecutorHelpers::addRowToResult(this->reducedColumns.storage, this->result, rowIndex);
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
			if (this->filterExecutionEngine.instructions().size() == 1 && !reducedColumns.columns.empty()) {
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
								ExecutorHelpers::addRowToResult(this->reducedColumns.storage, this->result, rowIndex);
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

void SelectOperationExecutor::execute() {
	if (optimize) {
		reducedColumns.tryReduce(operation->projections, table);
	}

	// First, try the optimized executors
	for (auto& executor : executors) {
		if (executor()) {
			return;
		}
	}

	// Else fallback to default executor
	if (reducedColumns.columns.empty()) {
		for (std::size_t rowIndex = 0; rowIndex < table.numRows(); rowIndex++) {
			filterExecutionEngine.execute(rowIndex);

			if (filterExecutionEngine.popEvaluation().getValue<bool>()) {
				ExecutorHelpers::addRowToResult(projectionExecutionEngines, result, rowIndex);
			}
		}
	} else {
		for (std::size_t rowIndex = 0; rowIndex < table.numRows(); rowIndex++) {
			filterExecutionEngine.execute(rowIndex);

			if (filterExecutionEngine.popEvaluation().getValue<bool>()) {
				ExecutorHelpers::addRowToResult(reducedColumns.storage, result, rowIndex);
			}
		}
	}
}