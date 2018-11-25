#pragma once
#include "token.h"
#include "../query.h"
#include "operator.h"

#include <vector>
#include <memory>
#include <unordered_map>

class Token;
struct QueryExpression;

/**
 * Represents a tokenizer
 */
namespace Tokenizer {
	/**
	 * Tokenizes the given string
	 * @param str The string to tokenize
	 */
	std::vector<Token> tokenize(std::string str);
};

/**
 * Represents a query parser
 */
class QueryParser {
private:
	std::vector<Token> mTokens;
	Token mCurrentToken;
	int mTokenIndex;
	std::unordered_map<OperatorChar, int> mOperators;

	/**
	 * Signals that a parse error has occurred
	 * @param message The message
	 */
	void parseError(std::string message);

	/**
	 * Advances to the next token
	 */
	Token& nextToken();

	/**
	 * Returns the next token
	 * @param delta The number of tokens to look forward
	 */
	Token& peekToken(int delta = 1);

	/**
	 * Returns the precedence for the current token or -1 if not an operator
	 */
	int getTokenPrecedence();

	/**
	 * Parses an Int32 expression
	 */
	std::unique_ptr<QueryExpression> parseInt32Expression();

	/**
	 * Parses an Float32 expression
	 */
	std::unique_ptr<QueryExpression> parseFloat32Expression();

	/**
	 * Parses an bool expression
	 */
	std::unique_ptr<QueryExpression> parseBoolExpression();

	/**
	 * Parses an identifier expression
	 */
	std::unique_ptr<QueryExpression> parseIdentifierExpression();

	/**
	 * Parses a primary expression
	 */
	std::unique_ptr<QueryExpression> parsePrimaryExpression();

	/**
	 * Parses a parenthesis expression
	 */
	std::unique_ptr<QueryExpression> parseParenthesisExpression();

	/**
	 * Parses the right hand side of an binary op expression
	 * @param precedence The precedence
	 * @param lhs The left side
	 */
	std::unique_ptr<QueryExpression> parseBinaryOpRHS(int precedence, std::unique_ptr<QueryExpression> lhs);

	/**
	 * Parses a unary expression
	 */
	std::unique_ptr<QueryExpression> parseUnaryExpression();

	/**
	 * Parses an expression
	 */
	std::unique_ptr<QueryExpression> parseExpression();

	/**
	 * Parses ordering clause
	 * @param ordering The ordering clause
	 */
	void parseOrder(OrderingClause& ordering);

	/**
	 * Parses a join clause
	 * @param join The join clause
	 */
	void parseJoin(JoinClause& join);

	/**
	 * Parses a select operation
	 */
	std::unique_ptr<QueryOperation> parseSelect();

	/**
	 * Parses an update operation
	 */
	std::unique_ptr<QueryOperation> parseUpdate();

	/**
	 * Parses an insert operation
	 */
	std::unique_ptr<QueryOperation> parseInsert();
public:
	/**
	 * Creates a new parser
	 * @param tokens The tokens
	 */
	explicit QueryParser(std::vector<Token> tokens);

	/**
	 * Parses the tokens
	 */
	std::unique_ptr<QueryOperation> parse();
};