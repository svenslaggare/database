#pragma once
#pragma once
#include <iostream>
#include <cxxtest/TestSuite.h>
#include "test_helpers.h"

class UpdateSimpleTestSuite : public CxxTest::TestSuite {
public:
	void testSimple() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData);

		std::vector<std::unique_ptr<QueryAssignExpression>> sets;
		sets.emplace_back(std::make_unique<QueryAssignExpression>(
			"y",
			std::make_unique<QueryMathExpression>(
				createColumn("y"),
				createValue(QueryValue(1000.0f)),
				MathOperator::Add)));

		auto query = createQuery(std::make_unique<QueryUpdateOperation>(
			"test_table",
			std::move(sets)
		));

		QueryResult result;
		databaseEngine->execute(query, result);

		auto& table = databaseEngine->getTable("test_table");
		for (std::size_t i = 0; i < table.numRows(); i++) {
			ASSERT_EQUALS_DB_ENTRY(
				table.getColumn("x").getValue(i),
				tableData[i][0],
				i,
				0);

			ASSERT_EQUALS_DB_ENTRY(
				table.getColumn("y").getValue(i).getValue<float>(),
				tableData[i][1].getValue<float>() + 1000.0f,
				i,
				1);

			ASSERT_EQUALS_DB_ENTRY(
				table.getColumn("z").getValue(i),
				tableData[i][2],
				i,
				2);
		}
	}

	void testSimpleIndexing() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData, defaultTestConfig(), { "y" });

		std::vector<std::unique_ptr<QueryAssignExpression>> sets;
		sets.emplace_back(std::make_unique<QueryAssignExpression>(
			"y",
			std::make_unique<QueryMathExpression>(
				createColumn("y"),
				createValue(QueryValue(1000.0f)),
				MathOperator::Add)));

		auto query = createQuery(std::make_unique<QueryUpdateOperation>(
			"test_table",
			std::move(sets)
		));

		QueryResult result;
		databaseEngine->execute(query, result);

		auto& table = databaseEngine->getTable("test_table");
		for (std::size_t i = 0; i < table.numRows(); i++) {
			ASSERT_EQUALS_DB_ENTRY(
				table.getColumn("x").getValue(i),
				tableData[i][0],
				i,
				0);

			ASSERT_EQUALS_DB_ENTRY(
				table.getColumn("y").getValue(i).getValue<float>(),
				tableData[i][1].getValue<float>() + 1000.0f,
				i,
				1);

			ASSERT_EQUALS_DB_ENTRY(
				table.getColumn("z").getValue(i),
				tableData[i][2],
				i,
				2);
		}

		auto& index = table.indices()[0];
		auto& underlyingIndex = index->getUnderlyingStorage<float>();
		for (auto& row : underlyingIndex) {
			auto rowIndex = row.second;

			ASSERT_EQUALS_DB_ENTRY(
				table.getColumn("y").getValue(rowIndex).getValue<float>(),
				row.first,
				rowIndex,
				1);
		}
	}
};