#include "table.h"

ColumnDefinition::ColumnDefinition(std::size_t index, const std::string& name, ColumnType type)
	:  mName(name), mType(type), mIndex(index) {

}
const std::string& ColumnDefinition::name() const {
	return mName;
}

ColumnType ColumnDefinition::type() const {
	return mType;
}

std::size_t ColumnDefinition::index() const {
	return mIndex;
}

Schema::Schema(const std::string& name, std::vector<ColumnDefinition> columns, std::vector<std::string> indices)
	: mName(name), mColumns(std::move(columns)), mIndices(std::move(indices)) {

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

const std::vector<std::string>& Schema::indices() const {
	return mIndices;
}

const std::vector<ColumnDefinition>& Schema::columns() const {
	return mColumns;
}

Table::Table(Schema schema)
	: mSchema(std::move(schema)) {
	for (auto& indexColumn : mSchema.indices()) {
		mIndices.push_back(std::make_unique<TreeIndex>(mSchema.getDefinition(indexColumn)));
	}

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

const std::vector<std::unique_ptr<TreeIndex>>& Table::indices() const {
	return mIndices;
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
