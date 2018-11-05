#include "query.h"
#include "execution/operation_visitor.h"

Row::Row(std::unordered_map<std::string, QueryValue> columns)
	: columns(std::move(columns)) {

}

QuerySelectOperation::QuerySelectOperation(std::string table, std::vector<std::string> columns,	 std::unique_ptr<QueryExpression> filter)
	: table(std::move(table)), columns(std::move(columns)), filter(std::move(filter)) {

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