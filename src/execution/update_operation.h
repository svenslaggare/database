#pragma once

#include <vector>
#include <memory>
#include "helpers.h"

struct ExpressionExecutionEngine;
struct QueryUpdateOperation;

/**
 * Represents an executor for an update operation
 */
class UpdateOperationExecutor {
private:
	VirtualTable& mTable;
	QueryUpdateOperation* mOperation;
	std::vector<std::unique_ptr<ExpressionExecutionEngine>>& mSetExecutionEngines;
	ExpressionExecutionEngine& mFilterExecutionEngine;

	std::vector<ColumnStorage> mWorkingStorage;
	std::vector<std::size_t> mWorkingRowIndexStorage;

	bool tryExecuteTreeIndexScan();
public:
	/**
	 * Creates a new update operation executor
	 * @param table The table
	 * @param operation The operation
	 * @param setExecutionEngines The set execution engines
	 * @param filterExecutionEngine The filter execution engine
	 */
	UpdateOperationExecutor(VirtualTable& table,
							QueryUpdateOperation* operation,
							std::vector<std::unique_ptr<ExpressionExecutionEngine>>& setExecutionEngines,
							ExpressionExecutionEngine& filterExecutionEngine);

	/**
	 * Executes the operation
	 */
	void execute();
};