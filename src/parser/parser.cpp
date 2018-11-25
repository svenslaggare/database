#include "parser.h"
#include "../query_expressions/expressions.h"

#include <cmath>
#include <unordered_set>
#include <algorithm>
#include <unordered_map>

namespace {
//	std::unordered_set<char> twoCharOps = { '<', '>' };
//
//	OperatorChar tokenAsOperator(const Token& token) {
//		if (token.type() == TokenType::TWO_CHAR_OPERATOR) {
//			return OperatorChar(token.charValue(), token.charValue2());
//		} else {
//			return OperatorChar(token.charValue());
//		}
//	}
}

std::vector<Token> Tokenizer::tokenize(std::string str) {
	std::vector<Token> tokens;
	for (std::size_t i = 0; i < str.size(); i++) {
		char current = str[i];

		//Skip whitespaces
		if (isspace(current)) {
			continue;
		}

		//Parenthesis
		if (current == '(') {
			tokens.emplace_back(TokenType::LeftParenthesis);
			continue;
		}

		if (current == ')') {
			tokens.emplace_back(TokenType::RightParenthesis);
			continue;
		}

		//Comma
		if (current == ',') {
			tokens.emplace_back(TokenType::Comma);
			continue;
		}

		if (current == '.') {
			tokens.emplace_back(TokenType::Dot);
			continue;
		}

		//Number
		if (std::isdigit(current)) {
			std::string number { current };
			auto type = TokenType::Int32;
			bool hasDecimalPoint = false;

			while (true) {
				std::size_t next = i + 1;

				if (next >= str.size()) {
					break;
				}

				current = str[next];

				if (current == '.') {
					if (hasDecimalPoint) {
						throw std::runtime_error("This token already has a decimal point.");
					}

					type = TokenType::Float32;
					hasDecimalPoint = true;
				}

				if (!(isdigit(current) || current == '.')) {
					break;
				}

				number += current;
				i = next;
			}

			if (type == TokenType::Int32) {
				tokens.emplace_back(std::stoi(number, nullptr, 10));
			} else if (type == TokenType::Float32) {
				tokens.emplace_back(std::stof(number, nullptr));
			}

			continue;
		}

		//Identifier
		if (isalpha(current)) {
			std::string identifier { current };

			while (true) {
				std::size_t next = i + 1;

				if (next >= str.size()) {
					break;
				}

				current = str[next];

				if (!(isdigit(current) || isalpha(current) || current == '.' || current == '_')) {
					break;
				}

				identifier += current;
				i = next;
			}

			auto identifierLower = identifier;
			std::transform(identifierLower.begin(), identifierLower.end(), identifierLower.begin(), ::tolower);

			static std::unordered_map<std::string, TokenType> keywords {
				{ "select", TokenType::Select },
				{ "update", TokenType::Update },
				{ "insert", TokenType::Insert },
				{ "from", TokenType::From },
				{ "where", TokenType::Where },
				{ "order", TokenType::Order },
				{ "by", TokenType::By },
				{ "asc", TokenType::Asc },
				{ "desc", TokenType::Desc },
				{ "inner", TokenType::Inner },
				{ "join", TokenType::Join },
				{ "on", TokenType::On },
			};

			if (keywords.count(identifierLower) > 0) {
				tokens.emplace_back(keywords[identifierLower]);
				continue;
			}

			if (identifierLower == "true") {
				tokens.emplace_back(TokenType::Bool, true);
				continue;
			}

			if (identifierLower == "false") {
				tokens.emplace_back(TokenType::Bool, false);
				continue;
			}

			tokens.emplace_back(identifier);
			continue;
		}

		//Operator
		tokens.emplace_back(TokenType::Operator, current);
	}

	return tokens;
}

QueryParser::QueryParser(std::vector<Token> tokens)
	:  mTokens(std::move(tokens)), mTokenIndex(-1) {
	mOperators = {
		{ '*', 7 },
		{ '/', 7 },
		{ '+', 6 },
		{ '-', 6 },
		{ '>', 5 },
		{ '<', 5 },
	};

	mTokens.push_back(TokenType::EndOfTokens);
}

void QueryParser::parseError(std::string message) {
	throw std::runtime_error(message);
}

Token& QueryParser::nextToken() {
	mTokenIndex++;

	if ((std::size_t)mTokenIndex >= mTokens.size()) {
		parseError("Reached end of tokens.");
	}

	mCurrentToken = mTokens[mTokenIndex];
	return mCurrentToken;
}

Token& QueryParser::peekToken(int delta) {
	int nextTokenIndex = mTokenIndex + delta;

	if ((std::size_t)nextTokenIndex >= mTokens.size()) {
		parseError("Reached end of tokens.");
	}

	return mTokens[nextTokenIndex];
}

int QueryParser::getTokenPrecedence() {
	if (mCurrentToken.type() != TokenType::Operator) {
		return -1;
	}

	auto op = mCurrentToken.charValue();
	if (mOperators.count(op) > 0) {
		return mOperators.at(op);
	} else {
		parseError("'" + std::string { op } + "' is not a defined binary operator.");
	}

	return -1;
}

