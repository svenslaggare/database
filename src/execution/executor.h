#pragma once
#include "operation_visitor.h"
#include "../query_expressions/compiler.h"

struct DatabaseEngine;

/**
 * Represents a operations executor visitor
 */
struct OperationExecutorVisitor : public QueryOperationVisitor {
	DatabaseEngine& databaseEngine;
	QueryResult& result;

	explicit OperationExecutorVisitor(DatabaseEngine& databaseEngine, QueryResult& result);

	virtual void visit(QuerySelectOperation* operation) override;
	virtual void visit(QueryInsertOperation* operation) override;
};