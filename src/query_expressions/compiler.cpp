#include "compiler.h"
#include "helpers.h"

QueryExpressionCompilerVisitor::QueryExpressionCompilerVisitor(Table& table, ExpressionExecutionEngine& executionEngine, bool optimize)
	: mTable(table), mExecutionEngine(executionEngine), mOptimize(optimize) {

}

void QueryExpressionCompilerVisitor::compile(QueryExpression* rootExpression) {
	rootExpression->accept(*this, nullptr);

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

	mExecutionEngine.addInstruction(std::make_unique<AndExpressionIR>());
	mTypeEvaluationStack.push(ColumnType::Bool);
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

	auto& instructions = mExecutionEngine.instructions();
	auto& rhs = instructions[instructions.size() - 1];
	auto& lhs = instructions[instructions.size() - 2];

	auto rhsValue = dynamic_cast<QueryValueExpressionIR*>(rhs.get());
	auto rhsColumn = dynamic_cast<ColumnReferenceExpressionIR*>(rhs.get());
	auto lhsValue = dynamic_cast<QueryValueExpressionIR*>(lhs.get());
	auto lhsColumn = dynamic_cast<ColumnReferenceExpressionIR*>(lhs.get());

	if (mOptimize) {
		if (lhsValue != nullptr && rhsColumn != nullptr) {
			mExecutionEngine.removeLastInstruction();
			mExecutionEngine.removeLastInstruction();

			auto handleForType = [&](auto dummy) {
				using Type = decltype(dummy);
				mExecutionEngine.addInstruction(std::make_unique<CompareExpressionLeftValueKnownTypeRightColumnIR<Type>>(
					lhsValue->value.getValue<Type>(),
					rhsColumn->columnSlot,
					expression->op));
			};

			handleGenericType(lhsValue->value.type, handleForType);
			return;
		} else if (lhsColumn != nullptr && rhsValue != nullptr) {
			mExecutionEngine.removeLastInstruction();
			mExecutionEngine.removeLastInstruction();

			auto handleForType = [&](auto dummy) {
				using Type = decltype(dummy);
				mExecutionEngine.addInstruction(std::make_unique<CompareExpressionLeftColumnRightValueKnownTypeIR<Type>>(
					lhsColumn->columnSlot,
					rhsValue->value.getValue<Type>(),
					expression->op));
			};

			handleGenericType(rhsValue->value.type, handleForType);
			return;
		} else if (lhsColumn != nullptr && rhsColumn != nullptr) {
			mExecutionEngine.removeLastInstruction();
			mExecutionEngine.removeLastInstruction();
			mExecutionEngine.addInstruction(std::make_unique<CompareExpressionLeftColumnRightColumnIR>(
				lhsColumn->columnSlot,
				rhsColumn->columnSlot,
				expression->op));
			return;
		}
	}

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
