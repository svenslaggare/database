#include "executor.h"
#include "../table.h"
#include "../database_engine.h"
#include "../query_expressions/helpers.h"
#include "../query_expressions/compiler.h"
#include "select_operation.h"

#include <iostream>

OperationExecutorVisitor::OperationExecutorVisitor(DatabaseEngine& databaseEngine, QueryResult& result)
	: databaseEngine(databaseEngine), result(result) {

}

void OperationExecutorVisitor::visit(QuerySelectOperation* operation) {
	std::unique_ptr<QueryExpression> filterExpression;
	if (operation->filter) {
		filterExpression = std::make_unique<QueryRootExpression>(std::move(operation->filter));
	} else {
		filterExpression = std::make_unique<QueryValueExpression>(QueryValue(true));
	}

	VirtualTable virtualTable(databaseEngine.getTable(operation->table));

	auto filterExecutionEngine = ExecutorHelpers::compile(virtualTable, filterExpression.get());

	std::vector<std::unique_ptr<ExpressionExecutionEngine>> projectionExecutionEngines;
	for (auto& projection : operation->projections) {
		projectionExecutionEngines.emplace_back(std::make_unique<ExpressionExecutionEngine>(
			ExecutorHelpers::compile(virtualTable, projection.get())));

		result.columns.emplace_back(projectionExecutionEngines.back()->expressionType());
	}

	SelectOperationExecutor executor(
		virtualTable,
		operation,
		projectionExecutionEngines,
		filterExecutionEngine,
		result,
		true);

	executor.execute();
}

void OperationExecutorVisitor::visit(QueryInsertOperation* operation) {
	auto& table = databaseEngine.getTable(operation->table);
	std::size_t columnIndex = 0;
	if (operation->columns.size() != table.schema().columns().size()) {
		throw std::runtime_error("Wrong number of columns.");
	}

	for (auto& column : operation->columns) {
		auto& columnDefinition = table.schema().getDefinition(column);

		auto insertForType = [&](auto dummy) {
			using ColumnType = decltype(dummy);

			for (auto& columnValues : operation->values) {
				auto& value = columnValues[columnIndex];
				if (value.type != columnDefinition.type()) {
					throw std::runtime_error("Wrong type.");
				}

				table.insertColumn(column, value.getValue<ColumnType>());
			}
		};

		handleGenericType(columnDefinition.type(), insertForType);
		columnIndex++;
	}
}

void OperationExecutorVisitor::visit(QueryUpdateOperation* operation) {
	VirtualTable virtualTable(databaseEngine.getTable(operation->table));

	std::unique_ptr<QueryExpression> filterExpression;
	if (operation->filter) {
		filterExpression = std::make_unique<QueryRootExpression>(std::move(operation->filter));
	} else {
		filterExpression = std::make_unique<QueryValueExpression>(QueryValue(true));
	}

	auto filterExecutionEngine = ExecutorHelpers::compile(virtualTable, filterExpression.get());

	std::vector<std::unique_ptr<ExpressionExecutionEngine>> setExecutionEngines;
	for (auto& set : operation->sets) {
		setExecutionEngines.emplace_back(std::make_unique<ExpressionExecutionEngine>(
			ExecutorHelpers::compile(virtualTable, set.get())));
	}

	for (std::size_t rowIndex = 0; rowIndex < virtualTable.numRows(); rowIndex++) {
		filterExecutionEngine.execute(rowIndex);

		if (filterExecutionEngine.popEvaluation().getValue<bool>()) {
			std::size_t setIndex = 0;
			for (auto& setExecutionEngine : setExecutionEngines) {
				auto& setColumnName = operation->sets[setIndex]->column;

				setExecutionEngine->execute(rowIndex);
				auto newValue = setExecutionEngine->popEvaluation();

				auto handleForType = [&](auto dummy) {
					using Type = decltype(dummy);
					virtualTable.getColumn(setColumnName).storage()->getUnderlyingStorage<Type>()[rowIndex] = newValue.getValue<Type>();
				};

				handleGenericType(newValue.type, handleForType);
				setIndex++;
			}
		}
	}
}
