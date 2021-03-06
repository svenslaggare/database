#include "parser.h"
#include "../query_expressions/expressions.h"

#include <cmath>
#include <unordered_set>
#include <algorithm>
#include <unordered_map>

namespace {
	std::unordered_set<char> twoCharOps = { '<', '>', '!', '=' };
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
				{ "and", TokenType::And },
				{ "set", TokenType::Set },
				{ "into", TokenType::Into },
				{ "values", TokenType::Values },
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
		if (!tokens.empty()
			&& tokens.back().type() == TokenType::Operator
			&& (twoCharOps.count(tokens.back().operatorValue().op1()) > 0)) {
			//If the previous token is an operator and the current one also is, upgrade to a two-op char
			tokens.back() = Token(TokenType::Operator, OperatorChar(tokens.back().operatorValue().op1(), current));
		} else {
			tokens.emplace_back(TokenType::Operator, OperatorChar(current));
		}
	}

	return tokens;
}

QueryParser::QueryParser(std::vector<Token> tokens)
	:  mTokens(std::move(tokens)), mTokenIndex(-1) {
	mOperators = {
		{ OperatorChar('*'), 7 },
		{ OperatorChar('/'), 7 },
		{ OperatorChar('+'), 6 },
		{ OperatorChar('-'), 6 },
		{ OperatorChar('>'), 5 },
		{ OperatorChar('<'), 5 },
		{ OperatorChar('<', '='), 5 },
		{ OperatorChar('>', '='), 5 },
		{ OperatorChar('=', '='), 5 },
		{ OperatorChar('!', '='), 5 },
	};

	mTokens.emplace_back(TokenType::EndOfTokens);
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
	if (mCurrentToken.type() == TokenType::And) {
		return 3;
	}

	if (mCurrentToken.type() != TokenType::Operator) {
		return -1;
	}

	auto op = mCurrentToken.operatorValue();
	if (mOperators.count(op) > 0) {
		return mOperators.at(op);
	} else {
		parseError("'" + op.toString() + "' is not a defined binary operator.");
	}

	return -1;
}

void QueryParser::assertAndConsume(TokenType type, const std::string& errorMessage) {
	if (mCurrentToken.type() != type) {
		parseError(errorMessage);
	}

	nextToken();
}

std::string QueryParser::consumeIdentifier() {
	if (mCurrentToken.type() != TokenType::Identifier) {
		parseError("Expected an identifier.");
	}

	auto identifier = mCurrentToken.identifier();
	nextToken();

	return identifier;
}

std::unique_ptr<QueryValueExpression> QueryParser::parseInt32Expression() {
	auto value = mCurrentToken.int32Value();
	nextToken();
	return std::make_unique<QueryValueExpression>(QueryValue(value));
}

std::unique_ptr<QueryValueExpression> QueryParser::parseFloat32Expression() {
	auto value = mCurrentToken.float32Value();
	nextToken();
	return std::make_unique<QueryValueExpression>(QueryValue(value));
}

std::unique_ptr<QueryValueExpression> QueryParser::parseBoolExpression() {
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
	auto expression = parseExpression();
	assertAndConsume(TokenType::RightParenthesis, "Expected ')'.");
	return expression;
}

std::unique_ptr<QueryValueExpression> QueryParser::parseValueExpression() {
	switch (mCurrentToken.type()) {
		case TokenType::Int32:
			return parseInt32Expression();
		case TokenType::Float32:
			return parseFloat32Expression();
		case TokenType::Bool:
			return parseBoolExpression();
		default:
			return {};
	}
}

