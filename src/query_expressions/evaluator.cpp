#include "evaluator.h"
#include "helpers.h"

QueryExpressionEvaluatorVisitor::QueryExpressionEvaluatorVisitor(const Table& table, std::size_t rowIndex)
	: table(table), rowIndex(rowIndex) {

}

void QueryExpressionEvaluatorVisitor::visit(QueryExpression* parent, QueryRootExpression* expression) {
	expression->root->accept(*this, expression);
}

void QueryExpressionEvaluatorVisitor::visit(QueryExpression* parent, QueryColumnReferenceExpression* expression) {
	evaluationStack.push(QueryExpressionHelpers::getValueForColumn(table.getColumn(expression->name), rowIndex));
}

void QueryExpressionEvaluatorVisitor::visit(QueryExpression* parent, QueryValueExpression* expression) {
	evaluationStack.push(expression->value);
}

void QueryExpressionEvaluatorVisitor::visit(QueryExpression* parent, QueryAndExpression* expression) {
	expression->lhs->accept(*this, expression);
	expression->rhs->accept(*this, expression);

	if (evaluationStack.size() < 2) {
		throw std::runtime_error("Expected two values on the stack.");
	}

	auto op2 = std::move(evaluationStack.top());
	evaluationStack.pop();

	auto op1 = std::move(evaluationStack.top());
	evaluationStack.pop();

	if (op1.type != op2.type) {
		throw std::runtime_error("Different types.");
	}

	if (op1.type != ColumnType::Bool) {
		throw std::runtime_error("Expected bool type.");
	}

	evaluationStack.push(QueryValue(op1.getValue<bool>() && op2.getValue<bool>()));
}

void QueryExpressionEvaluatorVisitor::visit(QueryExpression* parent, QueryCompareExpression* expression) {
	expression->lhs->accept(*this, expression);
	expression->rhs->accept(*this, expression);

	if (evaluationStack.size() < 2) {
		throw std::runtime_error("Expected two values on the stack.");
	}

	auto op2 = std::move(evaluationStack.top());
	evaluationStack.pop();

	auto op1 = std::move(evaluationStack.top());
	evaluationStack.pop();

	if (op1.type != op2.type) {
		throw std::runtime_error("Different types.");
	}

	auto handleForType = [&](auto dummy) {
		using Type = decltype(dummy);
		auto op1Value = op1.getValue<Type>();
		auto op2Value = op2.getValue<Type>();
		evaluationStack.push(QueryValue(QueryExpressionHelpers::compare(expression->op, op1Value, op2Value)));
	};

	handleGenericType(op1.type, handleForType);
}