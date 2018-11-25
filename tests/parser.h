#pragma once
#include <iostream>
#include <cxxtest/TestSuite.h>
#include "../src/parser/parser.h"
#include "../src/query_expressions/expressions.h"
#include "../src/query.h"

class ParserTestSuite : public CxxTest::TestSuite {
public:
	void testSelect1() {
		auto tokens = Tokenizer::tokenize("SELECT x FROM test_table");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto selectOperation = dynamic_cast<QuerySelectOperation*>(operation.get());

		TS_ASSERT_DIFFERS(selectOperation, nullptr);
		TS_ASSERT_EQUALS(selectOperation->table, "test_table");
		TS_ASSERT_EQUALS(selectOperation->projections.size(), 1);
		TS_ASSERT_EQUALS(selectOperation->filter.get(), nullptr);

		auto projection0 = dynamic_cast<QueryColumnReferenceExpression*>(selectOperation->projections[0].get());
		TS_ASSERT_DIFFERS(projection0, nullptr);
		TS_ASSERT_EQUALS(projection0->name, "x");
	}

	void testSelect2() {
		auto tokens = Tokenizer::tokenize("SELECT 0 FROM test_table");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto selectOperation = dynamic_cast<QuerySelectOperation*>(operation.get());

		TS_ASSERT_DIFFERS(selectOperation, nullptr);
		TS_ASSERT_EQUALS(selectOperation->table, "test_table");
		TS_ASSERT_EQUALS(selectOperation->projections.size(), 1);
		TS_ASSERT_EQUALS(selectOperation->filter.get(), nullptr);

		auto projection0 = dynamic_cast<QueryValueExpression*>(selectOperation->projections[0].get());
		TS_ASSERT_DIFFERS(projection0, nullptr);
		TS_ASSERT_EQUALS(projection0->value, QueryValue(0));
	}

	void testSelect3() {
		auto tokens = Tokenizer::tokenize("SELECT 0.0 FROM test_table");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto selectOperation = dynamic_cast<QuerySelectOperation*>(operation.get());

		TS_ASSERT_DIFFERS(selectOperation, nullptr);
		TS_ASSERT_EQUALS(selectOperation->table, "test_table");
		TS_ASSERT_EQUALS(selectOperation->projections.size(), 1);
		TS_ASSERT_EQUALS(selectOperation->filter.get(), nullptr);

		auto projection0 = dynamic_cast<QueryValueExpression*>(selectOperation->projections[0].get());
		TS_ASSERT_DIFFERS(projection0, nullptr);
		TS_ASSERT_EQUALS(projection0->value, QueryValue(0.0f));
	}

	void testSelect4() {
		auto tokens = Tokenizer::tokenize("SELECT false FROM test_table");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto selectOperation = dynamic_cast<QuerySelectOperation*>(operation.get());

		TS_ASSERT_DIFFERS(selectOperation, nullptr);
		TS_ASSERT_EQUALS(selectOperation->table, "test_table");
		TS_ASSERT_EQUALS(selectOperation->projections.size(), 1);
		TS_ASSERT_EQUALS(selectOperation->filter.get(), nullptr);

		auto projection0 = dynamic_cast<QueryValueExpression*>(selectOperation->projections[0].get());
		TS_ASSERT_DIFFERS(projection0, nullptr);
		TS_ASSERT_EQUALS(projection0->value, QueryValue(false));
	}

	void testSelect5() {
		auto tokens = Tokenizer::tokenize("SELECT x, y, z FROM test_table");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto selectOperation = dynamic_cast<QuerySelectOperation*>(operation.get());

		TS_ASSERT_DIFFERS(selectOperation, nullptr);
		TS_ASSERT_EQUALS(selectOperation->table, "test_table");
		TS_ASSERT_EQUALS(selectOperation->projections.size(), 3);
		TS_ASSERT_EQUALS(selectOperation->filter.get(), nullptr);

		auto projection0 = dynamic_cast<QueryColumnReferenceExpression*>(selectOperation->projections[0].get());
		TS_ASSERT_DIFFERS(projection0, nullptr);
		TS_ASSERT_EQUALS(projection0->name, "x");

		auto projection1 = dynamic_cast<QueryColumnReferenceExpression*>(selectOperation->projections[1].get());
		TS_ASSERT_DIFFERS(projection1, nullptr);
		TS_ASSERT_EQUALS(projection1->name, "y");

		auto projection2 = dynamic_cast<QueryColumnReferenceExpression*>(selectOperation->projections[2].get());
		TS_ASSERT_DIFFERS(projection2, nullptr);
		TS_ASSERT_EQUALS(projection2->name, "z");
	}

