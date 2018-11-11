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
	ColumnType mExpressionType;

	std::size_t mCurrentRowIndex = 0;
	EvaluationStack mEvaluationStack;
public:
	/**
	 * Creates a new execution engine
	 */
	explicit ExpressionExecutionEngine();

	/**
	 * Returns the storage for the given column
	 * @param slot The slot of the column
	 */
	inline ColumnStorage* getStorage(std::size_t slot) {
		return mSlottedColumnStorage[slot];
	}

	/**
	 * Returns the number of slots
	 */
	std::size_t numSlots() const;

	/**
	 * Returns the slot for the given column
	 * @param name The name of the column
	 */
	std::size_t getSlot(const std::string& name);

	/**
	 * Returns the column of the given slot
	 * @param slot The slot
	 */
	std::string fromSlot(std::size_t slot) const;

	/**
	 * Fills the slots
	 * @param table The table
	 */
	void fillSlots(Table& table);

	/**
	 * Sets the slot storage
	 * @param storage The storage
	 */
	void setSlotStorage(std::vector<ColumnStorage*> storage);

	/**
	 * Returns the type of the final expression
	 */
	ColumnType expressionType() const;

	/**
	 * Sets the expression type
	 */
	void setExpressionType(ColumnType type);

	/**
	 * Returns the current row index
	 */
	inline std::size_t currentRowIndex() const {
		return mCurrentRowIndex;
	}

	/**
	 * Returns the instructions
	 */
	const std::vector<std::unique_ptr<ExpressionIR>>& instructions() const;

	/**
	 * Pops from the evaluation stack
	 */
	inline QueryValue popEvaluation() {
		auto result = mEvaluationStack.top();
		mEvaluationStack.pop();
		return result;
	}

	/**
	 * Pushes on the evaluation stack
	 * @param value The value to push
	 */
	inline void pushEvaluation(const QueryValue& value) {
		mEvaluationStack.push(value);
	}

	/**
	 * Adds the given instruction
	 * @param instruction The instruction
	 */
	void addInstruction(std::unique_ptr<ExpressionIR> instruction);

	/**
	 * Removes the last instruction
	 */
	void removeLastInstruction();

	/**
	 * Replaces the given instruction
	 * @param index The index to replace at
	 * @param instruction The new instruction
	 */
	void replaceInstruction(std::size_t index, std::unique_ptr<ExpressionIR> instruction);

	/**
	 * Executes the expression on the given row
	 * @param rowIndex The index of the row
	 */
	void execute(std::size_t rowIndex);
};

/**
 * Represents an IR for expressions
 */
struct ExpressionIR {
	virtual ~ExpressionIR() = default;

	/**
	 * Executes the instruction
	 * @param executionEngine The execution engine
	 */
	virtual void execute(ExpressionExecutionEngine& executionEngine) = 0;
};

/**
 * Represents expression IR for a query value
 */
struct QueryValueExpressionIR : public ExpressionIR {
	QueryValue value;

	/**
	 * Creates a new value expression IR
	 * @param value The value
	 */
	explicit QueryValueExpressionIR(QueryValue value);

	virtual void execute(ExpressionExecutionEngine& executionEngine) override;
};

/**
 * Represents expression IR for a column reference
 */
struct ColumnReferenceExpressionIR : public ExpressionIR {
	std::size_t columnSlot;

	/**
	 * Creates a new column reference IR
	 * @param columnSlot The column slot
	 */
	explicit ColumnReferenceExpressionIR(std::size_t columnSlot);

	virtual void execute(ExpressionExecutionEngine& executionEngine) override;
};

/**
 * Represents expression IR for a comparison
 */
struct CompareExpressionIR : public ExpressionIR {
	CompareOperator op;

	/**
	 * Creates a new compare expression IR
	 * @param op The compare operator
	 */
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

	/**
	 * Creates a new math expression IR
	 * @param op The operation
	 */
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

	/**
	 * Creates a new compare expression IR
	 * @param lhs The lhs value
	 * @param rhs The rhs column
	 * @param op The compare operation
	 */
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

	/**
	 * Creates a new compare expression IR
	 * @param lhs The lhs column
	 * @param rhs The rhs value
	 * @param op The compare operation
	 */
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

	/**
	 * Creates a new compare expression IR
	 * @param lhs The lhs column
	 * @param rhs The rhs column
	 * @param op The compare operation
	 */
	CompareExpressionLeftColumnRightColumnIR(std::size_t lhs, std::size_t rhs, CompareOperator op);

	virtual void execute(ExpressionExecutionEngine& executionEngine) override;
};
