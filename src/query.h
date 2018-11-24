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
 * Represents a column for an ordering
 */
struct OrderingColumn {
	std::string name;
	bool descending;
};

/**
 * Represents an ordering clause
 */
struct OrderingClause {
	std::vector<OrderingColumn> columns;

	OrderingClause() = default;

	/**
	 * Indicates if the ordering is empty
	 */
	bool empty() const;

	/**
	 * Creates a new ordering clause
	 * @param name The name of the field to order on
	 * @param descending Indicates if the ordering is descending
	 */
	explicit OrderingClause(const std::string& name, bool descending = false);

	/**
	 * Creates a new ordering clause
	 * @param columns The columns to order by
	 */
	explicit OrderingClause(std::vector<OrderingColumn> columns);
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
	std::unique_ptr<QueryOperation> operation;

	/**
	 * Creates a new query
	 * @param operation The operation
	 */
	explicit Query(std::unique_ptr<QueryOperation> operation);
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

	/**
	 * Returns the value of the given element
	 * @tparam T The type of the data
	 * @param rowIndex The row of the element
	 * @param columnIndex The column of the element
	 */
	template<typename T>
	const T& getValue(std::size_t rowIndex, std::size_t columnIndex) {
		auto& columnStorage = getColumn<T>(columnIndex);
		return columnStorage[rowIndex];
	}
};