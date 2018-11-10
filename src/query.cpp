#include "query.h"
#include "execution/operation_visitor.h"

OrderingClause::OrderingClause(const std::string& name, bool ascending)
	: name(name), descending(ascending) {

}

QuerySelectOperation::QuerySelectOperation(std::string table,
										   std::vector<std::unique_ptr<QueryExpression>> projection,
										   std::unique_ptr<QueryExpression> filter,
										   OrderingClause order)
	: table(std::move(table)),
	  projections(std::move(projection)),
	  filter(std::move(filter)),
	  order(std::move(order)) {

}

void QuerySelectOperation::accept(QueryOperationVisitor& visitor) {
	visitor.visit(this);
}

QueryInsertOperation::QueryInsertOperation(const std::string& table, std::vector<std::string> columns, std::vector<std::vector<QueryValue>> values)
	: table(table), columns(std::move(columns)), values(std::move(values)) {

}

void QueryInsertOperation::accept(QueryOperationVisitor& visitor) {
	visitor.visit(this);
}

Query::Query(std::vector<std::unique_ptr<QueryOperation>> operations)
	: operations(std::move(operations)) {

}