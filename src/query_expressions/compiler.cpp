#include "compiler.h"
#include "../column_storage.h"
#include "helpers.h"

QueryExpressionCompilerVisitor::QueryExpressionCompilerVisitor(Table& table, ExpressionExecutionEngine& executionEngine)
	: table(table), executionEngine(executionEngine) {

}

void QueryExpressionCompilerVisitor::compile(QueryExpression* rootExpression) {
	rootExpression->accept(*this, nullptr);

	executionEngine.fillSlots(table);

	if (typeEvaluationStack.size() != 1) {
		throw std::runtime_error("Expected one value on the stack.");
	}
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryRootExpression* expression) {
	expression->root->accept(*this, expression);
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryColumnReferenceExpression* expression) {
	executionEngine.addInstruction(std::make_unique<ColumnReferenceExpressionIR>(executionEngine.getSlot(expression->name)));
	typeEvaluationStack.push(table.getColumn(expression->name).type);
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryValueExpression* expression) {
	executionEngine.addInstruction(std::make_unique<QueryValueExpressionIR>(expression->value));
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

	executionEngine.addInstruction(std::make_unique<AndExpressionIR>());
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

	auto& instructions = executionEngine.instructions();
	auto& rhs = instructions[instructions.size() - 1];
	auto& lhs = instructions[instructions.size() - 2];

	auto rhsValue = dynamic_cast<QueryValueExpressionIR*>(rhs.get());
	auto rhsColumn = dynamic_cast<ColumnReferenceExpressionIR*>(rhs.get());
	auto lhsValue = dynamic_cast<QueryValueExpressionIR*>(lhs.get());
	auto lhsColumn = dynamic_cast<ColumnReferenceExpressionIR*>(lhs.get());

	if (lhsValue != nullptr && rhsColumn != nullptr) {
		executionEngine.removeLast();
		executionEngine.removeLast();

		auto handleForType = [&](auto dummy) {
			using Type = decltype(dummy);
			executionEngine.addInstruction(std::make_unique<CompareExpressionLeftValueKnownTypeRightColumnIR<Type>>(
				lhsValue->value.getValue<Type>(),
				rhsColumn->columnSlot,
				expression->op));
		};

		handleGenericType(lhsValue->value.type, handleForType);
	} else if (lhsColumn != nullptr && rhsValue != nullptr) {
		executionEngine.removeLast();
		executionEngine.removeLast();

		auto handleForType = [&](auto dummy) {
			using Type = decltype(dummy);
			executionEngine.addInstruction(std::make_unique<CompareExpressionLeftColumnRightValueKnownTypeIR<Type>>(
				lhsColumn->columnSlot,
				rhsValue->value.getValue<Type>(),
				expression->op));
		};

		handleGenericType(rhsValue->value.type, handleForType);
	} else if (lhsColumn != nullptr && rhsColumn != nullptr) {
		executionEngine.removeLast();
		executionEngine.removeLast();
		executionEngine.addInstruction(std::make_unique<CompareExpressionLeftColumnRightColumnIR>(
			lhsColumn->columnSlot,
			rhsColumn->columnSlot,
			expression->op));
	} else {
		executionEngine.addInstruction(std::make_unique<CompareExpressionIR>(expression->op));
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
	executionEngine.addInstruction(std::make_unique<MathOperationExpressionIR>(expression->op));
}
