#pragma once
#include "../query.h"

/**
 * Represents a visitor for a query operation
 */
struct QueryOperationVisitor {
	virtual void visit(QuerySelectOperation* operation) = 0;
	virtual void visit(QueryInsertOperation* operation) = 0;
};