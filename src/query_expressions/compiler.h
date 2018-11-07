#pragma once
#include <stack>
#include <unordered_map>

#include "visitor.h"
#include "helpers.h"

struct ColumnStorage;
class Table;
struct ExpressionIR;

/**
 * Represents an execution engine for expression IR
 */
struct ExpressionExecutionEngine {
	std::vector<ColumnStorage*> slottedColumnStorage;
	std::unordered_map<std::string, std::size_t> columnNameToSlot;

	std::vector<std::unique_ptr<ExpressionIR>> instructions;

	std::size_t currentRowIndex = 0;
	std::stack<QueryValue, std::vector<QueryValue>> evaluationStack;

	explicit ExpressionExecutionEngine(Table& table);

	void execute(std::size_t rowIndex);
};

/**
 * Represents an IR for expressions
 */
struct ExpressionIR {
	virtual ~ExpressionIR() = default;
	virtual void execute(ExpressionExecutionEngine& executionEngine) = 0;
};

/**
 * Represents expression IR for a query value
 */
struct QueryValueExpressionIR : public ExpressionIR {
	QueryValue value;

	explicit QueryValueExpressionIR(QueryValue value);
	virtual void execute(ExpressionExecutionEngine& executionEngine) override;
};

/**
 * Represents expression IR for a column reference
 */
struct ColumnReferenceExpressionIR : public ExpressionIR {
	std::size_t columnSlot;

	explicit ColumnReferenceExpressionIR(std::size_t columnSlot);
	virtual void execute(ExpressionExecutionEngine& executionEngine) override;
};

/**
 * Represents expression IR for a comparison
 */
struct CompareExpressionIR : public ExpressionIR {
	CompareOperator op;

	explicit CompareExpressionIR(CompareOperator op);
	virtual void execute(ExpressionExecutionEngine& executionEngine) override;
};

/**
 * Represents expression IR for an and operator
 */
struct AndExpressionIR : public ExpressionIR {
	virtual void execute(ExpressionExecutionEngine& executionEngine) override;
};

/**
 * Represents expression IR for a math operation
 */
struct MathOperationExpressionIR : public ExpressionIR {
	MathOperator op;

	explicit MathOperationExpressionIR(MathOperator op);
	virtual void execute(ExpressionExecutionEngine& executionEngine) override;
};

/**
 * Represents expression IR for a comparison with specialized values
 */
template<typename T>
struct CompareExpressionLeftValueKnownTypeRightColumnIR : public ExpressionIR {
	T lhs;
	std::size_t rhs;
	CompareOperator op;

	CompareExpressionLeftValueKnownTypeRightColumnIR(T lhs, std::size_t rhs, CompareOperator op)
		: lhs(lhs), rhs(rhs), op(op) {

	}

	virtual void execute(ExpressionExecutionEngine& executionEngine) override {
		T rhsValue = executionEngine.slottedColumnStorage[rhs]->template getUnderlyingStorage<T>()[executionEngine.currentRowIndex];
		executionEngine.evaluationStack.push(QueryValue(QueryExpressionHelpers::compare(op, lhs, rhsValue)));
	}
};

/**
 * Represents expression IR for a comparison with specialized values
 */
template<typename T>
struct CompareExpressionLeftColumnRightValueKnownTypeIR : public ExpressionIR {
	std::size_t lhs;
	T rhs;
	CompareOperator op;

	CompareExpressionLeftColumnRightValueKnownTypeIR(std::size_t lhs, T rhs, CompareOperator op)
		: lhs(lhs), rhs(rhs), op(op) {

	}

	virtual void execute(ExpressionExecutionEngine& executionEngine) override {
		T lhsValue = executionEngine.slottedColumnStorage[lhs]->template getUnderlyingStorage<T>()[executionEngine.currentRowIndex];
		executionEngine.evaluationStack.push(QueryValue(QueryExpressionHelpers::compare(op, lhsValue, rhs)));
	}
};

/**
 * Represents expression IR for a comparison with specialized values
 */
struct CompareExpressionLeftColumnRightColumnIR : public ExpressionIR {
	std::size_t lhs;
	std::size_t rhs;
	CompareOperator op;

	CompareExpressionLeftColumnRightColumnIR(std::size_t lhs, std::size_t rhs, CompareOperator op);
	virtual void execute(ExpressionExecutionEngine& executionEngine) override;
};

/**
 * Represents a compiler for query expressions to expression IR
 */
struct QueryExpressionCompilerVisitor : public QueryExpressionVisitor {
	Table& table;
	ExpressionExecutionEngine& executionEngine;
	std::size_t nextColumnSlot = 0;
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