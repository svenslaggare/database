#include "storage.h"
#include "table.h"

std::unique_ptr<std::uint8_t[]> ColumnStorage::createUnderlyingStorage(ColumnType type) {
	auto handleForType = [&](auto dummy) {
		using Type = decltype(dummy);
		return std::unique_ptr<std::uint8_t[]>((std::uint8_t*)(new UnderlyingColumnStorage<Type>()));
	};

	return handleGenericTypeResult(std::unique_ptr<std::uint8_t[]>, type, handleForType);
}

ColumnStorage::ColumnStorage(ColumnType type)
	: mType(type), mUnderlyingStorage(createUnderlyingStorage(mType)) {

}

ColumnStorage::ColumnStorage(const ColumnDefinition& column)
	: ColumnStorage(column.type()) {

}

ColumnType ColumnStorage::type() const {
	return mType;
}

std::size_t ColumnStorage::size() const {
	auto handleForType = [&](auto dummy) {
		using Type = decltype(dummy);
		return this->getUnderlyingStorage<Type>().size();
	};

	return handleGenericTypeResult(std::size_t, mType, handleForType);
}
