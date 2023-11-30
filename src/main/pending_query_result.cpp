#include "duckdb/main/pending_query_result.hpp"
#include "duckdb/main/client_context.hpp"
#include "duckdb/main/prepared_statement_data.hpp"

namespace duckdb {

PendingQueryResult::PendingQueryResult(shared_ptr<ClientContext> context_p, PreparedStatementData &statement,
                                       vector<LogicalType> types_p, bool allow_stream_result)
    : BaseQueryResult(QueryResultType::PENDING_RESULT, statement.statement_type, statement.properties,
                      std::move(types_p), statement.names),
      context(std::move(context_p)), allow_stream_result(allow_stream_result) {
}

PendingQueryResult::PendingQueryResult(PreservedError error)
    : BaseQueryResult(QueryResultType::PENDING_RESULT, std::move(error)) {
}

PendingQueryResult::~PendingQueryResult() {
}

unique_ptr<ClientContextLock> PendingQueryResult::LockContext() {
	if (!context) {
		if (HasError()) {
			throw InvalidInputException(
			    "Attempting to execute an unsuccessful or closed pending query result\nError: %s", GetError());
		}
		throw InvalidInputException("Attempting to execute an unsuccessful or closed pending query result");
	}
	return context->LockContext();
}

void PendingQueryResult::CheckExecutableInternal(ClientContextLock &lock) {
	bool invalidated = HasError() || !context;
	if (!invalidated) {
		invalidated = !context->IsActiveResult(lock, *this);
	}
	if (invalidated) {
		if (HasError()) {
			throw InvalidInputException(
			    "Attempting to execute an unsuccessful or closed pending query result\nError: %s", GetError());
		}
		throw InvalidInputException("Attempting to execute an unsuccessful or closed pending query result");
	}
}

PendingExecutionResult PendingQueryResult::ExecuteTask() {
	auto lock = LockContext();
	return ExecuteTaskInternal(*lock);
}

bool PendingQueryResult::AllowStreamResult() const {
	return allow_stream_result;
}

PendingExecutionResult PendingQueryResult::ExecuteTaskInternal(ClientContextLock &lock) {
	CheckExecutableInternal(lock);
	return context->ExecuteTaskInternal(lock, *this);
}

unique_ptr<QueryResult> PendingQueryResult::ExecuteInternal(ClientContextLock &lock) {
	CheckExecutableInternal(lock);
// Busy wait while execution is not finished
#ifdef DUCKDB_DEBUG_BUFFERED_STREAMING_RESULT
	if (allow_stream_result) {
		while (!IsFinishedOrBlocked(ExecuteTaskInternal(lock))) {
		}
	} else {
		while (!IsFinished(ExecuteTaskInternal(lock))) {
		}
	}
#else
	while (!IsFinished(ExecuteTaskInternal(lock))) {
	}
#endif
	if (HasError()) {
		return make_uniq<MaterializedQueryResult>(error);
	}
	auto result = context->FetchResultInternal(lock, *this);
	Close();
	return result;
}

unique_ptr<QueryResult> PendingQueryResult::Execute() {
	auto lock = LockContext();
	return ExecuteInternal(*lock);
}

void PendingQueryResult::Close() {
	context.reset();
}

bool PendingQueryResult::IsFinished(PendingExecutionResult result) {
	return (result == PendingExecutionResult::RESULT_READY || result == PendingExecutionResult::EXECUTION_ERROR);
}

bool PendingQueryResult::IsFinishedOrBlocked(PendingExecutionResult result) {
	return (result == PendingExecutionResult::RESULT_READY || result == PendingExecutionResult::EXECUTION_ERROR ||
	        result == PendingExecutionResult::BLOCKED);
}

} // namespace duckdb
