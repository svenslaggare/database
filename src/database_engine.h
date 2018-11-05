#pragma once
#include <unordered_map>
#include <memory>

#include "table.h"

struct Query;
struct QueryResult;

/**
 * Represents the database engine
 */
class DatabaseEngine {
private:
	std::unordered_map<std::string, std::unique_ptr<Table>> mTables;
public:
	void addTable(std::string name, std::unique_ptr<Table> table);

	Table& getTable(const std::string& name) const;

	void execute(const Query& query, QueryResult& result);
};