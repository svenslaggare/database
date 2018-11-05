#include <iostream>
#include "executor.h"
#include "../query_expressions/optimizer.h"
#include "../table.h"
#include "../database_engine.h"
#include "../query_expressions/evaluator.h"
#include "../query_expressions/helpers.h"
#include "../query_expressions/compiler.h"

OperationExecutorVisitor::OperationExecutorVisitor(DatabaseEngine& databaseEngine, QueryResult& result)
	: databaseEngine(databaseEngine), result(result) {

}

namespace {
	inline std::vector<ColumnStorage*> getColumnsStorage(Table& table, const std::vector<std::string>& columns) {
		std::vector<ColumnStorage*> columnsStorage;
		columnsStorage.reserve(columns.size());

		for (auto& column : columns) {
			columnsStorage.push_back(&table.getColumn(column));
		}

		return columnsStorage;
	}

	inline void addRowToResult(std::vector<ColumnStorage*> columnsStorage, QueryResult& result, std::size_t rowIndex) {
		for (std::size_t columnIndex = 0; columnIndex < columnsStorage.size(); columnIndex++) {
			auto& columnStorage = *columnsStorage[columnIndex];
			auto& resultStorage = result.columns[columnIndex];

			auto handleForType = [&](auto dummy) {
				using Type = decltype(dummy);
				resultStorage.getUnderlyingStorage<Type>().push_back(columnStorage.getUnderlyingStorage<Type>()[rowIndex]);
			};

			handleGenericType(columnStorage.type, handleForType);
		}
	}

	struct SelectOperationExecutorVisitor : public QueryExpressionVisitor {
		QuerySelectOperation* selectOperation;
		Table& table;
		QueryExpressionOptimizerData& optimizerData;
		QueryResult& result;
		bool optimize;

		SelectOperationExecutorVisitor(DatabaseEngine& databaseEngine,
									   QuerySelectOperation* selectOperation,
									   QueryExpressionOptimizerData& optimizerData,
									   QueryResult& result,
									   bool optimize = true)
			: selectOperation(selectOperation),
			  table(databaseEngine.getTable(selectOperation->table)),
			  optimizerData(optimizerData),
			  result(result),
			  optimize(optimize) {
			for (auto& column : selectOperation->columns) {
				result.columns.emplace_back(table.schema().getDefinition(column));
			}
		}

		void defaultExecutor(QueryExpression* expression) {
			QueryExpressionEvaluatorVisitor expressionEvaluator(table, optimizerData, 0);

			auto columnsStorage = getColumnsStorage(table, selectOperation->columns);
			for (std::size_t rowIndex = 0; rowIndex < table.numRows(); rowIndex++) {
				expressionEvaluator.rowIndex = rowIndex;

				expression->accept(expressionEvaluator, nullptr);
				if (expressionEvaluator.evaluationStack.size() != 1) {
					throw std::runtime_error("Expected one value on the evaluation stack.");
				}

				if (expressionEvaluator.evaluationStack.top().getValue<bool>()) {
					addRowToResult(columnsStorage, result, rowIndex);
				}

				expressionEvaluator.evaluationStack.pop();
			}
		}

		virtual void visit(QueryExpression* parent, QueryRootExpression* expression) override {
			expression->root->accept(*this, expression);
		}

		virtual void visit(QueryExpression* parent, QueryColumnReferenceExpression* expression) override {
			defaultExecutor(expression);
		}

		virtual void visit(QueryExpression* parent, QueryValueExpression* expression) override {
			auto value = expression->value.getValue<bool>();
			if (value) {
				std::size_t columnIndex = 0;
				for (auto& column : selectOperation->columns) {
					auto& columnDefinition = table.schema().getDefinition(column);
					auto& resultStorage = result.columns[columnIndex];

					auto handleForType = [&](auto dummy) {
						using Type = decltype(dummy);
						resultStorage.getUnderlyingStorage<Type>() = table.getColumnValues<Type>(column);
					};

					handleGenericType(columnDefinition.type, handleForType);
					columnIndex++;
				}

//				auto columnsStorage = getColumnsStorage(table, selectOperation->columns);
//				for (std::size_t rowIndex = 0; rowIndex < table.numRows(); rowIndex++) {
//					addRowToResult(columnsStorage, result, rowIndex);
//				}
			}
		}

		virtual void visit(QueryExpression* parent, QueryAndExpression* expression) override {
			defaultExecutor(expression);
		}

		virtual void visit(QueryExpression* parent, QueryCompareExpression* expression) override {
			defaultExecutor(expression);
		}

