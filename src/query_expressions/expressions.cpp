#include "expressions.h"
#include "visitor.h"

void QueryExpression::update(QueryExpression* oldExpression, std::unique_ptr<QueryExpression> newExpression) {

}

QueryRootExpression::QueryRootExpression(std::unique_ptr<QueryExpression> expression)
	: root(std::move(expression)) {

}

void QueryRootExpression::accept(QueryExpressionVisitor& visitor, QueryExpression* parent) {
	visitor.visit(parent, this);
}

void QueryRootExpression::update(QueryExpression* oldExpression, std::unique_ptr<QueryExpression> newExpression) {
	if (oldExpression == root.get()) {
		root = std::move(newExpression);
		return;;
	}

	throw std::runtime_error("old expression is not expression.");
}

QueryMultipleExpressions::QueryMultipleExpressions(std::vector<std::unique_ptr<QueryExpression>> expressions)
	: expressions(std::move(expressions)) {

}

void QueryMultipleExpressions::accept(QueryExpressionVisitor& visitor, QueryExpression* parent) {
	visitor.visit(parent, this);
}

void QueryMultipleExpressions::update(QueryExpression* oldExpression, std::unique_ptr<QueryExpression> newExpression) {
	for (auto& expression : expressions) {
		if (expression.get() == oldExpression) {
			expression = std::move(newExpression);
		}
	}

	throw std::runtime_error("old expression not sub-expression.");
}

QueryColumnReferenceExpression::QueryColumnReferenceExpression(const std::string& name)
	: name(name) {

}

void QueryColumnReferenceExpression::accept(QueryExpressionVisitor& visitor, QueryExpression* parent) {
	visitor.visit(parent, this);
}

QueryValueExpression::QueryValueExpression(QueryValue value)
	: value(std::move(value)) {

}

void QueryValueExpression::accept(QueryExpressionVisitor& visitor, QueryExpression* parent) {
	visitor.visit(parent, this);
}

QueryAndExpression::QueryAndExpression(std::unique_ptr<QueryExpression> lhs, std::unique_ptr<QueryExpression> rhs)
	: lhs(std::move(lhs)), rhs(std::move(rhs)) {

}

void QueryAndExpression::accept(QueryExpressionVisitor& visitor, QueryExpression* parent) {
	visitor.visit(parent, this);
}

void QueryAndExpression::update(QueryExpression* oldExpression,	std::unique_ptr<QueryExpression> newExpression) {
	if (oldExpression == lhs.get()) {
		lhs = std::move(newExpression);
		return;
	}

	if (oldExpression == rhs.get()) {
		rhs = std::move(newExpression);
		return;
	}

	throw std::runtime_error("old expression not sub-expression.");
}

QueryCompareExpression::QueryCompareExpression(std::unique_ptr<QueryExpression> lhs,
											   std::unique_ptr<QueryExpression> rhs,
											   CompareOperator op)
	: lhs(std::move(lhs)), rhs(std::move(rhs)),	op(op) {

}

void QueryCompareExpression::accept(QueryExpressionVisitor& visitor, QueryExpression* parent) {
	visitor.visit(parent, this);
}

void QueryCompareExpression::update(QueryExpression* oldExpression, std::unique_ptr<QueryExpression> newExpression) {
	if (oldExpression == lhs.get()) {
		lhs = std::move(newExpression);
		return;;
	}

	if (oldExpression == rhs.get()) {
		rhs = std::move(newExpression);
		return;
	}

	throw std::runtime_error("old expression not sub-expression.");
}

std::vector<std::unique_ptr<QueryExpression>> QueryExpressionHelpers::createColumnReferences(std::initializer_list<std::string> columns) {
	std::vector<std::unique_ptr<QueryExpression>> references;

	for (auto& column : columns) {
		references.emplace_back(std::make_unique<QueryColumnReferenceExpression>(column));
	}

	return references;
}

QueryMathExpression::QueryMathExpression(std::unique_ptr<QueryExpression> lhs,
										 std::unique_ptr<QueryExpression> rhs,
										 MathOperator op)
	: lhs(std::move(lhs)), rhs(std::move(rhs)),	op(op) {

}

void QueryMathExpression::accept(QueryExpressionVisitor& visitor, QueryExpression* parent) {
	visitor.visit(parent, this);
}

void QueryMathExpression::update(QueryExpression* oldExpression, std::unique_ptr<QueryExpression> newExpression) {

}

QueryAssignExpression::QueryAssignExpression(std::string column, std::unique_ptr<QueryExpression> value)
	: column(column), value(std::move(value)) {

}

void QueryAssignExpression::accept(QueryExpressionVisitor& visitor, QueryExpression* parent) {
	visitor.visit(parent, this);
}

void QueryAssignExpression::update(QueryExpression* oldExpression, std::unique_ptr<QueryExpression> newExpression) {
	if (oldExpression == value.get()) {
		value = std::move(newExpression);
		return;
	}

	throw std::runtime_error("old expression not sub-expression.");
}
