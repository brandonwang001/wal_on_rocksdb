#include "wal_stream.h"

#include <mutex>
#include <string>
#include <vector>

#include "common/error.h"
#include "idl/wal.pb.h"

namespace wal {

Error WalStream::Init() {
  RETURN_OK();
}

Error WalStream::AppendLog(int64_t log_id,
    int64_t term,
    const std::string& log) {
  RETURN_OK();
}

Error WalStream::GetLog(int64_t log_id,
    std::string* log) {
  RETURN_OK();
}

Error WalStream::GetLastLogId(int64_t* log_id) {
  RETURN_OK();
}

Error WalStream::GetFirstLogId(int64_t* log_id) {
  RETURN_OK();
}

Error WalStream::GetLastTerm(int64_t* term) {
  RETURN_OK();
}

Error WalStream::Release(int64_t log_id) {
  RETURN_OK();
}

Error WalStream::GetLogIdRange(int64_t* min_log_id,
    int64_t* max_log_id) {
  RETURN_OK();
}

Error WalStream::Truncate(int64_t log_id) {
  RETURN_OK();
}

Error WalStream::ReadKey(const std::string& key, std::string* value) {
  RETURN_OK();
}

Error WalStream::BatchWrite(const std::vector<
    std::pair<std::string, std::string>>& pairs) {
  RETURN_OK();
}

Error WalStream::AssembleData(int64_t log_id, int64_t term,
    const std::string& log, std::string* key,
    std::string* value) {
  RETURN_OK();
}

Error WalStream::DeleteFrom(int64_t log_id_start) {
  RETURN_OK();
}

Error WalStream::DeleteTo(int64_t log_id_end) {
  RETURN_OK();
}

Error WalStream::UpdateLogIdLowerBound(int64_t log_id) {
  RETURN_OK();
}

Error WalStream::GetLogIdLowerBound(int64_t* log_id) {
  RETURN_OK();
}

Error WalStream::UpdateLogIdUpperBound(int64_t log_id) {
  RETURN_OK();
}

Error WalStream::GetLogIdUpperBound(int64_t* log_id) {
  RETURN_OK();
}

Error WalStream::UpdateLogIdRange(int64_t log_id_lower_bound,
    int64_t log_id_upper_bound) {
  RETURN_OK();
}

Error WalStream::AssembleLowerBoundMeta(int64_t log_id,
    std::string* key, std::string* value) {
  RETURN_OK();
}

Error WalStream::AssembleUpperBoundMeta(int64_t log_id,
    std::string* key, std::string* value) {
  RETURN_OK();
}

}  // namespace wal