std::unique_ptr<QueryExpression> QueryParser::parseInt32Expression() {
	auto value = mCurrentToken.int32Value();
	nextToken();
	return std::make_unique<QueryValueExpression>(QueryValue(value));
}

std::unique_ptr<QueryExpression> QueryParser::parseFloat32Expression() {
	auto value = mCurrentToken.float32Value();
	nextToken();
	return std::make_unique<QueryValueExpression>(QueryValue(value));
}

std::unique_ptr<QueryExpression> QueryParser::parseBoolExpression() {
	auto value = mCurrentToken.boolValue();
	nextToken();
	return std::make_unique<QueryValueExpression>(QueryValue(value));
}

std::unique_ptr<QueryExpression> QueryParser::parseIdentifierExpression() {
	std::string identifier = mCurrentToken.identifier();

	//Eat the identifier.
	nextToken();

	if (mCurrentToken.type() != TokenType::LeftParenthesis) {
		return std::make_unique<QueryColumnReferenceExpression>(identifier);
	}

	throw std::runtime_error("not implemented");

//	//Function call
//	nextToken(); //Eat the '('
//	std::vector<std::unique_ptr<QueryExpression>> arguments;
//
//	if (mCurrentToken.type() != TokenType::RightParenthesis) {
//		while (true) {
//			auto arg = parseExpression();
//			arguments.push_back(std::move(arg));
//
//			if (mCurrentToken.type() == TokenType::RightParenthesis) {
//				break;
//			}
//
//			if (mCurrentToken.type() != TokenType::Comma) {
//				parseError("Expected ',' or ')' in argument list.");
//			}
//
//			nextToken();
//		}
//	}
//
//	//Eat the ')'
//	nextToken();
//
//	return std::unique_ptr<FunctionCallExpression>(
//		new FunctionCallExpression(identifier, std::move(arguments)));
}

std::unique_ptr<QueryExpression> QueryParser::parseParenthesisExpression() {
	nextToken(); //Eat the '('
	auto expr = parseExpression();

	if (mCurrentToken.type() != TokenType::RightParenthesis) {
		parseError("Expected ').'");
	}

	nextToken(); //Eat the ')'
	return expr;
}

std::unique_ptr<QueryExpression> QueryParser::parsePrimaryExpression() {
	switch (mCurrentToken.type()) {
	case TokenType::Int32:
		return parseInt32Expression();
	case TokenType::Float32:
		return parseFloat32Expression();
	case TokenType::Bool:
		return parseBoolExpression();
	case TokenType::Identifier:
		return parseIdentifierExpression();
	case TokenType::LeftParenthesis:
		return parseParenthesisExpression();
	default:
		parseError("Expected an expression");
		return nullptr;
	}
}

std::unique_ptr<QueryExpression> QueryParser::parseBinaryOpRHS(int precedence, std::unique_ptr<QueryExpression> lhs) {
	while (true) {
		//If this is a bin op, find its precedence
		int tokPrecedence = getTokenPrecedence();

		//If this is a binary operator that binds at least as tightly as the current operator, consume it, otherwise we are done.
		if (tokPrecedence < precedence) {
			return lhs;
		}

		auto opChar = mCurrentToken.charValue();
		nextToken(); //Eat the operator

		//Parse the unary expression after the binary operator
		auto rhs = parseUnaryExpression();

		//If the binary operator binds less tightly with RHS than the operator after RHS, let the pending operator take RHS as its LHS
		int nextPrecedence = getTokenPrecedence();
		if (tokPrecedence < nextPrecedence) {
			rhs = parseBinaryOpRHS(tokPrecedence + 1, std::move(rhs));
		}

		//Merge LHS and RHS
		switch (opChar) {
			case '+':
				lhs = std::make_unique<QueryMathExpression>(std::move(lhs), std::move(rhs), MathOperator::Add);
				break;
			case '-':
				lhs = std::make_unique<QueryMathExpression>(std::move(lhs), std::move(rhs), MathOperator::Sub);
				break;
			case '*':
				lhs = std::make_unique<QueryMathExpression>(std::move(lhs), std::move(rhs), MathOperator::Mul);
				break;
			case '/':
				lhs = std::make_unique<QueryMathExpression>(std::move(lhs), std::move(rhs), MathOperator::Div);
				break;
			case '<':
				lhs = std::make_unique<QueryCompareExpression>(std::move(lhs), std::move(rhs), CompareOperator::LessThan);
				break;
			case '>':
				lhs = std::make_unique<QueryCompareExpression>(std::move(lhs), std::move(rhs), CompareOperator::GreaterThan);
				break;
			default:
				throw std::runtime_error("Not a valid operator.");
		}
	}
}

