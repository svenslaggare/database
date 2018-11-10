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

	/**
	 * Returns the type of the column
	 */
	ColumnType type() const;

	/**
	 * Create underlying storage for the given type
	 * @param type The type of the data
	 */
	static std::unique_ptr<std::uint8_t[]> createUnderlyingStorage(ColumnType type);

	/**
	 * Returns the number of rows stored
	 */
	std::size_t size() const;

	/**
	 * Returns the underlying storage
	 * @tparam T The type of the element
	 */
	template<typename T>
	UnderlyingColumnStorage<T>& getUnderlyingStorage() const {
		return *((UnderlyingColumnStorage<T>*)mUnderlyingStorage.get());
	}
};