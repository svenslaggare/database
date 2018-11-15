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

TreeIndex::TreeIndex(const ColumnDefinition& column)
	: mColumn(column), mUnderlyingStorage(createTreeIndexStorage(column.type())) {

}

const ColumnDefinition& TreeIndex::column() const {
	return mColumn;
}

