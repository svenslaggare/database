#include "table.h"

ColumnDefinition::ColumnDefinition(const std::string& name, ColumnType type)
	:  mName(name), mType(type) {

}
const std::string& ColumnDefinition::name() const {
	return mName;
}

ColumnType ColumnDefinition::type() const {
	return mType;
}

Schema::Schema(std::string name, std::vector<ColumnDefinition> columns)
	: mName(name), mColumns(std::move(columns)) {

}

const std::string Schema::name() const {
	return mName;
}

const ColumnDefinition& Schema::getDefinition(const std::string& name) const {
	for (auto& column : mColumns) {
		if (column.name() == name) {
			return column;
		}
	}

	throw std::runtime_error("Element not found.");
}

const std::vector<ColumnDefinition>& Schema::columns() const {
	return mColumns;
}

Table::Table(Schema schema)
	: mSchema(std::move(schema)), mPrimaryIndex(mSchema.columns()[0]) {
	for (auto& column : mSchema.columns()) {
		mColumnsStorage.emplace(column.name(), ColumnStorage(column));
	}

	for (auto& column : mSchema.columns()) {
		mColumnIndexToStorage.push_back(&mColumnsStorage.at(column.name()));
	}
}

const Schema& Table::schema() const {
	return mSchema;
}

const TreeIndex& Table::primaryIndex() const {
	return mPrimaryIndex;
}

std::size_t Table::numRows() const {
	return mColumnsStorage.begin()->second.size();
}

ColumnStorage& Table::getColumn(const std::string& name) {
	return mColumnsStorage.at(name);
}

const ColumnStorage& Table::getColumn(const std::string& name) const {
	return mColumnsStorage.at(name);
}
