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
	virtual void accept(QueryOperationVisitor& visitor) = 0;
};

/**
 * Represents an ordering clause
 */
struct OrderingClause {
	std::string name;
	bool ascending;

	OrderingClause() = default;
	explicit OrderingClause(const std::string& name, bool ascending = false);
};

/**
 * Represents a select operation
 */
struct QuerySelectOperation : public QueryOperation {
	std::string table;
	std::vector<std::unique_ptr<QueryExpression>> projections;
	std::unique_ptr<QueryExpression> filter;
	OrderingClause order;

	QuerySelectOperation(std::string table,
						 std::vector<std::unique_ptr<QueryExpression>> projection,
						 std::unique_ptr<QueryExpression> filter = {},
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

	QueryInsertOperation(const std::string& table, std::vector<std::string> columns, std::vector<std::vector<QueryValue>> values);

	virtual void accept(QueryOperationVisitor& visitor) override;
};

/**
 * Represents an update operation
 */
struct QueryUpdateOperation : public QueryOperation {

};

/**
 * Represents a database query
 */
struct Query {
	std::vector<std::unique_ptr<QueryOperation>> operations;

	explicit Query(std::vector<std::unique_ptr<QueryOperation>> operations);
};

/**
 * Represents result from a database query
 */
struct QueryResult {
	std::vector<ColumnStorage> columns;

	template<typename T>
	const UnderlyingColumnStorage<T>& getColumn(std::size_t index) const {
		auto& column = columns.at(index);
		if (ColumnTypeHelpers::getType<T>() != column.type) {
			throw std::runtime_error("Wrong type.");
		}

		return column.getUnderlyingStorage<T>();
	}
};