#include "internal_expressions.h"
#include "visitor.h"

QueryColumnReferenceSlottedExpression::QueryColumnReferenceSlottedExpression(std::size_t index)
	: index(index) {

}

void QueryColumnReferenceSlottedExpression::accept(QueryExpressionVisitor& visitor, QueryExpression* parent) {
	visitor.visit(parent, this);
}

QueryCompareLeftValueRightColumnExpression::QueryCompareLeftValueRightColumnExpression(QueryValue lhs, std::size_t rhs, CompareOperator op)
	: lhs(lhs), rhs(rhs), op(op) {

}

void QueryCompareLeftValueRightColumnExpression::accept(QueryExpressionVisitor& visitor, QueryExpression* parent) {
	visitor.visit(parent, this);
}

QueryCompareLeftColumnRightValueExpression::QueryCompareLeftColumnRightValueExpression(std::size_t lhs, QueryValue rhs, CompareOperator op)
	: lhs(lhs), rhs(rhs), op(op) {

}

void QueryCompareLeftColumnRightValueExpression::accept(QueryExpressionVisitor& visitor, QueryExpression* parent) {
	visitor.visit(parent, this);
}

QueryCompareLeftColumnRightColumnExpression::QueryCompareLeftColumnRightColumnExpression(std::size_t lhs, std::size_t rhs, CompareOperator op)
	: lhs(lhs), rhs(rhs), op(op) {

}

void QueryCompareLeftColumnRightColumnExpression::accept(QueryExpressionVisitor& visitor, QueryExpression* parent) {
	visitor.visit(parent, this);
}

QueryCompareLeftValueInt32RightColumnExpression::QueryCompareLeftValueInt32RightColumnExpression(std::int32_t lhs, std::size_t rhs, CompareOperator op)
	: lhs(lhs), rhs(rhs), op(op) {

}

void QueryCompareLeftValueInt32RightColumnExpression::accept(QueryExpressionVisitor& visitor, QueryExpression* parent) {
	visitor.visit(parent, this);
}