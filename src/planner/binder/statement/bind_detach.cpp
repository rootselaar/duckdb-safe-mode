#include "duckdb/parser/statement/detach_statement.hpp"
#include "duckdb/planner/binder.hpp"
#include "duckdb/planner/operator/logical_simple.hpp"
#include "duckdb/main/config.hpp"
#include "duckdb/main/client_context.hpp"

namespace duckdb {

BoundStatement Binder::Bind(DetachStatement &stmt) {

	if(context.IsSafeMode()) {
		throw BinderException("DETACH cannot be done with a READ_ONLY connection.");
	}

	BoundStatement result;

	result.plan = make_uniq<LogicalSimple>(LogicalOperatorType::LOGICAL_DETACH, std::move(stmt.info));
	result.names = {"Success"};
	result.types = {LogicalType::BOOLEAN};
	properties.allow_stream_result = false;
	properties.return_type = StatementReturnType::NOTHING;
	return result;
}

} // namespace duckdb
