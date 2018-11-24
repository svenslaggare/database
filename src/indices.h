#pragma once
#include <unordered_map>
#include <map>
#include <memory>

class ColumnDefinition;
class Schema;

/**
 * Represents a Tree index
 */
class TreeIndex {
private:
	const Schema& mSchema;
	const ColumnDefinition& mColumn;
	std::unique_ptr<std::uint8_t[]> mUnderlyingStorage;
public:
	template<typename T>
	using UnderlyingStorage = std::multimap<T, std::size_t>;

	/**
	 * Creates a new tree index
	 * @param schema The schema to add the index for
	 * @param column The column to index on
	 */
	TreeIndex(const Schema& schema, const ColumnDefinition& column);

	/**
	 * Returns the full name of the column being indexed on
	 */
	std::string columnName() const;

	/**
	 * Returns the column that is index on
	 */
	const ColumnDefinition& column() const;

	/**
	 * Returns the underlying storage
	 * @tparam T The type of the value
	 */
	template<typename T>
	const UnderlyingStorage<T>& getUnderlyingStorage() const {
		return *((UnderlyingStorage<T>*)mUnderlyingStorage.get());
	}

	/**
	 * Returns the underlying storage
	 * @tparam T The type of the value
	 */
	template<typename T>
	UnderlyingStorage<T>& getUnderlyingStorage() {
		return *((UnderlyingStorage<T>*)mUnderlyingStorage.get());
	}

	/**
	 * Inserts an index entry for the given value
	 * @tparam T The type of the value
	 * @param value The value
	 * @param rowIndex The row index of the value
	 */
	template<typename T>
	void insert(const T& value, std::size_t rowIndex) {
		auto& underlyingIndex = getUnderlyingStorage<T>();
		underlyingIndex.emplace(value, rowIndex);
	}

	/**
	 * Updates the index entry for the given value
	 * @tparam T The type of the value
	 * @param oldValue The old value
	 * @param newValue The new value
	 * @param rowIndex The row index for the value
	 */
	template<typename T>
	void update(const T& oldValue, const T& newValue, std::size_t rowIndex) {
		auto& underlyingIndex = getUnderlyingStorage<T>();
		auto rowIndexIterator = underlyingIndex.equal_range(oldValue);

		for (auto it = rowIndexIterator.first; it != rowIndexIterator.second; ++it) {
			if (it->second == rowIndex) {
				underlyingIndex.erase(it);
				break;
			}
		}

		underlyingIndex.emplace(newValue, rowIndex);
	}
};