#include "helpers.h"
#include "../query_expressions/expressions.h"
#include "../query_expressions/compiler.h"
#include "../storage.h"
#include "../table.h"
#include "../query.h"
#include "../helpers.h"

#include <algorithm>

namespace {
	inline void addColumnToResult(std::vector<VirtualColumn*> columnsStorage, QueryResult& result, std::size_t rowIndex, std::size_t columnIndex) {
		auto& columnStorage = *columnsStorage[columnIndex]->storage();
		auto& resultStorage = result.columns[columnIndex];

		auto handleForType = [&](auto dummy) {
			using Type = decltype(dummy);
			resultStorage.getUnderlyingStorage<Type>().push_back(columnStorage.getUnderlyingStorage<Type>()[rowIndex]);
		};

		handleGenericType(columnStorage.type(), handleForType);
	}
}

ExpressionExecutionEngine ExecutorHelpers::compile(VirtualTableContainer& tableContainer,
												   const std::string& mainTable,
												   QueryExpression* rootExpression,
												   const DatabaseConfiguration& config,
												   std::size_t numReturnValues) {
	ExpressionExecutionEngine executionEngine;
	QueryExpressionCompilerVisitor expressionCompilerVisitor(
		tableContainer,
		mainTable,
		executionEngine,
		config.optimizeExpressions,
		numReturnValues);

	expressionCompilerVisitor.compile(rootExpression);
	return executionEngine;
}

void ExecutorHelpers::forEachRowFiltered(VirtualTable& table,
										 ExpressionExecutionEngine& filterExecutionEngine,
										 std::function<void (std::size_t)> applyRow) {
	for (std::size_t rowIndex = 0; rowIndex < table.numRows(); rowIndex++) {
		filterExecutionEngine.execute(rowIndex);
		if (filterExecutionEngine.popEvaluation().getValue<bool>()) {
			applyRow(rowIndex);
		}
	}
}

void ExecutorHelpers::addRowToResult(std::vector<VirtualColumn*> columnsStorage, QueryResult& result, std::size_t rowIndex) {
	for (std::size_t columnIndex = 0; columnIndex < columnsStorage.size(); columnIndex++) {
		addColumnToResult(columnsStorage, result, rowIndex, columnIndex);
	}
}

void ExecutorHelpers::addRowToResult(std::vector<std::unique_ptr<ExpressionExecutionEngine>>& projections,
									 ReducedProjections& reducedProjections,
									 QueryResult& result,
									 std::size_t rowIndex) {
	std::size_t projectionIndex = 0;
	for (auto& projection : projections) {
		if (reducedProjections.storage[projectionIndex] == nullptr) {
			projection->execute(rowIndex);

			auto resultValue = projection->popEvaluation();

			auto& resultStorage = result.columns[projectionIndex];
			auto handleForType = [&](auto dummy) {
				using Type = decltype(dummy);
				resultStorage.getUnderlyingStorage<Type>().push_back(resultValue.getValue<Type>());
			};

			handleGenericType(resultValue.type, handleForType);
		} else {
			addColumnToResult(reducedProjections.storage, result, rowIndex, projectionIndex);
		}

		projectionIndex++;
	}
}

