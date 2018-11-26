#pragma once
#include <iostream>
#include <cxxtest/TestSuite.h>
#include "../src/query_parser/parser.h"

class TokenizerTestSuite : public CxxTest::TestSuite {
public:
	void testInt1() {
		auto tokens = Tokenizer::tokenize("4343");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Int32);
		TS_ASSERT_EQUALS(tokens[0].int32Value(), 4343);
	}

	void testInt2() {
		auto tokens = Tokenizer::tokenize("1");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Int32);
		TS_ASSERT_EQUALS(tokens[0].int32Value(), 1);
	}

	void testFloat1() {
		auto tokens = Tokenizer::tokenize("1.0");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Float32);
		TS_ASSERT_EQUALS(tokens[0].float32Value(), 1);
	}

	void testFloat2() {
		auto tokens = Tokenizer::tokenize("12.40");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Float32);
		TS_ASSERT_DELTA(tokens[0].float32Value(), 12.40f, 1E-7);
	}

	void testTrue() {
		auto tokens = Tokenizer::tokenize("true");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Bool);
		TS_ASSERT_EQUALS(tokens[0].boolValue(), true);
	}

	void testFalse() {
		auto tokens = Tokenizer::tokenize("false");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Bool);
		TS_ASSERT_EQUALS(tokens[0].boolValue(), false);
	}

	void testOperator1() {
		auto tokens = Tokenizer::tokenize(">");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Operator);
		TS_ASSERT_EQUALS(tokens[0].operatorValue(), '>');
	}

	void testOperator2() {
		auto tokens = Tokenizer::tokenize(">=");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Operator);
		TS_ASSERT_EQUALS(tokens[0].operatorValue(), OperatorChar('>', '='));
	}

	void testOperator3() {
		auto tokens = Tokenizer::tokenize("!=");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Operator);
		TS_ASSERT_EQUALS(tokens[0].operatorValue(), OperatorChar('!', '='));
	}

	void testOperator4() {
		auto tokens = Tokenizer::tokenize("<=");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Operator);
		TS_ASSERT_EQUALS(tokens[0].operatorValue(), OperatorChar('<', '='));
	}

	void testOperator5() {
		auto tokens = Tokenizer::tokenize("==");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Operator);
		TS_ASSERT_EQUALS(tokens[0].operatorValue(), OperatorChar('=', '='));
	}

	void testIdentifier1() {
		auto tokens = Tokenizer::tokenize("haha");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Identifier);
		TS_ASSERT_EQUALS(tokens[0].identifier(), "haha");
	}

	void testIdentifier2() {
		auto tokens = Tokenizer::tokenize("select");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Select);
	}

	void testIdentifier3() {
		auto tokens = Tokenizer::tokenize("SELECT");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Select);
	}

	void testAllIdentifiers() {
		auto tokens = Tokenizer::tokenize("select");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Select);

		tokens = Tokenizer::tokenize("update");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Update);

		tokens = Tokenizer::tokenize("insert");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Insert);

		tokens = Tokenizer::tokenize("from");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::From);

		tokens = Tokenizer::tokenize("where");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Where);

		tokens = Tokenizer::tokenize("inner");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Inner);

		tokens = Tokenizer::tokenize("join");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Join);

		tokens = Tokenizer::tokenize("on");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::On);

		tokens = Tokenizer::tokenize("order");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Order);

		tokens = Tokenizer::tokenize("by");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::By);

		tokens = Tokenizer::tokenize("asc");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Asc);

		tokens = Tokenizer::tokenize("desc");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Desc);

		tokens = Tokenizer::tokenize("and");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::And);

		tokens = Tokenizer::tokenize("set");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Set);

		tokens = Tokenizer::tokenize("into");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Into);

		tokens = Tokenizer::tokenize("values");
		TS_ASSERT_EQUALS(tokens.size(), 1);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::Values);
	}

	void testSeparators1() {
		auto tokens = Tokenizer::tokenize("(),.");
		TS_ASSERT_EQUALS(tokens.size(), 4);
		TS_ASSERT_EQUALS(tokens[0].type(), TokenType::LeftParenthesis);
		TS_ASSERT_EQUALS(tokens[1].type(), TokenType::RightParenthesis);
		TS_ASSERT_EQUALS(tokens[2].type(), TokenType::Comma);
		TS_ASSERT_EQUALS(tokens[3].type(), TokenType::Dot);
	}

	void testComplex1() {
		auto tokens = Tokenizer::tokenize("SELECT x FROM test_table");
		TS_ASSERT_EQUALS(tokens.size(), 4);
		TS_ASSERT_EQUALS(tokens[0], Token(TokenType::Select));
		TS_ASSERT_EQUALS(tokens[1], Token("x"));
		TS_ASSERT_EQUALS(tokens[2], Token(TokenType::From));
		TS_ASSERT_EQUALS(tokens[3], Token("test_table"));
	}

	void testComplex2() {
		auto tokens = Tokenizer::tokenize("SELECT test_table.x FROM test_table WHERE x > 5");
		TS_ASSERT_EQUALS(tokens.size(), 8);
		TS_ASSERT_EQUALS(tokens[0], Token(TokenType::Select));
		TS_ASSERT_EQUALS(tokens[1], Token("test_table.x"));
		TS_ASSERT_EQUALS(tokens[2], Token(TokenType::From));
		TS_ASSERT_EQUALS(tokens[3], Token("test_table"));
		TS_ASSERT_EQUALS(tokens[4], Token(TokenType::Where));
		TS_ASSERT_EQUALS(tokens[5], Token("x"));
		TS_ASSERT_EQUALS(tokens[6], Token(TokenType::Operator, OperatorChar('>')));
		TS_ASSERT_EQUALS(tokens[7], Token(5));
	}

	void testComplex3() {
		auto tokens = Tokenizer::tokenize("SELECT x, y, z FROM test_table");
		TS_ASSERT_EQUALS(tokens.size(), 8);
		TS_ASSERT_EQUALS(tokens[0], Token(TokenType::Select));
		TS_ASSERT_EQUALS(tokens[1], Token("x"));
		TS_ASSERT_EQUALS(tokens[2], Token(TokenType::Comma));
		TS_ASSERT_EQUALS(tokens[3], Token("y"));
		TS_ASSERT_EQUALS(tokens[4], Token(TokenType::Comma));
		TS_ASSERT_EQUALS(tokens[5], Token("z"));
		TS_ASSERT_EQUALS(tokens[6], Token(TokenType::From));
		TS_ASSERT_EQUALS(tokens[7], Token("test_table"));
	}

	void testComplex4() {
		auto tokens = Tokenizer::tokenize("SELECT test_table.x FROM test_table WHERE x >= 5");
		TS_ASSERT_EQUALS(tokens.size(), 8);
		TS_ASSERT_EQUALS(tokens[0], Token(TokenType::Select));
		TS_ASSERT_EQUALS(tokens[1], Token("test_table.x"));
		TS_ASSERT_EQUALS(tokens[2], Token(TokenType::From));
		TS_ASSERT_EQUALS(tokens[3], Token("test_table"));
		TS_ASSERT_EQUALS(tokens[4], Token(TokenType::Where));
		TS_ASSERT_EQUALS(tokens[5], Token("x"));
		TS_ASSERT_EQUALS(tokens[6], Token(TokenType::Operator, OperatorChar('>', '=')));
		TS_ASSERT_EQUALS(tokens[7], Token(5));
	}

	void testComplex5() {
		auto tokens = Tokenizer::tokenize("SELECT test_table.x FROM test_table WHERE x != 5");
		TS_ASSERT_EQUALS(tokens.size(), 8);
		TS_ASSERT_EQUALS(tokens[0], Token(TokenType::Select));
		TS_ASSERT_EQUALS(tokens[1], Token("test_table.x"));
		TS_ASSERT_EQUALS(tokens[2], Token(TokenType::From));
		TS_ASSERT_EQUALS(tokens[3], Token("test_table"));
		TS_ASSERT_EQUALS(tokens[4], Token(TokenType::Where));
		TS_ASSERT_EQUALS(tokens[5], Token("x"));
		TS_ASSERT_EQUALS(tokens[6], Token(TokenType::Operator, OperatorChar('!', '=')));
		TS_ASSERT_EQUALS(tokens[7], Token(5));
	}
};