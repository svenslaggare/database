#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "storage.h"
#include "common.h"
#include "indices.h"

/**
 * Represents a column in a database schema
 */
class ColumnDefinition {
private:
	std::string mName;
	ColumnType mType;
	std::size_t mIndex;
public:
	/**
	 * Creates a new column definition
	 * @param name The name of the column
	 * @param type The type of the column
	 * @param index The index of the column
	 */
	ColumnDefinition(std::size_t index, const std::string& name, ColumnType type);

	/**
	 * Returns the namn of the column
	 */
	const std::string& name() const;

	/**
	 * Returns the type of the column
	 */
	ColumnType type() const;

	/**
	 * Returns the index of the column
	 */
	std::size_t index() const;
};

/**
 * Represents the schema for a database table
 */
class Schema {
private:
	std::string mName;
	std::vector<ColumnDefinition> mColumns;
	std::vector<std::string> mIndices;
public:
	/**
	 * Creates a new schema
	 * @param name The name of the table
	 * @param columns The columns
	 * @param indices The indices
	 */
	Schema(const std::string& name, std::vector<ColumnDefinition> columns, std::vector<std::string> indices);

	/**
	 * Returns the name of the schema
	 */
	const std::string name() const;

	/**
	 * Returns the columns
	 */
	const std::vector<ColumnDefinition>& columns() const;

	/**
	 * Returns the definition of the given column
	 * @param name The name of the column
	 */
	const ColumnDefinition& getDefinition(const std::string& name) const;

	/**
	 * Returns the indices
	 */
	const std::vector<std::string>& indices() const;
};

/**
 * Represents a database table
 */
class Table {
private:
	Schema mSchema;
	std::unordered_map<std::string, ColumnStorage> mColumnsStorage;
	std::vector<ColumnStorage*> mColumnIndexToStorage;

	std::vector<std::unique_ptr<TreeIndex>> mIndices;
public:
	/**
	 * Creates a new table
	 * @param schema The schema for the table
	 */
	explicit Table(Schema schema);

	/**
	 * Returns the schema
	 */
	const Schema& schema() const;

	/**
	 * Returns the indices
	 */
	const std::vector<std::unique_ptr<TreeIndex>>& indices() const;

	/**
	 * Returns the number of rows in the table
	 */
	std::size_t numRows() const;

	/**
	 * Inserts a new entry for a column into the table
	 * @tparam T The type of the data
	 * @param name The name of the column
	 * @param value The value
	 */
	template<typename T>
	void insertColumn(const std::string& name, T value) {
		auto& columnStorage = mColumnsStorage.at(name);
		std::size_t rowIndex = columnStorage.size();
		columnStorage.getUnderlyingStorage<T>().push_back(value);

		for (auto& index : mIndices) {
			if (index->column().name() == name) {
				index->insert(value, rowIndex);
			}
		}
	}

	inline void insertRow() {

	}

	template<typename T>
	using ColumnNameValue = std::pair<std::string, T>;

	/**
	 * Inserts a new row into the table
	 * @param column The first column entry
	 * @param columns The other column entires
	 */
	template<typename T, typename ...Ts>
	void insertRow(ColumnNameValue<T> column, ColumnNameValue<Ts>... columns) {
		insertColumn(column.first, column.second);
		insertRow(columns...);
	}

	/**
	 * Updates the indices for the given value
	 * @tparam T The type of the value
	 * @param name The name of column
	 * @param oldValue The old value
	 * @param newValue The new value
	 * @param rowIndex The row index for the value
	 */
	template<typename T>
	void updateIndices(const std::string& name, const T& oldValue, const T& newValue, std::size_t rowIndex) {
		for (auto& index : mIndices) {
			if (index->column().name() == name) {
				index->update(oldValue, newValue, rowIndex);
			}
		}
	}

	/**
	 * Returns the underlying storage for the given column
	 * @tparam T The type of the column values
	 * @param name The name of the column
	 */
	template<typename T>
	UnderlyingColumnStorage<T>& getColumnValues(const std::string& name) const {
		auto& columnStorage = mColumnsStorage.at(name);
		return columnStorage.getUnderlyingStorage<T>();
	}

	/**
	 * Returns the storage for the given column
	 * @param name The name of the column
	 */
	ColumnStorage& getColumn(const std::string& name);

	/**
	 * Returns the storage for the given column
	 * @param name The name of the column
	 */
	const ColumnStorage& getColumn(const std::string& name) const;
};