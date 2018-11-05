#include "database_engine.h"
#include "query.h"
#include "execution/executor.h"

#include <iostream>
#include <stack>

void DatabaseEngine::addTable(std::string name, std::unique_ptr<Table> table) {
	mTables.insert(std::make_pair(name, std::move(table)));
}

Table& DatabaseEngine::getTable(const std::string& name) const {
	return *mTables.at(name);
}

void DatabaseEngine::execute(const Query& query, QueryResult& result) {
	OperationExecutorVisitor operationExecutor(*this, result);

	for (auto& op : query.operations) {
		op->accept(operationExecutor);
	}
}