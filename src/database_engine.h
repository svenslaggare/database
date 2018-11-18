#pragma once
#include <unordered_map>
#include <memory>

#include "table.h"

struct Query;
struct QueryResult;

/**
 * The database configuration
 */
struct DatabaseConfiguration {
	bool optimizeExpressions = true;
	bool optimizeExecution = false;
};

/**
 * Represents the database engine
 */
class DatabaseEngine {
private:
	DatabaseConfiguration mConfig;
	std::unordered_map<std::string, std::unique_ptr<Table>> mTables;
public:
	/**
	 * Creates a new database engine
	 */
	explicit DatabaseEngine(DatabaseConfiguration config = {});

	/**
	 * Returns the configuration
	 */
	const DatabaseConfiguration& config() const;

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