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

std::pair<std::string, std::string> QueryExpressionHelpers::splitColumnName(const std::string& name, const std::string& mainTable) {
	auto fullName = name;
	if (fullName.find('.') == std::string::npos) {
		fullName = mainTable + "." + fullName;
	}

	auto dotPosition = fullName.find('.');
	if (dotPosition == std::string::npos) {
		throw std::runtime_error("Invalid column name.");
	}

	return std::make_pair(fullName.substr(0, dotPosition), fullName.substr(dotPosition + 1));
}

std::string QueryExpressionHelpers::fullColumnName(const std::string& table, const std::string& column) {
	auto parts = QueryExpressionHelpers::splitColumnName(
		column,
		table);
	return parts.first + "." + parts.second;
}
