#include "execution.h"
#include "compiler.h"
#include "../table.h"

ExpressionExecutionEngine::ExpressionExecutionEngine()
	: mExpressionType(ColumnType::Int32) {

}

const std::vector<std::unique_ptr<ExpressionIR>>& ExpressionExecutionEngine::instructions() const {
	return mInstructions;
}

void ExpressionExecutionEngine::addInstruction(std::unique_ptr<ExpressionIR> instruction) {
	mInstructions.push_back(std::move(instruction));
}

void ExpressionExecutionEngine::removeLastInstruction() {
	mInstructions.erase(mInstructions.end() - 1);
}

void ExpressionExecutionEngine::replaceInstruction(std::size_t index, std::unique_ptr<ExpressionIR> instruction) {
	mInstructions.at(index) = std::move(instruction);
}

std::size_t ExpressionExecutionEngine::numSlots() const {
	return mSlottedColumnStorage.size();
}

std::size_t ExpressionExecutionEngine::getSlot(const std::string& name) {
	if (mColumnNameToSlot.count(name) == 0) {
		mColumnNameToSlot[name] = mNextColumnSlot++;
	}

	return mColumnNameToSlot[name];
}

std::string ExpressionExecutionEngine::fromSlot(std::size_t slot) const {
	for (auto& current : mColumnNameToSlot) {
		if (current.second == slot) {
			return current.first;
		}
	}

	return "";
}

void ExpressionExecutionEngine::fillSlots(Table& table) {
	mSlottedColumnStorage.resize(mColumnNameToSlot.size());
	for (auto& column : mColumnNameToSlot) {
		mSlottedColumnStorage[column.second] = &table.getColumn(column.first);
	}
}

void ExpressionExecutionEngine::setSlotStorage(std::vector<ColumnStorage*> storage) {
	mSlottedColumnStorage = std::move(storage);
}

ColumnType ExpressionExecutionEngine::expressionType() const {
	return mExpressionType;
}

void ExpressionExecutionEngine::setExpressionType(ColumnType type) {
	mExpressionType = type;
}

void ExpressionExecutionEngine::execute(std::size_t rowIndex) {
	mCurrentRowIndex = rowIndex;

	for (auto& instruction : mInstructions) {
		instruction->execute(*this);
	}
}


QueryValueExpressionIR::QueryValueExpressionIR(QueryValue value)
	: value(value) {

}

void QueryValueExpressionIR::execute(ExpressionExecutionEngine& executionEngine) {
	executionEngine.pushEvaluation(value);
}

ColumnReferenceExpressionIR::ColumnReferenceExpressionIR(std::size_t columnSlot)
	: columnSlot(columnSlot) {

}

void ColumnReferenceExpressionIR::execute(ExpressionExecutionEngine& executionEngine) {
	executionEngine.pushEvaluation(QueryExpressionHelpers::getValueForColumn(
		*executionEngine.getStorage(columnSlot),
		executionEngine.currentRowIndex()));
}

CompareExpressionIR::CompareExpressionIR(CompareOperator op)
	: op(op) {

}

void CompareExpressionIR::execute(ExpressionExecutionEngine& executionEngine) {
	auto op2 = executionEngine.popEvaluation();
	auto op1 = executionEngine.popEvaluation();

	auto handleForType = [&](auto dummy) {
		using Type = decltype(dummy);
		auto op1Value = op1.getValue<Type>();
		auto op2Value = op2.getValue<Type>();
		executionEngine.pushEvaluation(QueryValue(QueryExpressionHelpers::compare(op, op1Value, op2Value)));
	};

	handleGenericType(op1.type, handleForType);
}

void AndExpressionIR::execute(ExpressionExecutionEngine& executionEngine) {
	auto op2 = executionEngine.popEvaluation();
	auto op1 = executionEngine.popEvaluation();

	auto op1Value = op1.getValue<bool>();
	auto op2Value = op2.getValue<bool>();
	executionEngine.pushEvaluation(QueryValue(op1Value && op2Value));
}

MathOperationExpressionIR::MathOperationExpressionIR(MathOperator op)
	: op(op) {

}

void MathOperationExpressionIR::execute(ExpressionExecutionEngine& executionEngine) {
	auto op2 = executionEngine.popEvaluation();
	auto op1 = executionEngine.popEvaluation();

	auto handleForType = [&](auto dummy) {
		using Type = decltype(dummy);
		auto op1Value = op1.getValue<Type>();
		auto op2Value = op2.getValue<Type>();
		executionEngine.pushEvaluation(QueryValue(QueryExpressionHelpers::apply(op, op1Value, op2Value)));
	};

	handleGenericType(op1.type, handleForType);
}

CompareExpressionLeftColumnRightColumnIR::CompareExpressionLeftColumnRightColumnIR(std::size_t lhs, std::size_t rhs, CompareOperator op)
	: lhs(lhs), rhs(rhs), op(op) {

}

void CompareExpressionLeftColumnRightColumnIR::execute(ExpressionExecutionEngine& executionEngine) {
	auto lhsColumn = executionEngine.getStorage(lhs);

	auto handleForType = [&](auto dummy) {
		using Type = decltype(dummy);
		Type lhsValue = lhsColumn->getUnderlyingStorage<Type>()[executionEngine.currentRowIndex()];
		Type rhsValue = executionEngine.getStorage(rhs)->getUnderlyingStorage<Type>()[executionEngine.currentRowIndex()];
		executionEngine.pushEvaluation(QueryValue(QueryExpressionHelpers::compare(op, lhsValue, rhsValue)));
	};

	handleGenericType(lhsColumn->type(), handleForType);
}