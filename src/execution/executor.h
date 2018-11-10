#pragma once
#include "operation_visitor.h"
#include "../query_expressions/compiler.h"

struct DatabaseEngine;

/**
 * Represents a operations executor visitor
 */
class OperationExecutorVisitor : public QueryOperationVisitor {
private:
	DatabaseEngine& databaseEngine;
	QueryResult& result;
public:
	/**
	 * Creates a new operation executor
	 * @param databaseEngine The database engine
	 * @param result The result
	 */
	explicit OperationExecutorVisitor(DatabaseEngine& databaseEngine, QueryResult& result);

	virtual void visit(QuerySelectOperation* operation) override;
	virtual void visit(QueryInsertOperation* operation) override;
};