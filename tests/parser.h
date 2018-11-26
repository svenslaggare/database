#pragma once
#include <iostream>
#include <cxxtest/TestSuite.h>
#include "../src/query_parser/parser.h"
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
		auto tokens = Tokenizer::tokenize("SELECT x FROM test_table WHERE x > 5 AND x < 10");
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
		auto compareExpression = dynamic_cast<QueryAndExpression*>(selectOperation->filter.get());
		TS_ASSERT_DIFFERS(compareExpression, nullptr);

		auto compareExpressionSub0 = dynamic_cast<QueryCompareExpression*>(compareExpression->lhs.get());
		TS_ASSERT_DIFFERS(compareExpressionSub0, nullptr);
		TS_ASSERT_EQUALS(compareExpressionSub0->op, CompareOperator::GreaterThan);

		auto compareExpressionSub00 = dynamic_cast<QueryColumnReferenceExpression*>(compareExpressionSub0->lhs.get());
		TS_ASSERT_DIFFERS(compareExpressionSub00, nullptr);
		TS_ASSERT_EQUALS(compareExpressionSub00->name, "x");

		auto compareExpressionSub01 = dynamic_cast<QueryValueExpression*>(compareExpressionSub0->rhs.get());
		TS_ASSERT_DIFFERS(compareExpressionSub01, nullptr);
		TS_ASSERT_EQUALS(compareExpressionSub01->value, QueryValue(5));

		auto compareExpressionSub1 = dynamic_cast<QueryCompareExpression*>(compareExpression->rhs.get());
		TS_ASSERT_DIFFERS(compareExpressionSub1, nullptr);
		TS_ASSERT_EQUALS(compareExpressionSub1->op, CompareOperator::LessThan);

		auto compareExpressionSub10 = dynamic_cast<QueryColumnReferenceExpression*>(compareExpressionSub1->lhs.get());
		TS_ASSERT_DIFFERS(compareExpressionSub10, nullptr);
		TS_ASSERT_EQUALS(compareExpressionSub10->name, "x");

		auto compareExpressionSub11 = dynamic_cast<QueryValueExpression*>(compareExpressionSub1->rhs.get());
		TS_ASSERT_DIFFERS(compareExpressionSub11, nullptr);
		TS_ASSERT_EQUALS(compareExpressionSub11->value, QueryValue(10));
	}

	void testSelectOrder1() {
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

	void testSelectOrder2() {
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

	void testSelectOrder3() {
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

	void testSelectOrder4() {
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

	void testSelectOrder5() {
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

	void testSelectOrder6() {
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

	void testSelectJoin1() {
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

	void testUpdate1() {
		auto tokens = Tokenizer::tokenize("UPDATE test_table SET x=y");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto updateOperation = dynamic_cast<QueryUpdateOperation*>(operation.get());

		TS_ASSERT_DIFFERS(updateOperation, nullptr);
		TS_ASSERT_EQUALS(updateOperation->table, "test_table");
		TS_ASSERT_EQUALS(updateOperation->sets.size(), 1);
		TS_ASSERT_EQUALS(updateOperation->filter.get(), nullptr);

		TS_ASSERT_EQUALS(updateOperation->sets[0]->column, "x");

		auto set0RHS = dynamic_cast<QueryColumnReferenceExpression*>(updateOperation->sets[0]->value.get());
		TS_ASSERT_DIFFERS(set0RHS, nullptr);
		TS_ASSERT_EQUALS(set0RHS->name, "y");
	}

	void testUpdate2() {
		auto tokens = Tokenizer::tokenize("UPDATE test_table SET x=y, y=z");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto updateOperation = dynamic_cast<QueryUpdateOperation*>(operation.get());

		TS_ASSERT_DIFFERS(updateOperation, nullptr);
		TS_ASSERT_EQUALS(updateOperation->table, "test_table");
		TS_ASSERT_EQUALS(updateOperation->sets.size(), 2);
		TS_ASSERT_EQUALS(updateOperation->filter.get(), nullptr);

		TS_ASSERT_EQUALS(updateOperation->sets[0]->column, "x");
		auto set0RHS = dynamic_cast<QueryColumnReferenceExpression*>(updateOperation->sets[0]->value.get());
		TS_ASSERT_DIFFERS(set0RHS, nullptr);
		TS_ASSERT_EQUALS(set0RHS->name, "y");

		TS_ASSERT_EQUALS(updateOperation->sets[1]->column, "y");
		auto set1RHS = dynamic_cast<QueryColumnReferenceExpression*>(updateOperation->sets[1]->value.get());
		TS_ASSERT_DIFFERS(set1RHS, nullptr);
		TS_ASSERT_EQUALS(set1RHS->name, "z");
	}

	void testUpdate3() {
		auto tokens = Tokenizer::tokenize("UPDATE test_table SET x=x + 5, y=z");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto updateOperation = dynamic_cast<QueryUpdateOperation*>(operation.get());

		TS_ASSERT_DIFFERS(updateOperation, nullptr);
		TS_ASSERT_EQUALS(updateOperation->table, "test_table");
		TS_ASSERT_EQUALS(updateOperation->sets.size(), 2);
		TS_ASSERT_EQUALS(updateOperation->filter.get(), nullptr);

		TS_ASSERT_EQUALS(updateOperation->sets[0]->column, "x");
		auto set0RHS = dynamic_cast<QueryMathExpression*>(updateOperation->sets[0]->value.get());
		TS_ASSERT_DIFFERS(set0RHS, nullptr);
		TS_ASSERT_EQUALS(set0RHS->op, MathOperator::Add);

		auto set0RHSLHS = dynamic_cast<QueryColumnReferenceExpression*>(set0RHS->lhs.get());
		TS_ASSERT_DIFFERS(set0RHSLHS, nullptr);
		TS_ASSERT_EQUALS(set0RHSLHS->name, "x");

		auto set0RHSRHS = dynamic_cast<QueryValueExpression*>(set0RHS->rhs.get());
		TS_ASSERT_DIFFERS(set0RHSRHS, nullptr);
		TS_ASSERT_EQUALS(set0RHSRHS->value, QueryValue(5));

		TS_ASSERT_EQUALS(updateOperation->sets[1]->column, "y");
		auto set1RHS = dynamic_cast<QueryColumnReferenceExpression*>(updateOperation->sets[1]->value.get());
		TS_ASSERT_DIFFERS(set1RHS, nullptr);
		TS_ASSERT_EQUALS(set1RHS->name, "z");
	}

	void testUpdateWhere1() {
		auto tokens = Tokenizer::tokenize("UPDATE test_table SET x=y WHERE x > 5");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto updateOperation = dynamic_cast<QueryUpdateOperation*>(operation.get());

		TS_ASSERT_DIFFERS(updateOperation, nullptr);
		TS_ASSERT_EQUALS(updateOperation->table, "test_table");
		TS_ASSERT_EQUALS(updateOperation->sets.size(), 1);

		TS_ASSERT_EQUALS(updateOperation->sets[0]->column, "x");

		auto set0RHS = dynamic_cast<QueryColumnReferenceExpression*>(updateOperation->sets[0]->value.get());
		TS_ASSERT_DIFFERS(set0RHS, nullptr);
		TS_ASSERT_EQUALS(set0RHS->name, "y");

		TS_ASSERT_DIFFERS(updateOperation->filter.get(), nullptr);
		auto compareExpression = dynamic_cast<QueryCompareExpression*>(updateOperation->filter.get());
		TS_ASSERT_DIFFERS(compareExpression, nullptr);
		TS_ASSERT_EQUALS(compareExpression->op, CompareOperator::GreaterThan);

		auto compareExpressionSub0 = dynamic_cast<QueryColumnReferenceExpression*>(compareExpression->lhs.get());
		TS_ASSERT_DIFFERS(compareExpressionSub0, nullptr);
		TS_ASSERT_EQUALS(compareExpressionSub0->name, "x");

		auto compareExpressionSub1 = dynamic_cast<QueryValueExpression*>(compareExpression->rhs.get());
		TS_ASSERT_DIFFERS(compareExpressionSub1, nullptr);
		TS_ASSERT_EQUALS(compareExpressionSub1->value, QueryValue(5));
	}

	void testUpdateWhere2() {
		auto tokens = Tokenizer::tokenize("UPDATE test_table SET x=x + 5, y=z WHERE x > 5");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto updateOperation = dynamic_cast<QueryUpdateOperation*>(operation.get());

		TS_ASSERT_DIFFERS(updateOperation, nullptr);
		TS_ASSERT_EQUALS(updateOperation->table, "test_table");
		TS_ASSERT_EQUALS(updateOperation->sets.size(), 2);

		TS_ASSERT_EQUALS(updateOperation->sets[0]->column, "x");
		auto set0RHS = dynamic_cast<QueryMathExpression*>(updateOperation->sets[0]->value.get());
		TS_ASSERT_DIFFERS(set0RHS, nullptr);
		TS_ASSERT_EQUALS(set0RHS->op, MathOperator::Add);

		auto set0RHSLHS = dynamic_cast<QueryColumnReferenceExpression*>(set0RHS->lhs.get());
		TS_ASSERT_DIFFERS(set0RHSLHS, nullptr);
		TS_ASSERT_EQUALS(set0RHSLHS->name, "x");

		auto set0RHSRHS = dynamic_cast<QueryValueExpression*>(set0RHS->rhs.get());
		TS_ASSERT_DIFFERS(set0RHSRHS, nullptr);
		TS_ASSERT_EQUALS(set0RHSRHS->value, QueryValue(5));

		TS_ASSERT_EQUALS(updateOperation->sets[1]->column, "y");
		auto set1RHS = dynamic_cast<QueryColumnReferenceExpression*>(updateOperation->sets[1]->value.get());
		TS_ASSERT_DIFFERS(set1RHS, nullptr);
		TS_ASSERT_EQUALS(set1RHS->name, "z");

		TS_ASSERT_DIFFERS(updateOperation->filter.get(), nullptr);
		auto compareExpression = dynamic_cast<QueryCompareExpression*>(updateOperation->filter.get());
		TS_ASSERT_DIFFERS(compareExpression, nullptr);
		TS_ASSERT_EQUALS(compareExpression->op, CompareOperator::GreaterThan);

		auto compareExpressionSub0 = dynamic_cast<QueryColumnReferenceExpression*>(compareExpression->lhs.get());
		TS_ASSERT_DIFFERS(compareExpressionSub0, nullptr);
		TS_ASSERT_EQUALS(compareExpressionSub0->name, "x");

		auto compareExpressionSub1 = dynamic_cast<QueryValueExpression*>(compareExpression->rhs.get());
		TS_ASSERT_DIFFERS(compareExpressionSub1, nullptr);
		TS_ASSERT_EQUALS(compareExpressionSub1->value, QueryValue(5));
	}

	void testInsert1() {
		auto tokens = Tokenizer::tokenize("INSERT INTO test_table (x, y, z) VALUES (10, 12.0, false)");
		QueryParser parser(tokens);
		auto operation = parser.parse();
		auto insertOperation = dynamic_cast<QueryInsertOperation*>(operation.get());

		TS_ASSERT_DIFFERS(insertOperation, nullptr);
		TS_ASSERT_EQUALS(insertOperation->table, "test_table");

		TS_ASSERT_EQUALS(insertOperation->columns.size(), 3);
		TS_ASSERT_EQUALS(insertOperation->columns[0], "x");
		TS_ASSERT_EQUALS(insertOperation->columns[1], "y");
		TS_ASSERT_EQUALS(insertOperation->columns[2], "z");

		TS_ASSERT_EQUALS(insertOperation->values.size(), 1);
		TS_ASSERT_EQUALS(insertOperation->values[0].size(), 3);
		TS_ASSERT_EQUALS(insertOperation->values[0][0], QueryValue(10));
		TS_ASSERT_EQUALS(insertOperation->values[0][1], QueryValue(12.0f));
		TS_ASSERT_EQUALS(insertOperation->values[0][2], QueryValue(false));

	}
};