#pragma once
#include <unordered_map>
#include <map>
#include <memory>

class ColumnDefinition;

/**
 * Represents a Tree index
 */
class TreeIndex {
private:
	const ColumnDefinition& mColumn;
	std::unique_ptr<std::uint8_t[]> mUnderlyingStorage;
public:
	template<typename T>
	using UnderlyingStorage = std::map<T, std::size_t>;

	/**
	 * Creates a new tree index
	 * @param column The column to index on
	 */
	explicit TreeIndex(const ColumnDefinition& column);

	/**
	 * Returns the column that is hashed on
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
	 * Finds the row index for the given value
	 * @tparam T The type of the value
	 * @param value The value
	 * @param rowIndex Sets to found row index
	 * @return True if found else false
	 */
	template<typename T>
	bool findRowIndex(const T& value, std::size_t& rowIndex) const {
		auto& underlyingIndex = getUnderlyingStorage<T>();
		auto rowIndexIterator = underlyingIndex.find(value);
		if (rowIndexIterator != underlyingIndex.end()) {
			rowIndex = rowIndexIterator->second;
			return true;
		}

		return false;
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
};