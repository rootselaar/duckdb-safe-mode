#include "duckdb/planner/binder.hpp"
#include "duckdb/parser/statement/attach_statement.hpp"
#include "duckdb/parser/tableref/table_function_ref.hpp"
#include "duckdb/planner/tableref/bound_table_function.hpp"
#include "duckdb/planner/operator/logical_simple.hpp"
#include "duckdb/main/client_context.hpp"

namespace duckdb {

BoundStatement Binder::Bind(AttachStatement &stmt) {

	if(context.IsSafeMode()) {
		throw BinderException("ATTACH cannot be done with a READ_ONLY connection.");
	}

	BoundStatement result;
	result.types = {LogicalType::BOOLEAN};
	result.names = {"Success"};

	result.plan = make_uniq<LogicalSimple>(LogicalOperatorType::LOGICAL_ATTACH, std::move(stmt.info));
	properties.allow_stream_result = false;
	properties.return_type = StatementReturnType::NOTHING;
	return result;
}

} // namespace duckdb
