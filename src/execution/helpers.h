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
	/**
	 * Compiles the given expression
	 * @param table The table
	 * @param rootExpression The root expression
	 */
	ExpressionExecutionEngine compile(Table& table, QueryExpression* rootExpression);

	/**
	 * Adds given row to the result
	 * @param columnsStorage The storage of the columns
	 * @param result The result
	 * @param rowIndex The index of the row
	 */
	void addRowToResult(std::vector<ColumnStorage*> columnsStorage, QueryResult& result, std::size_t rowIndex);

	/**
	 * Adds the given row to the result
	 * @param projections The projections
	 * @param reducedProjections The reduced projections. Nullptr if not reduced
	 * @param result The result
	 * @param rowIndex The index of the row
	 */
	void addRowToResult(std::vector<std::unique_ptr<ExpressionExecutionEngine>>& projections,
						std::vector<ColumnStorage*>& reducedProjections,
						QueryResult& result,
						std::size_t rowIndex);

	/**
	 * Orders the given result
	 * @param orderDataType The type of ordering column
	 * @param orderingData The ordering data
	 * @param result The result to order
	 */
	void orderResult(ColumnType orderDataType, const std::vector<RawQueryValue>& orderingData, QueryResult& result);
}

/**
 * Tries to reduce projections to column references
 */
struct ReducedProjections {
	std::vector<std::string> columns;
	std::vector<ColumnStorage*> storage;
	bool allReduced = false;

	/**
	 * Tries to reduce the given projections
	 * @param projections The projection
	 * @param table The table
	 */
	void tryReduce(std::vector<std::unique_ptr<QueryExpression>>& projections, Table& table);

	/**
	 * Returns the index of the reduced column
	 * @param name The name of the column
	 */
	std::int64_t indexOfColumn(const std::string& name);
};