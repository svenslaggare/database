#include "indices.h"
#include "common.h"
#include "table.h"

namespace {
	std::unique_ptr<std::uint8_t[]> createTreeIndexStorage(ColumnType type) {
		auto handleForType = [&](auto dummy) {
			using Type = decltype(dummy);
			return std::unique_ptr<std::uint8_t[]>((std::uint8_t*)(new TreeIndex::UnderlyingStorage<Type>()));
		};

		return handleGenericTypeResult(std::unique_ptr<std::uint8_t[]>, type, handleForType);
	}
}

TreeIndex::TreeIndex(const Schema& schema, const ColumnDefinition& column)
	: mSchema(schema), mColumn(column), mUnderlyingStorage(createTreeIndexStorage(column.type())) {

}

std::string TreeIndex::columnName() const {
	return mSchema.name() + "." + mColumn.name();
}

const ColumnDefinition& TreeIndex::column() const {
	return mColumn;
}

