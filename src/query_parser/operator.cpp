#include "operator.h"

OperatorChar::OperatorChar()
	: mIsTwoChars(false), mOp1(0), mOp2(0) {

}

OperatorChar::OperatorChar(char op)
	: mIsTwoChars(false), mOp1(op), mOp2(0) {

}

OperatorChar::OperatorChar(char op1, char op2)
	: mIsTwoChars(true), mOp1(op1), mOp2(op2) {

}

bool OperatorChar::isTwoChars() const {
	return mIsTwoChars;
}

char OperatorChar::op1() const {
	return mOp1;
}

char OperatorChar::op2() const {
	return mOp2;
}

std::string OperatorChar::toString() const {
	if (mIsTwoChars) {
		return std::string({ mOp1, mOp2 });
	} else {
		return std::string({ mOp1 });
	}
}

bool OperatorChar::operator==(const OperatorChar& rhs) const {
	return mIsTwoChars == rhs.mIsTwoChars && mOp1 == rhs.mOp1 && mOp2 == rhs.mOp2;
}

bool OperatorChar::operator!=(const OperatorChar& rhs) const {
	return !(*this == rhs);
}