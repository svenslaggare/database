#include <functional>
#include <iostream>
#include "index_scanner.h"
#include "../indices.h"
#include "expression_execution.h"

PossibleIndexScan::PossibleIndexScan(std::size_t instructionIndex,
									 TreeIndex& index,
									 CompareOperator op,
									 QueryValue indexSearchValue)
	: instructionIndex(instructionIndex),
	  index(index),
	  op(op),
	  indexSearchValue(indexSearchValue) {

}

IndexScanContext::IndexScanContext(VirtualTable& table, ExpressionExecutionEngine& executionEngine)
	: table(table), executionEngine(executionEngine) {

}

namespace {
	template<typename T>
	using TreeIndexIterator = typename TreeIndex::UnderlyingStorage<T>::const_iterator;

	template<typename T>
	using TreeIndexRange = std::pair<TreeIndexIterator<T>, TreeIndexIterator<T>>;

	template<typename T>
	TreeIndexRange<T> findTreeIndexIterators(const TreeIndex::UnderlyingStorage<T>& underlyingIndex,
											 CompareOperator op,
											 const T& indexSearchValue) {
		auto startIterator = underlyingIndex.end();
		auto endIterator = underlyingIndex.end();

		switch (op) {
			case CompareOperator::Equal:
				startIterator = underlyingIndex.lower_bound(indexSearchValue);
				endIterator = underlyingIndex.upper_bound(indexSearchValue);

				if (endIterator == underlyingIndex.end()) {
					startIterator = underlyingIndex.end();
					endIterator = underlyingIndex.end();
				}
				break;
			case CompareOperator::NotEqual:
				break;
			case CompareOperator::LessThan: {
				startIterator = underlyingIndex.begin();
				endIterator = underlyingIndex.lower_bound(indexSearchValue);

				if (endIterator == underlyingIndex.end()) {
					startIterator = underlyingIndex.end();
					endIterator = underlyingIndex.end();
				}
				break;
			}
			case CompareOperator::LessThanOrEqual:
				startIterator = underlyingIndex.begin();
				endIterator = underlyingIndex.upper_bound(indexSearchValue);

				if (endIterator == underlyingIndex.end()) {
					startIterator = underlyingIndex.end();
					endIterator = underlyingIndex.end();
				}
				break;
			case CompareOperator::GreaterThan:
				startIterator = underlyingIndex.upper_bound(indexSearchValue);
				endIterator = underlyingIndex.end();

				if (startIterator == underlyingIndex.end()) {
					startIterator = underlyingIndex.end();
					endIterator = underlyingIndex.end();
				}
				break;
			case CompareOperator::GreaterThanOrEqual:
				startIterator = underlyingIndex.lower_bound(indexSearchValue);
				endIterator = underlyingIndex.end();

				if (startIterator == underlyingIndex.end()) {
					startIterator = underlyingIndex.end();
					endIterator = underlyingIndex.end();
				}
				break;
		}

		return std::make_pair(startIterator, endIterator);
	}

	bool canTreeIndexScan(const TreeIndex& index, const std::string& column, CompareOperator op) {
		return index.columnName() == column && op != CompareOperator::NotEqual;
	}
}

std::vector<PossibleIndexScan> TreeIndexScanner::findPossibleIndexScans(IndexScanContext& context) {
	std::vector<PossibleIndexScan> possibleScans;

	for (std::size_t instructionIndex = 0; instructionIndex < context.executionEngine.instructions().size(); instructionIndex++) {
		auto instruction = context.executionEngine.instructions()[instructionIndex].get();

		auto tryAddIndexScan = [&](std::size_t columnSlot, CompareOperator op, QueryValue indexSearchValue) {
			for (auto& index : context.table.underlying().indices()) {
				if (canTreeIndexScan(*index, context.executionEngine.fromSlot(columnSlot), op)) {
					possibleScans.emplace_back(instructionIndex, *index, op, indexSearchValue);
				}
			}
		};

		auto handleForType = [&](auto dummy) {
			using Type = decltype(dummy);

			if (auto compareInstruction = dynamic_cast<CompareExpressionLeftColumnRightValueKnownTypeIR<Type>*>(instruction)) {
				tryAddIndexScan(
					compareInstruction->lhs,
					compareInstruction->op,
					QueryValue(compareInstruction->rhs));

				return true;
			} else if (auto compareInstruction = dynamic_cast<CompareExpressionLeftValueKnownTypeRightColumnIR<Type>*>(instruction)) {
				tryAddIndexScan(
					compareInstruction->rhs,
					QueryExpressionHelpers::otherSideCompareOp(compareInstruction->op),
					QueryValue(compareInstruction->lhs));

				return true;
			}

			return false;
		};

		anyGenericType(handleForType);
	}

	return possibleScans;
}

