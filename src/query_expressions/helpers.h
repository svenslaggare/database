#pragma once
#include "../common.h"
#include "../storage.h"
#include "../table.h"
#include "../query.h"

/**
 * Contains helper functions for query expressions
 */
namespace QueryExpressionHelpers {
	/**
	 * Applies the given compare operation
	 * @tparam T The type of the values
	 * @param op The operation
	 * @param x The lhs
	 * @param y The rhs
	 */
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

	/**
	 * Applies the given math operation
	 * @tparam T The type of the values
	 * @param op The operation
	 * @param x The lhs
	 * @param y The rhs
	 */
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

	/**
	 * Returns the value of the given row in the given column
	 * @param storage The storage of the column
	 * @param rowIndex The index of the row
	 */
	QueryValue getValueForColumn(const ColumnStorage& storage, std::size_t rowIndex);

	/**
	 * Returns the compare operator if it was on the other side (if on lhs return operator with same effect but on the rhs)
	 * @param op The operator
	 */
	CompareOperator otherSideCompareOp(CompareOperator op);
}