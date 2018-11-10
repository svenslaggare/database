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
public:
	/**
	 * Creates a new column definition
	 * @param name The name of the column
	 * @param type The type of the column
	 */
	ColumnDefinition(const std::string& name, ColumnType type);

	/**
	 * Returns the namn of the column
	 */
	const std::string& name() const;

	/**
	 * Returns the type of the column
	 */
	ColumnType type() const;
};

/**
 * Represents the schema for a database table
 */
class Schema {
private:
	std::string mName;
	std::vector<ColumnDefinition> mColumns;

public:
	/**
	 * Creates a new schema
	 * @param name The name of the table
	 * @param columns The columns
	 */
	Schema(std::string name, std::vector<ColumnDefinition> columns);

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
};

/**
 * Represents a database table
 */
class Table {
private:
	Schema mSchema;
	std::unordered_map<std::string, ColumnStorage> mColumnsStorage;
	std::vector<ColumnStorage*> mColumnIndexToStorage;

	TreeIndex mPrimaryIndex;
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
	 * Returns the primary index
	 */
	const TreeIndex& primaryIndex() const;

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

		if (name == mPrimaryIndex.column().name()) {
			mPrimaryIndex.insert(value, rowIndex);
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