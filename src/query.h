#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "storage.h"
#include "query_expressions/expressions.h"

struct QueryOperationVisitor;

/**
 * Represents an operation in a database query
 */
struct QueryOperation {
	virtual ~QueryOperation() = default;

	/**
	 * Accepts the given visitor
	 * @param visitor The visitor
	 */
	virtual void accept(QueryOperationVisitor& visitor) = 0;
};

/**
 * Represents an ordering clause
 */
struct OrderingClause {
	bool empty = true;
	std::string name;
	bool descending;

	OrderingClause() = default;

	/**
	 * Creates a new ordering clause
	 * @param name The name of the field to order on
	 * @param descending Indicates if the ordering is descending
	 */
	explicit OrderingClause(const std::string& name, bool descending = false);
};

/**
 * Represents a join clause
 */
struct JoinClause {
	bool empty = true;
	std::string joinFromColumn;
	std::string joinOnTable;
	std::string joinOnColumn;

	JoinClause() = default;

	/**
	 * Creates a new join clause
	 * @param joinFromColumn The join column to join from
	 * @param joinOnTable The table to join on
	 * @param joinOnColumn The column in the table to join on
	 */
	JoinClause(const std::string& joinFromColumn,
			   const std::string& joinOnTable,
			   const std::string& joinOnColumn);
};

/**
 * Represents a select operation
 */
struct QuerySelectOperation : public QueryOperation {
	std::string table;
	std::vector<std::unique_ptr<QueryExpression>> projections;
	std::unique_ptr<QueryExpression> filter;
	JoinClause join;
	OrderingClause order;

	/**
	 * Creates a new select operation
	 * @param table The table
	 * @param projection The projections
	 * @param filter The filtering
	 * @param join The join
	 * @param order The ordering
	 */
	QuerySelectOperation(std::string table,
						 std::vector<std::unique_ptr<QueryExpression>> projection,
						 std::unique_ptr<QueryExpression> filter = {},
						 JoinClause join = {},
						 OrderingClause order = {});

	virtual void accept(QueryOperationVisitor& visitor) override;
};

/**
 * Represents an insert operation
 */
struct QueryInsertOperation : public QueryOperation {
	std::string table;
	std::vector<std::string> columns;
	std::vector<std::vector<QueryValue>> values;

	/**
	 * Creates a new insert operation
	 * @param table The table
	 * @param columns The columns to insert for
	 * @param values The values for the columns.
	 */
	QueryInsertOperation(const std::string& table,
						 std::vector<std::string> columns,
						 std::vector<std::vector<QueryValue>> values);

	virtual void accept(QueryOperationVisitor& visitor) override;
};

/**
 * Represents an update operation
 */
struct QueryUpdateOperation : public QueryOperation {
	std::string table;
	std::vector<std::unique_ptr<QueryAssignExpression>> sets;
	std::unique_ptr<QueryExpression> filter;

	/**
	 * Creates a new update operation
	 * @param table The table
	 * @param sets The set operations
	 * @param filter The filtering
	 */
	QueryUpdateOperation(std::string table,
						 std::vector<std::unique_ptr<QueryAssignExpression>> sets,
						 std::unique_ptr<QueryExpression> filter = {});

	virtual void accept(QueryOperationVisitor& visitor) override;
};

/**
 * Represents a database query
 */
struct Query {
	std::vector<std::unique_ptr<QueryOperation>> operations;

	/**
	 * Creates a new query
	 * @param operations The operations
	 */
	explicit Query(std::vector<std::unique_ptr<QueryOperation>> operations);
};

/**
 * Represents result from a database query
 */
struct QueryResult {
	std::vector<ColumnStorage> columns;

	/**
	 * Returns the underlying storage for the given column
	 * @tparam T Type of the data
	 * @param index The index of the column in the result
	 */
	template<typename T>
	const UnderlyingColumnStorage<T>& getColumn(std::size_t index) const {
		auto& column = columns.at(index);
		if (ColumnTypeHelpers::getType<T>() != column.type()) {
			throw std::runtime_error("Wrong type.");
		}

		return column.getUnderlyingStorage<T>();
	}
};