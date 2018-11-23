#pragma once
#include <memory>
#include <string>

#include "../common.h"

struct QueryExpressionVisitor;

/**
 * Represents a query expression
 */
struct QueryExpression {
	virtual ~QueryExpression() = default;

	/**
	 * Accepts the given visitor
	 * @param visitor The visitor
	 * @param parent The parent expression
	 */
	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) = 0;

	/**
	 * Replaces the old expression with the new expression
	 * @param oldExpression The expression to replace
	 * @param newExpression The replacement
	 */
	virtual void update(QueryExpression* oldExpression, std::unique_ptr<QueryExpression> newExpression) {}
};

/**
 * Represents a root query expression
 */
struct QueryRootExpression : public QueryExpression {
	std::unique_ptr<QueryExpression> root;

	/**
	 * Creates a new root expression
	 * @param expression The expression
	 */
	explicit QueryRootExpression(std::unique_ptr<QueryExpression> expression);

	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) override;
	virtual void update(QueryExpression* oldExpression, std::unique_ptr<QueryExpression> newExpression) override;
};

/**
 * Represents an expressions that contains many
 */
struct QueryMultipleExpressions : public QueryExpression {
	std::vector<std::unique_ptr<QueryExpression>> expressions;

	/**
	 * Creates a new list of expressions
	 * @param expressions The expressions
	 */
	explicit QueryMultipleExpressions(std::vector<std::unique_ptr<QueryExpression>> expressions);

	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) override;
	virtual void update(QueryExpression* oldExpression, std::unique_ptr<QueryExpression> newExpression) override;
};

/**
 * Represents a query expression for a column reference
 */
struct QueryColumnReferenceExpression : public QueryExpression {
	std::string name;

	/**
	 * Creates a new column reference
	 * @param name The name of the column
	 */
	explicit QueryColumnReferenceExpression(const std::string& name);

	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) override;
};

/**
 * Represents a query expression for a value
 */
struct QueryValueExpression : public QueryExpression {
	QueryValue value;

	/**
	 * Creates a new value expression
	 * @param value The value
	 */
	explicit QueryValueExpression(QueryValue value);

	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) override;
};

/**
 * Represents and query expression
 */
struct QueryAndExpression : public QueryExpression {
	std::unique_ptr<QueryExpression> lhs;
	std::unique_ptr<QueryExpression> rhs;

	/**
	 * Creates a new And expression
	 * @param lhs The lhs
	 * @param rhs The rhs
	 */
	QueryAndExpression(std::unique_ptr<QueryExpression> lhs, std::unique_ptr<QueryExpression> rhs);

	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) override;
	virtual void update(QueryExpression* oldExpression, std::unique_ptr<QueryExpression> newExpression) override;
};

/**
 * Represents a query compare expression
 */
struct QueryCompareExpression : public QueryExpression {
	std::unique_ptr<QueryExpression> lhs;
	std::unique_ptr<QueryExpression> rhs;
	CompareOperator op;

	/**
	 * Creates a new compare expression
	 * @param lhs The lhs
	 * @param rhs The rhs
	 * @param op The compare operation
	 */
	QueryCompareExpression(std::unique_ptr<QueryExpression> lhs,
						   std::unique_ptr<QueryExpression> rhs,
						   CompareOperator op);

	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) override;
	virtual void update(QueryExpression* oldExpression, std::unique_ptr<QueryExpression> newExpression) override;
};

/**
 * Represents a math query expression
 */
struct QueryMathExpression : public QueryExpression {
	std::unique_ptr<QueryExpression> lhs;
	std::unique_ptr<QueryExpression> rhs;
	MathOperator op;

	/**
	 * Creates a math expression
	 * @param lhs The lhs
	 * @param rhs The rhs
	 * @param op The math operation
	 */
	QueryMathExpression(std::unique_ptr<QueryExpression> lhs,
						std::unique_ptr<QueryExpression> rhs,
						MathOperator op);

	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) override;
	virtual void update(QueryExpression* oldExpression, std::unique_ptr<QueryExpression> newExpression) override;
};

/**
 * Represents an assign expression
 */
struct QueryAssignExpression : public QueryExpression {
	std::string column;
	std::unique_ptr<QueryExpression> value;

	/**
	 * Creates a new assign expression
	 * @param column The column to assign the value too
	 * @param value The value to assign
	 */
	QueryAssignExpression(std::string column, std::unique_ptr<QueryExpression> value);

	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) override;
	virtual void update(QueryExpression* oldExpression, std::unique_ptr<QueryExpression> newExpression) override;
};

/**
 * Helper functions for query expressions
 */
namespace QueryExpressionHelpers {
	/**
	 * Creates column references
	 * @param columns The name of the columns
	 */
	std::vector<std::unique_ptr<QueryExpression>> createColumnReferences(std::initializer_list<std::string> columns);
}