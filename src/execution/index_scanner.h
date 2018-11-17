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

	using OnColumnDefined = std::function<void (std::size_t, const ColumnDefinition&)>;
	using ApplyRow = std::function<void (std::size_t, std::size_t, ColumnStorage*, bool)>;

	/**
	 * Executes the given index scan
	 * @param context The scan context
	 * @param indexScan The scan to execute
	 * @param applyRow Called on each row with (rowIndex, columnIndex, columnStorage)
	 */
	void execute(IndexScanContext& context,
				 const PossibleIndexScan& indexScan,
				 OnColumnDefined onColumnDef,
				 ApplyRow applyRow);

	/**
	 * Executes the given index scan and copies the results
	 * @param context The scan context
	 * @param indexScan The scan to execute
	 * @param resultsStorage Where to store the result
	 */
	void execute(IndexScanContext& context,
				 const PossibleIndexScan& indexScan,
				 std::vector<ColumnStorage>& resultsStorage);

	/**
	 * Executes the given index scan and copies the results
	 * @param context The scan context
	 * @param indexScan The scan to execute
	 * @param resultsStorage Where to store the result
	 * @param resultRowIndices Saves which real row each result row corresponds to
	 */
	void execute(IndexScanContext& context,
				 const PossibleIndexScan& indexScan,
				 std::vector<ColumnStorage>& resultsStorage,
				 std::vector<std::size_t>& resultRowIndices);
};