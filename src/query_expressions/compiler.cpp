#include "compiler.h"
#include "../column_storage.h"
#include "helpers.h"

QueryValueExpressionIR::QueryValueExpressionIR(QueryValue value)
	: value(value) {

}

void QueryValueExpressionIR::execute(ExpressionExecutionEngine& executionEngine) {
	executionEngine.evaluationStack.push(value);
}

ColumnReferenceExpressionIR::ColumnReferenceExpressionIR(std::size_t columnSlot)
	: columnSlot(columnSlot) {

}

void ColumnReferenceExpressionIR::execute(ExpressionExecutionEngine& executionEngine) {
	executionEngine.evaluationStack.push(QueryExpressionHelpers::getValueForColumn(
		*executionEngine.slottedColumnStorage[columnSlot],
		executionEngine.currentRowIndex));
}

CompareExpressionIR::CompareExpressionIR(CompareOperator op)
	: op(op) {

}

void CompareExpressionIR::execute(ExpressionExecutionEngine& executionEngine) {
	auto& evaluationStack = executionEngine.evaluationStack;

	auto op2 = std::move(evaluationStack.top());
	executionEngine.evaluationStack.pop();

	auto op1 = std::move(evaluationStack.top());
	executionEngine.evaluationStack.pop();

	auto handleForType = [&](auto dummy) {
		using Type = decltype(dummy);
		auto op1Value = op1.getValue<Type>();
		auto op2Value = op2.getValue<Type>();
		evaluationStack.push(QueryValue(QueryExpressionHelpers::compare(op, op1Value, op2Value)));
	};

	handleGenericType(op1.type, handleForType);
}

void AndExpressionIR::execute(ExpressionExecutionEngine& executionEngine) {
	auto& evaluationStack = executionEngine.evaluationStack;

	auto op2 = std::move(evaluationStack.top());
	executionEngine.evaluationStack.pop();

	auto op1 = std::move(evaluationStack.top());
	executionEngine.evaluationStack.pop();

	auto op1Value = op1.getValue<bool>();
	auto op2Value = op2.getValue<bool>();
	evaluationStack.push(QueryValue(op1Value && op2Value));
}

MathOperationExpressionIR::MathOperationExpressionIR(MathOperator op)
	: op(op) {

}

void MathOperationExpressionIR::execute(ExpressionExecutionEngine& executionEngine) {
	auto& evaluationStack = executionEngine.evaluationStack;

	auto op2 = std::move(evaluationStack.top());
	executionEngine.evaluationStack.pop();

	auto op1 = std::move(evaluationStack.top());
	executionEngine.evaluationStack.pop();

	auto handleForType = [&](auto dummy) {
		using Type = decltype(dummy);
		auto op1Value = op1.getValue<Type>();
		auto op2Value = op2.getValue<Type>();
		evaluationStack.push(QueryValue(QueryExpressionHelpers::apply(op, op1Value, op2Value)));
	};

	handleGenericType(op1.type, handleForType);
}

CompareExpressionLeftColumnRightColumnIR::CompareExpressionLeftColumnRightColumnIR(std::size_t lhs, std::size_t rhs, CompareOperator op)
	: lhs(lhs), rhs(rhs), op(op) {

}

void CompareExpressionLeftColumnRightColumnIR::execute(ExpressionExecutionEngine& executionEngine) {
	auto& lhsColumn = executionEngine.slottedColumnStorage[lhs];

	auto handleForType = [&](auto dummy) {
		using Type = decltype(dummy);
		Type lhsValue = lhsColumn->getUnderlyingStorage<Type>()[executionEngine.currentRowIndex];
		Type rhsValue = executionEngine.slottedColumnStorage[rhs]->getUnderlyingStorage<Type>()[executionEngine.currentRowIndex];
		executionEngine.evaluationStack.push(QueryValue(QueryExpressionHelpers::compare(op, lhsValue, rhsValue)));
	};

	handleGenericType(lhsColumn->type, handleForType);
}

ExpressionExecutionEngine::ExpressionExecutionEngine(Table& table) {

}

QueryExpressionCompilerVisitor::QueryExpressionCompilerVisitor(Table& table, ExpressionExecutionEngine& executionEngine)
	: table(table), executionEngine(executionEngine) {

}

