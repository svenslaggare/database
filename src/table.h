#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "column_storage.h"
#include "common.h"

/**
 * Represents a column in a database schema
 */
struct ColumnDefinition {
	std::string name;
	ColumnType type;

	ColumnDefinition(const std::string& name, ColumnType type);
};

/**
 * Represents the schema for a database table
 */
struct Schema {
	std::string name;
	std::vector<ColumnDefinition> columns;

	Schema(std::string name, std::vector<ColumnDefinition> columns);

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
public:
	explicit Table(Schema schema);

	const Schema& schema() const;

	std::size_t numRows() const;

	template<typename T>
	void insertColumn(const std::string& name, T value) {
		auto& columnStorage = mColumnsStorage.at(name);
		columnStorage.getUnderlyingStorage<T>().push_back(value);
	}

	inline void insertRow() {

	}

	template<typename T>
	using ColumnNameValue = std::pair<std::string, T>;

	template<typename T, typename ...Ts>
	void insertRow(ColumnNameValue<T> column, ColumnNameValue<Ts>... columns) {
		insertColumn(column.first, column.second);
		insertRow(columns...);
	}

	template<typename T>
	UnderlyingColumnStorage<T>& getColumnValues(const std::string& name) const {
		auto& columnStorage = mColumnsStorage.at(name);
		return columnStorage.getUnderlyingStorage<T>();
	}

//	template<typename T>
//	UnderlyingColumnStorage<T>& getColumnValues(std::size_t index) const {
//		auto& columnStorage = mColumnIndexToStorage[index];
//		return columnStorage->getUnderlyingStorage<T>();
//	}

	ColumnStorage& getColumn(const std::string& name);
	const ColumnStorage& getColumn(const std::string& name) const;
};