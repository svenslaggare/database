#pragma once
#include <string>
#include <memory>
#include <unordered_map>

#include "../common.h"

class Table;
class ColumnStorage;
class Schema;
class TreeIndex;

/**
 * Represents a column for a virtual table
 */
class VirtualColumn {
private:
	ColumnStorage* mStorage;
public:
	/**
	 * Creates a new virtual column
	 * @param storage The storage
	 */
	explicit VirtualColumn(ColumnStorage* storage);

	VirtualColumn(const VirtualColumn&) = delete;
	VirtualColumn& operator=(const VirtualColumn&) = delete;

	/**
	 * Returns the type of the column
	 */
	ColumnType type() const;

	/**
	 * Returns the storage of the column
	 */
	ColumnStorage* storage() const;

	/**
	 * Sets the storage
	 * @param storage The new storage
	 */
	void setStorage(ColumnStorage* storage);
};

/**
 * Represents a virtual table
 */
class VirtualTable {
private:
	Table& mTable;
	std::unordered_map<std::string, std::unique_ptr<VirtualColumn>> mColumns;
	std::size_t mNumRows;
public:
	/**
	 * Creates a new virtual table
	 * @param table The backing table
	 */
	explicit VirtualTable(Table& table);

	VirtualTable(const VirtualTable&) = delete;
	VirtualTable& operator=(const VirtualTable&) = delete;

	/**
	 * Returns the underlying table.
	 */
	const Table& underlying() const;

	/**
	 * Returns the underlying table.
	 */
	Table& underlying();

	/**
	 * Returns the given virtual column
	 * @param name The name of the column
	 */
	VirtualColumn& getColumn(const std::string& name);

	/**
	 * Returns the number of rows in the table
	 */
	std::size_t numRows() const;

	/**
	 * Sets the storage of the virtual table to the given
	 * @param storage The new storage
	 */
	void setStorage(std::vector<ColumnStorage>& storage);
};

class DatabaseEngine;

/**
 * Represents a container of virtual tables
 */
class VirtualTableContainer {
private:
	DatabaseEngine& mDatabaseEngine;
	std::unordered_map<std::string, std::unique_ptr<VirtualTable>> mTables;
public:
	/**
	 * Creates a new virtual table container
	 * @param databaseEngine The database engine
	 */
	explicit VirtualTableContainer(DatabaseEngine& databaseEngine);

	VirtualTableContainer(const VirtualTableContainer&) = delete;
	VirtualTableContainer& operator=(const VirtualTableContainer&) = delete;

	/**
	 * Returns a virtual table for a table with the given name
	 * @param name The name of the table
	 */
	VirtualTable& getTable(const std::string& name);
};