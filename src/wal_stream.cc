#include "wal_stream.h"

#include <mutex>
#include <string>
#include <vector>

#include "common/error.h"
#include "idl/wal.pb.h"

namespace wal {

Error WalStream::Init() {
  rocksdb::Options options;
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
  options.create_if_missing = true;
  auto path = dir_ + "/" + stream_uuid_;
  auto status = rocksdb::DB::Open(
      options, path, &db_);
  if (!status.ok()) {
    RETURN_NOT_OK(ErrorCode::kRocksDBOpenFailed);
  }
  int64_t lower_bound = 0;
  int64_t upper_bound = 0;
  int64_t next_log_id = 0;
  auto ret1 = GetLogIdLowerBound(&lower_bound);
  auto ret2 = GetLogIdUpperBound(&upper_bound);
  auto ret3 = GetNextLogIdInner(&next_log_id);
  if (ret1 == ErrorCode::kKeyNotExist &&
      ret2 == ErrorCode::kKeyNotExist &&
      ret3 == ErrorCode::kKeyNotExist) {
    next_log_id = 1;
    auto ret = WriteMeta(lower_bound,
        upper_bound, next_log_id);
    IF_NOT_OK_RETURN(ret);
  } else if (ret1 == Ok() && ret2 == Ok()
      & ret3 == Ok()) {
      LOG(INFO) << "start rocksdb succ, " 
          << "lower_bound : " << lower_bound << " "
          << "upper_bound : " << upper_bound << " "
          << "next_log_id : " << next_log_id;
  } else {
    LOG(FATAL) << ret1.ToString() << " "
        << ret2.ToString() << " "
        << ret3.ToString(); 
  }
  RETURN_OK();
}

Error WalStream::AppendLog(int64_t log_id,
    int64_t term,
    const std::string& log) {
  std::lock_guard<std:: mutex> lock(mutex_);
  int64_t log_id_lower_bound = 0;
  int64_t log_id_upper_bound = 0;
  int64_t next_log_id = 0;
  auto ret = ReadMeta(&log_id_lower_bound,
      &log_id_upper_bound,
      &next_log_id);
  IF_NOT_OK_RETURN(ret);
  std::vector<std::tuple<BatchWriteType,
      std::string, std::string>> batchs;
  std::string key;
  std::string value;
  ret = AssembleData(log_id, term, log,
      &key, &value);
  IF_NOT_OK_RETURN(ret);
  LOG(INFO) << "log_id : " << log_id;
  batchs.emplace_back(std::tuple<BatchWriteType,
      std::string, std::string>(
      BatchWriteType::kPut, key, value));
  if (log_id_lower_bound == 0 && next_log_id == 1) {
    key.clear();
    value.clear();
    ret = AssembleLowerBoundMeta(next_log_id,
        &key, &value);
    IF_NOT_OK_RETURN(ret);
    batchs.emplace_back(std::tuple<BatchWriteType,
        std::string, std::string>(
        BatchWriteType::kPut, key, value));
  } 
  key.clear();
  value.clear();
  ret = AssembleUpperBoundMeta(
      next_log_id, &key, &value); 
  IF_NOT_OK_RETURN(ret);
  batchs.emplace_back(std::tuple<BatchWriteType,
      std::string, std::string>(
      BatchWriteType::kPut, key, value));
  key.clear();
  value.clear();
  ret = AssembleNextLogIdMeta(
      next_log_id + 1, &key, &value);
  IF_NOT_OK_RETURN(ret);
  batchs.emplace_back(std::tuple<BatchWriteType,
      std::string, std::string>(
      BatchWriteType::kPut, key, value));
  ret = BatchWrite(batchs);
  IF_NOT_OK_RETURN(ret);
  RETURN_OK();
}

Error WalStream::GetLog(int64_t log_id,
    int64_t * term,
    std::string* log) {
  std::lock_guard<std:: mutex> lock(mutex_);
  auto ret = ReadData(log_id, term, log);
  IF_NOT_OK_RETURN(ret);
  RETURN_OK();
}

Error WalStream::GetNextLogId(int64_t* log_id) {
  std::lock_guard<std:: mutex> lock(mutex_);
  auto ret = GetNextLogIdInner(log_id);
  IF_NOT_OK_RETURN(ret);
  RETURN_OK();
}

Error WalStream::GetLastLogId(int64_t* log_id) {
  std::lock_guard<std:: mutex> lock(mutex_);
  auto ret = GetLogIdUpperBound(log_id);
  IF_NOT_OK_RETURN(ret);
  RETURN_OK();
}

Error WalStream::GetFirstLogId(int64_t* log_id) {
  std::lock_guard<std:: mutex> lock(mutex_);
  auto ret = GetLogIdLowerBound(log_id);
  IF_NOT_OK_RETURN(ret);
  RETURN_OK();
}

Error WalStream::GetLastTerm(int64_t* term) {
  std::lock_guard<std:: mutex> lock(mutex_);
  int64_t log_id = 0;
  auto ret = GetLogIdUpperBound(&log_id);
  IF_NOT_OK_RETURN(ret);
  if (log_id == 0) {
    *term = 0;
  }
  std::string log;
  ret = ReadData(log_id, term, &log);
  IF_NOT_OK_RETURN(ret);
  RETURN_OK();
}

Error WalStream::Release(int64_t log_id) {
  std::lock_guard<std:: mutex> lock(mutex_);
  int64_t min_log_id = 0;
  int64_t max_log_id = 0;
  int64_t next_log_id = 0;
  auto ret = ReadMeta(&min_log_id, &max_log_id,
      &next_log_id);
  IF_NOT_OK_RETURN(ret);
  if (log_id < min_log_id) {
    RETURN_OK();
  }
  if (log_id > max_log_id) {
    LOG(FATAL) << "invalid para";
  }
  ret = DeleteTo(log_id);
  IF_NOT_OK_RETURN(ret);
  RETURN_OK();
}

Error WalStream::GetLogIdRange(int64_t* min_log_id,
    int64_t* max_log_id) {
  std::lock_guard<std:: mutex> lock(mutex_);
  int64_t next_log_id = 0;
  auto ret = ReadMeta(min_log_id, max_log_id,
      &next_log_id);
  IF_NOT_OK_RETURN(ret);
  RETURN_OK();
}

Error WalStream::Truncate(int64_t log_id) {
  std::lock_guard<std:: mutex> lock(mutex_);
  int64_t lower_bound = 0;
  int64_t upper_bound = 0;
  int64_t next_log_id = 0;
  auto ret = ReadMeta(&lower_bound, &upper_bound,
      &next_log_id);
  IF_NOT_OK_RETURN(ret);
  if (log_id < lower_bound) {
    ret = DeleteFrom(lower_bound);
    IF_NOT_OK_RETURN(ret);
  } else if (log_id > upper_bound) {
    ret = DeleteTo(upper_bound);
    IF_NOT_OK_RETURN(ret);
  } else {
    ret = DeleteFrom(log_id);
    IF_NOT_OK_RETURN(ret);
  }
  RETURN_OK();
}

Error WalStream::AssembleData(int64_t log_id,
    int64_t term, const std::string& log,
    std::string* key, std::string* value) {
  *key = stream_uuid_ + "_" + std::to_string(log_id);
  WalRecord record;
  record.set_term(term);
  record.set_log(log);
  auto succ = record.AppendToString(value);
  if (!succ) {
    RETURN_NOT_OK(ErrorCode::kSerializeFailed);
  }
  RETURN_OK();
}

Error WalStream::ReadData(int64_t log_id,
    int64_t* term, std::string* log) {
  auto key = stream_uuid_ + "_" + std::to_string(log_id);
  std::string value;
  auto ret = ReadKey(key, &value);
  IF_NOT_OK_RETURN(ret);
  WalRecord record;
  auto succ = record.ParseFromString(value);
  if (!succ) {
    RETURN_NOT_OK(ErrorCode::kParseFailed);
  }
  *term = record.term();
  *log = record.log();
  RETURN_OK();
}

Error WalStream::DeleteFrom(int64_t log_id_start) {
  CHECK_GE(log_id_start, 1);
  int64_t lower_bound = 0;
  int64_t upper_bound = 0;
  int64_t next_log_id = 0;
  auto ret = ReadMeta(&lower_bound,
      &upper_bound, &next_log_id);
  IF_NOT_OK_RETURN(ret);
  if (log_id_start < lower_bound ||
      log_id_start > upper_bound) {
    RETURN_NOT_OK(ErrorCode::kInvalidPara);
  }
  std::vector<std::tuple<BatchWriteType, std::string,
      std::string>> batchs;
  for (int64_t i = log_id_start; i <= upper_bound; i++) {
    auto key= stream_uuid_ + "_" + std::to_string(i);
    batchs.emplace_back(std::tuple<BatchWriteType,
        std::string, std::string>(
        BatchWriteType::kDelete, key, ""));
  }
  if (log_id_start == lower_bound) {
    lower_bound = 0;
    upper_bound = 0;
    next_log_id = log_id_start;
  } else {
    upper_bound = log_id_start - 1;
    next_log_id = log_id_start;
  }
  std::string key;
  std::string value;
  ret = AssembleLowerBoundMeta(
      lower_bound, &key, &value);
  IF_NOT_OK_RETURN(ret);
  batchs.emplace_back(std::tuple<BatchWriteType,
      std::string, std::string>(
      BatchWriteType::kPut, key, value));
  key.clear();
  value.clear();
  ret = AssembleUpperBoundMeta(
      upper_bound, &key, &value); 
  IF_NOT_OK_RETURN(ret);
  batchs.emplace_back(std::tuple<BatchWriteType,
      std::string, std::string>(
      BatchWriteType::kPut, key, value));
  key.clear();
  value.clear();
  ret = AssembleNextLogIdMeta(
      next_log_id, &key, &value);
  batchs.emplace_back(std::tuple<BatchWriteType,
      std::string, std::string>(
      BatchWriteType::kPut, key, value));
  IF_NOT_OK_RETURN(ret);
  ret = BatchWrite(batchs);
  IF_NOT_OK_RETURN(ret);
  RETURN_OK();
}

Error WalStream::DeleteTo(int64_t log_id_end) {
  CHECK_GE(log_id_end, 1);
  int64_t lower_bound = 0;
  int64_t upper_bound = 0;
  int64_t next_log_id = 0;
  auto ret = ReadMeta(&lower_bound,
      &upper_bound, &next_log_id); 
  IF_NOT_OK_RETURN(ret);
  if (log_id_end < lower_bound ||
      log_id_end > upper_bound) {
    RETURN_NOT_OK(ErrorCode::kInvalidPara);
  };
  std::vector<std::tuple<BatchWriteType, std::string,
      std::string>> batchs;
  for (int64_t i = lower_bound; i <= log_id_end; i++) {
    auto key= stream_uuid_ + "_" + std::to_string(i);
    batchs.emplace_back(std::tuple<BatchWriteType,
        std::string, std::string>(
        BatchWriteType::kDelete, key, ""));
  }
  if (log_id_end == upper_bound) {
    lower_bound = 0;
    upper_bound = 0;
    next_log_id = log_id_end + 1;
  } else {
    lower_bound = log_id_end + 1;
    next_log_id = upper_bound + 1;
  }
  std::string key;
  std::string value;
  ret = AssembleLowerBoundMeta(
      lower_bound, &key, &value);
  IF_NOT_OK_RETURN(ret);
  batchs.emplace_back(std::tuple<BatchWriteType,
      std::string, std::string>(
      BatchWriteType::kPut, key, value));
  key.clear();
  value.clear();
  ret = AssembleUpperBoundMeta(
      upper_bound, &key, &value); 
  IF_NOT_OK_RETURN(ret);
  batchs.emplace_back(std::tuple<BatchWriteType,
      std::string, std::string>(
      BatchWriteType::kPut, key, value));
  key.clear();
  value.clear();
  ret = AssembleNextLogIdMeta(
      next_log_id, &key, &value);
  IF_NOT_OK_RETURN(ret);
  batchs.emplace_back(std::tuple<BatchWriteType,
      std::string, std::string>(
      BatchWriteType::kPut, key, value));
  ret = BatchWrite(batchs);
  IF_NOT_OK_RETURN(ret);
  RETURN_OK();
}

Error WalStream::WriteMeta(int64_t log_id_lower_bound,
    int64_t log_id_upper_bound,
    int64_t next_log_id) {
  std::vector<std::tuple<BatchWriteType, std::string,
      std::string>> batchs;
  std::string key;
  std::string value;
  auto ret = AssembleLowerBoundMeta(
      log_id_lower_bound, &key, &value);
  IF_NOT_OK_RETURN(ret);
  batchs.emplace_back(std::tuple<BatchWriteType,
      std::string, std::string>(BatchWriteType::kPut,
      key, value));
  key.clear();
  value.clear();
  ret = AssembleUpperBoundMeta(
      log_id_upper_bound, &key, &value); 
  IF_NOT_OK_RETURN(ret);
  batchs.emplace_back(std::tuple<BatchWriteType,
      std::string, std::string>(BatchWriteType::kPut,
      key, value));
  key.clear();
  value.clear();
  ret = AssembleNextLogIdMeta(
      next_log_id, &key, &value);
  IF_NOT_OK_RETURN(ret);
  batchs.emplace_back(std::tuple<BatchWriteType,
      std::string, std::string>(BatchWriteType::kPut,
      key, value));
  ret = BatchWrite(batchs);
  IF_NOT_OK_RETURN(ret);
  RETURN_OK();
}

Error WalStream::ReadMeta(int64_t* log_id_lower_bound,
    int64_t* log_id_upper_bound,
    int64_t* next_log_id) {
  auto ret = GetLogIdLowerBound(log_id_lower_bound);
  IF_NOT_OK_RETURN(ret);
  ret = GetLogIdUpperBound(log_id_upper_bound);
  IF_NOT_OK_RETURN(ret);
  ret = GetNextLogIdInner(next_log_id);
  IF_NOT_OK_RETURN(ret);
  RETURN_OK();
}

Error WalStream::BatchWrite(const std::vector<
    std::tuple<BatchWriteType, std::string,
    std::string>>& batchs) {
  rocksdb::WriteBatch wb;
  for (auto& batch : batchs) {
    if (std::get<0>(batch) == BatchWriteType::kPut) {
      wb.Put(std::get<1>(batch), std::get<2>(batch));
    } else if (std::get<0>(batch) ==
        BatchWriteType::kDelete) {
      wb.Delete(std::get<1>(batch));
    } else {
      LOG(FATAL) << "invalid BatchWriteType";
    }
  }
  auto status = db_->Write(rocksdb::WriteOptions(),
      &wb);
  if (!status.ok()) {
    RETURN_NOT_OK(ErrorCode::kRocksDBWriteFailed);
  }
  RETURN_OK();
}

Error WalStream::ReadKey(const std::string& key,
    std::string* value) {
  auto status = db_->Get(rocksdb::ReadOptions(),
      key, value);
  if (!status.ok() && !status.IsNotFound()) {
    RETURN_NOT_OK(ErrorCode::kRocksDBGetFailed);
  }
  if (status.IsNotFound()) {
    RETURN_NOT_OK(ErrorCode::kKeyNotExist);
  }
  RETURN_OK();
}

Error WalStream::GetLogIdLowerBound(int64_t* log_id) {
  auto key = stream_uuid_ + "_" + kLowerBoundMetaSuffix;
  std::string value;
  auto ret = ReadKey(key, &value);
  IF_NOT_OK_RETURN(ret);
  WalStreamLowerBoundMeta meta;
  auto succ = meta.ParseFromString(value);
  if (!succ) {
    RETURN_NOT_OK(ErrorCode::kParseFailed);
  }
  *log_id = meta.lower_bound();
  RETURN_OK();
}

Error WalStream::GetLogIdUpperBound(int64_t* log_id) {
  auto key = stream_uuid_ + "_" + kUpperBoundMetaSuffix;
  std::string value;
  auto ret = ReadKey(key, &value);
  IF_NOT_OK_RETURN(ret);
  WalStreamUpperBoundMeta meta;
  auto succ = meta.ParseFromString(value);
  if (!succ) {
    RETURN_NOT_OK(ErrorCode::kParseFailed);
  }
  *log_id = meta.upper_bound();
  RETURN_OK();
}

Error WalStream::GetNextLogIdInner(int64_t* log_id) {
  auto key = stream_uuid_ + "_" + kNextLogIdMetaSuffix;
  std::string value;
  auto ret = ReadKey(key, &value);
  IF_NOT_OK_RETURN(ret);
  WalStreamNextLogIdMeta meta;
  auto succ = meta.ParseFromString(value);
  if (!succ) {
    RETURN_NOT_OK(ErrorCode::kParseFailed);
  }
  *log_id = meta.next_log_id();
  RETURN_OK();
}

Error WalStream::AssembleLowerBoundMeta(int64_t log_id,
    std::string* key, std::string* value) {
  *key = stream_uuid_ + "_" + kLowerBoundMetaSuffix;
  WalStreamLowerBoundMeta meta;
  meta.set_lower_bound(log_id);
  auto succ = meta.AppendToString(value);
  if (!succ) {
    RETURN_NOT_OK(ErrorCode::kSerializeFailed);
  }
  RETURN_OK();
}

Error WalStream::AssembleUpperBoundMeta(int64_t log_id,
    std::string* key, std::string* value) {
  *key = stream_uuid_ + "_" + kUpperBoundMetaSuffix;
  WalStreamUpperBoundMeta meta;
  meta.set_upper_bound(log_id);
  auto succ = meta.AppendToString(value);
  if (!succ) {
    RETURN_NOT_OK(ErrorCode::kSerializeFailed);
  }
  RETURN_OK();
}

Error WalStream::AssembleNextLogIdMeta(int64_t log_id,
    std::string* key, std::string* value) {
  *key = stream_uuid_ + "_" + kNextLogIdMetaSuffix;
  WalStreamNextLogIdMeta meta;
  meta.set_next_log_id(log_id);
  auto succ = meta.AppendToString(value);
  if (!succ) {
    RETURN_NOT_OK(ErrorCode::kSerializeFailed);
  }
  RETURN_OK();
}

}  // namespace wal