void TreeIndexScanner::execute(IndexScanContext& context,
							   const PossibleIndexScan& indexScan,
							   OnColumnDefined onColumnDef,
							   ApplyRow applyRow) {
	auto handleForType = [&](auto dummy) -> void {
		using Type = decltype(dummy);

		std::vector<ColumnStorage*> columnsStorage;
		std::size_t columnIndex = 0;
		for (auto& column : context.table.underlying().schema().columns()) {
			onColumnDef(columnIndex, column);
			columnsStorage.push_back(context.table.getColumn(column.name()).storage());
			columnIndex++;
		}

		auto& underlyingIndex = indexScan.index.getUnderlyingStorage<Type>();
		auto iteratorRange = findTreeIndexIterators(
			underlyingIndex,
			indexScan.op,
			indexScan.indexSearchValue.getValue<Type>());

		for (auto it = iteratorRange.first; it != iteratorRange.second; ++it) {
			auto rowIndex = it->second;

			bool isFirstColumn = true;
			for (std::size_t columnIndex = 0; columnIndex < columnsStorage.size(); columnIndex++) {
				applyRow(rowIndex, columnIndex, columnsStorage[columnIndex], isFirstColumn);
				isFirstColumn = false;
			}
		}
	};

	handleGenericType(indexScan.indexSearchValue.type, handleForType);
}

void TreeIndexScanner::execute(IndexScanContext& context,
							   const PossibleIndexScan& indexScan,
							   std::vector<ColumnStorage>& resultsStorage) {
	execute(
		context,
		indexScan,
		[&](std::size_t columnIndex, const ColumnDefinition& columnDefinition) {
			resultsStorage.emplace_back(columnDefinition);
		},
		[&](std::size_t rowIndex, std::size_t columnIndex, const ColumnStorage* columnStorage, bool isFirstColumn) {
			// Copy matched rows
			auto& resultStorage = resultsStorage[columnIndex];

			auto handleForType = [&](auto dummy) {
				using Type = decltype(dummy);
				resultStorage.getUnderlyingStorage<Type>().push_back(columnStorage->getUnderlyingStorage<Type>()[rowIndex]);
			};

			handleGenericType(columnStorage->type(), handleForType);
		});
}

void TreeIndexScanner::execute(IndexScanContext& context,
							   const PossibleIndexScan& indexScan,
							   std::vector<ColumnStorage>& resultsStorage,
							   std::vector<std::size_t>& resultRowIndices) {
	execute(
		context,
		indexScan,
		[&](std::size_t columnIndex, const ColumnDefinition& columnDefinition) {
			resultsStorage.emplace_back(columnDefinition);
		},
		[&](std::size_t rowIndex, std::size_t columnIndex, const ColumnStorage* columnStorage, bool isFirstColumn) {
			// Save which row it correspond to
			if (isFirstColumn) {
				resultRowIndices.push_back(rowIndex);
			}

			// Copy matched rows
			auto& resultStorage = resultsStorage[columnIndex];

			auto handleForType = [&](auto dummy) {
				using Type = decltype(dummy);
				resultStorage.getUnderlyingStorage<Type>().push_back(columnStorage->getUnderlyingStorage<Type>()[rowIndex]);
			};

			handleGenericType(columnStorage->type(), handleForType);
		});
}
