#include "helpers.h"
#include "../query_expressions/expressions.h"
#include "../query_expressions/compiler.h"
#include "../column_storage.h"
#include "../table.h"
#include "../query.h"

void ExecutorHelpers::addRowToResult(std::vector<ColumnStorage*> columnsStorage, QueryResult& result, std::size_t rowIndex) {
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

void ExecutorHelpers::addRowToResult(std::vector<std::unique_ptr<ExpressionExecutionEngine>>& projections, QueryResult& result, std::size_t rowIndex) {
	std::size_t projectionIndex = 0;
	for (auto& projection : projections) {
		projection->execute(rowIndex);

		auto resultValue = projection->popEvaluation();

		auto& resultStorage = result.columns[projectionIndex];
		auto handleForType = [&](auto dummy) {
			using Type = decltype(dummy);
			resultStorage.getUnderlyingStorage<Type>().push_back(resultValue.getValue<Type>());
		};

		handleGenericType(resultValue.type, handleForType);

		projectionIndex++;
	}
}

bool ReducedColumns::tryReduce(std::vector<std::unique_ptr<QueryExpression>>& projections, Table& table) {
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
