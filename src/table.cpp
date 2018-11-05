#include "table.h"

Schema::Schema(std::string name, std::vector<ColumnDefinition> columns)
	: name(name), columns(std::move(columns)) {

}

const ColumnDefinition& Schema::getDefinition(const std::string& name) const {
	for (auto& column : columns) {
		if (column.name == name) {
			return column;
		}
	}

	throw std::runtime_error("Element not found.");
}

ColumnDefinition::ColumnDefinition(const std::string& name, ColumnType type)
	:  name(name), type(type) {

}

Table::Table(Schema schema)
	: mSchema(std::move(schema)) {
	for (auto& column : mSchema.columns) {
		mColumnsStorage.emplace(column.name, column);
	}

	for (auto& column : mSchema.columns) {
		mColumnIndexToStorage.push_back(&mColumnsStorage.at(column.name));
	}
}

const Schema& Table::schema() const {
	return mSchema;
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
