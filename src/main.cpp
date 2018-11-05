#include <iostream>
#include <random>

#include "query.h"
#include "table.h"
#include "database_engine.h"
#include "helpers.h"
#include "query_expressions/internal_expressions.h"

Query createQuery(std::unique_ptr<QueryOperation> operation) {
	std::vector<std::unique_ptr<QueryOperation>> operations;
	operations.push_back(std::move(operation));
	return Query(std::move(operations));
}

int main() {
	std::mt19937 random(1337);
	std::uniform_int_distribution<int> distributionX(0, 1000);
	std::uniform_real_distribution<float> distributionY(0.0f, 1000.0f);
	std::uniform_real_distribution<float> distributionZ(0.0f, 1000.0f);
	auto generateX = [&]() { return distributionX(random); };
	auto generateY = [&]() { return distributionY(random); };
	auto generateZ = [&]() { return distributionZ(random); };
	std::size_t count = 20000;

	std::vector<std::vector<QueryValue>> rows;
	for (std::size_t i = 0; i < count; i++) {
		rows.emplace_back(std::vector<QueryValue> {
			QueryValue(generateX()),
			QueryValue(generateY()),
			QueryValue(generateZ())
		});
	}

	auto insertQuery = createQuery(std::make_unique<QueryInsertOperation>(
		"test_table",
		std::vector<std::string> { "x", "y", "z" },
		std::move(rows)
	));

	auto selectQuery = createQuery(std::make_unique<QuerySelectOperation>(
		"test_table",
		std::vector<std::string> { "x", "y" },
//		std::make_unique<QueryCompareExpression>(
//			std::make_unique<QueryColumnReferenceExpression>("y"),
//			std::make_unique<QueryValueExpression>(QueryValue(35.0f)),
//			CompareOperator::LessThan)
//		std::make_unique<QueryCompareExpression>(
//			std::make_unique<QueryValueExpression>(QueryValue(33)),
//			std::make_unique<QueryColumnReferenceExpression>("x"),
//			CompareOperator::GreaterThan)
//		std::make_unique<QueryCompareExpression>(
//			std::make_unique<QueryColumnReferenceExpression>("y"),
//			std::make_unique<QueryColumnReferenceExpression>("z"),
//			CompareOperator::GreaterThan)
		std::make_unique<QueryAndExpression>(
			std::make_unique<QueryCompareExpression>(
				std::make_unique<QueryValueExpression>(QueryValue(33)),
				std::make_unique<QueryColumnReferenceExpression>("x"),
				CompareOperator::GreaterThan),
			std::make_unique<QueryCompareExpression>(
				std::make_unique<QueryColumnReferenceExpression>("y"),
				std::make_unique<QueryColumnReferenceExpression>("z"),
				CompareOperator::GreaterThan))
//		std::make_unique<QueryAndExpression>(
//			std::make_unique<QueryCompareExpression>(
//				std::make_unique<QueryValueExpression>(QueryValue(33)),
//				std::make_unique<QueryColumnReferenceExpression>("x"),
//				CompareOperator::GreaterThan),
//			std::make_unique<QueryCompareExpression>(
//				std::make_unique<QueryColumnReferenceExpression>("y"),
//				std::make_unique<QueryValueExpression>(QueryValue(35.0f)),
//				CompareOperator::LessThan))
//		std::unique_ptr<QueryExpression>()
//		std::make_unique<QueryValueExpression>(QueryValue(true))
//		std::make_unique<QueryCompareExpression>(
//			std::make_unique<QueryColumnReferenceExpression>("y"),
//			std::make_unique<QueryValueExpression>(QueryValue(350000.0f)),
//			CompareOperator::LessThan)
	));

	Schema schema(
		"test_table",
		{
			ColumnDefinition("x", ColumnType::Int32),
			ColumnDefinition("y", ColumnType::Float32),
			ColumnDefinition("z", ColumnType::Float32),
		}
	);

	DatabaseEngine databaseEngine;
	databaseEngine.addTable("test_table", std::make_unique<Table>(std::move(schema)));
	auto& table = databaseEngine.getTable("test_table");

//	table.insertRow(
//		std::make_pair<std::string, std::int32_t>("x", 32),
//		std::make_pair<std::string, float>("y", 23.0f));
//
//	table.insertRow(
//		std::make_pair<std::string, std::int32_t>("x", 12),
//		std::make_pair<std::string, float>("y", 48.0f));
//
//	table.insertRow(
//		std::make_pair<std::string, std::int32_t>("x", 42),
//		std::make_pair<std::string, float>("y", 47.11f));
//
//	DatabaseQueryResult result;
//	databaseEngine.execute(query, result);
//
//	auto& x = result.getColumn<std::int32_t>(0);
//	auto& y = result.getColumn<float>(1);

	QueryResult result;

	{
		Timing timing("execute insert query: ");
		databaseEngine.execute(insertQuery, result);
	}

	{
		Timing timing("execute select query: ");
		databaseEngine.execute(selectQuery, result);
	}

//	auto& x = table.getColumnValues<std::int32_t>("x");
//	auto& y = table.getColumnValues<float>("y");

	auto& x = result.getColumn<std::int32_t>(0);
	auto& y = result.getColumn<float>(1);
	std::cout << x.size() << std::endl;

	return 0;
}