#pragma once
#include <memory>
#include <string>

#include "../common.h"

struct QueryExpressionVisitor;

/**
 * Represents a database query expression
 */
struct QueryExpression {
	virtual ~QueryExpression() = default;

	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) = 0;
	virtual void update(QueryExpression* oldExpression, std::unique_ptr<QueryExpression> newExpression) {}
};

/**
 * Represents a root database query expression
 */
struct QueryRootExpression : public QueryExpression {
	std::unique_ptr<QueryExpression> root;

	explicit QueryRootExpression(std::unique_ptr<QueryExpression> expression);

	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) override;
	virtual void update(QueryExpression* oldExpression, std::unique_ptr<QueryExpression> newExpression) override;
};

/**
 * Represents a database query expression for a column reference
 */
struct QueryColumnReferenceExpression : public QueryExpression {
	std::string name;

	explicit QueryColumnReferenceExpression(const std::string& name);

	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) override;
};

/**
 * Represents a database query expression for a value
 */
struct QueryValueExpression : public QueryExpression {
	QueryValue value;

	explicit QueryValueExpression(QueryValue value);

	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) override;
};

/**
 * Represents a bool value database query expression
 */
struct QueryBoolExpression : public QueryExpression {

};

/**
 * Represents an and database query expression
 */
struct QueryAndExpression : public QueryBoolExpression {
	std::unique_ptr<QueryBoolExpression> lhs;
	std::unique_ptr<QueryBoolExpression> rhs;

	QueryAndExpression(std::unique_ptr<QueryBoolExpression> lhs,
							   std::unique_ptr<QueryBoolExpression> rhs);

	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) override;
	virtual void update(QueryExpression* oldExpression, std::unique_ptr<QueryExpression> newExpression) override;
};

/**
 * Represents a database query compare expression
 */
struct QueryCompareExpression : public QueryBoolExpression {
	std::unique_ptr<QueryExpression> lhs;
	std::unique_ptr<QueryExpression> rhs;
	CompareOperator op;

	QueryCompareExpression(std::unique_ptr<QueryExpression> lhs,
								   std::unique_ptr<QueryExpression> rhs,
								   CompareOperator op);

	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) override;
	virtual void update(QueryExpression* oldExpression, std::unique_ptr<QueryExpression> newExpression) override;
};