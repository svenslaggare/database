#include "expressions.h"
#include "visitor.h"

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

QueryAndExpression::QueryAndExpression(std::unique_ptr<QueryBoolExpression> lhs, std::unique_ptr<QueryBoolExpression> rhs)
	: lhs(std::move(lhs)), rhs(std::move(rhs)) {

}

void QueryAndExpression::accept(QueryExpressionVisitor& visitor, QueryExpression* parent) {
	visitor.visit(parent, this);
}

void QueryAndExpression::update(QueryExpression* oldExpression,	std::unique_ptr<QueryExpression> newExpression) {
	auto newExpressionBool = dynamic_cast<QueryBoolExpression*>(newExpression.get());
	if (newExpressionBool == nullptr) {
		throw std::runtime_error("Not bool expression.");
	}

	if (oldExpression == lhs.get()) {
		lhs = std::unique_ptr<QueryBoolExpression>((QueryBoolExpression*)newExpression.release());
		return;
	}

	if (oldExpression == rhs.get()) {
		rhs = std::unique_ptr<QueryBoolExpression>((QueryBoolExpression*)newExpression.release());
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