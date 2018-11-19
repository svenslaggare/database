#pragma once
#include <cxxtest/TestSuite.h>
#include <algorithm>
#include <iostream>

#include "test_helpers.h"

class JoinTestSuite : public CxxTest::TestSuite {
public:
	void testExplicitColumnNaming() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData);
		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "test_table.x" }),
			std::make_unique<QueryCompareExpression>(
				createColumn("test_table.x"),
				createValue(QueryValue(500)),
				CompareOperator::LessThan)
		));

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 1);
		TS_ASSERT_EQUALS(result.columns[0].size(), 500);

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(i), tableData[i][0], i, 0);
		}
	}

	void testSimple() {
//		Schema table1Schema(
//			"test_table1",
//			{
//				ColumnDefinition(0, "x", ColumnType::Int32),
//				ColumnDefinition(1, "y", ColumnType::Float32),
//				ColumnDefinition(2, "z", ColumnType::Int32),
//			},
//			{}
//		);
//
//		Schema table2Schema(
//			"test_table2",
//			{
//				ColumnDefinition(0, "x", ColumnType::Int32),
//				ColumnDefinition(1, "y", ColumnType::Float32),
//			},
//			{}
//		);
//
//		auto databaseEngine = std::make_unique<DatabaseEngine>(defaultTestConfig());
//		databaseEngine->addTable("test_table1", std::make_unique<Table>(std::move(table1Schema)));
//		databaseEngine->addTable("test_table2", std::make_unique<Table>(std::move(table2Schema)));
//
//		std::mt19937 random(1337);
//		std::uniform_real_distribution<float> distributionFloat(0.0f, 1000.0f);
//		std::uniform_int_distribution<int> distributionInt(0, 1000);
//		auto generateFloat = [&]() { return distributionFloat(random); };
//		auto generateInt = [&]() { return distributionInt(random); };
//
//		auto& table1 = databaseEngine->getTable("test_table1");
//		std::vector<std::vector<QueryValue>> tableData1;
//		for (std::size_t i = 0; i < 1000; i++) {
//			auto x = (std::int32_t)i;
//			auto y = generateFloat();
//			auto z = generateInt();
//
//			table1.insertRow(
//				std::make_pair(std::string("x"), x),
//				std::make_pair(std::string("y"), y),
//				std::make_pair(std::string("z"), z)
//			);
//
//			tableData1.push_back({ QueryValue(x), QueryValue(y), QueryValue(z) });
//		}
//
//		auto& table2 = databaseEngine->getTable("test_table2");
//		std::vector<std::vector<QueryValue>> tableData2;
//		for (std::size_t i = 0; i < 1000; i++) {
//			auto x = generateInt();
//			auto y = generateFloat();
//
//			table2.insertRow(
//				std::make_pair(std::string("x"), x),
//				std::make_pair(std::string("y"), y)
//			);
//
//			tableData2.push_back({ QueryValue(x), QueryValue(y) });
//		}
//
//		std::size_t sameCount = 0;
//		for (std::size_t i = 0; i < tableData1.size(); i++) {
//			for (std::size_t j = 0; j < tableData2.size(); j++) {
//				auto left = tableData1[i][2].getValue<std::int32_t>();
//				auto right = tableData2[j][0].getValue<std::int32_t>();
//				if (left == right) {
//					sameCount++;
////					std::cout << i << ", " << j << std::endl;
//				}
//			}
//		}
//
//		std::cout << sameCount << std::endl;
//
//		auto query = createQuery(std::make_unique<QuerySelectOperation>(
//			"test_table",
//			QueryExpressionHelpers::createColumnReferences({ "x" }),
//			std::unique_ptr<QueryExpression>(),
//			JoinClause(),
//			OrderingClause("x")
//		));
//
//		QueryResult result;
//		databaseEngine->execute(query, result);
	}
};