std::unique_ptr<QueryExpression> QueryParser::parseUnaryExpression() {
	return parsePrimaryExpression();

//	//If the current token isn't an operator, is must be a primary expression
//	if (mCurrentToken.type() != TokenType::Operator) {
//		return parsePrimaryExpression();
//	}
//
//	//If this is a unary operator, read it.
//	auto op = tokenAsOperator(mCurrentToken);
//	nextToken(); //Eat the operator
//
//	auto operand = parseUnaryExpression();
//
//	if (operand != nullptr) {
//		if (mCalcEngine.unaryOperators().count(op) == 0) {
//			parseError("'" + op.toString() + "' is not a defined unary operator.");
//		}
//
//		return std::unique_ptr<UnaryOperatorExpression>(
//			new UnaryOperatorExpression(op, std::move(operand)));
//	}
//
//	return operand;
}


std::unique_ptr<QueryExpression> QueryParser::parseExpression() {
	return parseBinaryOpRHS(0, parseUnaryExpression());
}

void QueryParser::parseOrder(OrderingClause& ordering) {
	nextToken();
	if (mCurrentToken.type() != TokenType::By) {
		parseError("Expected 'by' keyword.");
	}
	nextToken();

	while (true) {
		if (mCurrentToken.type() != TokenType::Identifier) {
			parseError("Expected an identifier.");
		}

		auto orderBy = mCurrentToken.identifier();
		nextToken();

		bool descending = false;
		if (mCurrentToken.type() == TokenType::Asc) {
			descending = false;
			nextToken();
		} else if (mCurrentToken.type() == TokenType::Desc) {
			descending = true;
			nextToken();
		}

		ordering.columns.push_back(OrderingColumn{ orderBy, descending });

		if (mCurrentToken.type() == TokenType::Comma) {
			nextToken();
		} else {
			break;
		}
	}
}

void QueryParser::parseJoin(JoinClause& join) {
	nextToken();
	if (mCurrentToken.type() != TokenType::Join) {
		parseError("Expected 'join' keyword.");
	}
	nextToken();

	if (mCurrentToken.type() != TokenType::Identifier) {
		parseError("Expected an identifier.");
	}
	auto joinOnTable = mCurrentToken.identifier();
	nextToken();

	if (mCurrentToken.type() != TokenType::On) {
		parseError("Expected 'on' keyword.");
	}
	nextToken();

	if (mCurrentToken.type() != TokenType::Identifier) {
		parseError("Expected an identifier.");
	}
	auto joinFromColumn = mCurrentToken.identifier();
	nextToken();

	if (!(mCurrentToken.type() == TokenType::Operator && mCurrentToken.charValue() == '=')) {
		parseError("Expected '='.");
	}
	nextToken();

	if (mCurrentToken.type() != TokenType::Identifier) {
		parseError("Expected an identifier.");
	}
	auto joinOnColumn = mCurrentToken.identifier();
	nextToken();

	join = JoinClause(joinFromColumn, joinOnTable, joinOnColumn);
}

std::unique_ptr<QueryOperation> QueryParser::parseSelect() {
	std::vector<std::unique_ptr<QueryExpression>> projections;
	nextToken();
	while (true) {
		projections.push_back(parseExpression());

		if (mCurrentToken.type() == TokenType::Comma) {
			nextToken();
		}

		if (mCurrentToken.type() == TokenType::From) {
			nextToken();
			break;
		}
	}

	if (mCurrentToken.type() != TokenType::Identifier) {
		parseError("Expected an identifier.");
	}

	auto tableName = mCurrentToken.identifier();
	nextToken();

	std::unique_ptr<QueryExpression> filterExpression;
	OrderingClause ordering;
	JoinClause join;

	if (mCurrentToken.type() != TokenType::EndOfTokens) {
		while (true) {
			switch (mCurrentToken.type()) {
				case TokenType::Where:
					nextToken();
					filterExpression = parseExpression();
					break;
				case TokenType::Order:
					parseOrder(ordering);
					break;
				case TokenType::Inner: {
					parseJoin(join);
					break;
				}
				default:
					parseError("Expected where, order or inner.");
					return nullptr;
			}

			if (mCurrentToken.type() == TokenType::EndOfTokens) {
				break;
			}
		}
	}

	return std::make_unique<QuerySelectOperation>(
		tableName,
		std::move(projections),
		std::move(filterExpression),
		join,
		ordering);
}

std::unique_ptr<QueryOperation> QueryParser::parseUpdate() {
	return std::unique_ptr<QueryOperation>();
}

std::unique_ptr<QueryOperation> QueryParser::parseInsert() {
	return std::unique_ptr<QueryOperation>();
}

std::unique_ptr<QueryOperation> QueryParser::parse() {
	nextToken();
	switch (mCurrentToken.type()) {
		case TokenType::Select:
			return parseSelect();
		case TokenType::Update:
			return parseUpdate();
		case TokenType::Insert:
			return parseInsert();
		default:
			throw std::runtime_error("Expected: select, update or insert.");
	}
}
