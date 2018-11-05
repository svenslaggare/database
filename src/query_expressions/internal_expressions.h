#pragma once
#include "expressions.h"

/**
 * Represents a column reference with a known slot
 */
struct QueryColumnReferenceSlottedExpression : public QueryExpression {
	std::size_t index;

	explicit QueryColumnReferenceSlottedExpression(std::size_t index);

	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) override;
};

/**
 * Represents a database query compare expression with specialized values
 */
struct QueryCompareLeftValueRightColumnExpression : public QueryBoolExpression {
	QueryValue lhs;
	std::size_t rhs;
	CompareOperator op;

	QueryCompareLeftValueRightColumnExpression(QueryValue lhs, std::size_t rhs, CompareOperator op);

	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) override;
};

/**
 * Represents a database query compare expression with specialized values
 */
struct QueryCompareLeftColumnRightValueExpression : public QueryBoolExpression {
	std::size_t lhs;
	QueryValue rhs;
	CompareOperator op;

	QueryCompareLeftColumnRightValueExpression(std::size_t lhs, QueryValue rhs, CompareOperator op);

	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) override;
};

/**
 * Represents a database query compare expression with specialized values
 */
struct QueryCompareLeftColumnRightColumnExpression : public QueryBoolExpression {
	std::size_t lhs;
	std::size_t rhs;
	CompareOperator op;

	QueryCompareLeftColumnRightColumnExpression(std::size_t lhs, std::size_t rhs, CompareOperator op);

	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) override;
};

/**
 * Represents a database query compare expression with specialized values
 */
struct QueryCompareLeftValueInt32RightColumnExpression : public QueryBoolExpression {
	std::int32_t lhs;
	std::size_t rhs;
	CompareOperator op;

	QueryCompareLeftValueInt32RightColumnExpression(std::int32_t lhs, std::size_t rhs, CompareOperator op);
	virtual void accept(QueryExpressionVisitor& visitor, QueryExpression* parent) override;
};
