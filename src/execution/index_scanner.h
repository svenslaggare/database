#pragma once

#include "../common.h"
#include "../storage.h"

class TreeIndex;
class Table;
class VirtualTable;
class ExpressionExecutionEngine;

/**
 * Represents a possible index scan
 */
struct PossibleIndexScan {
	std::size_t instructionIndex;

	TreeIndex& index;

	CompareOperator op;
	QueryValue indexSearchValue;

	/**
	 * Creates a new index scan
	 * @param instructionIndex The instruction that the scan will replace
	 * @param index The index
	 * @param op The search operator
	 * @param indexSearchValue The index search value
	 */
	PossibleIndexScan(std::size_t instructionIndex,
					  TreeIndex& index,
					  CompareOperator op,
					  QueryValue indexSearchValue);
};

/**
 * Represents the context for an index scan
 */
struct IndexScanContext {
	VirtualTable& table;
	ExpressionExecutionEngine& executionEngine;

	/**
	 * Creates a new context
	 * @param table The table
	 * @param executionEngine The execution engine
	 */
	IndexScanContext(VirtualTable& table, ExpressionExecutionEngine& executionEngine);
};

/**
 * Represents a tree index scanner
 */
class TreeIndexScanner {
public:
	/**
	 * Finds the possible index scans
	 * @param context The scan context
	 */
	std::vector<PossibleIndexScan> findPossibleIndexScans(IndexScanContext& context);

	/**
	 * Executes the given index scan
	 * @param context The scan context
	 * @param indexScan The scan to execute
	 * @param resultsStorage The results storage
	 */
	void execute(IndexScanContext& context,
				 const PossibleIndexScan& indexScan,
				 std::vector<ColumnStorage>& resultsStorage);
};