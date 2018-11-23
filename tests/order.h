#pragma once
#include <iostream>
#include <algorithm>
#include <cxxtest/TestSuite.h>
#include "test_helpers.h"

class OrderTestSuite : public CxxTest::TestSuite {
public:
	void testComplexOrdering() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData);
		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "x" }),
			std::unique_ptr<QueryExpression>(),
			JoinClause(),
			OrderingClause({ OrderingColumn { "z", false }, OrderingColumn{ "y", false } })
		));

		std::vector<std::size_t> sortedIndices;
		for (std::size_t i = 0; i < tableData.size(); i++) {
			sortedIndices.push_back(i);
		}

		std::sort(
			sortedIndices.begin(),
			sortedIndices.end(),
			[&](std::size_t x, std::size_t y) {
				auto lhsZ = tableData[x][2].getValue<std::int32_t>();
				auto rhsZ = tableData[y][2].getValue<std::int32_t>();
				if (lhsZ < rhsZ) {
					return true;
				} else if (lhsZ == rhsZ) {
					auto lhsY = tableData[x][1].getValue<float>();
					auto rhsY = tableData[y][1].getValue<float>();
					return lhsY < rhsY;
				} else {
					return false;
				}
			});

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 1);
		TS_ASSERT_EQUALS(result.columns[0].size(), tableData.size());

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(i), tableData[sortedIndices[i]][0], i, 0);
		}
	}

	void testComplexOrdering2() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData);
		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "x" }),
			std::unique_ptr<QueryExpression>(),
			JoinClause(),
			OrderingClause({ OrderingColumn { "z", false }, OrderingColumn{ "y", true } })
		));

		std::vector<std::size_t> sortedIndices;
		for (std::size_t i = 0; i < tableData.size(); i++) {
			sortedIndices.push_back(i);
		}

		std::sort(
			sortedIndices.begin(),
			sortedIndices.end(),
			[&](std::size_t x, std::size_t y) {
				auto lhsZ = tableData[x][2].getValue<std::int32_t>();
				auto rhsZ = tableData[y][2].getValue<std::int32_t>();
				if (lhsZ < rhsZ) {
					return true;
				} else if (lhsZ == rhsZ) {
					auto lhsY = tableData[x][1].getValue<float>();
					auto rhsY = tableData[y][1].getValue<float>();
					return lhsY > rhsY;
				} else {
					return false;
				}
			});

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 1);
		TS_ASSERT_EQUALS(result.columns[0].size(), tableData.size());

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(i), tableData[sortedIndices[i]][0], i, 0);
		}
	}

	void testComplexOrdering3() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData);
		auto query = createQuery(std::make_unique<QuerySelectOperation>(
			"test_table",
			QueryExpressionHelpers::createColumnReferences({ "z" }),
			std::unique_ptr<QueryExpression>(),
			JoinClause(),
			OrderingClause({ OrderingColumn { "z", false }, OrderingColumn{ "y", false } })
		));

		std::vector<std::size_t> sortedIndices;
		for (std::size_t i = 0; i < tableData.size(); i++) {
			sortedIndices.push_back(i);
		}

		std::sort(
			sortedIndices.begin(),
			sortedIndices.end(),
			[&](std::size_t x, std::size_t y) {
				auto lhsZ = tableData[x][2].getValue<std::int32_t>();
				auto rhsZ = tableData[y][2].getValue<std::int32_t>();
				if (lhsZ < rhsZ) {
					return true;
				} else if (lhsZ == rhsZ) {
					auto lhsY = tableData[x][1].getValue<float>();
					auto rhsY = tableData[y][1].getValue<float>();
					return lhsY < rhsY;
				} else {
					return false;
				}
			});

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 1);
		TS_ASSERT_EQUALS(result.columns[0].size(), tableData.size());

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(result.columns[0].getValue(i), tableData[sortedIndices[i]][2], i, 0);
		}
	}
};