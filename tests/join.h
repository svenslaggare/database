#pragma once
#include <cxxtest/TestSuite.h>
#include <algorithm>
#include <iostream>

#include "test_helpers.h"

std::unique_ptr<DatabaseEngine> setupJoinTest(std::vector<std::vector<QueryValue>>& tableData1, std::vector<std::vector<QueryValue>>& tableData2) {
	Schema table1Schema(
		"test_table1",
		{
			ColumnDefinition(0, "i", ColumnType::Int32),
			ColumnDefinition(1, "y", ColumnType::Float32),
			ColumnDefinition(2, "z", ColumnType::Int32),
		},
		{}
	);

	Schema table2Schema(
		"test_table2",
		{
			ColumnDefinition(0, "i", ColumnType::Int32),
			ColumnDefinition(1, "x", ColumnType::Int32),
			ColumnDefinition(2, "y", ColumnType::Float32),
		},
		{}
	);

	auto databaseEngine = std::make_unique<DatabaseEngine>(defaultTestConfig());
	databaseEngine->addTable("test_table1", std::make_unique<Table>(std::move(table1Schema)));
	databaseEngine->addTable("test_table2", std::make_unique<Table>(std::move(table2Schema)));

	std::mt19937 random(1337);
	std::uniform_real_distribution<float> distributionFloat(0.0f, 1000.0f);
	std::uniform_int_distribution<int> distributionInt(0, 1000);
	auto generateFloat = [&]() { return distributionFloat(random); };
	auto generateInt = [&]() { return distributionInt(random); };

	auto& table1 = databaseEngine->getTable("test_table1");
	for (std::int32_t i = 0; i < 1000; i++) {
		auto y = generateFloat();
		auto z = generateInt();

		table1.insertRow(
			std::make_pair(std::string("i"), i),
			std::make_pair(std::string("y"), y),
			std::make_pair(std::string("z"), z)
		);

		tableData1.push_back({ QueryValue(i), QueryValue(y), QueryValue(z) });
	}

	auto& table2 = databaseEngine->getTable("test_table2");
	for (std::int32_t i = 0; i < 1000; i++) {
		auto x = generateInt();
		auto y = generateFloat();

		table2.insertRow(
			std::make_pair(std::string("i"), i),
			std::make_pair(std::string("x"), x),
			std::make_pair(std::string("y"), y)
		);

		tableData2.push_back({ QueryValue(i), QueryValue(x), QueryValue(y) });
	}

	return databaseEngine;
}

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

	void testExplicitColumnNameOrdering() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData);
		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "test_table.x" }),
			std::unique_ptr<QueryExpression>(),
			JoinClause(),
			OrderingClause("test_table.x")
		));

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 1);
		TS_ASSERT_EQUALS(result.columns[0].size(), tableData.size());

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(i), tableData[i][0], i, 0);
		}
	}

	void testSimple() {
		std::vector<std::vector<QueryValue>> tableData1;
		std::vector<std::vector<QueryValue>> tableData2;
		auto databaseEngine = setupJoinTest(tableData1, tableData2);

		std::vector<std::vector<QueryValue>> expectedResults;
		for (std::size_t i = 0; i < tableData1.size(); i++) {
			for (std::size_t j = 0; j < tableData2.size(); j++) {
				auto left = tableData1[i][2].getValue<std::int32_t>();
				auto right = tableData2[j][1].getValue<std::int32_t>();
				if (left == right) {
					std::vector<QueryValue> row;

					row.push_back(tableData1[i][2]);
					row.push_back(tableData2[j][1]);
					row.push_back(tableData1[i][1]);
					row.push_back(tableData2[j][2]);

					expectedResults.push_back(std::move(row));
				}
			}
		}

		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table1",
			QueryExpressionHelpers::createColumnReferences({
				"test_table1.z",
				"test_table2.x",
				"test_table1.y",
				"test_table2.y"
			}),
			std::unique_ptr<QueryExpression>(),
			JoinClause("z", "test_table2", "x"),
			OrderingClause({
				OrderingColumn { "test_table1.i", false },
				OrderingColumn { "test_table2.i", false },
			})
		));

		QueryResult result;
		databaseEngine->execute(query, result);

		TS_ASSERT_EQUALS(result.columns.size(), 4);
		TS_ASSERT_EQUALS(result.columns[0].size(), expectedResults.size());
		TS_ASSERT_EQUALS(result.columns[1].size(), expectedResults.size());
		TS_ASSERT_EQUALS(result.columns[2].size(), expectedResults.size());
		TS_ASSERT_EQUALS(result.columns[3].size(), expectedResults.size());

		for (std::size_t rowIndex = 0; rowIndex < result.columns[0].size(); rowIndex++) {
//			std::cout
//				<< result.columns[0].getValue(rowIndex).getValue<std::int32_t>()
//				<< ", "  << result.columns[1].getValue(rowIndex).getValue<std::int32_t>()
//				<< ", "  << result.columns[2].getValue(rowIndex).getValue<float>()
//				<< ", " << result.columns[3].getValue(rowIndex).getValue<float>()
//				<< std::endl;
//
//			std::cout
//				<< expectedResults[rowIndex][0].getValue<std::int32_t>()
//				<< ", "  << expectedResults[rowIndex][1].getValue<std::int32_t>()
//				<< ", "  << expectedResults[rowIndex][2].getValue<float>()
//				<< ", " << expectedResults[rowIndex][3].getValue<float>()
//				<< std::endl;
//
//			std::cout << std::endl;

			ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(rowIndex), expectedResults[rowIndex][0], rowIndex, 0);
			ASSERT_EQUALS_DB_ENTRY(result.columns[1].getValue(rowIndex), expectedResults[rowIndex][1], rowIndex, 1);
			ASSERT_EQUALS_DB_ENTRY(result.columns[2].getValue(rowIndex), expectedResults[rowIndex][2], rowIndex, 2);
			ASSERT_EQUALS_DB_ENTRY(result.columns[3].getValue(rowIndex), expectedResults[rowIndex][3], rowIndex, 3);
		}
	}

	void testSimple2() {
		std::vector<std::vector<QueryValue>> tableData1;
		std::vector<std::vector<QueryValue>> tableData2;
		auto databaseEngine = setupJoinTest(tableData1, tableData2);

		std::vector<std::vector<QueryValue>> expectedResults;
		for (std::size_t i = 0; i < tableData1.size(); i++) {
			for (std::size_t j = 0; j < tableData2.size(); j++) {
				if (i == j) {
					std::vector<QueryValue> row;

					row.push_back(tableData1[i][0]);
					row.push_back(tableData2[j][0]);
					row.push_back(tableData1[i][1]);
					row.push_back(tableData2[j][2]);

					expectedResults.push_back(std::move(row));
				}
			}
		}

		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table1",
			QueryExpressionHelpers::createColumnReferences({
				"test_table1.i",
				"test_table2.i",
				"test_table1.y",
				"test_table2.y"
			}),
			std::unique_ptr<QueryExpression>(),
			JoinClause("i", "test_table2", "i"),
			OrderingClause({
				OrderingColumn{ "test_table1.i", false },
				OrderingColumn{ "test_table2.i", false },
			})
		));

		QueryResult result;
		databaseEngine->execute(query, result);

		TS_ASSERT_EQUALS(result.columns.size(), 4);
		TS_ASSERT_EQUALS(result.columns[0].size(), expectedResults.size());
		TS_ASSERT_EQUALS(result.columns[1].size(), expectedResults.size());
		TS_ASSERT_EQUALS(result.columns[2].size(), expectedResults.size());
		TS_ASSERT_EQUALS(result.columns[3].size(), expectedResults.size());

		for (std::size_t rowIndex = 0; rowIndex < result.columns[0].size(); rowIndex++) {
//			std::cout
//				<< result.columns[0].getValue(rowIndex).getValue<std::int32_t>()
//				<< ", "  << result.columns[1].getValue(rowIndex).getValue<std::int32_t>()
//				<< ", "  << result.columns[2].getValue(rowIndex).getValue<float>()
//				<< ", " << result.columns[3].getValue(rowIndex).getValue<float>()
//				<< std::endl;
//
//			std::cout
//				<< expectedResults[rowIndex][0].getValue<std::int32_t>()
//				<< ", "  << expectedResults[rowIndex][1].getValue<std::int32_t>()
//				<< ", "  << expectedResults[rowIndex][2].getValue<float>()
//				<< ", " << expectedResults[rowIndex][3].getValue<float>()
//				<< std::endl;
//
//			std::cout << std::endl;

			ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(rowIndex), expectedResults[rowIndex][0], rowIndex, 0);
			ASSERT_EQUALS_DB_ENTRY(result.columns[1].getValue(rowIndex), expectedResults[rowIndex][1], rowIndex, 1);
			ASSERT_EQUALS_DB_ENTRY(result.columns[2].getValue(rowIndex), expectedResults[rowIndex][2], rowIndex, 2);
			ASSERT_EQUALS_DB_ENTRY(result.columns[3].getValue(rowIndex), expectedResults[rowIndex][3], rowIndex, 3);
		}
	}
};