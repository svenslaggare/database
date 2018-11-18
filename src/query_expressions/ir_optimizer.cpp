#include "ir_optimizer.h"
#include "../execution/expression_execution.h"

#include <iostream>

ExpressionIROptimizer::ExpressionIROptimizer(ExpressionExecutionEngine& executionEngine)
	: mExecutionEngine(executionEngine) {

}

void ExpressionIROptimizer::replaceInstructions(ExpressionIROptimizer::InstructionsIterator& it,
											   std::unique_ptr<ExpressionIR> newInstruction,
											   std::size_t numToReplace) {
	mExecutionEngine.instructions().insert(++it, std::move(newInstruction));
	for (std::size_t i = 0; i < numToReplace; i++) {
		it = mExecutionEngine.instructions().erase(--it);
	}
}

bool ExpressionIROptimizer::optimizeCompare(InstructionsIterator& it, ExpressionIR* current) {
	if (auto compareInstruction = dynamic_cast<CompareExpressionIR*>(current)) {
		auto& rhs = *(it - 1);
		auto& lhs = *(it - 2);

		auto rhsValue = dynamic_cast<QueryValueExpressionIR*>(rhs.get());
		auto rhsColumn = dynamic_cast<ColumnReferenceExpressionIR*>(rhs.get());
		auto lhsValue = dynamic_cast<QueryValueExpressionIR*>(lhs.get());
		auto lhsColumn = dynamic_cast<ColumnReferenceExpressionIR*>(lhs.get());

		if (lhsValue != nullptr && rhsColumn != nullptr) {
			auto handleForType = [&](auto dummy) {
				using Type = decltype(dummy);
				this->replaceInstructions(
					it,
					std::make_unique<CompareExpressionLeftValueKnownTypeRightColumnIR<Type>>(
						lhsValue->value.getValue<Type>(),
						rhsColumn->columnSlot,
						compareInstruction->op),
					3);
			};

			handleGenericType(lhsValue->value.type, handleForType);
			return true;
		} else if (lhsColumn != nullptr && rhsValue != nullptr) {
			auto handleForType = [&](auto dummy) {
				using Type = decltype(dummy);
				this->replaceInstructions(
					it,
					std::make_unique<CompareExpressionLeftColumnRightValueKnownTypeIR<Type>>(
						lhsColumn->columnSlot,
						rhsValue->value.getValue<Type>(),
						compareInstruction->op),
					3);
			};

			handleGenericType(rhsValue->value.type, handleForType);
			return true;
		} else if (lhsColumn != nullptr && rhsColumn != nullptr) {
			this->replaceInstructions(
				it,
				std::make_unique<CompareExpressionLeftColumnRightColumnIR>(
					lhsColumn->columnSlot,
					rhsColumn->columnSlot,
					compareInstruction->op),
				3);
			return true;
		}
	}

	return false;
}

bool ExpressionIROptimizer::optimizeAnd(InstructionsIterator& it, ExpressionIR* current) {
	if (auto andInstruction = dynamic_cast<AndExpressionIR*>(current)) {
		auto& rhs = *(it - 1);
		auto& lhs = *(it - 2);

		auto rhsValue = dynamic_cast<QueryValueExpressionIR*>(rhs.get());
		auto lhsValue = dynamic_cast<QueryValueExpressionIR*>(lhs.get());

		if (lhsValue != nullptr && rhsValue == nullptr) {
			if (lhsValue->value.getValue<bool>()) {
				it = mExecutionEngine.instructions().erase(it - 2);
				it = mExecutionEngine.instructions().erase(it + 1);
			} else {
				replaceInstructions(it, std::make_unique<QueryValueExpressionIR>(QueryValue(false)), 3);
			}

			return true;
		} else if (lhsValue == nullptr && rhsValue != nullptr) {
			if (rhsValue->value.getValue<bool>()) {
				it = mExecutionEngine.instructions().erase(--it);
				it = mExecutionEngine.instructions().erase(it);
			} else {
				replaceInstructions(it, std::make_unique<QueryValueExpressionIR>(QueryValue(false)), 3);
			}

			return true;
		} else if (lhsValue != nullptr && rhsValue != nullptr) {
			replaceInstructions(
				it,
				std::make_unique<QueryValueExpressionIR>(
					QueryValue(lhsValue->value.getValue<bool>() == rhsValue->value.getValue<bool>())),
				3);
			return true;
		}
	}

	return false;
}

void ExpressionIROptimizer::optimize() {
	auto& instructions = mExecutionEngine.instructions();
	auto it = instructions.begin();

//	std::size_t before = instructions.size();
	while (it != instructions.end()) {
		auto current = it->get();
		if (optimizeCompare(it, current)) {
			continue;
		} else if (optimizeAnd(it, current)) {
			continue;
		}

		++it;
	}

//	std::cout << "Optimizations reduced instructions by " << ((std::int64_t)before - (std::int64_t)instructions.size()) << std::endl;
}
