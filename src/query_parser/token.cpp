#include "token.h"
#include <cmath>

Token::Token()
	: mType(TokenType::Int32) {

}

Token::Token(TokenType type)
	: mType(type) {

}

Token::Token(TokenType type, bool value)
	: mType(type),
	  mBoolValue(value) {

}

Token::Token(std::int32_t value)
	: mType(TokenType::Int32),
	  mInt32Value(value) {

}

Token::Token(float value)
	: mType(TokenType::Float32),
	  mFloat32Value(value) {

}

Token::Token(TokenType type, OperatorChar op)
	: mType(type), mOperator(op) {

}

Token::Token(std::string identifier)
	: mType(TokenType::Identifier),
	  mIdentifier(std::move(identifier)) {

}

TokenType Token::type() const {
	return mType;
}

bool Token::boolValue() const {
	return mBoolValue;
}

std::int32_t Token::int32Value() const {
	return mInt32Value;
}

float Token::float32Value() const {
	return mFloat32Value;
}

OperatorChar Token::operatorValue() const {
	return mOperator;
}

std::string Token::identifier() const {
	return mIdentifier;
}

bool Token::operator==(const Token& rhs) const {
	if (mType != rhs.mType) {
		return false;
	}

	if (mInt32Value != rhs.mInt32Value) {
		return false;
	}

	if (mFloat32Value != rhs.mFloat32Value) {
		return false;
	}

	if (mBoolValue != rhs.mBoolValue) {
		return false;
	}

	if (mOperator != rhs.mOperator) {
		return false;
	}

	if (mIdentifier != rhs.mIdentifier) {
		return false;
	}

	return true;
}

bool Token::operator!=(const Token& rhs) const {
	return !(*this == rhs);
}