void ExecutorHelpers::orderResult(const std::vector<ColumnType>& orderingDataTypes,
								  const std::vector<OrderingColumn>& ordering,
							   	  const std::vector<std::vector<RawQueryValue>>& orderingData,
							   	  QueryResult& result) {
	// Find the indices of the ordering
	std::vector<std::size_t> sortedIndices;
	{
		Timing timing("sort: ");
		for (std::size_t i = 0; i < orderingData[0].size(); i++) {
			sortedIndices.push_back(i);
		}

		handleGenericType(orderingDataTypes.front(), [&](auto dummy) -> void {
			using Type = decltype(dummy);
			std::sort(
				sortedIndices.begin(),
				sortedIndices.end(),
				[&](std::size_t x, std::size_t y) {
					for (std::size_t columnIndex = 0; columnIndex < orderingDataTypes.size(); columnIndex++) {
						auto lhs = orderingData[columnIndex][x].getValue<Type>();
						auto rhs = orderingData[columnIndex][y].getValue<Type>();

						if (ordering[columnIndex].descending) {
							if (lhs > rhs) {
								return true;
							} else if (lhs < rhs) {
								return false;
							}
						} else {
							if (lhs < rhs) {
								return true;
							} else if (lhs > rhs) {
								return false;
							}
						}
					}

					return false;
				});
		});


//		if (ordering.front().descending) {
//			handleGenericType(orderingDataTypes.front(), [&](auto dummy) -> void {
//				using Type = decltype(dummy);
//				std::sort(
//					sortedIndices.begin(),
//					sortedIndices.end(),
//					[&](std::size_t x, std::size_t y) {
//						return orderingData[0][x].getValue<Type>() > orderingData[0][y].getValue<Type>();
//					});
//			});
//		} else {
//			handleGenericType(orderingDataTypes.front(), [&](auto dummy) -> void {
//				using Type = decltype(dummy);
//				std::sort(
//					sortedIndices.begin(),
//					sortedIndices.end(),
//					[&](std::size_t x, std::size_t y) {
//						return orderingData[0][x].getValue<Type>() < orderingData[0][y].getValue<Type>();
//					});
//			});
//		}
	}

	// Now update the result
	{
		Timing timing("updateResult: ");
		for (auto& column : result.columns) {
			handleGenericType(column.type(), [&](auto dummy) -> void {
				using Type = decltype(dummy);

				ColumnStorage sortedValues(column.type());
				auto& underlyingStorageOriginal = column.getUnderlyingStorage<Type>();
				auto& underlyingStorageSorted = sortedValues.getUnderlyingStorage<Type>();
				underlyingStorageSorted.reserve(underlyingStorageOriginal.size());

				for (std::size_t index : sortedIndices) {
					underlyingStorageSorted.push_back(underlyingStorageOriginal[index]);
				}

				column = std::move(sortedValues);
			});
		}
	}
}

void ExecutorHelpers::copyRow(VirtualTable& table, std::vector<ColumnStorage>& resultsStorage, std::size_t rowIndex) {
	std::size_t columnIndex = 0;
	for (auto& column : table.underlying().schema().columns()) {
		auto columnStorage = table.getColumn(column.name()).storage();
		auto& resultStorage = resultsStorage[columnIndex];

		auto handleForType = [&](auto dummy) {
			using Type = decltype(dummy);
			resultStorage.getUnderlyingStorage<Type>().push_back(columnStorage->getUnderlyingStorage<Type>()[rowIndex]);
		};

		handleGenericType(columnStorage->type(), handleForType);
		columnIndex++;
	}
}

ReducedProjections::ReducedProjections(const std::string& mainTable)
	: mainTable(mainTable) {

}

void ReducedProjections::tryReduce(std::vector<std::unique_ptr<QueryExpression>>& projections,
								   std::function<VirtualColumn* (const std::string&)> getColumnStorage) {
	allReduced = true;

	for (auto& projection : projections) {
		auto column = dynamic_cast<QueryColumnReferenceExpression*>(projection.get());
		if (column != nullptr) {
			columns.push_back(column->name);
		} else {
			columns.emplace_back("");
			allReduced = false;
		}
	}

	for (auto& column : columns) {
		if (!column.empty()) {
			storage.push_back(getColumnStorage(column));
		} else {
			storage.push_back(nullptr);
		}
	}
}

void ReducedProjections::tryReduce(std::vector<std::unique_ptr<QueryExpression>>& projections, VirtualTableContainer& tableContainer) {
	tryReduce(
		projections,
		[&](const std::string& column) {
			auto columnParts = QueryExpressionHelpers::splitColumnName(column, mainTable);
			return &tableContainer.getTable(columnParts.first).getColumn(columnParts.second);
		});
}

std::int64_t ReducedProjections::indexOfColumn(const std::string& name) const {
	std::size_t currentIndex = 0;
	for (auto& column : columns) {
		if (column == name) {
			return currentIndex;
		}

		currentIndex++;
	}

	return -1;
}

VirtualColumn* ReducedProjections::getStorage(const std::string& name) const {
	return storage[indexOfColumn(name)];
}

void ReducedProjections::clear() {
	for (auto& column : storage) {
		column = nullptr;
	}

	for (auto& column : columns) {
		column = "";
	}

	allReduced = false;
}
