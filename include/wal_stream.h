#pragma once
#include <glog/logging.h>

#include <rocksdb/env.h>
#include <rocksdb/slice.h>
#include <rocksdb/db.h>
#include <rocksdb/options.h>

#include <string>
#include <mutex>
#include <vector>
#include <utility>

#include "common/error.h"
#include "wal_stream_inf.h"
#include "idl/wal.pb.h"

namespace rocksdb_wal {

struct WalStreamOptions {
 public:
  rocksdb::DB* db_;
  std::string stream_uuid_;
};

class WalStream : public WalStreamInf {
 public:
  ~WalStream() override {
  }

  WalStream(const WalStreamOptions& options)
      : options_(options) {
    CHECK(options_.db_);
    db_ = options_.db_;
    stream_uuid_ = options_.stream_uuid_;
  }
 
  Error Init();

  Error AppendLog(int64_t log_id,
      int64_t term,
      const std::string& log) override;

  Error GetLog(int64_t log_id,
      std::string* log) override;

  Error GetLastLogId(int64_t* log_id) override;

  Error GetFirstLogId(int64_t* log_id) override;

  Error GetLastTerm(int64_t* term) override;

  Error Release(int64_t log_id) override;

  Error GetLogIdRange(int64_t* min_log_id,
      int64_t* max_log_id) override;

  Error Truncate(int64_t log_id) override;
 
 private:
  Error ReadKey(const std::string& key, std::string* value);

  Error BatchWrite(const std::vector<
      std::pair<std::string, std::string>>& pairs);

  Error AssembleData(int64_t log_id, int64_t term,
      const std::string& log, std::string* key,
      std::string* value);

  Error DeleteFrom(int64_t log_id_start);

  Error DeleteTo(int64_t log_id_end);

  Error UpdateLogIdLowerBound(int64_t log_id);

  Error GetLogIdLowerBound(int64_t* log_id);

  Error UpdateLogIdUpperBound(int64_t log_id);

  Error GetLogIdUpperBound(int64_t* log_id);

  Error UpdateLogIdRange(int64_t log_id_lower_bound,
      int64_t log_id_upper_bound);

  Error AssembleLowerBoundMeta(int64_t log_id,
      std::string* key, std::string* value);

  Error AssembleUpperBoundMeta(int64_t log_id,
      std::string* key, std::string* value);

  std::mutex mutex_;
  WalStreamOptions options_;
  rocksdb::DB* db_;
  std::string stream_uuid_;
};

}  // namespace rocksdb_wal
