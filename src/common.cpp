#include "common.h"

namespace {
	std::vector<std::uint8_t> createBinaryRepresentation(std::int32_t value) {
		std::vector<uint8_t> binary;
		binary.resize(sizeof(std::int32_t));
		*reinterpret_cast<std::int32_t*>(binary.data())	= value;
		return binary;
	}

	std::vector<std::uint8_t> createBinaryRepresentation(float value) {
		std::vector<uint8_t> binary;
		binary.resize(sizeof(std::int32_t));
		*reinterpret_cast<float*>(binary.data()) = value;
		return binary;
	}

	void setBinaryRepresentation(std::uint8_t* data, std::int32_t value) {
		*reinterpret_cast<std::int32_t*>(data)	= value;
	}

	void setBinaryRepresentation(std::uint8_t* data, float value) {
		*reinterpret_cast<float*>(data)	= value;
	}
}

QueryValue::QueryValue()
	: type(ColumnType::Int32) {
	setBinaryRepresentation(data, 0);
}

QueryValue::QueryValue(std::int32_t value)
	: type(ColumnType::Int32) {
	setBinaryRepresentation(data, value);
}

QueryValue::QueryValue(float value)
	: type(ColumnType::Float32) {
	setBinaryRepresentation(data, value);
}

QueryValue::QueryValue(bool value)
	: type(ColumnType::Bool) {
	setBinaryRepresentation(data, value);
}