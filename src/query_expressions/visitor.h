#pragma once
#include "expressions.h"

/**
 * Represents a visitor for database query expressions
 */
struct QueryExpressionVisitor {
	virtual ~QueryExpressionVisitor() = default;

	virtual void visit(QueryExpression* parent, QueryRootExpression* expression) = 0;
	virtual void visit(QueryExpression* parent, QueryColumnReferenceExpression* expression) = 0;
	virtual void visit(QueryExpression* parent, QueryValueExpression* expression) = 0;
	virtual void visit(QueryExpression* parent, QueryAndExpression* expression) = 0;
	virtual void visit(QueryExpression* parent, QueryCompareExpression* expression) = 0;
};