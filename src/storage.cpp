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
	: type(type) {
	auto handleForType = [&](auto dummy) {
		using Type = decltype(dummy);
		underlyingStorage = std::unique_ptr<std::uint8_t[]>((std::uint8_t*)(new UnderlyingColumnStorage<Type>()));
	};

	handleGenericType(type, handleForType);
}

ColumnStorage::ColumnStorage(const ColumnDefinition& column)
	: ColumnStorage(column.type) {

}

std::size_t ColumnStorage::size() const {
	auto handleForType = [&](auto dummy) {
		using Type = decltype(dummy);
		return this->getUnderlyingStorage<Type>().size();
	};

	return handleGenericTypeResult(std::size_t, type, handleForType);
}
