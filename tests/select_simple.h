#pragma once
#include <iostream>
#include <cxxtest/TestSuite.h>
#include "test_helpers.h"

class SelectSimpleTestSuite : public CxxTest::TestSuite {
public:
	void testNoFilteringOneColumn() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData);
		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "x" })
		));

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 1);
		TS_ASSERT_EQUALS(result.columns[0].size(), tableData.size());

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(i), tableData[i][0], i, 0);
		}
	}

	void testNoFilteringManyColumns() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData);
		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "x", "y", "z" })
		));

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 3);
		TS_ASSERT_EQUALS(result.columns[0].size(), tableData.size());

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(i), tableData[i][0], i, 0);
			ASSERT_EQUALS_DB_ENTRY(result.columns[1].getValue(i), tableData[i][1], i, 1);
			ASSERT_EQUALS_DB_ENTRY(result.columns[2].getValue(i), tableData[i][2], i, 2);
		}
	}

	void testFilteringOneColumn() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData);
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

	void testFilteringOneColumn2() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData);
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

	void testFilteringManyColumns() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData);
		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "x", "y", "z" }),
			std::make_unique<QueryCompareExpression>(
				createColumn("x"),
				createValue(QueryValue(500)),
				CompareOperator::LessThan)
		));

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 3);
		TS_ASSERT_EQUALS(result.columns[0].size(), 500);

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(i), tableData[i][0], i, 0);
			ASSERT_EQUALS_DB_ENTRY(result.columns[1].getValue(i), tableData[i][1], i, 1);
			ASSERT_EQUALS_DB_ENTRY(result.columns[2].getValue(i), tableData[i][2], i, 2);
		}
	}

	void testComplexFilteringManyColumns() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData);
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

	void testFilteringProjections() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData);

		std::vector<std::unique_ptr<QueryExpression>> projections;
		projections.emplace_back(std::make_unique<QueryMathExpression>(
			createColumn("x"),
			createValue(QueryValue(100)),
			MathOperator::Add));

		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			std::move(projections),
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
			ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(i), QueryValue(tableData[i][0].getValue<std::int32_t>() + 100), i, 0);
		}
	}

	void testComplexFiltering() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData);

		std::int32_t searchValue = 403;

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
		for (std::size_t i = 0; i < tableData.size(); i++) {
			if (tableData[i][2].getValue<std::int32_t>() < searchValue) {
				column1.push_back(tableData[i][0]);
				column3.push_back(tableData[i][2]);
			}
		}

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 2);
		TS_ASSERT_EQUALS(result.columns[0].size(), column1.size());

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(i), column1[i], i, 0);
			ASSERT_EQUALS_DB_ENTRY(result.columns[1].getValue(i), column3[i], i, 2);
		}
	}

	void testOrdering() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData);
		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "x" }),
			std::unique_ptr<QueryExpression>(),
			OrderingClause("x")
		));

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 1);
		TS_ASSERT_EQUALS(result.columns[0].size(), tableData.size());

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(i), tableData[i][0], i, 0);
		}
	}

	void testOrdering2() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData);
		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "x" }),
			std::unique_ptr<QueryExpression>(),
			OrderingClause("x", true)
		));

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 1);
		TS_ASSERT_EQUALS(result.columns[0].size(), tableData.size());

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(i), tableData[result.columns[0].size() - 1 - i][0], i, 0);
		}
	}

	void testOrderingWithFiltering() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData);
		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "x" }),
			std::make_unique<QueryCompareExpression>(
				createColumn("x"),
				createValue(QueryValue(500)),
				CompareOperator::LessThan),
			OrderingClause("x")
		));

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 1);
		TS_ASSERT_EQUALS(result.columns[0].size(), 500);

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(i), tableData[i][0], i, 0);
		}
	}
};