#pragma once
#include <vector>
#include <memory>
#include "common.h"

struct ColumnDefinition;

template<typename T>
using UnderlyingColumnStorage = std::vector<T>;

/**
 * Represents the storage of database columns
 */
struct ColumnStorage {
	ColumnType type;
	std::unique_ptr<std::uint8_t[]> underlyingStorage;

	ColumnStorage(const ColumnDefinition& column);

	std::size_t size() const;

	template<typename T>
	UnderlyingColumnStorage<T>& getUnderlyingStorage() const {
		return *((UnderlyingColumnStorage<T>*)underlyingStorage.get());
	}
};