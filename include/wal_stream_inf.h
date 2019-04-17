#pragma once

#include <string>

#include "common/error.h"

namespace wal {

struct WalStreamInf {
 public:
  virtual ~WalStreamInf() = default;
  // Append log
  virtual Error AppendLog(int64_t log_id,
      int64_t term,
      const std::string& log) = 0;
  // Get log
  virtual Error GetLog(int64_t log_id,
      std::string* log) = 0;
  // Get the id of last log
  virtual Error GetLastLogId(int64_t* log_id) = 0;
  // Get the id of first log
  virtual Error GetFirstLogId(int64_t* log_id) = 0;

  // Get the term of last log
  virtual Error GetLastTerm(int64_t* term) = 0;
  // Release the log which log_id is less than
  // the input log_id.
  virtual Error Release(int64_t log_id) = 0;
  // Get the range of log_id
  virtual Error GetLogIdRange(int64_t* min_log_id,
      int64_t* max_log_id) = 0;
  // Truncate the log to the given log_id.
  virtual Error Truncate(int64_t log_id) = 0;
};

}  // namespace rocksdb_wal
