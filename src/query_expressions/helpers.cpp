#include "helpers.h"

QueryValue QueryExpressionHelpers::getValueForColumn(const ColumnStorage& storage, std::size_t rowIndex) {
	auto handleForType = [&](auto dummy) {
		using Type = decltype(dummy);
		return QueryValue(storage.getUnderlyingStorage<Type>().at(rowIndex));
	};

	return handleGenericTypeResult(QueryValue, storage.type(), handleForType);
}

CompareOperator QueryExpressionHelpers::otherSideCompareOp(CompareOperator op) {
	switch (op) {
		case CompareOperator::Equal:
			return CompareOperator::Equal;
		case CompareOperator::NotEqual:
			return CompareOperator::NotEqual;
		case CompareOperator::LessThan:
			return CompareOperator::GreaterThan;
		case CompareOperator::LessThanOrEqual:
			return CompareOperator::GreaterThanOrEqual;
		case CompareOperator::GreaterThan:
			return CompareOperator::LessThan;
		case CompareOperator::GreaterThanOrEqual:
			return CompareOperator::LessThanOrEqual;
	}
}