		void executeLeftValueRightColumn(QueryExpression* expression, QueryValue lhs, const std::size_t& rhs, CompareOperator op) {
			if (!optimize) {
				defaultExecutor(expression);
				return;
			}

			auto& rhsColumn = *optimizerData.columnStorage[rhs];

			auto handleForType = [&](auto dummy) {
				using Type = decltype(dummy);
				auto& rhsColumnValues = rhsColumn.getUnderlyingStorage<Type>();
				auto lhsValue = lhs.getValue<Type>();

				auto columnsStorage = getColumnsStorage(table, selectOperation->columns);
				for (std::size_t rowIndex = 0; rowIndex < rhsColumnValues.size(); rowIndex++) {
					if (QueryExpressionHelpers::compare<Type>(op, lhsValue, rhsColumnValues[rowIndex])) {
						addRowToResult(columnsStorage, result, rowIndex);
					}
				}
			};

			handleGenericType(rhsColumn.type, handleForType);
		}

		virtual void visit(QueryExpression* parent, QueryCompareLeftValueRightColumnExpression* expression) override {
			executeLeftValueRightColumn(expression, expression->lhs, expression->rhs, expression->op);
		}

		virtual void visit(QueryExpression* parent, QueryCompareLeftValueInt32RightColumnExpression* expression) override {
			executeLeftValueRightColumn(expression, QueryValue(expression->lhs), expression->rhs, expression->op);
		}

		virtual void visit(QueryExpression* parent, QueryColumnReferenceSlottedExpression* expression) override {
			defaultExecutor(expression);
		}

		virtual void visit(QueryExpression* parent, QueryCompareLeftColumnRightValueExpression* expression) override {
			if (!optimize) {
				defaultExecutor(expression);
				return;
			}

			auto& lhsColumn = *optimizerData.columnStorage[expression->lhs];

			auto handleForType = [&](auto dummy) {
				using Type = decltype(dummy);
				auto& lhsColumnValues = lhsColumn.getUnderlyingStorage<Type>();
				auto rhsValue = expression->rhs.getValue<Type>();

				auto columnsStorage = getColumnsStorage(table, selectOperation->columns);
				for (std::size_t rowIndex = 0; rowIndex < lhsColumnValues.size(); rowIndex++) {
					if (QueryExpressionHelpers::compare<Type>(expression->op, lhsColumnValues[rowIndex], rhsValue)) {
						addRowToResult(columnsStorage, result, rowIndex);
					}
				}
			};

			handleGenericType(lhsColumn.type, handleForType);
		}

		virtual void visit(QueryExpression* parent, QueryCompareLeftColumnRightColumnExpression* expression) override {
			if (!optimize) {
				defaultExecutor(expression);
				return;
			}

			auto& lhsColumn = *optimizerData.columnStorage[expression->lhs];
			auto& rhsColumn = *optimizerData.columnStorage[expression->rhs];

			auto handleForType = [&](auto dummy) {
				using Type = decltype(dummy);
				auto& lhsColumnValues = lhsColumn.getUnderlyingStorage<Type>();
				auto& rhsColumnValues = rhsColumn.getUnderlyingStorage<Type>();

				auto columnsStorage = getColumnsStorage(table, selectOperation->columns);
				for (std::size_t rowIndex = 0; rowIndex < lhsColumnValues.size(); rowIndex++) {
					if (QueryExpressionHelpers::compare<Type>(expression->op, lhsColumnValues[rowIndex], rhsColumnValues[rowIndex])) {
						addRowToResult(columnsStorage, result, rowIndex);
					}
				}
			};

			handleGenericType(lhsColumn.type, handleForType);
		}
	};

	struct SelectOperationExecutor {
		Table& table;
		ExpressionExecutionEngine& executionEngine;
		QuerySelectOperation* operation;
		QueryResult& result;
		bool optimize;

		std::vector<ColumnStorage*> columnsStorage;
		std::vector<std::function<bool ()>> executors;

