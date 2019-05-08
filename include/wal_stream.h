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

namespace wal {

enum class BatchWriteType : int {
  kPut = 1,
  kDelete = 2,
};

class WalStream : public WalStreamInf {
 public:
  const std::string kLowerBoundMetaSuffix = "lowerbound";
  const std::string kUpperBoundMetaSuffix = "upperbound";
  const std::string kNextLogIdMetaSuffix = "nextlogid";

  ~WalStream() override {
  }

  WalStream(const std::string& stream_uuid,
      const std::string& dir) {
    stream_uuid_ = stream_uuid;
    dir_ = dir;
  }
 
  Error Init();

  Error AppendLog(int64_t log_id,
      int64_t term,
      const std::string& log) override;

  Error GetLog(int64_t log_id,
      int64_t* term,
      std::string* log) override;

  Error GetNextLogId(int64_t* log_id) override;

  Error GetLastLogId(int64_t* log_id) override;

  Error GetFirstLogId(int64_t* log_id) override;

  Error GetLastTerm(int64_t* term) override;

  Error Release(int64_t log_id) override;

  Error GetLogIdRange(int64_t* min_log_id,
      int64_t* max_log_id) override;

  Error Truncate(int64_t log_id) override;
 
 private:
  Error AssembleData(int64_t log_id, int64_t term,
      const std::string& log, std::string* key,
      std::string* value);

  Error ReadData(int64_t log_id, int64_t* term,
      std::string* log);

  Error DeleteFrom(int64_t log_id_start,
      int64_t start_from);

  Error DeleteTo(int64_t log_id_end,
      int64_t start_from);

  Error WriteMeta(int64_t log_id_lower_bound,
      int64_t log_id_upper_bound,
      int64_t next_log_id);
 
  Error ReadMeta(int64_t* log_id_lower_bound,
      int64_t* log_id_upper_bound,
      int64_t* next_log_id);

  Error BatchWrite(const std::vector<
      std::tuple<BatchWriteType, std::string,
      std::string>>& batchs);

  Error ReadKey(const std::string& key,
      std::string* value);

  Error GetLogIdLowerBound(int64_t* log_id);

  Error GetLogIdUpperBound(int64_t* log_id);

  Error GetNextLogIdInner(int64_t* log_id);

  Error AssembleLowerBoundMeta(int64_t log_id,
      std::string* key, std::string* value);

  Error AssembleUpperBoundMeta(int64_t log_id,
      std::string* key, std::string* value);

  Error AssembleNextLogIdMeta(int64_t log_id,
      std::string* key, std::string* value);

  std::mutex mutex_;
  rocksdb::DB* db_;
  std::string stream_uuid_;
  std::string dir_;
};

}  // namespace rocksdb_wal
