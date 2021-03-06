#pragma once
#include <unordered_map>
#include <stack>
#include <memory>

#include "../common.h"
#include "../query_expressions/helpers.h"
#include "virtual_table.h"

class ColumnStorage;
struct ExpressionIR;

/**
 * Contains metadata for a column slot
 */
struct ColumnSlotData {
	std::size_t slotIndex;
	std::string table;
	std::string column;
};

/**
 * Represents an execution engine for expression IR
 */
class ExpressionExecutionEngine {
public:
	using EvaluationStack = std::stack<QueryValue, std::vector<QueryValue>>;
private:
	std::vector<VirtualColumn*> mSlottedColumnStorage;
	std::unordered_map<std::string, ColumnSlotData> mColumnNameToSlot;
	std::size_t mNextColumnSlot = 0;

	std::vector<std::unique_ptr<ExpressionIR>> mInstructions;
	std::vector<ColumnType> mExpressionTypes;

	std::size_t mCurrentRowIndex = 0;
	EvaluationStack mEvaluationStack;
public:
	/**
	 * Creates a new execution engine
	 */
	explicit ExpressionExecutionEngine();

	ExpressionExecutionEngine(const ExpressionExecutionEngine&) = delete;
	ExpressionExecutionEngine& operator=(const ExpressionExecutionEngine&) = delete;

	ExpressionExecutionEngine(ExpressionExecutionEngine&&) = default;
	ExpressionExecutionEngine& operator=(ExpressionExecutionEngine&&) = default;

	/**
	 * Returns the storage for the given column
	 * @param slot The slot of the column
	 */
	inline VirtualColumn* columnFromSlot(std::size_t slot) {
		return mSlottedColumnStorage[slot];
	}

	/**
	 * Returns the number of slots
	 */
	std::size_t numSlots() const;

	/**
	 * Returns the slot for the given column
	 * @param table The name of the table
	 * @param column The name of the column
	 */
	std::size_t getSlot(const std::string& table, const std::string& column);

	/**
	 * Returns the column of the given slot
	 * @param slot The slot
	 */
	std::string fromSlot(std::size_t slot) const;

	/**
	 * Fills the slots
	 * @param tableContainer The table container
	 */
	void fillSlots(VirtualTableContainer& tableContainer);

	/**
	 * Returns the type of the final expression
	 */
	ColumnType expressionType() const;

	/**
	 * Returns all the return types.
	 */
	const std::vector<ColumnType>& expressionTypes() const;

	/**
	 * Sets the expression types
	 * @param types The list of types
	 */
	void setExpressionTypes(const std::vector<ColumnType>& types);

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
	 * Returns the instructions
	 */
	std::vector<std::unique_ptr<ExpressionIR>>& instructions();

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

	std::size_t evaluationStackSize() const;

	/**
	 * Adds the given instruction
	 * @param instruction The instruction
	 */
	void addInstruction(std::unique_ptr<ExpressionIR> instruction);

	/**
	 * Removes the instruction at the given index
	 * @param index The index of the instruction to remove
	 */
	void removeInstruction(std::size_t index);

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
	 * Makes the given compare operation always true
	 * @param index The index of the instruction
	 */
	void makeCompareAlwaysTrue(std::size_t index);

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
		T rhsValue = executionEngine.columnFromSlot(rhs)->storage()->template getUnderlyingStorage<T>()[executionEngine.currentRowIndex()];
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
		auto lhsColumn = executionEngine.columnFromSlot(lhs)->storage();
		T lhsValue = lhsColumn->template getUnderlyingStorage<T>()[executionEngine.currentRowIndex()];
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
