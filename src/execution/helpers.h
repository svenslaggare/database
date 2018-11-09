#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../common.h"

struct ColumnStorage;
struct QueryResult;
struct ExpressionExecutionEngine;
struct QueryExpression;
struct Table;

namespace ExecutorHelpers {
	ExpressionExecutionEngine compile(Table& table, QueryExpression* rootExpression);

	void addRowToResult(std::vector<ColumnStorage*> columnsStorage, QueryResult& result, std::size_t rowIndex);
	void addRowToResult(std::vector<std::unique_ptr<ExpressionExecutionEngine>>& projections,
						std::vector<ColumnStorage*>& reducedProjections,
						QueryResult& result,
						std::size_t rowIndex);

	void orderResult(ColumnType orderDataType, const std::vector<RawQueryValue>& orderingData, QueryResult& result);
}

/**
 * Tries to reduce projections to column references
 */
struct ReducedProjections {
	std::vector<std::string> columns;
	std::vector<ColumnStorage*> storage;
	bool allReduced = false;

	void tryReduce(std::vector<std::unique_ptr<QueryExpression>>& projections, Table& table);

	std::int64_t indexOfColumn(const std::string& name);
};