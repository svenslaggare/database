#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "column_storage.h"
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
 * Represents a select operation
 */
struct QuerySelectOperation : public QueryOperation {
	std::string table;
	std::vector<std::unique_ptr<QueryExpression>> projections;
	std::unique_ptr<QueryExpression> filter;

	QuerySelectOperation(std::string table,
						 std::vector<std::unique_ptr<QueryExpression>> projection,
						 std::unique_ptr<QueryExpression> filter = {});

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
 * Represents a database row
 */
struct Row {
	std::unordered_map<std::string, QueryValue> columns;

	explicit Row(std::unordered_map<std::string, QueryValue> columns);
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