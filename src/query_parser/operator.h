#pragma once
#include <string>

/**
 * Represents an operator character
 */
class OperatorChar {
private:
	bool mIsTwoChars;
	char mOp1;
	char mOp2;
public:
	OperatorChar();

	/**
	 * Creates a new single-character operator
	 * @param op The operator
	 */
	OperatorChar(char op);

	/**
	 * Creates a new two-character operator
	 * @param op1 The first operator
	 * @param op2 The second operator
	 */
	OperatorChar(char op1, char op2);

	/**
	 * Indicates if the current operator is two chars
	 */
	bool isTwoChars() const;

	/**
	 * Returns the first operator character
	 */
	char op1() const;

	/**
	 * Returns the second operator character
	 * @return
	 */
	char op2() const;

	/**
	 * Returns a string representation of the operator
	 * @return
	 */
	std::string toString() const;

	bool operator==(const OperatorChar& rhs) const;
	bool operator!=(const OperatorChar& rhs) const;
};

namespace std {
	template <>
	struct hash<OperatorChar> {
		std::size_t operator()(const OperatorChar& op) const {
			return (37 * (std::size_t)op.isTwoChars()) + (37 * (std::size_t)op.op1()) + (37 * (std::size_t)op.op2());
		}
	};
}