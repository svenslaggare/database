#include "evaluator.h"
#include "helpers.h"
#include "optimizer.h"

QueryExpressionEvaluatorVisitor::QueryExpressionEvaluatorVisitor(const Table& table, QueryExpressionOptimizerData& optimizerData, std::size_t rowIndex)
	: table(table), optimizerData(optimizerData), rowIndex(rowIndex) {

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

void QueryExpressionEvaluatorVisitor::visit(QueryExpression* parent, QueryColumnReferenceSlottedExpression* expression) {
	evaluationStack.push(QueryExpressionHelpers::getValueForColumn(
		*optimizerData.columnStorage[expression->index],
		rowIndex));
}

void QueryExpressionEvaluatorVisitor::visit(QueryExpression* parent, QueryCompareLeftValueRightColumnExpression* expression) {
	auto handleForType = [&](auto dummy) {
		using Type = decltype(dummy);
		Type lhs = expression->lhs.getValue<Type>();
		Type rhs = optimizerData.columnStorage[expression->rhs]->getUnderlyingStorage<Type>()[rowIndex];
		evaluationStack.push(QueryValue(QueryExpressionHelpers::compare(expression->op, lhs, rhs)));
	};

	handleGenericType(expression->lhs.type, handleForType);
}

void QueryExpressionEvaluatorVisitor::visit(QueryExpression* parent, QueryCompareLeftValueInt32RightColumnExpression* expression) {
	auto rhs = optimizerData.columnStorage[expression->rhs]->getUnderlyingStorage<std::int32_t>()[rowIndex];
	evaluationStack.push(QueryValue(QueryExpressionHelpers::compare(expression->op, expression->lhs, rhs)));
}

void QueryExpressionEvaluatorVisitor::visit(QueryExpression* parent, QueryCompareLeftColumnRightValueExpression* expression) {
	auto handleForType = [&](auto dummy) {
		using Type = decltype(dummy);
		Type lhs = optimizerData.columnStorage[expression->lhs]->getUnderlyingStorage<Type>()[rowIndex];
		Type rhs = expression->rhs.getValue<Type>();
		evaluationStack.push(QueryValue(QueryExpressionHelpers::compare(expression->op, lhs, rhs)));
	};

	handleGenericType(expression->rhs.type, handleForType);
}

void QueryExpressionEvaluatorVisitor::visit(QueryExpression* parent, QueryCompareLeftColumnRightColumnExpression* expression) {
	auto& lhsColumn = *optimizerData.columnStorage[expression->lhs];

	auto handleForType = [&](auto dummy) {
		using Type = decltype(dummy);
		Type lhs = lhsColumn.getUnderlyingStorage<Type>()[rowIndex];
		Type rhs = optimizerData.columnStorage[expression->rhs]->getUnderlyingStorage<Type>()[rowIndex];
		evaluationStack.push(QueryValue(QueryExpressionHelpers::compare(expression->op, lhs, rhs)));
	};

	handleGenericType(lhsColumn.type, handleForType);
}