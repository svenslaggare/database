#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../common.h"

struct ColumnStorage;
struct QueryResult;
struct ExpressionExecutionEngine;
struct QueryExpression;
class Table;
class VirtualTable;
class VirtualColumn;
struct ReducedProjections;

namespace ExecutorHelpers {
	/**
	 * Compiles the given expression
	 * @param table The table
	 * @param rootExpression The root expression
	 */
	ExpressionExecutionEngine compile(VirtualTable& table, QueryExpression* rootExpression);

	/**
	 * Adds given row to the result
	 * @param columnsStorage The storage of the columns
	 * @param result The result
	 * @param rowIndex The index of the row
	 */
	void addRowToResult(std::vector<VirtualColumn*> columnsStorage, QueryResult& result, std::size_t rowIndex);

	/**
	 * Adds the given row to the result
	 * @param projections The projections
	 * @param reducedProjections The reduced projections. Nullptr if not reduced
	 * @param result The result
	 * @param rowIndex The index of the row
	 */
	void addRowToResult(std::vector<std::unique_ptr<ExpressionExecutionEngine>>& projections,
						ReducedProjections& reducedProjections,
						QueryResult& result,
						std::size_t rowIndex);

	/**
	 * Orders the given result
	 * @param orderDataType The type of ordering column
	 * @param orderingData The ordering data
	 * @param descending Indicates if sorted descending
	 * @param result The result to order
	 */
	void orderResult(ColumnType orderDataType,
					 const std::vector<RawQueryValue>& orderingData,
					 bool descending,
					 QueryResult& result);
}

/**
 * Tries to reduce projections to column references
 */
struct ReducedProjections {
	std::vector<std::string> columns;
	std::vector<VirtualColumn*> storage;
	bool allReduced = false;

	/**
	 * Tries to reduce the given projections
	 * @param projections The projection
	 * @param getColumnStorage Function for getting the storage of a column
	 */
	void tryReduce(std::vector<std::unique_ptr<QueryExpression>>& projections,
				   std::function<VirtualColumn* (const std::string&)> getColumnStorage);

	/**
	 * Tries to reduce the given projections
	 * @param projections The projection
	 * @param table The table
	 */
	void tryReduce(std::vector<std::unique_ptr<QueryExpression>>& projections, VirtualTable& table);

	/**
	 * Returns the index of the reduced column
	 * @param name The name of the column
	 */
	std::int64_t indexOfColumn(const std::string& name) const;

	/**
	 * Returns the storage of the given reduced column
	 * @param name The name of the column
	 */
	VirtualColumn* getStorage(const std::string& name) const;

	/**
	 * Clears all reduced projections
	 */
	void clear();
};