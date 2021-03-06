#include "update_operation.h"
#include "expression_execution.h"
#include "../query.h"
#include "index_scanner.h"
#include "../query_expressions/ir_optimizer.h"

#include <iostream>

UpdateOperationExecutor::UpdateOperationExecutor(VirtualTable& table,
												 QueryUpdateOperation* operation,
												 std::vector<std::unique_ptr<ExpressionExecutionEngine>>& setExecutionEngines,
												 ExpressionExecutionEngine& filterExecutionEngine)
	: mTable(table),
	  mOperation(operation),
	  mSetExecutionEngines(setExecutionEngines),
	  mFilterExecutionEngine(filterExecutionEngine) {

}

bool UpdateOperationExecutor::tryExecuteTreeIndexScan() {
	TreeIndexScanner treeIndexScanner;

	auto possibleIndexScans = treeIndexScanner.findPossibleIndexScans(mTable, mFilterExecutionEngine);
	if (!possibleIndexScans.empty()) {
		auto& indexScan = possibleIndexScans[0];
		std::cout << "Using index: " << indexScan.index.column().name() << std::endl;

		treeIndexScanner.execute(mTable, indexScan, mWorkingStorage, mWorkingRowIndexStorage);
		mFilterExecutionEngine.makeCompareAlwaysTrue(indexScan.instructionIndex);
	} else {
		return false;
	}

	ExpressionIROptimizer optimizer(mFilterExecutionEngine);
	optimizer.optimize();

	// Change storage to the result of the index scan
	mTable.setStorage(mWorkingStorage);
	return true;
}

void UpdateOperationExecutor::execute() {
	tryExecuteTreeIndexScan();

	ExecutorHelpers::forEachRowFiltered(
		mTable,
		mFilterExecutionEngine,
		[&](std::size_t rowIndex) {
			std::size_t setIndex = 0;
			for (auto& setExecutionEngine : mSetExecutionEngines) {
				auto& setColumnName = mOperation->sets[setIndex]->column;

				setExecutionEngine->execute(rowIndex);
				auto newValue = setExecutionEngine->popEvaluation();

				std::size_t realRowIndex = rowIndex;
				if (!mWorkingRowIndexStorage.empty()) {
					realRowIndex = mWorkingRowIndexStorage[rowIndex];
				}

				auto handleForType = [&](auto dummy) {
					using Type = decltype(dummy);
					auto& underlyingStorage = mTable.underlying()
						.getColumn(setColumnName)
						.getUnderlyingStorage<Type>();

					Type oldValueRaw = underlyingStorage[realRowIndex];
					Type newValueRaw = newValue.getValue<Type>();
					mTable.underlying().updateIndices(
						setColumnName,
						oldValueRaw,
						newValueRaw,
						realRowIndex);

					underlyingStorage[realRowIndex] = newValueRaw;
				};

				handleGenericType(newValue.type, handleForType);
				setIndex++;
			}
		});
}