void QueryExpressionCompilerVisitor::compile(QueryExpression* rootExpression) {
	rootExpression->accept(*this, nullptr);

	executionEngine.slottedColumnStorage.resize(executionEngine.columnNameToSlot.size());
	for (auto& column : executionEngine.columnNameToSlot) {
		executionEngine.slottedColumnStorage[column.second] = &table.getColumn(column.first);
	}

	if (typeEvaluationStack.size() != 1) {
		throw std::runtime_error("Expected one value on the stack.");
	}
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryRootExpression* expression) {
	expression->root->accept(*this, expression);
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryColumnReferenceExpression* expression) {
	if (executionEngine.columnNameToSlot.count(expression->name) == 0) {
		executionEngine.columnNameToSlot[expression->name] = nextColumnSlot++;
	}

	executionEngine.instructions.push_back(std::make_unique<ColumnReferenceExpressionIR>(
		executionEngine.columnNameToSlot[expression->name]));

	typeEvaluationStack.push(table.getColumn(expression->name).type);
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryValueExpression* expression) {
	executionEngine.instructions.push_back(std::make_unique<QueryValueExpressionIR>(expression->value));
	typeEvaluationStack.push(expression->value.type);
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryAndExpression* expression) {
	expression->lhs->accept(*this, expression);
	expression->rhs->accept(*this, expression);

	if (typeEvaluationStack.size() < 2) {
		throw std::runtime_error("Expected two values on the stack.");
	}

	auto op2 = typeEvaluationStack.top();
	typeEvaluationStack.pop();

	auto op1 = typeEvaluationStack.top();
	typeEvaluationStack.pop();

	if (op1 != op2) {
		throw std::runtime_error("Different types.");
	}

	if (op1 != ColumnType::Bool) {
		throw std::runtime_error("Expected bool type.");
	}

	executionEngine.instructions.push_back(std::make_unique<AndExpressionIR>());
	typeEvaluationStack.push(ColumnType::Bool);
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryCompareExpression* expression) {
	expression->lhs->accept(*this, expression);
	expression->rhs->accept(*this, expression);

	auto op2 = typeEvaluationStack.top();
	typeEvaluationStack.pop();

	auto op1 = typeEvaluationStack.top();
	typeEvaluationStack.pop();

	if (op1 != op2) {
		throw std::runtime_error("Different types.");
	}

	typeEvaluationStack.push(ColumnType::Bool);

	auto& instructions = executionEngine.instructions;

	auto& rhs = instructions[instructions.size() - 1];
	auto& lhs = instructions[instructions.size() - 2];

	auto rhsValue = dynamic_cast<QueryValueExpressionIR*>(rhs.get());
	auto rhsColumn = dynamic_cast<ColumnReferenceExpressionIR*>(rhs.get());
	auto lhsValue = dynamic_cast<QueryValueExpressionIR*>(lhs.get());
	auto lhsColumn = dynamic_cast<ColumnReferenceExpressionIR*>(lhs.get());

	if (lhsValue != nullptr && rhsColumn != nullptr) {
		executionEngine.instructions.erase(executionEngine.instructions.end() - 1);
		executionEngine.instructions.erase(executionEngine.instructions.end() - 1);

		auto handleForType = [&](auto dummy) {
			using Type = decltype(dummy);
			executionEngine.instructions.push_back(std::make_unique<CompareExpressionLeftValueKnownTypeRightColumnIR<Type>>(
				lhsValue->value.getValue<Type>(),
				rhsColumn->columnSlot,
				expression->op));
		};

		handleGenericType(lhsValue->value.type, handleForType);
	} else if (lhsColumn != nullptr && rhsValue != nullptr) {
		executionEngine.instructions.erase(executionEngine.instructions.end() - 1);
		executionEngine.instructions.erase(executionEngine.instructions.end() - 1);

		auto handleForType = [&](auto dummy) {
			using Type = decltype(dummy);
			executionEngine.instructions.push_back(std::make_unique<CompareExpressionLeftColumnRightValueKnownTypeIR<Type>>(
				lhsColumn->columnSlot,
				rhsValue->value.getValue<Type>(),
				expression->op));
		};

		handleGenericType(rhsValue->value.type, handleForType);
	} else if (lhsColumn != nullptr && rhsColumn != nullptr) {
		executionEngine.instructions.erase(executionEngine.instructions.end() - 1);
		executionEngine.instructions.erase(executionEngine.instructions.end() - 1);

		executionEngine.instructions.push_back(std::make_unique<CompareExpressionLeftColumnRightColumnIR>(
			lhsColumn->columnSlot,
			rhsColumn->columnSlot,
			expression->op));
	} else {
		executionEngine.instructions.push_back(std::make_unique<CompareExpressionIR>(expression->op));
	}
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryMathExpression* expression) {
	expression->lhs->accept(*this, expression);
	expression->rhs->accept(*this, expression);

	auto op2 = typeEvaluationStack.top();
	typeEvaluationStack.pop();

	auto op1 = typeEvaluationStack.top();
	typeEvaluationStack.pop();

	if (op1 != op2) {
		throw std::runtime_error("Different types.");
	}

	if (!(op1 == ColumnType::Float32 || op1 == ColumnType::Int32)) {
		throw std::runtime_error("Only Int32 and Float32 supported.");
	}

	typeEvaluationStack.push(op1);
	executionEngine.instructions.push_back(std::make_unique<MathOperationExpressionIR>(expression->op));
}
