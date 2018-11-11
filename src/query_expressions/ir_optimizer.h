#pragma once
#include <vector>
#include <memory>

struct ExpressionExecutionEngine;
struct ExpressionIR;

/**
 * Represents an optimizer for expression IR
 */
class ExpressionIROptimizer {
private:
	using InstructionsIterator = std::vector<std::unique_ptr<ExpressionIR>>::iterator;

	ExpressionExecutionEngine& mExecutionEngine;

	void replaceInstructions(InstructionsIterator& it,
							std::unique_ptr<ExpressionIR> newInstruction,
							std::size_t numToReplace);

	bool optimizeCompare(InstructionsIterator& it, ExpressionIR* current);
	bool optimizeAnd(InstructionsIterator& it, ExpressionIR* current);
public:
	/**
	 * Creates a new IR optimizer
	 * @param executionEngine The execution engine to optimize the instruction stream for
	 */
	explicit ExpressionIROptimizer(ExpressionExecutionEngine& executionEngine);

	/**
	 * Optimizes the instructions
	 */
	void optimize();
};