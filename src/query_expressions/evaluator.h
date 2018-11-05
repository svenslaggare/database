#pragma once
#include <stack>

#include "../table.h"
#include "visitor.h"

/**
 * Represents an evaluator of database query expressions
 */
struct QueryExpressionEvaluatorVisitor : public QueryExpressionVisitor {
	const Table& table;

	std::stack<QueryValue, std::vector<QueryValue>> evaluationStack;
	std::size_t rowIndex;

	QueryExpressionEvaluatorVisitor(const Table& table, std::size_t rowIndex);

	virtual void visit(QueryExpression* parent, QueryRootExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryColumnReferenceExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryValueExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryAndExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryCompareExpression* expression) override;
};