#include "virtual_table.h"
#include "../storage.h"
#include "../table.h"
#include "../database_engine.h"

VirtualColumn::VirtualColumn(ColumnStorage* storage)
	: mStorage(storage) {

}

ColumnType VirtualColumn::type() const {
	return mStorage->type();
}

ColumnStorage* VirtualColumn::storage() const {
	return mStorage;
}

void VirtualColumn::setStorage(ColumnStorage* storage) {
	mStorage = storage;
}

VirtualTable::VirtualTable(Table& table)
	: mTable(table) {
	mNumRows = mTable.numRows();
}

const Table& VirtualTable::underlying() const {
	return mTable;
}

Table& VirtualTable::underlying() {
	return mTable;
}

VirtualColumn& VirtualTable::getColumn(const std::string& name) {
	auto columnsIterator = mColumns.find(name);
	if (columnsIterator != mColumns.end()) {
		return *columnsIterator->second;
	}

	auto virtualColumn = std::make_unique<VirtualColumn>(&mTable.getColumn(name));
	auto virtualColumnPtr = virtualColumn.get();
	mColumns[name] = std::move(virtualColumn);
	return *virtualColumnPtr;
}

std::size_t VirtualTable::numRows() const {
	return mNumRows;
}

void VirtualTable::setStorage(std::vector<ColumnStorage>& storage) {
	mNumRows = storage[0].size();

	for (std::size_t columnIndex = 0; columnIndex < storage.size(); columnIndex++) {
		mColumns[mTable.schema().columns()[columnIndex].name()]->setStorage(&storage[columnIndex]);
	}
}


VirtualTableContainer::VirtualTableContainer(DatabaseEngine& databaseEngine)
	: mDatabaseEngine(databaseEngine) {

}

VirtualTable& VirtualTableContainer::getTable(const std::string& name) {
	auto virtualTableIterator = mTables.find(name);
	if (virtualTableIterator != mTables.end()) {
		return *virtualTableIterator->second;
	} else {
		auto virtualTable = std::make_unique<VirtualTable>(mDatabaseEngine.getTable(name));
		auto virtualTablePtr = virtualTable.get();
		mTables[name] = std::move(virtualTable);
		return *virtualTablePtr;
	}
}
