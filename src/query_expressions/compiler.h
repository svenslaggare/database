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
	VirtualTableContainer& mTableContainer;
	VirtualTable& mMainTable;
	ExpressionExecutionEngine& mExecutionEngine;
	std::stack<ColumnType> mTypeEvaluationStack;
	bool mOptimize;
	std::size_t mNumReturnValues;
public:
	/**
	 * Creates a new query expression compiler
	 * @param tableContainer The table container
	 * @param mainTable The main table
	 * @param executionEngine The execution engine
	 * @param optimize Indicates if optimizations are enabled
	 * @param numReturnValues The number of return values
	 */
	QueryExpressionCompilerVisitor(VirtualTableContainer& tableContainer,
								   const std::string& mainTable,
								   ExpressionExecutionEngine& executionEngine,
								   bool optimize,
								   std::size_t numReturnValues);

	/**
	 * Compiles the given expression
	 * @param rootExpression The root expression
	 */
	void compile(QueryExpression* rootExpression);

	virtual void visit(QueryExpression* parent, QueryRootExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryMultipleExpressions* expression) override;
	virtual void visit(QueryExpression* parent, QueryColumnReferenceExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryValueExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryAndExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryCompareExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryMathExpression* expression) override;
	virtual void visit(QueryExpression* parent, QueryAssignExpression* expression) override;
};