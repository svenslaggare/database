#pragma once
#include <cxxtest/TestSuite.h>
#include "../src/common.h"
#include "../src/query_parser/parser.h"
#include "test_helpers.h"

class IntegrationTestSuite : public CxxTest::TestSuite {
public:
	void testSelect1() {
		std::vector<std::vector<QueryValue>> tableData;
		auto databaseEngine = setupTest(tableData);

		auto query = createQuery(databaseEngine->parse("SELECT x + 5 FROM test_table WHERE x < 500"));

		QueryResult result;
		databaseEngine->execute(query, result);
		TS_ASSERT_EQUALS(result.columns.size(), 1);
		TS_ASSERT_EQUALS(result.columns[0].size(), 500);

		for (std::size_t i = 0; i < result.columns[0].size(); i++) {
			ASSERT_EQUALS_DB_ENTRY(
				result.columns[0].getValue(i).getValue<std::int32_t>(),
				tableData[i][0].getValue<std::int32_t>() + 5,
				i,
				0);
		}
	}
};