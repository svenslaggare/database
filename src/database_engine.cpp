#include "database_engine.h"
#include "query.h"
#include "execution/executor.h"
#include "query_parser/parser.h"

#include <iostream>
#include <stack>

DatabaseEngine::DatabaseEngine(DatabaseConfiguration config)
	: mConfig(config) {

}

const DatabaseConfiguration& DatabaseEngine::config() const {
	return mConfig;
}

void DatabaseEngine::addTable(std::string name, std::unique_ptr<Table> table) {
	mTables.insert(std::make_pair(name, std::move(table)));
}

Table& DatabaseEngine::getTable(const std::string& name) const {
	return *mTables.at(name);
}

std::unique_ptr<QueryOperation> DatabaseEngine::parse(const std::string& text) const {
	QueryParser parser(Tokenizer::tokenize(text));
	return parser.parse();
}

void DatabaseEngine::execute(const Query& query, QueryResult& result) {
	OperationExecutorVisitor operationExecutor(*this, result);
	query.operation->accept(operationExecutor);
}