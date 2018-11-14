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
				endIterator = underlyingIndex.upper_bound(indexSearchValue);

				if (endIterator != underlyingIndex.end()) {
					--endIterator;
				} else {
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
				startIterator = underlyingIndex.upper_bound(indexSearchValue);
				endIterator = underlyingIndex.end();

				if (startIterator != underlyingIndex.end()) {
					--startIterator;
				} else {
					startIterator = underlyingIndex.end();
					endIterator = underlyingIndex.end();
				}
				break;
		}

		return std::make_pair(startIterator, endIterator);
	}

	bool canTreeIndexScan(const TreeIndex& index, const std::string& column, CompareOperator op) {
		return index.column().name() == column && op != CompareOperator::NotEqual;
	}
}

std::vector<PossibleIndexScan> TreeIndexScanner::findPossibleIndexScans(const Table& table, ExpressionExecutionEngine& executionEngine) {
	std::vector<PossibleIndexScan> possibleScans;

	for (std::size_t instructionIndex = 0; instructionIndex < executionEngine.instructions().size(); instructionIndex++) {
		auto instruction = executionEngine.instructions()[instructionIndex].get();

		auto tryAddIndexScan = [&](std::size_t columnSlot, CompareOperator op, QueryValue indexSearchValue) {
			for (auto& index : table.indices()) {
				if (canTreeIndexScan(*index, executionEngine.fromSlot(columnSlot), op)) {
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

void TreeIndexScanner::execute(const Table& table,
							   ExpressionExecutionEngine& executionEngine,
							   const PossibleIndexScan& indexScan,
							   std::vector<ColumnStorage>& resultsStorage) {
	auto handleSearchForType = [&](auto dummy) -> void {
		using Type = decltype(dummy);

		std::vector<const ColumnStorage*> columnsStorage;
		for (auto& column : table.schema().columns()) {
			resultsStorage.emplace_back(column);
			columnsStorage.push_back(&table.getColumn(column.name()));
		}

		auto& underlyingIndex = indexScan.index.getUnderlyingStorage<Type>();
		auto iteratorRange = findTreeIndexIterators(
			underlyingIndex,
			indexScan.op,
			indexScan.indexSearchValue.getValue<Type>());

		for (auto it = iteratorRange.first; it != iteratorRange.second; ++it) {
			auto rowIndex = it->second;

			// Copy matched rows
			for (std::size_t columnIndex = 0; columnIndex < columnsStorage.size(); columnIndex++) {
				auto& columnStorage = columnsStorage[columnIndex];
				auto& resultStorage = resultsStorage[columnIndex];

				auto handleForType = [&](auto dummy) {
					using Type = decltype(dummy);
					resultStorage.getUnderlyingStorage<Type>().push_back(columnStorage->getUnderlyingStorage<Type>()[rowIndex]);
				};

				handleGenericType(columnStorage->type(), handleForType);
			}
		}
	};

	handleGenericType(indexScan.indexSearchValue.type, handleSearchForType);
}
