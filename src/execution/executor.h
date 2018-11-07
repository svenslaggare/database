#pragma once
#include "operation_visitor.h"
#include "../query_expressions/compiler.h"

struct DatabaseEngine;

namespace ExecutorHelpers {
	inline void addRowToResult(std::vector<ColumnStorage*> columnsStorage, QueryResult& result, std::size_t rowIndex) {
		for (std::size_t columnIndex = 0; columnIndex < columnsStorage.size(); columnIndex++) {
			auto& columnStorage = *columnsStorage[columnIndex];
			auto& resultStorage = result.columns[columnIndex];

			auto handleForType = [&](auto dummy) {
				using Type = decltype(dummy);
				resultStorage.getUnderlyingStorage<Type>().push_back(columnStorage.getUnderlyingStorage<Type>()[rowIndex]);
			};

			handleGenericType(columnStorage.type, handleForType);
		}
	}

	inline void addRowToResult(std::vector<std::unique_ptr<ExpressionExecutionEngine>>& projections, QueryResult& result) {
		std::size_t projectionIndex = 0;
		for (auto& projection : projections) {
			for (auto& instruction : projection->instructions) {
				instruction->execute(*projection);
			}

			auto resultValue = projection->evaluationStack.top();

			auto& resultStorage = result.columns[projectionIndex];
			auto handleForType = [&](auto dummy) {
				using Type = decltype(dummy);
				resultStorage.getUnderlyingStorage<Type>().push_back(resultValue.getValue<Type>());
			};

			handleGenericType(resultValue.type, handleForType);

			projection->evaluationStack.pop();
			projectionIndex++;
		}
	}
}

struct ReducedColumns {
	std::vector<std::string> columns;
	std::vector<ColumnStorage*> storage;

	bool tryReduce(std::vector<std::unique_ptr<QueryExpression>>& projections, Table& table) {
		for (auto& projection : projections) {
			auto column = dynamic_cast<QueryColumnReferenceExpression*>(projection.get());
			if (column != nullptr) {
				columns.push_back(column->name);
			}
		}

		if (columns.size() == projections.size()) {
			for (auto& column : columns) {
				storage.push_back(&table.getColumn(column));
			}

			return true;
		} else {
			columns.clear();
			return false;
		}
	}
};

/**
 * Represents a operations executor visitor
 */
struct OperationExecutorVisitor : public QueryOperationVisitor {
	DatabaseEngine& databaseEngine;
	QueryResult& result;

	explicit OperationExecutorVisitor(DatabaseEngine& databaseEngine, QueryResult& result);

	virtual void visit(QuerySelectOperation* operation) override;
	virtual void visit(QueryInsertOperation* operation) override;
};