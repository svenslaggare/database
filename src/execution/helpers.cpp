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

ExpressionExecutionEngine ExecutorHelpers::compile(VirtualTable& table, QueryExpression* rootExpression) {
	ExpressionExecutionEngine executionEngine;
	QueryExpressionCompilerVisitor expressionCompilerVisitor(table, executionEngine);
	expressionCompilerVisitor.compile(rootExpression);
	return executionEngine;
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

void ExecutorHelpers::orderResult(ColumnType orderDataType, const std::vector<RawQueryValue>& orderingData, bool descending, QueryResult& result) {
	// Find the indices of the ordering
	std::vector<std::size_t> sortedIndices;
	{
		Timing timing("sort: ");
		for (std::size_t i = 0; i < orderingData.size(); i++) {
			sortedIndices.push_back(i);
		}

		if (descending) {
			handleGenericType(orderDataType, [&](auto dummy) -> void {
				using Type = decltype(dummy);
				std::sort(
					sortedIndices.begin(),
					sortedIndices.end(),
					[&](std::size_t x, std::size_t y) {
						return orderingData[x].getValue<Type>() > orderingData[y].getValue<Type>();
					});
			});
		} else {
			handleGenericType(orderDataType, [&](auto dummy) -> void {
				using Type = decltype(dummy);
				std::sort(
					sortedIndices.begin(),
					sortedIndices.end(),
					[&](std::size_t x, std::size_t y) {
						return orderingData[x].getValue<Type>() < orderingData[y].getValue<Type>();
					});
			});
		}
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

void ReducedProjections::tryReduce(std::vector<std::unique_ptr<QueryExpression>>& projections, VirtualTable& table) {
	tryReduce(projections, [&](const std::string& column) { return &table.getColumn(column); });
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