std::unique_ptr<QueryExpression> QueryParser::parsePrimaryExpression() {
	auto valueExpression = parseValueExpression();
	if (valueExpression) {
		return valueExpression;
	}

	switch (mCurrentToken.type()) {
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
		int tokenPrecedence = getTokenPrecedence();

		//If this is a binary operator that binds at least as tightly as the current operator, consume it, otherwise we are done.
		if (tokenPrecedence < precedence) {
			return lhs;
		}

		auto opChar = mCurrentToken.operatorValue();
		auto opTokenType = mCurrentToken.type();
		nextToken(); //Eat the operator

		//Parse the unary expression after the binary operator
		auto rhs = parseUnaryExpression();

		//If the binary operator binds less tightly with RHS than the operator after RHS, let the pending operator take RHS as its LHS
		int nextPrecedence = getTokenPrecedence();
		if (tokenPrecedence < nextPrecedence) {
			rhs = parseBinaryOpRHS(tokenPrecedence + 1, std::move(rhs));
		}

		//Merge LHS and RHS
		if (opChar == OperatorChar('+')) {
			lhs = std::make_unique<QueryMathExpression>(std::move(lhs), std::move(rhs), MathOperator::Add);
		} else if (opChar == OperatorChar('-')) {
			lhs = std::make_unique<QueryMathExpression>(std::move(lhs), std::move(rhs), MathOperator::Sub);
		} else if (opChar == OperatorChar('*')) {
			lhs = std::make_unique<QueryMathExpression>(std::move(lhs), std::move(rhs), MathOperator::Mul);
		} else if (opChar == OperatorChar('/')) {
			lhs = std::make_unique<QueryMathExpression>(std::move(lhs), std::move(rhs), MathOperator::Div);
		} else if (opChar == OperatorChar('<')) {
			lhs = std::make_unique<QueryCompareExpression>(std::move(lhs), std::move(rhs), CompareOperator::LessThan);
		} else if (opChar == OperatorChar('>')) {
			lhs = std::make_unique<QueryCompareExpression>(std::move(lhs), std::move(rhs), CompareOperator::GreaterThan);
		} else if (opChar == OperatorChar('<', '=')) {
			lhs = std::make_unique<QueryCompareExpression>(std::move(lhs), std::move(rhs), CompareOperator::LessThanOrEqual);
		} else if (opChar == OperatorChar('>', '=')) {
			lhs = std::make_unique<QueryCompareExpression>(std::move(lhs), std::move(rhs), CompareOperator::GreaterThanOrEqual);
		} else if (opChar == OperatorChar('=', '=')) {
			lhs = std::make_unique<QueryCompareExpression>(std::move(lhs), std::move(rhs), CompareOperator::Equal);
		} else if (opChar == OperatorChar('!', '=')) {
			lhs = std::make_unique<QueryCompareExpression>(std::move(lhs), std::move(rhs), CompareOperator::NotEqual);
		} else if (opTokenType == TokenType::And) {
			lhs = std::make_unique<QueryAndExpression>(std::move(lhs), std::move(rhs));
		} else {
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
	assertAndConsume(TokenType::By, "Expected 'by' keyword.");

	while (true) {
		auto orderBy = consumeIdentifier();

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

	assertAndConsume(TokenType::Join, "Expected 'join' keyword.");
	auto joinOnTable = consumeIdentifier();

	assertAndConsume(TokenType::On, "Expected 'on' keyword.");
	auto joinFromColumn = consumeIdentifier();

	if (!(mCurrentToken.type() == TokenType::Operator && mCurrentToken.operatorValue() == '=')) {
		parseError("Expected '='.");
	}
	nextToken();

	auto joinOnColumn = consumeIdentifier();

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

	auto tableName = consumeIdentifier();

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
	nextToken();

	auto tableName = consumeIdentifier();
	assertAndConsume(TokenType::Set, "Expected 'set' keyword.");

	std::vector<std::unique_ptr<QueryAssignExpression>> sets;
	while (true) {
		auto lhs = consumeIdentifier();

		if (!(mCurrentToken.type() == TokenType::Operator && mCurrentToken.operatorValue() == '=')) {
			parseError("Expected '='.");
		}
		nextToken();

		auto rhs = parseExpression();
		sets.push_back(std::make_unique<QueryAssignExpression>(lhs, std::move(rhs)));

		if (mCurrentToken.type() == TokenType::Comma) {
			nextToken();
		} else {
			break;
		}
	}

	std::unique_ptr<QueryExpression> filterExpression;
	if (mCurrentToken.type() != TokenType::EndOfTokens && mCurrentToken.type() == TokenType::Where) {
		nextToken();
		filterExpression = parseExpression();
	}

	return std::make_unique<QueryUpdateOperation>(
		tableName,
		std::move(sets),
		std::move(filterExpression));
}

std::unique_ptr<QueryOperation> QueryParser::parseInsert() {
	nextToken();

	assertAndConsume(TokenType::Into, "Expected 'into' keyword.");
	auto tableName = consumeIdentifier();
	assertAndConsume(TokenType::LeftParenthesis, "Expected '('");

	std::vector<std::string> columnNames;
	while (true) {
		auto columnName = consumeIdentifier();
		columnNames.push_back(columnName);

		if (mCurrentToken.type() != TokenType::Comma) {
			break;
		} else {
			nextToken();
		}
	}

	assertAndConsume(TokenType::RightParenthesis, "Expected ')'");
	assertAndConsume(TokenType::Values, "Expected 'values' keyword");
	assertAndConsume(TokenType::LeftParenthesis, "Expected '('");

	std::vector<QueryValue> values;
	while (true) {
		auto valueExpression = parseValueExpression();
		if (!valueExpression) {
			parseError("Expected a value.");
		}

		values.push_back(valueExpression->value);
		if (mCurrentToken.type() != TokenType::Comma) {
			break;
		} else {
			nextToken();
		}
	}

	assertAndConsume(TokenType::RightParenthesis, "Expected ')'");

	return std::make_unique<QueryInsertOperation>(
		tableName,
		columnNames,
		std::vector<std::vector<QueryValue>> { values });
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
