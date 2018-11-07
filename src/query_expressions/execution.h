#pragma once
#include <unordered_map>
#include <stack>
#include <memory>

#include "../common.h"
#include "helpers.h"

struct ColumnStorage;
class Table;
struct ExpressionIR;

/**
 * Represents an execution engine for expression IR
 */
class ExpressionExecutionEngine {
public:
	using EvaluationStack = std::stack<QueryValue, std::vector<QueryValue>>;
private:
	std::vector<ColumnStorage*> mSlottedColumnStorage;
	std::unordered_map<std::string, std::size_t> mColumnNameToSlot;
	std::size_t mNextColumnSlot = 0;

	std::vector<std::unique_ptr<ExpressionIR>> mInstructions;

	std::size_t mCurrentRowIndex = 0;
	EvaluationStack mEvaluationStack;
public:
	explicit ExpressionExecutionEngine();

	inline ColumnStorage* getStorage(std::size_t slot) {
		return mSlottedColumnStorage[slot];
	}

	std::size_t getSlot(const std::string& name);

	void fillSlots(Table& table);

	inline std::size_t currentRowIndex() const {
		return mCurrentRowIndex;
	}

	const std::vector<std::unique_ptr<ExpressionIR>>& instructions() const;

	inline QueryValue popEvaluation() {
		auto result = mEvaluationStack.top();
		mEvaluationStack.pop();
		return result;
	}

	inline void pushEvaluation(const QueryValue& value) {
		mEvaluationStack.push(value);
	}

	void addInstruction(std::unique_ptr<ExpressionIR> instruction);
	void removeLast();

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
		T rhsValue = executionEngine.getStorage(rhs)->template getUnderlyingStorage<T>()[executionEngine.currentRowIndex()];
		executionEngine.pushEvaluation(QueryValue(QueryExpressionHelpers::compare(op, lhs, rhsValue)));
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
		T lhsValue = executionEngine.getStorage(lhs)->template getUnderlyingStorage<T>()[executionEngine.currentRowIndex()];
		executionEngine.pushEvaluation(QueryValue(QueryExpressionHelpers::compare(op, lhsValue, rhs)));
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
