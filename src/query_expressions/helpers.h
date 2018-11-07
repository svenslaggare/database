#pragma once
#include "../common.h"
#include "../column_storage.h"
#include "../table.h"
#include "../query.h"

/**
 * Contains helper functions for query expressions
 */
namespace QueryExpressionHelpers {
	template<typename T>
	bool compare(CompareOperator op, const T& x, const T& y) {
		switch (op) {
			case CompareOperator::Equal:
				return x == y;
			case CompareOperator::NotEqual:
				return x != y;
			case CompareOperator::LessThan:
				return x < y;
			case CompareOperator::LessThanOrEqual:
				return x <= y;
			case CompareOperator::GreaterThan:
				return x > y;
			case CompareOperator::GreaterThanOrEqual:
				return x >= y;
		}
	}

	template<typename T>
	T apply(MathOperator op, const T& x, const T& y) {
		switch (op) {
			case MathOperator::Add:
				return x + y;
			case MathOperator::Sub:
				return x - y;
			case MathOperator::Mul:
				return x * y;
			case MathOperator::Div:
				return x / y;
		}
	}

	QueryValue getValueForColumn(const ColumnStorage& storage, std::size_t rowIndex);
	Row getRow(const Table& table, std::size_t rowIndex);
}