		SelectOperationExecutor(Table& table, ExpressionExecutionEngine& executionEngine, QuerySelectOperation* operation, QueryResult& result, bool optimize = true)
			: table(table), executionEngine(executionEngine), operation(operation), result(result), optimize(optimize) {
			columnsStorage = getColumnsStorage(table, operation->columns);
			for (auto& column : operation->columns) {
				result.columns.emplace_back(table.schema().getDefinition(column));
			}

			executors.emplace_back([this]() {
				if (this->executionEngine.instructions.size() == 1) {
					auto firstInstruction = this->executionEngine.instructions.front().get();
					if (auto instruction = dynamic_cast<QueryValueExpressionIR*>(firstInstruction)) {
						auto value = instruction->value.getValue<bool>();
						if (value) {
							std::size_t columnIndex = 0;
							for (auto& column : this->operation->columns) {
								auto& columnDefinition = this->table.schema().getDefinition(column);
								auto& resultStorage = this->result.columns[columnIndex];

								auto handleForType = [&](auto dummy) {
									using Type = decltype(dummy);
									resultStorage.getUnderlyingStorage<Type>() = this->table.getColumnValues<Type>(column);
								};

								handleGenericType(columnDefinition.type, handleForType);
								columnIndex++;
							}
						}

						return true;
					}
				}

				return false;
			});

			if (optimize) {
				executors.emplace_back([this]() {
					if (this->executionEngine.instructions.size() == 1) {
						auto firstInstruction = this->executionEngine.instructions.front().get();

						return anyGenericType([&](auto dummy) {
							using Type = decltype(dummy);
							if (auto instruction = dynamic_cast<CompareExpressionLeftValueKnownTypeRightColumnIR<Type>*>(firstInstruction)) {
								auto& rhsColumn = *(this->executionEngine.slottedColumnStorage[instruction->rhs]);
								auto& rhsColumnValues = rhsColumn.template getUnderlyingStorage<Type>();

								for (std::size_t rowIndex = 0; rowIndex < rhsColumnValues.size(); rowIndex++) {
									if (QueryExpressionHelpers::compare<Type>(instruction->op, instruction->lhs, rhsColumnValues[rowIndex])) {
										addRowToResult(this->columnsStorage, this->result, rowIndex);
									}
								}

								return true;
							}

							return false;
						});
					}

					return false;
				});

				executors.emplace_back([this]() {
					if (this->executionEngine.instructions.size() == 1) {
						auto firstInstruction = this->executionEngine.instructions.front().get();

						if (auto instruction = dynamic_cast<CompareExpressionLeftColumnRightColumnIR*>(firstInstruction)) {
							auto& lhsColumn = *this->executionEngine.slottedColumnStorage[instruction->lhs];
							auto& rhsColumn = *this->executionEngine.slottedColumnStorage[instruction->rhs];

							auto handleForType = [&](auto dummy) {
								using Type = decltype(dummy);
								auto& lhsColumnValues = lhsColumn.getUnderlyingStorage<Type>();
								auto& rhsColumnValues = rhsColumn.getUnderlyingStorage<Type>();

								auto columnsStorage = getColumnsStorage(this->table, this->operation->columns);
								for (std::size_t rowIndex = 0; rowIndex < lhsColumnValues.size(); rowIndex++) {
									if (QueryExpressionHelpers::compare<Type>(instruction->op, lhsColumnValues[rowIndex], rhsColumnValues[rowIndex])) {
										addRowToResult(columnsStorage, this->result, rowIndex);
									}
								}
							};

							handleGenericType(lhsColumn.type, handleForType);
							return true;
						}
					}

					return false;
				});
			}
		}

		void execute() {
			// First, try the optimized executors.
			for (auto& executor : executors) {
				if (executor()) {
					return;
				}
			}

			// Fallback to default executor
			for (std::size_t rowIndex = 0; rowIndex < table.numRows(); rowIndex++) {
				executionEngine.currentRowIndex = rowIndex;

				for (auto& instruction : executionEngine.instructions) {
					instruction->execute(executionEngine);
				}

				if (executionEngine.evaluationStack.top().getValue<bool>()) {
					addRowToResult(columnsStorage, result, rowIndex);
				}

				executionEngine.evaluationStack.pop();
			}
		}
	};
}

void OperationExecutorVisitor::visit(QuerySelectOperation* operation) {
	std::unique_ptr<QueryExpression> rootExpression;
	if (operation->filter) {
		rootExpression = std::make_unique<QueryRootExpression>(std::move(operation->filter));
	} else {
		rootExpression = std::make_unique<QueryValueExpression>(QueryValue(true));
	}

//	QueryExpressionOptimizerVisitor expressionOptimizer(databaseEngine.getTable(operation->table));
//	expressionOptimizer.optimize(rootExpression.get());
//
//	SelectOperationExecutorVisitor selectOperationExecutor(
//		databaseEngine,
//		operation,
//		expressionOptimizer.optimizerData,
//		result,
//		false);
//
//	rootExpression->accept(selectOperationExecutor, nullptr);

	auto& table = databaseEngine.getTable(operation->table);
	ExpressionExecutionEngine executionEngine(table);
	QueryExpressionCompilerVisitor expressionCompilerVisitor(table, executionEngine);
	expressionCompilerVisitor.compile(rootExpression.get());

	SelectOperationExecutor executor(
		table,
		executionEngine,
		operation,
		result,
		false);

	executor.execute();
}

void OperationExecutorVisitor::visit(QueryInsertOperation* operation) {
	auto& table = databaseEngine.getTable(operation->table);
	std::size_t columnIndex = 0;
	if (operation->columns.size() != table.schema().columns.size()) {
		throw std::runtime_error("Wrong number of columns.");
	}

	for (auto& column : operation->columns) {
		auto& columnDefinition = table.schema().getDefinition(column);

		auto insertForType = [&](auto dummy) {
			using ColumnType = decltype(dummy);

			for (auto& columnValues : operation->values) {
				auto& value = columnValues[columnIndex];
				if (value.type != columnDefinition.type) {
					throw std::runtime_error("Wrong type.");
				}

				table.insertColumn(column, value.getValue<ColumnType>());
			}
		};

		handleGenericType(columnDefinition.type, insertForType);
		columnIndex++;
	}
}
