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
	/**
	 * Adds a new table
	 * @param name The name of the table
	 * @param table The table
	 */
	void addTable(std::string name, std::unique_ptr<Table> table);

	/**
	 * Returns the given table
	 * @param name The name of the table
	 */
	Table& getTable(const std::string& name) const;

	/**
	 * Executes the given query
	 * @param query The query
	 * @param result The result
	 */
	void execute(const Query& query, QueryResult& result);
};