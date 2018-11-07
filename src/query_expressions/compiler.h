#pragma once
#include <stack>
#include <unordered_map>

#include "visitor.h"
#include "helpers.h"
#include "execution.h"

/**
 * Represents a compiler for query expressions to expression IR
 */
struct QueryExpressionCompilerVisitor : public QueryExpressionVisitor {
	Table& table;
	ExpressionExecutionEngine& executionEngine;
	std::stack<ColumnType> typeEvaluationStack;

	explicit QueryExpressionCompilerVisitor(Table& table, ExpressionExecutionEngine& executionEngine);

	void compile(QueryExpression* rootExpression);

	virtual void visit(QueryExpression* parent, QueryRootExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryColumnReferenceExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryValueExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryAndExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryCompareExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryMathExpression* expression) override;
};