#include "helpers.h"

QueryValue QueryExpressionHelpers::getValueForColumn(const ColumnStorage& storage, std::size_t rowIndex) {
	auto handleForType = [&](auto dummy) {
		using Type = decltype(dummy);
		return QueryValue(storage.getUnderlyingStorage<Type>().at(rowIndex));
	};

	return handleGenericTypeResult(QueryValue, storage.type, handleForType);
}