	void testSelect6() {
		auto tokens = Tokenizer::tokenize("SELECT x + 5 FROM test_table");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto selectOperation = dynamic_cast<QuerySelectOperation*>(operation.get());

		TS_ASSERT_DIFFERS(selectOperation, nullptr);
		TS_ASSERT_EQUALS(selectOperation->table, "test_table");
		TS_ASSERT_EQUALS(selectOperation->projections.size(), 1);
		TS_ASSERT_EQUALS(selectOperation->filter.get(), nullptr);

		auto projection0 = dynamic_cast<QueryMathExpression*>(selectOperation->projections[0].get());
		TS_ASSERT_DIFFERS(projection0, nullptr);
		TS_ASSERT_EQUALS(projection0->op, MathOperator::Add);

		auto projection0Sub0 = dynamic_cast<QueryColumnReferenceExpression*>(projection0->lhs.get());
		TS_ASSERT_DIFFERS(projection0Sub0, nullptr);
		TS_ASSERT_EQUALS(projection0Sub0->name, "x");

		auto projection0Sub1 = dynamic_cast<QueryValueExpression*>(projection0->rhs.get());
		TS_ASSERT_DIFFERS(projection0Sub1, nullptr);
		TS_ASSERT_EQUALS(projection0Sub1->value, QueryValue(5));
	}

	void testSelect7() {
		auto tokens = Tokenizer::tokenize("SELECT x + 5, y FROM test_table");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto selectOperation = dynamic_cast<QuerySelectOperation*>(operation.get());

		TS_ASSERT_DIFFERS(selectOperation, nullptr);
		TS_ASSERT_EQUALS(selectOperation->table, "test_table");
		TS_ASSERT_EQUALS(selectOperation->projections.size(), 2);
		TS_ASSERT_EQUALS(selectOperation->filter.get(), nullptr);

		auto projection0 = dynamic_cast<QueryMathExpression*>(selectOperation->projections[0].get());
		TS_ASSERT_DIFFERS(projection0, nullptr);
		TS_ASSERT_EQUALS(projection0->op, MathOperator::Add);

		auto projection0Sub0 = dynamic_cast<QueryColumnReferenceExpression*>(projection0->lhs.get());
		TS_ASSERT_DIFFERS(projection0Sub0, nullptr);
		TS_ASSERT_EQUALS(projection0Sub0->name, "x");

		auto projection0Sub1 = dynamic_cast<QueryValueExpression*>(projection0->rhs.get());
		TS_ASSERT_DIFFERS(projection0Sub1, nullptr);
		TS_ASSERT_EQUALS(projection0Sub1->value, QueryValue(5));

		auto projection1 = dynamic_cast<QueryColumnReferenceExpression*>(selectOperation->projections[1].get());
		TS_ASSERT_DIFFERS(projection1, nullptr);
		TS_ASSERT_EQUALS(projection1->name, "y");
	}

