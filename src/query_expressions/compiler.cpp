#include "compiler.h"
#include "helpers.h"
#include "ir_optimizer.h"

namespace {

}

QueryExpressionCompilerVisitor::QueryExpressionCompilerVisitor(VirtualTableContainer& tableContainer,
															   const std::string& mainTable,
															   ExpressionExecutionEngine& executionEngine,
															   bool optimize,
															   std::size_t numReturnValues)
	: mTableContainer(tableContainer),
	  mMainTable(tableContainer.getTable(mainTable)),
	  mExecutionEngine(executionEngine),
	  mOptimize(optimize),
	  mNumReturnValues(numReturnValues) {

}

void QueryExpressionCompilerVisitor::compile(QueryExpression* rootExpression) {
	rootExpression->accept(*this, nullptr);
	if (mOptimize) {
		ExpressionIROptimizer optimizer(mExecutionEngine);
		optimizer.optimize();
	}

	mExecutionEngine.fillSlots(mTableContainer);

	if (mTypeEvaluationStack.size() != mNumReturnValues) {
		throw std::runtime_error(
			"Expected " + std::to_string(mNumReturnValues)
			+ " value(s) on the stack, but got "
			+ std::to_string(mTypeEvaluationStack.size()) + ".");
	}

	auto copyTypeEvaluationStack = mTypeEvaluationStack;
	std::vector<ColumnType> expressionTypes;
	while (!copyTypeEvaluationStack.empty()) {
		expressionTypes.push_back(copyTypeEvaluationStack.top());
		copyTypeEvaluationStack.pop();
	}

	mExecutionEngine.setExpressionTypes(expressionTypes);
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryRootExpression* expression) {
	expression->root->accept(*this, expression);
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryMultipleExpressions* expression) {
	for (auto it = expression->expressions.rbegin(); it != expression->expressions.rend(); ++it) {
		auto& subExpression = *it;
		subExpression->accept(*this, expression);
	}
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryColumnReferenceExpression* expression) {
	auto columnNameParts = QueryExpressionHelpers::splitColumnName(
		expression->name,
		mMainTable.underlying().schema().name());

	auto columnSlot = mExecutionEngine.getSlot(columnNameParts.first, columnNameParts.second);

	mExecutionEngine.addInstruction(std::make_unique<ColumnReferenceExpressionIR>(columnSlot));
	mTypeEvaluationStack.push(mTableContainer.getTable(columnNameParts.first).getColumn(columnNameParts.second).type());
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryValueExpression* expression) {
	mExecutionEngine.addInstruction(std::make_unique<QueryValueExpressionIR>(expression->value));
	mTypeEvaluationStack.push(expression->value.type);
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryAndExpression* expression) {
	expression->lhs->accept(*this, expression);
	expression->rhs->accept(*this, expression);

	if (mTypeEvaluationStack.size() < 2) {
		throw std::runtime_error("Expected two values on the stack.");
	}

	auto op2 = mTypeEvaluationStack.top();
	mTypeEvaluationStack.pop();

	auto op1 = mTypeEvaluationStack.top();
	mTypeEvaluationStack.pop();

	if (op1 != op2) {
		throw std::runtime_error("Different types.");
	}

	if (op1 != ColumnType::Bool) {
		throw std::runtime_error("Expected bool type.");
	}

	mTypeEvaluationStack.push(ColumnType::Bool);
	mExecutionEngine.addInstruction(std::make_unique<AndExpressionIR>());
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryCompareExpression* expression) {
	expression->lhs->accept(*this, expression);
	expression->rhs->accept(*this, expression);

	auto op2 = mTypeEvaluationStack.top();
	mTypeEvaluationStack.pop();

	auto op1 = mTypeEvaluationStack.top();
	mTypeEvaluationStack.pop();

	if (op1 != op2) {
		throw std::runtime_error("Different types.");
	}

	mTypeEvaluationStack.push(ColumnType::Bool);
	mExecutionEngine.addInstruction(std::make_unique<CompareExpressionIR>(expression->op));
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryMathExpression* expression) {
	expression->lhs->accept(*this, expression);
	expression->rhs->accept(*this, expression);

	auto op2 = mTypeEvaluationStack.top();
	mTypeEvaluationStack.pop();

	auto op1 = mTypeEvaluationStack.top();
	mTypeEvaluationStack.pop();

	if (op1 != op2) {
		throw std::runtime_error("Different types.");
	}

	if (!(op1 == ColumnType::Float32 || op1 == ColumnType::Int32)) {
		throw std::runtime_error("Only Int32 and Float32 supported.");
	}

	mTypeEvaluationStack.push(op1);
	mExecutionEngine.addInstruction(std::make_unique<MathOperationExpressionIR>(expression->op));
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryAssignExpression* expression) {
	expression->value->accept(*this, expression);
	auto& column = mMainTable.getColumn(expression->column);

	if (column.type() != mTypeEvaluationStack.top()) {
		throw std::runtime_error(
			"Expected type '" +
			std::to_string((int)column.type()) + "' but got type '"
			+ std::to_string((int)mTypeEvaluationStack.top()) + "'.");
	}
}
