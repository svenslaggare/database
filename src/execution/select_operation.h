#pragma once
#include <vector>
#include <memory>
#include "helpers.h"

struct ExpressionExecutionEngine;
struct QuerySelectOperation;
struct QueryResult;

/**
 * Represents an executor for select operation
 */
class SelectOperationExecutor {
private:
	DatabaseEngine& mDatabaseEngine;
	VirtualTable& mTable;
	QuerySelectOperation* mOperation;
	std::vector<std::unique_ptr<ExpressionExecutionEngine>>& mProjectionExecutionEngines;
	ExpressionExecutionEngine& mFilterExecutionEngine;
	QueryResult& mResult;

	bool mOrderResult = false;
	std::unique_ptr<ExpressionExecutionEngine> mOrderExecutionEngine;
	std::vector<RawQueryValue> mOrderingData;

	std::vector<ColumnStorage> mWorkingStorage;
	ReducedProjections mReducedProjections;

	std::vector<std::function<bool ()>> mExecutors;

	bool hasReducedToOneInstruction() const;

	void addForOrdering(std::size_t rowIndex);

	bool executeNoFilter();

	bool executeFilterLeftIsColumn();
	bool executeFilterRightIsColumn();
	bool executeFilterBothColumn();

	bool tryExecuteTreeIndexScan();
	bool executeDefault();
public:
	/**
	 * Creates a new select operation executor
	 * @param databaseEngine The database engine
	 * @param table The table
	 * @param operation The operation
	 * @param projectionExecutionEngines The projection execution engines
	 * @param filterExecutionEngine The filter execution engine
	 * @param result The result
	 */
	SelectOperationExecutor(DatabaseEngine& databaseEngine,
							VirtualTable& table,
							QuerySelectOperation* operation,
							std::vector<std::unique_ptr<ExpressionExecutionEngine>>& projectionExecutionEngines,
							ExpressionExecutionEngine& filterExecutionEngine,
							QueryResult& result);

	/**
	 * Executes the operation
	 */
	void execute();
};