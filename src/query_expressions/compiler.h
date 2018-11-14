#pragma once
#include <stack>
#include <unordered_map>

#include "visitor.h"
#include "../execution/expression_execution.h"

/**
 * Represents a compiler for query expressions to expression IR
 */
class QueryExpressionCompilerVisitor : public QueryExpressionVisitor {
private:
	Table& mTable;
	ExpressionExecutionEngine& mExecutionEngine;
	std::stack<ColumnType> mTypeEvaluationStack;
	bool mOptimize;
public:
	/**
	 * Creates a new query expression compiler
	 * @param table The table
	 * @param executionEngine The execution engine
	 * @param optimize Indicates if optimizations are enabled
	 */
	explicit QueryExpressionCompilerVisitor(Table& table, ExpressionExecutionEngine& executionEngine, bool optimize = true);

	/**
	 * Compiles the given expression
	 * @param rootExpression The root expression
	 */
	void compile(QueryExpression* rootExpression);

	virtual void visit(QueryExpression* parent, QueryRootExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryColumnReferenceExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryValueExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryAndExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryCompareExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryMathExpression* expression) override;
};