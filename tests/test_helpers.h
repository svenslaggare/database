#pragma once
#include <random>

#include "../src/database_engine.h"
#include "../src/query.h"

#define TS_ASSERT_EQUALS_WITH_MESSAGE(x,y,m) ___TS_ASSERT_EQUALS(__FILE__,__LINE__,x,y,m)
#define ASSERT_EQUALS_DB_ENTRY(x,y,r,c) TS_ASSERT_EQUALS_WITH_MESSAGE(x, y, ("At row " + std::to_string(r) + ", col " + std::to_string(c)).c_str())

Query createQuery(std::unique_ptr<QueryOperation> operation) {
	std::vector<std::unique_ptr<QueryOperation>> operations;
	operations.push_back(std::move(operation));
	return Query(std::move(operations));
}

DatabaseConfiguration defaultTestConfig() {
	DatabaseConfiguration config;
	config.optimizeExpressions = false;
	config.optimizeExecution = false;
	return config;
}

DatabaseConfiguration optimizeExpressionsTestConfig() {
	DatabaseConfiguration config;
	config.optimizeExpressions = true;
	config.optimizeExecution = false;
	return config;
}

DatabaseConfiguration optimizeAllTestConfig() {
	DatabaseConfiguration config;
	config.optimizeExpressions = true;
	config.optimizeExecution = true;
	return config;
}

std::unique_ptr<DatabaseEngine> setupTest(std::vector<std::vector<QueryValue>>& tableData,
										  DatabaseConfiguration config = defaultTestConfig(),
										  std::initializer_list<std::string> indices = {},
										  std::size_t count = 1000) {
	Schema schema(
		"test_table",
		{
			ColumnDefinition(0, "x", ColumnType::Int32),
			ColumnDefinition(1, "y", ColumnType::Float32),
			ColumnDefinition(2, "z", ColumnType::Int32),
		},
		indices
	);

	auto databaseEngine = std::make_unique<DatabaseEngine>(config);
	databaseEngine->addTable("test_table", std::make_unique<Table>(std::move(schema)));

	std::mt19937 random(1337);
	std::uniform_real_distribution<float> distributionY(0.0f, 1000.0f);
	std::uniform_int_distribution<int> distributionZ(0, 1000);
	auto generateY = [&]() { return distributionY(random); };
	auto generateZ = [&]() { return distributionZ(random); };

	auto& table = databaseEngine->getTable("test_table");
	for (std::size_t i = 0; i < count; i++) {
		auto x = (std::int32_t)i;
		auto y = generateY();
		auto z = generateZ();

		table.insertRow(
			std::make_pair(std::string("x"), x),
			std::make_pair(std::string("y"), y),
			std::make_pair(std::string("z"), z)
		);

		tableData.push_back({ QueryValue(x), QueryValue(y), QueryValue(z) });
	}

	return databaseEngine;
}

std::unique_ptr<QueryValueExpression> createValue(QueryValue value) {
	return std::make_unique<QueryValueExpression>(value);
}

std::unique_ptr<QueryColumnReferenceExpression> createColumn(const std::string& column) {
	return std::make_unique<QueryColumnReferenceExpression>(column);
}