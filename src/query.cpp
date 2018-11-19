#include "query.h"
#include "execution/operation_visitor.h"

OrderingClause::OrderingClause(const std::string& name, bool ascending)
	: empty(false), name(name), descending(ascending) {

}

JoinClause::JoinClause(const std::string& joinFromColumn, const std::string& joinOnTable, const std::string& joinOnColumn)
	: empty(false), joinFromColumn(joinFromColumn), joinOnTable(joinOnTable), joinOnColumn(joinOnColumn) {

}

QuerySelectOperation::QuerySelectOperation(std::string table,
										   std::vector<std::unique_ptr<QueryExpression>> projection,
										   std::unique_ptr<QueryExpression> filter,
										   JoinClause join,
										   OrderingClause order)
	: table(std::move(table)),
	  projections(std::move(projection)),
	  filter(std::move(filter)),
	  join(std::move(join)),
	  order(std::move(order)) {

}

void QuerySelectOperation::accept(QueryOperationVisitor& visitor) {
	visitor.visit(this);
}

QueryInsertOperation::QueryInsertOperation(const std::string& table,
										   std::vector<std::string> columns,
										   std::vector<std::vector<QueryValue>> values)
	: table(table), columns(std::move(columns)), values(std::move(values)) {

}

void QueryInsertOperation::accept(QueryOperationVisitor& visitor) {
	visitor.visit(this);
}

QueryUpdateOperation::QueryUpdateOperation(std::string table,
										   std::vector<std::unique_ptr<QueryAssignExpression>> sets,
										   std::unique_ptr<QueryExpression> filter)
	: table(std::move(table)),
	  sets(std::move(sets)),
	  filter(std::move(filter)) {

}

void QueryUpdateOperation::accept(QueryOperationVisitor& visitor) {
	visitor.visit(this);
}

Query::Query(std::vector<std::unique_ptr<QueryOperation>> operations)
	: operations(std::move(operations)) {

}
