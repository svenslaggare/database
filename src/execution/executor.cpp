#include "executor.h"
#include "../table.h"
#include "../database_engine.h"
#include "../query_expressions/evaluator.h"
#include "../query_expressions/helpers.h"
#include "../query_expressions/compiler.h"

#include <iostream>

OperationExecutorVisitor::OperationExecutorVisitor(DatabaseEngine& databaseEngine, QueryResult& result)
	: databaseEngine(databaseEngine), result(result) {

}

namespace {
	struct SelectOperationExecutor {
		Table& table;
		std::vector<std::unique_ptr<ExpressionExecutionEngine>>& projectionExecutionEngines;
		ExpressionExecutionEngine& filterExecutionEngine;
		QuerySelectOperation* operation;
		QueryResult& result;
		bool optimize;

		ReducedColumns reducedColumns;

		std::vector<std::function<bool ()>> executors;

		SelectOperationExecutor(Table& table,
								std::vector<std::unique_ptr<ExpressionExecutionEngine>>& projectionExecutionEngines,
								ExpressionExecutionEngine& filterExecutionEngine,
								QuerySelectOperation* operation,
								QueryResult& result,
								bool optimize = true)
			: table(table),
			  projectionExecutionEngines(projectionExecutionEngines),
			  filterExecutionEngine(filterExecutionEngine),
			  operation(operation),
			  result(result),
			  optimize(optimize) {

			executors.emplace_back([this]() {
				if (this->filterExecutionEngine.instructions.size() == 1 && !reducedColumns.columns.empty()) {
					auto firstInstruction = this->filterExecutionEngine.instructions.front().get();
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
					if (this->filterExecutionEngine.instructions.size() == 1 && !reducedColumns.columns.empty()) {
						auto firstInstruction = this->filterExecutionEngine.instructions.front().get();

						return anyGenericType([&](auto dummy) {
							using Type = decltype(dummy);
							if (auto instruction = dynamic_cast<CompareExpressionLeftValueKnownTypeRightColumnIR<Type>*>(firstInstruction)) {
								auto& rhsColumn = *(this->filterExecutionEngine.slottedColumnStorage[instruction->rhs]);
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
					if (this->filterExecutionEngine.instructions.size() == 1 && !reducedColumns.columns.empty()) {
						auto firstInstruction = this->filterExecutionEngine.instructions.front().get();

						if (auto instruction = dynamic_cast<CompareExpressionLeftColumnRightColumnIR*>(firstInstruction)) {
							auto& lhsColumn = *this->filterExecutionEngine.slottedColumnStorage[instruction->lhs];
							auto& rhsColumn = *this->filterExecutionEngine.slottedColumnStorage[instruction->rhs];

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

		void execute() {
			if (optimize) {
				reducedColumns.tryReduce(operation->projections, table);
			}

			// First, try the optimized executors.
			for (auto& executor : executors) {
				if (executor()) {
					return;
				}
			}

			// Fallback to default executor
			if (reducedColumns.columns.empty()) {
				for (std::size_t rowIndex = 0; rowIndex < table.numRows(); rowIndex++) {
					filterExecutionEngine.currentRowIndex = rowIndex;

					for (auto& instruction : filterExecutionEngine.instructions) {
						instruction->execute(filterExecutionEngine);
					}

					if (filterExecutionEngine.evaluationStack.top().getValue<bool>()) {
						for (auto& projection : projectionExecutionEngines) {
							projection->currentRowIndex = rowIndex;
						}

						ExecutorHelpers::addRowToResult(projectionExecutionEngines, result);
					}

					filterExecutionEngine.evaluationStack.pop();
				}
			} else {
				for (std::size_t rowIndex = 0; rowIndex < table.numRows(); rowIndex++) {
					filterExecutionEngine.currentRowIndex = rowIndex;

					for (auto& instruction : filterExecutionEngine.instructions) {
						instruction->execute(filterExecutionEngine);
					}

					if (filterExecutionEngine.evaluationStack.top().getValue<bool>()) {
						ExecutorHelpers::addRowToResult(reducedColumns.storage, result, rowIndex);
					}

					filterExecutionEngine.evaluationStack.pop();
				}
			}
		}
	};
}

void OperationExecutorVisitor::visit(QuerySelectOperation* operation) {
	std::unique_ptr<QueryExpression> rootExpression;
	if (operation->filter) {
		rootExpression = std::make_unique<QueryRootExpression>(std::move(operation->filter));
	} else {
		rootExpression = std::make_unique<QueryValueExpression>(QueryValue(true));
	}

	auto& table = databaseEngine.getTable(operation->table);

	std::vector<std::unique_ptr<ExpressionExecutionEngine>> projectionExecutionEngines;
	for (auto& projection : operation->projections) {
		projectionExecutionEngines.emplace_back(std::make_unique<ExpressionExecutionEngine>(table));
		QueryExpressionCompilerVisitor expressionCompilerVisitor(table, *projectionExecutionEngines.back());
		expressionCompilerVisitor.compile(projection.get());

		result.columns.emplace_back(expressionCompilerVisitor.typeEvaluationStack.top());
	}

	ExpressionExecutionEngine filterExecutionEngine(table);
	QueryExpressionCompilerVisitor expressionCompilerVisitor(table, filterExecutionEngine);
	expressionCompilerVisitor.compile(rootExpression.get());

	SelectOperationExecutor executor(
		table,
		projectionExecutionEngines,
		filterExecutionEngine,
		operation,
		result,
		false);

	executor.execute();
}

void OperationExecutorVisitor::visit(QueryInsertOperation* operation) {
	auto& table = databaseEngine.getTable(operation->table);
	std::size_t columnIndex = 0;
	if (operation->columns.size() != table.schema().columns.size()) {
		throw std::runtime_error("Wrong number of columns.");
	}

	for (auto& column : operation->columns) {
		auto& columnDefinition = table.schema().getDefinition(column);

		auto insertForType = [&](auto dummy) {
			using ColumnType = decltype(dummy);

			for (auto& columnValues : operation->values) {
				auto& value = columnValues[columnIndex];
				if (value.type != columnDefinition.type) {
					throw std::runtime_error("Wrong type.");
				}

				table.insertColumn(column, value.getValue<ColumnType>());
			}
		};

		handleGenericType(columnDefinition.type, insertForType);
		columnIndex++;
	}
}
