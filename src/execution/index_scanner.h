#pragma once

#include "../common.h"
#include "../storage.h"

class TreeIndex;
class Table;
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
 * Represents a tree index scanner
 */
class TreeIndexScanner {
public:
	/**
	 * Finds the possible index scans
	 * @param table The table
	 * @param executionEngine The execution engine
	 * @return
	 */
	std::vector<PossibleIndexScan> findPossibleIndexScans(const Table& table, ExpressionExecutionEngine& executionEngine);

	/**
	 * Executes the given index scan
	 * @param table The table
	 * @param executionEngine The execution engine
	 * @param indexScan The scan to execute
	 * @param resultsStorage The results storage
	 */
	void execute(const Table& table,
				 ExpressionExecutionEngine& executionEngine,
				 const PossibleIndexScan& indexScan,
				 std::vector<ColumnStorage>& resultsStorage);
};