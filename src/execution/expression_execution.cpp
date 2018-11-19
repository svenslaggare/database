#include "expression_execution.h"
#include "../query_expressions/compiler.h"
#include "../table.h"
#include "virtual_table.h"

ExpressionExecutionEngine::ExpressionExecutionEngine()
	: mExpressionType(ColumnType::Int32) {

}

const std::vector<std::unique_ptr<ExpressionIR>>& ExpressionExecutionEngine::instructions() const {
	return mInstructions;
}

std::vector<std::unique_ptr<ExpressionIR>>& ExpressionExecutionEngine::instructions() {
	return mInstructions;
}

void ExpressionExecutionEngine::addInstruction(std::unique_ptr<ExpressionIR> instruction) {
	mInstructions.push_back(std::move(instruction));
}

void ExpressionExecutionEngine::removeInstruction(std::size_t index) {
	mInstructions.erase(mInstructions.begin() + index);
}

void ExpressionExecutionEngine::removeLastInstruction() {
	mInstructions.erase(mInstructions.end() - 1);
}

void ExpressionExecutionEngine::replaceInstruction(std::size_t index, std::unique_ptr<ExpressionIR> instruction) {
	mInstructions.at(index) = std::move(instruction);
}

void ExpressionExecutionEngine::makeCompareAlwaysTrue(std::size_t index) {
	replaceInstruction(
		index,
		std::make_unique<QueryValueExpressionIR>(QueryValue(true)));
}

std::size_t ExpressionExecutionEngine::numSlots() const {
	return mSlottedColumnStorage.size();
}

std::size_t ExpressionExecutionEngine::getSlot(const std::string& table, const std::string& column) {
	auto fullName = table + "." + column;
	if (mColumnNameToSlot.count(fullName) == 0) {
		mColumnNameToSlot[fullName] = {
			mNextColumnSlot++,
			table,
			column
		};
	}

	return mColumnNameToSlot[fullName].slotIndex;
}

std::string ExpressionExecutionEngine::fromSlot(std::size_t slot) const {
	for (auto& current : mColumnNameToSlot) {
		if (current.second.slotIndex == slot) {
			return current.first;
		}
	}

	return "";
}

void ExpressionExecutionEngine::fillSlots(VirtualTableContainer& tableContainer) {
	mSlottedColumnStorage.resize(mColumnNameToSlot.size());
	for (auto& column : mColumnNameToSlot) {
		auto& slotMetadata = column.second;
		auto& table = tableContainer.getTable(slotMetadata.table);
		mSlottedColumnStorage[slotMetadata.slotIndex] = &table.getColumn(slotMetadata.column);
	}
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
		*executionEngine.columnFromSlot(columnSlot)->storage(),
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
	auto lhsColumn = executionEngine.columnFromSlot(lhs)->storage();

	auto handleForType = [&](auto dummy) {
		using Type = decltype(dummy);
		auto rhsColumn = executionEngine.columnFromSlot(rhs)->storage();

		Type lhsValue = lhsColumn->getUnderlyingStorage<Type>()[executionEngine.currentRowIndex()];
		Type rhsValue = rhsColumn->getUnderlyingStorage<Type>()[executionEngine.currentRowIndex()];
		executionEngine.pushEvaluation(QueryValue(QueryExpressionHelpers::compare(op, lhsValue, rhsValue)));
	};

	handleGenericType(lhsColumn->type(), handleForType);
}