	void testSelect8() {
		auto tokens = Tokenizer::tokenize("SELECT x FROM test_table WHERE x > 5");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto selectOperation = dynamic_cast<QuerySelectOperation*>(operation.get());

		TS_ASSERT_DIFFERS(selectOperation, nullptr);
		TS_ASSERT_EQUALS(selectOperation->table, "test_table");
		TS_ASSERT_EQUALS(selectOperation->projections.size(), 1);

		auto projection0 = dynamic_cast<QueryColumnReferenceExpression*>(selectOperation->projections[0].get());
		TS_ASSERT_DIFFERS(projection0, nullptr);
		TS_ASSERT_EQUALS(projection0->name, "x");

		TS_ASSERT_DIFFERS(selectOperation->filter.get(), nullptr);
		auto compareExpression = dynamic_cast<QueryCompareExpression*>(selectOperation->filter.get());
		TS_ASSERT_DIFFERS(compareExpression, nullptr);
		TS_ASSERT_EQUALS(compareExpression->op, CompareOperator::GreaterThan);

		auto compareExpressionSub0 = dynamic_cast<QueryColumnReferenceExpression*>(compareExpression->lhs.get());
		TS_ASSERT_DIFFERS(compareExpressionSub0, nullptr);
		TS_ASSERT_EQUALS(compareExpressionSub0->name, "x");

		auto compareExpressionSub1 = dynamic_cast<QueryValueExpression*>(compareExpression->rhs.get());
		TS_ASSERT_DIFFERS(compareExpressionSub1, nullptr);
		TS_ASSERT_EQUALS(compareExpressionSub1->value, QueryValue(5));
	}

	void testSelect9() {
		auto tokens = Tokenizer::tokenize("SELECT x FROM test_table ORDER BY x");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto selectOperation = dynamic_cast<QuerySelectOperation*>(operation.get());

		TS_ASSERT_DIFFERS(selectOperation, nullptr);
		TS_ASSERT_EQUALS(selectOperation->table, "test_table");
		TS_ASSERT_EQUALS(selectOperation->projections.size(), 1);
		TS_ASSERT_EQUALS(selectOperation->filter.get(), nullptr);
		TS_ASSERT_EQUALS(selectOperation->order.empty(), false);

		TS_ASSERT_EQUALS(selectOperation->order.columns.size(), 1);
		TS_ASSERT_EQUALS(selectOperation->order.columns[0].name, "x");
		TS_ASSERT_EQUALS(selectOperation->order.columns[0].descending, false);
	}

	void testSelect10() {
		auto tokens = Tokenizer::tokenize("SELECT x FROM test_table ORDER BY x DESC");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto selectOperation = dynamic_cast<QuerySelectOperation*>(operation.get());

		TS_ASSERT_DIFFERS(selectOperation, nullptr);
		TS_ASSERT_EQUALS(selectOperation->table, "test_table");
		TS_ASSERT_EQUALS(selectOperation->projections.size(), 1);
		TS_ASSERT_EQUALS(selectOperation->filter.get(), nullptr);
		TS_ASSERT_EQUALS(selectOperation->order.empty(), false);

		TS_ASSERT_EQUALS(selectOperation->order.columns.size(), 1);
		TS_ASSERT_EQUALS(selectOperation->order.columns[0].name, "x");
		TS_ASSERT_EQUALS(selectOperation->order.columns[0].descending, true);
	}

	void testSelect11() {
		auto tokens = Tokenizer::tokenize("SELECT x FROM test_table ORDER BY x ASC");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto selectOperation = dynamic_cast<QuerySelectOperation*>(operation.get());

		TS_ASSERT_DIFFERS(selectOperation, nullptr);
		TS_ASSERT_EQUALS(selectOperation->table, "test_table");
		TS_ASSERT_EQUALS(selectOperation->projections.size(), 1);
		TS_ASSERT_EQUALS(selectOperation->filter.get(), nullptr);
		TS_ASSERT_EQUALS(selectOperation->order.empty(), false);

		TS_ASSERT_EQUALS(selectOperation->order.columns.size(), 1);
		TS_ASSERT_EQUALS(selectOperation->order.columns[0].name, "x");
		TS_ASSERT_EQUALS(selectOperation->order.columns[0].descending, false);
	}

	void testSelect12() {
		auto tokens = Tokenizer::tokenize("SELECT x FROM test_table ORDER BY x, y");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto selectOperation = dynamic_cast<QuerySelectOperation*>(operation.get());

		TS_ASSERT_DIFFERS(selectOperation, nullptr);
		TS_ASSERT_EQUALS(selectOperation->table, "test_table");
		TS_ASSERT_EQUALS(selectOperation->projections.size(), 1);
		TS_ASSERT_EQUALS(selectOperation->filter.get(), nullptr);
		TS_ASSERT_EQUALS(selectOperation->order.empty(), false);

		TS_ASSERT_EQUALS(selectOperation->order.columns.size(), 2);
		TS_ASSERT_EQUALS(selectOperation->order.columns[0].name, "x");
		TS_ASSERT_EQUALS(selectOperation->order.columns[0].descending, false);

		TS_ASSERT_EQUALS(selectOperation->order.columns[1].name, "y");
		TS_ASSERT_EQUALS(selectOperation->order.columns[1].descending, false);
	}

