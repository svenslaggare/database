#pragma once
#include "expressions.h"
#include "internal_expressions.h"

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

	virtual void visit(QueryExpression* parent, QueryColumnReferenceSlottedExpression* expression) = 0;

	virtual void visit(QueryExpression* parent, QueryCompareLeftValueRightColumnExpression* expression) = 0;
	virtual void visit(QueryExpression* parent, QueryCompareLeftColumnRightValueExpression* expression) = 0;
	virtual void visit(QueryExpression* parent, QueryCompareLeftColumnRightColumnExpression* expression) = 0;

	virtual void visit(QueryExpression* parent, QueryCompareLeftValueInt32RightColumnExpression* expression) = 0;
};