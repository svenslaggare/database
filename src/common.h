#pragma once
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <functional>

/**
 * The primitive column types
 */
enum class ColumnType {
	Bool,
	Int32,
	Float32,
};

/**
 * Helper functions for database column types
 */
namespace DatabaseColumnTypeHelpers {
	template<typename T>
	inline ColumnType getType() {
		throw std::runtime_error("Not implemented.");
	}

	template<>
	constexpr inline ColumnType getType<bool>() {
		return ColumnType::Bool;
	}

	template<>
	constexpr inline ColumnType getType<std::int32_t>() {
		return ColumnType::Int32;
	}

	template<>
	constexpr inline ColumnType getType<float>() {
		return ColumnType::Float32;
	}
}

template<typename T>
T handleTypeResult(ColumnType type, std::function<T ()> boolHandle, std::function<T ()> int32Handle, std::function<T ()> float32Handle) {
	switch (type) {
		case ColumnType::Bool:
			return boolHandle();
		case ColumnType::Int32: {
			return int32Handle();
		}
		case ColumnType::Float32: {
			return float32Handle();
		}
	}
}

#define handleGenericTypeResult(T, type, handle) handleTypeResult<T>(type,\
	 [&]() { return handle(false); },\
	 [&]() { return handle((std::int32_t)0); },\
	 [&]() { return handle((float)0); })

#define handleGenericType(type, handle) handleGenericTypeResult(void, type, handle)

inline bool anyType(std::function<bool ()> boolHandle, std::function<bool ()> int32Handle, std::function<bool ()> float32Handle) {
	return boolHandle() || int32Handle() || float32Handle();
}

#define anyGenericType(handle) anyType(\
	 [&]() { return handle(false); },\
	 [&]() { return handle((std::int32_t)0); },\
	 [&]() { return handle((float)0); })

constexpr std::size_t MAX_VALUE_SIZE = std::max(std::max(sizeof(bool), sizeof(std::int32_t)), sizeof(float));

struct QueryValue {
	ColumnType type;
	std::uint8_t data[MAX_VALUE_SIZE];

	explicit QueryValue();
	explicit QueryValue(std::int32_t value);
	explicit QueryValue(float value);
	explicit QueryValue(bool value);

	template<typename T>
	T getValue() {
		if (type != DatabaseColumnTypeHelpers::getType<T>()) {
			throw std::runtime_error("Wrong type.");
		}

		return *reinterpret_cast<T*>(&data);
	}

	template<typename T>
	T getValue() const {
		if (type != DatabaseColumnTypeHelpers::getType<T>()) {
			throw std::runtime_error("Wrong type.");
		}

		return *reinterpret_cast<const T*>(&data);
	}
};


/**
 * Represents a database compare operator
 */
enum class CompareOperator {
	Equal,
	NotEqual,
	LessThan,
	LessThanOrEqual,
	GreaterThan,
	GreaterThanOrEqual
};