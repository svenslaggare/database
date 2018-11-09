#pragma once
#include <vector>
#include <memory>
#include "helpers.h"

struct Table;
struct ExpressionExecutionEngine;
struct QuerySelectOperation;
struct QueryResult;

/**
 * Represents an executor for select operation
 */
struct SelectOperationExecutor {
	Table& table;
	QuerySelectOperation* operation;
	std::vector<std::unique_ptr<ExpressionExecutionEngine>>& projectionExecutionEngines;
	ExpressionExecutionEngine& filterExecutionEngine;
	QueryResult& result;
	bool optimize;

	bool doOrdering = false;
	std::unique_ptr<ExpressionExecutionEngine> orderExecutionEngine;
	std::vector<RawQueryValue> orderingData;

	ReducedProjections reducedProjections;

	std::vector<std::function<bool ()>> executors;

	SelectOperationExecutor(Table& table,
							QuerySelectOperation* operation,
							std::vector<std::unique_ptr<ExpressionExecutionEngine>>& projectionExecutionEngines,
							ExpressionExecutionEngine& filterExecutionEngine,
							QueryResult& result,
							bool optimize = true);

	void addForOrdering(std::size_t rowIndex);
	void execute();
};