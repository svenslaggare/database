#pragma once
#include <string>
#include <vector>
#include <memory>

struct ColumnStorage;
struct QueryResult;
struct ExpressionExecutionEngine;
struct QueryExpression;
struct Table;

namespace ExecutorHelpers {
	void addRowToResult(std::vector<ColumnStorage*> columnsStorage, QueryResult& result, std::size_t rowIndex);
	void addRowToResult(std::vector<std::unique_ptr<ExpressionExecutionEngine>>& projections, QueryResult& result, std::size_t rowIndex);
}

struct ReducedColumns {
	std::vector<std::string> columns;
	std::vector<ColumnStorage*> storage;

	bool tryReduce(std::vector<std::unique_ptr<QueryExpression>>& projections, Table& table);
};