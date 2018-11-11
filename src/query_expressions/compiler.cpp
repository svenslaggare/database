#include "compiler.h"
#include "helpers.h"
#include "ir_optimizer.h"

QueryExpressionCompilerVisitor::QueryExpressionCompilerVisitor(Table& table, ExpressionExecutionEngine& executionEngine, bool optimize)
	: mTable(table), mExecutionEngine(executionEngine), mOptimize(optimize) {

}

void QueryExpressionCompilerVisitor::compile(QueryExpression* rootExpression) {
	rootExpression->accept(*this, nullptr);
	if (mOptimize) {
		ExpressionIROptimizer optimizer(mExecutionEngine);
		optimizer.optimize();
	}

	mExecutionEngine.fillSlots(mTable);

	if (mTypeEvaluationStack.size() != 1) {
		throw std::runtime_error("Expected one value on the stack.");
	}

	mExecutionEngine.setExpressionType(mTypeEvaluationStack.top());
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryRootExpression* expression) {
	expression->root->accept(*this, expression);
}

void QueryExpressionCompilerVisitor::visit(QueryExpression* parent, QueryColumnReferenceExpression* expression) {
	mExecutionEngine.addInstruction(std::make_unique<ColumnReferenceExpressionIR>(mExecutionEngine.getSlot(expression->name)));
	mTypeEvaluationStack.push(mTable.getColumn(expression->name).type());
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
