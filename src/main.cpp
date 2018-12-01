#include <iostream>
#include <random>

#include "query.h"
#include "table.h"
#include "database_engine.h"
#include "helpers.h"

Query createQuery(std::unique_ptr<QueryOperation> operation) {
	return Query(std::move(operation));
}

int main() {
	std::mt19937 random(1337);
	std::uniform_int_distribution<int> distributionX(0, 1000);
	std::uniform_real_distribution<float> distributionY(0.0f, 1000.0f);
	std::uniform_int_distribution<int> distributionZ(0, 1000);
	auto generateX = [&]() { return distributionX(random); };
	auto generateY = [&]() { return distributionY(random); };
	auto generateZ = [&]() { return distributionZ(random); };
	std::size_t count = 20000 * 10;

	std::vector<std::vector<QueryValue>> rows;
	for (std::size_t i = 0; i < count; i++) {
		rows.emplace_back(std::vector<QueryValue> {
			QueryValue((std::int32_t)i),
			QueryValue(generateY()),
			QueryValue(generateZ())
		});
	}

	auto insertQuery = createQuery(std::make_unique<QueryInsertOperation>(
		"test_table",
		std::vector<std::string> { "x", "y", "z" },
		std::move(rows)
	));

	std::vector<std::unique_ptr<QueryExpression>> projections;
	projections.emplace_back(std::make_unique<QueryColumnReferenceExpression>("x"));

	projections.emplace_back(std::make_unique<QueryColumnReferenceExpression>("y"));
//	projections.emplace_back(std::make_unique<QueryMathExpression>(
//		std::make_unique<QueryColumnReferenceExpression>("y"),
//	    std::make_unique<QueryValueExpression>(QueryValue(1000.0f)),
//	    MathOperator::Add));

	auto selectQuery = createQuery(std::make_unique<QuerySelectOperation>(
		"test_table",
//		std::vector<std::string> { "x", "y" },
//		QueryExpressionHelpers::createColumnReferences({ "x", "y" }),
		std::move(projections),
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
//		std::make_unique<QueryAndExpression>(
//			std::make_unique<QueryCompareExpression>(
//				std::make_unique<QueryValueExpression>(QueryValue(33)),
//				std::make_unique<QueryColumnReferenceExpression>("x"),
//				CompareOperator::GreaterThan),
//			std::make_unique<QueryCompareExpression>(
//				std::make_unique<QueryColumnReferenceExpression>("y"),
//				std::make_unique<QueryColumnReferenceExpression>("z"),
//				CompareOperator::GreaterThan))
//		std::make_unique<QueryAndExpression>(
//			std::make_unique<QueryCompareExpression>(
//				std::make_unique<QueryColumnReferenceExpression>("x"),
//				std::make_unique<QueryValueExpression>(QueryValue(33)),
//				CompareOperator::GreaterThan),
//			std::make_unique<QueryCompareExpression>(
//				std::make_unique<QueryColumnReferenceExpression>("y"),
//				std::make_unique<QueryValueExpression>(QueryValue(35.0f)),
//				CompareOperator::LessThan))
//		std::make_unique<QueryAndExpression>(
//			std::make_unique<QueryValueExpression>(QueryValue(true)),
//			std::make_unique<QueryCompareExpression>(
//				std::make_unique<QueryColumnReferenceExpression>("y"),
//				std::make_unique<QueryValueExpression>(QueryValue(35.0f)),
//				CompareOperator::LessThan)
//		 )
//		std::make_unique<QueryAndExpression>(
//			std::make_unique<QueryCompareExpression>(
//				std::make_unique<QueryColumnReferenceExpression>("y"),
//				std::make_unique<QueryValueExpression>(QueryValue(35.0f)),
//				CompareOperator::LessThan),
//			std::make_unique<QueryValueExpression>(QueryValue(true))
//		)
//		std::make_unique<QueryCompareExpression>(
//			std::make_unique<QueryColumnReferenceExpression>("x"),
//			std::make_unique<QueryValueExpression>(QueryValue((std::int32_t)(count * 0.85) * 1)),
//			CompareOperator::GreaterThan)
//		std::make_unique<QueryAndExpression>(
//			std::make_unique<QueryCompareExpression>(
//				std::make_unique<QueryColumnReferenceExpression>("x"),
//				std::make_unique<QueryValueExpression>(QueryValue((std::int32_t)(count * 0.85) * 1)),
//				CompareOperator::GreaterThan),
//			std::make_unique<QueryCompareExpression>(
//				std::make_unique<QueryColumnReferenceExpression>("x"),
//				std::make_unique<QueryValueExpression>(QueryValue((std::int32_t)(count * 0.90) * 1)),
//				CompareOperator::LessThan))
//		std::make_unique<QueryAndExpression>(
////			std::make_unique<QueryCompareExpression>(
////				std::make_unique<QueryColumnReferenceExpression>("y"),
////				std::make_unique<QueryValueExpression>(QueryValue(35.0f)),
////				CompareOperator::LessThan),
//			std::make_unique<QueryCompareExpression>(
//				std::make_unique<QueryColumnReferenceExpression>("y"),
//				std::make_unique<QueryMathExpression>(
//					std::make_unique<QueryValueExpression>(QueryValue(15.0f)),
//					std::make_unique<QueryValueExpression>(QueryValue(20.0f)),
//					MathOperator::Add),
//				CompareOperator::LessThan),
//			std::make_unique<QueryCompareExpression>(
//				std::make_unique<QueryColumnReferenceExpression>("x"),
//				std::make_unique<QueryValueExpression>(QueryValue((std::int32_t)(count * 0.85) * 1)),
//				CompareOperator::GreaterThan))
//		std::unique_ptr<QueryExpression>()
//		std::make_unique<QueryValueExpression>(QueryValue(true))
		std::make_unique<QueryCompareExpression>(
			std::make_unique<QueryColumnReferenceExpression>("y"),
			std::make_unique<QueryValueExpression>(QueryValue(900.0f)),
			CompareOperator::GreaterThan)
//		,std::make_unique<QueryColumnReferenceExpression>("x")
		,JoinClause()
		,OrderingClause({ OrderingColumn {"y", false } })
	));

	std::vector<std::unique_ptr<QueryAssignExpression>> sets;
	sets.emplace_back(std::make_unique<QueryAssignExpression>(
		"y",
		std::make_unique<QueryMathExpression>(
			std::make_unique<QueryColumnReferenceExpression>("y"),
			std::make_unique<QueryValueExpression>(QueryValue(1000.0f)),
			MathOperator::Add)
	));

//	sets.emplace_back(std::make_unique<QueryAssignExpression>(
//		"z",
//		std::make_unique<QueryMathExpression>(
//			std::make_unique<QueryColumnReferenceExpression>("z"),
//			std::make_unique<QueryValueExpression>(QueryValue(1000)),
//			MathOperator::Add)
//	));

	auto updateQuery = createQuery(std::make_unique<QueryUpdateOperation>(
		"test_table",
		std::move(sets),
		std::make_unique<QueryAndExpression>(
			std::make_unique<QueryCompareExpression>(
				std::make_unique<QueryColumnReferenceExpression>("x"),
				std::make_unique<QueryValueExpression>(QueryValue((std::int32_t)(count * 0.85) * 1)),
				CompareOperator::GreaterThan),
			std::make_unique<QueryCompareExpression>(
				std::make_unique<QueryColumnReferenceExpression>("x"),
				std::make_unique<QueryValueExpression>(QueryValue((std::int32_t)(count * 0.90) * 1)),
				CompareOperator::LessThan))
	));

	Schema schema(
		"test_table",
		{
			ColumnDefinition(0, "x", ColumnType::Int32),
			ColumnDefinition(1, "y", ColumnType::Float32),
			ColumnDefinition(2, "z", ColumnType::Int32),
		},
		{
//			"x",
			"y"
//			"z"
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
		Timing timing("execute update query: ");
		databaseEngine.execute(updateQuery, result);
	}

	{
		Timing timing("execute select query: ");
		databaseEngine.execute(selectQuery, result);
	}

//	auto& x = table.getColumnValues<std::int32_t>("x");
//	auto& y = table.getColumnValues<float>("y");

	auto& x = result.getColumn<std::int32_t>(0);
	auto& y = result.getColumn<float>(1);

	if (!x.empty()) {
		std::cout
			<< x.size() << ", " << x.front() << ", " << x.back()
			<< ", " << y.front() << ", " << y.back()
			<< std::endl;
	} else {
		std::cout << x.size() << std::endl;
	}

	return 0;
}