	void testSelect13() {
		auto tokens = Tokenizer::tokenize("SELECT x FROM test_table ORDER BY x ASC, y DESC");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto selectOperation = dynamic_cast<QuerySelectOperation*>(operation.get());

		TS_ASSERT_DIFFERS(selectOperation, nullptr);
		TS_ASSERT_EQUALS(selectOperation->table, "test_table");
		TS_ASSERT_EQUALS(selectOperation->projections.size(), 1);
		TS_ASSERT_EQUALS(selectOperation->filter.get(), nullptr);
		TS_ASSERT_EQUALS(selectOperation->order.empty(), false);

		TS_ASSERT_EQUALS(selectOperation->order.columns.size(), 2);
		TS_ASSERT_EQUALS(selectOperation->order.columns[0].name, "x");
		TS_ASSERT_EQUALS(selectOperation->order.columns[0].descending, false);

		TS_ASSERT_EQUALS(selectOperation->order.columns[1].name, "y");
		TS_ASSERT_EQUALS(selectOperation->order.columns[1].descending, true);
	}

	void testSelect14() {
		auto tokens = Tokenizer::tokenize("SELECT x FROM test_table WHERE x > 5 ORDER BY x");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto selectOperation = dynamic_cast<QuerySelectOperation*>(operation.get());

		TS_ASSERT_DIFFERS(selectOperation, nullptr);
		TS_ASSERT_EQUALS(selectOperation->table, "test_table");
		TS_ASSERT_EQUALS(selectOperation->projections.size(), 1);

		TS_ASSERT_DIFFERS(selectOperation->filter.get(), nullptr);
		auto compareExpression = dynamic_cast<QueryCompareExpression*>(selectOperation->filter.get());
		TS_ASSERT_DIFFERS(compareExpression, nullptr);
		TS_ASSERT_EQUALS(compareExpression->op, CompareOperator::GreaterThan);

		auto compareExpressionSub0 = dynamic_cast<QueryColumnReferenceExpression*>(compareExpression->lhs.get());
		TS_ASSERT_DIFFERS(compareExpressionSub0, nullptr);
		TS_ASSERT_EQUALS(compareExpressionSub0->name, "x");

		auto compareExpressionSub1 = dynamic_cast<QueryValueExpression*>(compareExpression->rhs.get());
		TS_ASSERT_DIFFERS(compareExpressionSub1, nullptr);
		TS_ASSERT_EQUALS(compareExpressionSub1->value, QueryValue(5));

		TS_ASSERT_EQUALS(selectOperation->order.empty(), false);
		TS_ASSERT_EQUALS(selectOperation->order.columns.size(), 1);
		TS_ASSERT_EQUALS(selectOperation->order.columns[0].name, "x");
		TS_ASSERT_EQUALS(selectOperation->order.columns[0].descending, false);
	}

	void testSelect15() {
		auto tokens = Tokenizer::tokenize("SELECT x FROM test_table1 INNER JOIN test_table2 ON x=y");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto selectOperation = dynamic_cast<QuerySelectOperation*>(operation.get());

		TS_ASSERT_DIFFERS(selectOperation, nullptr);
		TS_ASSERT_EQUALS(selectOperation->table, "test_table1");
		TS_ASSERT_EQUALS(selectOperation->projections.size(), 1);
		TS_ASSERT_EQUALS(selectOperation->filter.get(), nullptr);
		TS_ASSERT_EQUALS(selectOperation->join.empty, false);

		TS_ASSERT_EQUALS(selectOperation->join.joinFromColumn, "x");
		TS_ASSERT_EQUALS(selectOperation->join.joinOnTable, "test_table2");
		TS_ASSERT_EQUALS(selectOperation->join.joinOnColumn, "y");
	}
};