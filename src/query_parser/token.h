#pragma once
#include <string>
#include <iostream>
#include "operator.h"

/**
 * The token types
 */
enum class TokenType : unsigned char {
	Bool,
	Int32,
	Float32,
	Operator,
	Identifier,
	Select,
	Update,
	Insert,
	From,
	Where,
	Order,
	By,
	Asc,
	Desc,
	Inner,
	Join,
	On,
	And,
	LeftParenthesis,
	RightParenthesis,
	Comma,
	Dot,
	EndOfTokens
};

/**
 * Represents a token
 */
class Token {
private:
	TokenType mType;
	bool mBoolValue = false;
	std::int32_t mInt32Value = 0;
	float mFloat32Value = 0.0f;
	OperatorChar mOperator;
	std::string mIdentifier;
public:
	/**
	 * Creates an empty token
	 */
	Token();

	/**
	 * Creates a new token
	 * @param type The type of the token
	 */
	Token(TokenType type);

	/**
	 * Creates an Int32 token
	 * @param value The value
	 */
	Token(std::int32_t value);

	/**
	 * Creates an Float32 token
	 * @param value The value
	 */
	Token(float value);

	/**
	 * Creates a new operator token
	 * @param type The type
	 * @param op The operator
	 */
	Token(TokenType type, OperatorChar op);

	/**
	 * Creates a new identifier token
	 * @param identifier The identifier
	 */
	Token(std::string identifier);

	/**
	 * Creates an bool token
	 * @param type The type
	 * @param value The value
	 */
	explicit Token(TokenType type, bool value);

	/**
	 * Returns the type of the token
	 */
	TokenType type() const;

	/**
	 * Returns the value if bool token
	 */
	bool boolValue() const;

	/**
	 * Returns the value if int32 token
	 */
	std::int32_t int32Value() const;

	/**
	 * Returns the value if float32 token
	 */
	float float32Value() const;

	/**
	 * Returns the value if operator token
	 */
	OperatorChar operatorValue() const;

	/**
	 * Returns the identifier
	 */
	std::string identifier() const;

	bool operator==(const Token& rhs) const;
	bool operator!=(const Token& rhs) const;
};