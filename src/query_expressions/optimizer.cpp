#include "optimizer.h"
#include "../table.h"

QueryExpressionOptimizerVisitor::QueryExpressionOptimizerVisitor(Table& table)
	: table(table) {

}

void QueryExpressionOptimizerVisitor::optimize(QueryExpression* rootExpression) {
	rootExpression->accept(*this, nullptr);

	optimizerData.columnStorage.resize(optimizerData.columnIndices.size());
	for (auto& column : optimizerData.columnIndices) {
		optimizerData.columnStorage[column.second] = &table.getColumn(column.first);
	}
}

void QueryExpressionOptimizerVisitor::visit(QueryExpression* parent, QueryRootExpression* expression) {
	expression->root->accept(*this, expression);
}

void QueryExpressionOptimizerVisitor::visit(QueryExpression* parent, QueryColumnReferenceExpression* expression) {
	if (optimizerData.columnIndices.count(expression->name) == 0) {
		optimizerData.columnIndices[expression->name] = nextColumnIndex++;
	}

	parent->update(
		expression,
		std::make_unique<QueryColumnReferenceSlottedExpression>(optimizerData.columnIndices[expression->name]));
}

void QueryExpressionOptimizerVisitor::visit(QueryExpression* parent, QueryValueExpression* expression) {

}

void QueryExpressionOptimizerVisitor::visit(QueryExpression* parent, QueryAndExpression* expression) {
	expression->lhs->accept(*this, expression);
	expression->rhs->accept(*this, expression);
}

void QueryExpressionOptimizerVisitor::visit(QueryExpression* parent, QueryCompareExpression* expression) {
	expression->lhs->accept(*this, expression);
	expression->rhs->accept(*this, expression);

	auto lhsValue = dynamic_cast<QueryValueExpression*>(expression->lhs.get());
	auto lhsColumn = dynamic_cast<QueryColumnReferenceSlottedExpression*>(expression->lhs.get());
	auto rhsValue = dynamic_cast<QueryValueExpression*>(expression->rhs.get());
	auto rhsColumn = dynamic_cast<QueryColumnReferenceSlottedExpression*>(expression->rhs.get());

	if (lhsValue != nullptr && rhsColumn != nullptr) {
		if (lhsValue->value.type == ColumnType::Int32) {
			parent->update(
				expression,
				std::make_unique<QueryCompareLeftValueInt32RightColumnExpression>(
					lhsValue->value.getValue<std::int32_t>(),
					rhsColumn->index,
					expression->op));
		} else {
			parent->update(
				expression,
				std::make_unique<QueryCompareLeftValueRightColumnExpression>(lhsValue->value, rhsColumn->index, expression->op));
		}
	} else if (lhsColumn != nullptr && rhsValue != nullptr) {
		parent->update(
			expression,
			std::make_unique<QueryCompareLeftColumnRightValueExpression>(lhsColumn->index, rhsValue->value, expression->op));
	} else if (lhsColumn != nullptr && rhsColumn != nullptr) {
		parent->update(
			expression,
			std::make_unique<QueryCompareLeftColumnRightColumnExpression>(lhsColumn->index, rhsColumn->index, expression->op));
	}
}

void QueryExpressionOptimizerVisitor::visit(QueryExpression* parent, QueryColumnReferenceSlottedExpression* expression) {

}

void QueryExpressionOptimizerVisitor::visit(QueryExpression* parent, QueryCompareLeftValueRightColumnExpression* expression) {

}

void QueryExpressionOptimizerVisitor::visit(QueryExpression* parent, QueryCompareLeftColumnRightValueExpression* expression) {

}

void QueryExpressionOptimizerVisitor::visit(QueryExpression* parent, QueryCompareLeftColumnRightColumnExpression* expression) {

}

void QueryExpressionOptimizerVisitor::visit(QueryExpression* parent, QueryCompareLeftValueInt32RightColumnExpression* expression) {

}