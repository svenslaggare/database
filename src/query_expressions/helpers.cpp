#include "helpers.h"

QueryValue QueryExpressionHelpers::getValueForColumn(const ColumnStorage& storage, std::size_t rowIndex) {
	auto handleForType = [&](auto dummy) {
		using Type = decltype(dummy);
		return QueryValue(storage.getUnderlyingStorage<Type>().at(rowIndex));
	};

	return handleGenericTypeResult(QueryValue, storage.type, handleForType);
}

Row QueryExpressionHelpers::getRow(const Table& table, std::size_t rowIndex) {
	std::unordered_map<std::string, QueryValue> rowColumns;
	for (auto& column : table.schema().columns) {
		rowColumns[column.name] = getValueForColumn(table.getColumn(column.name), rowIndex);
	}

	return Row(std::move(rowColumns));
}
