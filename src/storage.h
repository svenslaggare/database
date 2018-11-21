#pragma once
#include <vector>
#include <memory>
#include "common.h"

struct ColumnDefinition;

template<typename T>
using UnderlyingColumnStorage = std::vector<T>;

/**
 * Represents the storage of a column
 */
class ColumnStorage {
private:
	ColumnType mType;
	std::unique_ptr<std::uint8_t[]> mUnderlyingStorage;
public:
	/**
	 * Creates new storage for a column of the given type
	 * @param type The type
	 */
	explicit ColumnStorage(ColumnType type);

	/**
	 * Creates new storage for the given column
	 * @param column The definition of the column
	 */
	explicit ColumnStorage(const ColumnDefinition& column);

	ColumnStorage(const ColumnStorage&) = delete;
	ColumnStorage& operator=(const ColumnStorage&) = delete;

	ColumnStorage(ColumnStorage&&) = default;
	ColumnStorage& operator=(ColumnStorage&&) = default;

	/**
	 * Returns the type of the column
	 */
	ColumnType type() const;

	/**
	 * Returns the number of rows stored
	 */
	std::size_t size() const;

	/**
	 * Returns the value at the given index
	 * @param index The index
	 */
	QueryValue getValue(std::size_t index) const;

	/**
	 * Create underlying storage for the given type
	 * @param type The type of the data
	 */
	static std::unique_ptr<std::uint8_t[]> createUnderlyingStorage(ColumnType type);

	/**
	 * Returns the underlying storage
	 * @tparam T The type of the element
	 */
	template<typename T>
	UnderlyingColumnStorage<T>& getUnderlyingStorage() const {
		return *((UnderlyingColumnStorage<T>*)mUnderlyingStorage.get());
	}
};