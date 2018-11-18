#pragma once
#include <cxxtest/TestSuite.h>
#include <algorithm>
#include <iostream>

#include "test_helpers.h"

class SelectIndexTestSuite : public CxxTest::TestSuite {
public:
	void testSimpleIndexing() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData, optimizeExpressionsTestConfig(), { "x" });
		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "x" }),
			std::make_unique<QueryCompareExpression>(
				createColumn("x"),
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

	void testSimpleIndexing2() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData, optimizeExpressionsTestConfig(), { "x" });
		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "x" }),
			std::make_unique<QueryCompareExpression>(
				createColumn("x"),
				createValue(QueryValue(500)),
				CompareOperator::LessThanOrEqual)
		));

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 1);
		TS_ASSERT_EQUALS(result.columns[0].size(), 501);

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(i), tableData[i][0], i, 0);
		}
	}

	void testSimpleIndexing3() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData, optimizeExpressionsTestConfig(), { "x" });
		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "x" }),
			std::make_unique<QueryCompareExpression>(
				createColumn("x"),
				createValue(QueryValue(500)),
				CompareOperator::Equal)
		));

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 1);
		TS_ASSERT_EQUALS(result.columns[0].size(), 1);

		ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(0), tableData[500][0], 0, 0);
	}

	void testSimpleIndexing4() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData, optimizeExpressionsTestConfig(), { "x" });
		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "x" }),
			std::make_unique<QueryCompareExpression>(
				createValue(QueryValue(500)),
				createColumn("x"),
				CompareOperator::GreaterThan)
		));

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 1);
		TS_ASSERT_EQUALS(result.columns[0].size(), 500);

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(i), tableData[i][0], i, 0);
		}
	}

	void testIndexNonPrimary() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData, optimizeExpressionsTestConfig(), { "z" });

		std::int32_t searchValue = 200;

		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "x", "z" }),
			std::make_unique<QueryCompareExpression>(
				createColumn("z"),
				createValue(QueryValue(searchValue)),
				CompareOperator::LessThan)
		));

		std::vector<QueryValue> column1;
		std::vector<QueryValue> column3;
		std::vector<std::size_t> sortIndices;

		for (std::size_t i = 0; i < tableData.size(); i++) {
			if (tableData[i][2].getValue<std::int32_t>() < searchValue) {
				column1.push_back(tableData[i][0]);
				column3.push_back(tableData[i][2]);
				sortIndices.push_back(sortIndices.size());
			}
		}

		std::sort(
			sortIndices.begin(),
			sortIndices.end(),
			[&](auto& x, auto& y) {
				return column3[x].template getValue<std::int32_t>() < column3[y].template getValue<std::int32_t>();
			});

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 2);
		TS_ASSERT_EQUALS(result.columns[0].size(), column1.size());

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[1].getValue(i), column3[sortIndices[i]], i, 0);
		}
	}

	void testIndexNonPrimary2() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData, optimizeExpressionsTestConfig(), { "z" });

		std::int32_t searchValue = 200;

		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "x", "z" }),
			std::make_unique<QueryCompareExpression>(
				createColumn("z"),
				createValue(QueryValue(searchValue)),
				CompareOperator::LessThanOrEqual)
		));

		std::vector<QueryValue> column1;
		std::vector<QueryValue> column3;
		std::vector<std::size_t> sortIndices;

		for (std::size_t i = 0; i < tableData.size(); i++) {
			if (tableData[i][2].getValue<std::int32_t>() <= searchValue) {
				column1.push_back(tableData[i][0]);
				column3.push_back(tableData[i][2]);
				sortIndices.push_back(sortIndices.size());
			}
		}

		std::sort(
			sortIndices.begin(),
			sortIndices.end(),
			[&](auto& x, auto& y) {
				return column3[x].template getValue<std::int32_t>() < column3[y].template getValue<std::int32_t>();
			});

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 2);
		TS_ASSERT_EQUALS(result.columns[0].size(), column1.size());

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[1].getValue(i), column3[sortIndices[i]], i, 0);
		}
	}

	void testIndexNonPrimary3() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData, optimizeExpressionsTestConfig(), { "z" });

		std::int32_t searchValue = 200;

		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "x", "z" }),
			std::make_unique<QueryCompareExpression>(
				createColumn("z"),
				createValue(QueryValue(searchValue)),
				CompareOperator::GreaterThan)
		));

		std::vector<QueryValue> column1;
		std::vector<QueryValue> column3;
		std::vector<std::size_t> sortIndices;

		for (std::size_t i = 0; i < tableData.size(); i++) {
			if (tableData[i][2].getValue<std::int32_t>() > searchValue) {
				column1.push_back(tableData[i][0]);
				column3.push_back(tableData[i][2]);
				sortIndices.push_back(sortIndices.size());
			}
		}

		std::sort(
			sortIndices.begin(),
			sortIndices.end(),
			[&](auto& x, auto& y) {
				return column3[x].template getValue<std::int32_t>() < column3[y].template getValue<std::int32_t>();
			});

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 2);
		TS_ASSERT_EQUALS(result.columns[0].size(), column1.size());

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[1].getValue(i), column3[sortIndices[i]], i, 0);
		}
	}

	void testIndexNonPrimary4() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData, optimizeExpressionsTestConfig(), { "z" });

		std::int32_t searchValue = 200;

		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "x", "z" }),
			std::make_unique<QueryCompareExpression>(
				createColumn("z"),
				createValue(QueryValue(searchValue)),
				CompareOperator::GreaterThanOrEqual)
		));

		std::vector<QueryValue> column1;
		std::vector<QueryValue> column3;
		std::vector<std::size_t> sortIndices;

		for (std::size_t i = 0; i < tableData.size(); i++) {
			if (tableData[i][2].getValue<std::int32_t>() >= searchValue) {
				column1.push_back(tableData[i][0]);
				column3.push_back(tableData[i][2]);
				sortIndices.push_back(sortIndices.size());
			}
		}

		std::sort(
			sortIndices.begin(),
			sortIndices.end(),
			[&](auto& x, auto& y) {
				return column3[x].template getValue<std::int32_t>() < column3[y].template getValue<std::int32_t>();
			});

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 2);
		TS_ASSERT_EQUALS(result.columns[0].size(), column1.size());

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[1].getValue(i), column3[sortIndices[i]], i, 0);
		}
	}

	void testIndexNonPrimary5() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData, optimizeExpressionsTestConfig(), { "z" });

		std::int32_t searchValue = 200;

		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "x", "z" }),
			std::make_unique<QueryCompareExpression>(
				createColumn("z"),
				createValue(QueryValue(searchValue)),
				CompareOperator::Equal)
		));

		std::vector<QueryValue> column1;
		std::vector<QueryValue> column3;
		std::vector<std::size_t> sortIndices;

		for (std::size_t i = 0; i < tableData.size(); i++) {
			if (tableData[i][2].getValue<std::int32_t>() == searchValue) {
				column1.push_back(tableData[i][0]);
				column3.push_back(tableData[i][2]);
				sortIndices.push_back(sortIndices.size());
			}
		}

		std::sort(
			sortIndices.begin(),
			sortIndices.end(),
			[&](auto& x, auto& y) {
				return column3[x].template getValue<std::int32_t>() < column3[y].template getValue<std::int32_t>();
			});

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 2);
		TS_ASSERT_EQUALS(result.columns[0].size(), column1.size());

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[1].getValue(i), column3[sortIndices[i]], i, 0);
		}
	}

	void testComplexIndexing() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData, optimizeExpressionsTestConfig());
		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "x", "y", "z" }),
			std::make_unique<QueryAndExpression>(
				std::make_unique<QueryCompareExpression>(
					createColumn("x"),
					createValue(QueryValue(500)),
					CompareOperator::GreaterThanOrEqual),
				std::make_unique<QueryCompareExpression>(
					createColumn("x"),
					createValue(QueryValue(600)),
					CompareOperator::LessThan))
		));

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 3);
		TS_ASSERT_EQUALS(result.columns[0].size(), 100);

		std::size_t resultOffset = 500;
		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(i), tableData[resultOffset + i][0], i, 0);
			ASSERT_EQUALS_DB_ENTRY(result.columns[1].getValue(i), tableData[resultOffset + i][1], i, 1);
			ASSERT_EQUALS_DB_ENTRY(result.columns[2].getValue(i), tableData[resultOffset + i][2], i, 2);
		}
	}
};