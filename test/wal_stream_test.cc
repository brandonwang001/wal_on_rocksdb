#include <gtest/gtest.h>
#include <glog/logging.h>

#include "common/error.h"
#include "wal_stream.h"

namespace wal {

TEST(WalStream, TestWriteAndRead) {
  std::string stream_uuid = "tablet_0";
  std::string dir = "./";
  WalStream wal_stream(stream_uuid, dir);
  auto ret = wal_stream.Init();
  CHECK(ret == Ok()) << ret.ToString();
  int64_t log_num = 10000;
  // write log
  int64_t last_log_id = 0;
  for (int64_t i = 1; i <= log_num; i++) {
    int64_t next_log_id = 0;
    ret = wal_stream.GetNextLogId(&next_log_id);
    CHECK(next_log_id == i) << "next_log_id : "
        << next_log_id << "  i : " << i;
    CHECK(ret == Ok()) << ret.ToString();
    CHECK_GT(next_log_id, 0) << next_log_id;
    int64_t term = i;
    std::string log = std::to_string(i);
    ret = wal_stream.AppendLog(next_log_id, term, log);
    CHECK(ret == Ok()) << ret.ToString();
    last_log_id = next_log_id;
    ret = wal_stream.GetNextLogId(&next_log_id);
    CHECK(ret == Ok()) << ret.ToString();
    CHECK((last_log_id + 1) == next_log_id)
        << "last_log_id : " << last_log_id << " "
        << "next_log_id : " << next_log_id;
  }
  LOG(INFO) << "append log succ";
  // read log
  int64_t lower_bound = 0;
  int64_t upper_bound = 0;
  ret = wal_stream.GetFirstLogId(&lower_bound);
  CHECK(ret == Ok()) << ret.ToString();
  CHECK(lower_bound == 1) << "lower_bound : "
      << lower_bound;
  ret = wal_stream.GetLastLogId(&upper_bound);
  CHECK(ret == Ok()) << ret.ToString();
  CHECK(upper_bound == log_num)
      << "upper_bound : " << upper_bound << " "
      << "log_num : " << log_num; 
  for (int64_t i = lower_bound; i <= upper_bound; i++) {
    int64_t term = 0;
    std::string log;
    ret = wal_stream.GetLog(i, &term, &log);
    CHECK(ret == Ok()) << ret.ToString();
    CHECK(std::to_string(term) == log) << "term : "
        << term << "  log : " << log;
  }
  rocksdb::Env* env = rocksdb::Env::Default();
  std::vector<std::string> result;
  env->GetChildren(dir + "/" + stream_uuid, &result);
  for (auto& file : result) {
    if (file != "." && file != "..") {
      env->DeleteFile(dir + "/" + stream_uuid + "/" + file);
    }
  }
  env->DeleteDir(dir + "/" + stream_uuid);
}

TEST(WalStream, TestBackwardTruncate) {
  std::string stream_uuid = "tablet_1";
  std::string dir = "./";
  WalStream wal_stream(stream_uuid, dir);
  auto ret = wal_stream.Init();
  CHECK(ret == Ok()) << ret.ToString();
  int64_t log_num = 100;
  // write log
  int64_t last_log_id = 0;
  for (int64_t i = 1; i <= log_num; i++) {
    int64_t next_log_id = 0;
    ret = wal_stream.GetNextLogId(&next_log_id);
    CHECK(next_log_id == i) << "next_log_id : "
        << next_log_id << "  i : " << i;
    CHECK(ret == Ok()) << ret.ToString();
    CHECK_GT(next_log_id, 0) << next_log_id;
    int64_t term = i;
    std::string log = std::to_string(i);
    ret = wal_stream.AppendLog(next_log_id, term, log);
    CHECK(ret == Ok()) << ret.ToString();
    last_log_id = next_log_id;
    ret = wal_stream.GetNextLogId(&next_log_id);
    CHECK(ret == Ok()) << ret.ToString();
    CHECK((last_log_id + 1) == next_log_id)
        << "last_log_id : " << last_log_id << " "
        << "next_log_id : " << next_log_id;
  }
  LOG(INFO) << "append log succ";
  // backward truncate
  ret = wal_stream.Truncate(log_num / 2);
  CHECK(ret == Ok()) << ret.ToString();
  last_log_id = 0;
  for (int64_t i = 1; i <= log_num; i++) {
    int64_t next_log_id = 0;
    ret = wal_stream.GetNextLogId(&next_log_id);
    CHECK(ret == Ok()) << ret.ToString();
    CHECK_GT(next_log_id, 0) << next_log_id;
    int64_t term = i;
    std::string log = std::to_string(i);
    ret = wal_stream.AppendLog(next_log_id, term, log);
    CHECK(ret == Ok()) << ret.ToString();
    last_log_id = next_log_id;
    ret = wal_stream.GetNextLogId(&next_log_id);
    CHECK(ret == Ok()) << ret.ToString();
    CHECK((last_log_id + 1) == next_log_id)
        << "last_log_id : " << last_log_id << " "
        << "next_log_id : " << next_log_id;
  }
  int64_t lower_bound = 0;
  int64_t upper_bound = 0;
  ret = wal_stream.GetFirstLogId(&lower_bound);
  CHECK(ret == Ok()) << ret.ToString();
  ret = wal_stream.GetLastLogId(&upper_bound);
  CHECK(ret == Ok()) << ret.ToString();
  CHECK((upper_bound - lower_bound + 1) ==
      (log_num + log_num / 2) - 1);
  // read log
  ret = wal_stream.GetFirstLogId(&lower_bound);
  CHECK(ret == Ok()) << ret.ToString();
  ret = wal_stream.GetLastLogId(&upper_bound);
  CHECK(ret == Ok()) << ret.ToString();
  for (int64_t i = lower_bound; i <= upper_bound; i++) {
    int64_t term = 0;
    std::string log;
    ret = wal_stream.GetLog(i, &term, &log);
    CHECK(ret == Ok()) << ret.ToString();
  }
  rocksdb::Env* env = rocksdb::Env::Default();
  std::vector<std::string> result;
  env->GetChildren(dir + "/" + stream_uuid, &result);
  for (auto& file : result) {
    if (file != "." && file != "..") {
      LOG(INFO) << "file : " << file;
      env->DeleteFile(dir + "/" + stream_uuid + "/" + file);
    }
  }
  env->DeleteDir(dir + "/" + stream_uuid);
}

TEST(WalStream, TestForwardTruncate) {
  std::string stream_uuid = "tablet_2";
  std::string dir = "./";
  WalStream wal_stream(stream_uuid, dir);
  auto ret = wal_stream.Init();
  CHECK(ret == Ok()) << ret.ToString();
  int64_t log_num = 100;
  // write log
  int64_t last_log_id = 0;
  for (int64_t i = 1; i <= log_num; i++) {
    int64_t next_log_id = 0;
    ret = wal_stream.GetNextLogId(&next_log_id);
    CHECK(next_log_id == i) << "next_log_id : "
        << next_log_id << "  i : " << i;
    CHECK(ret == Ok()) << ret.ToString();
    CHECK_GT(next_log_id, 0) << next_log_id;
    int64_t term = i;
    std::string log = std::to_string(i);
    ret = wal_stream.AppendLog(next_log_id, term, log);
    CHECK(ret == Ok()) << ret.ToString();
    last_log_id = next_log_id;
    ret = wal_stream.GetNextLogId(&next_log_id);
    CHECK(ret == Ok()) << ret.ToString();
    CHECK((last_log_id + 1) == next_log_id)
        << "last_log_id : " << last_log_id << " "
        << "next_log_id : " << next_log_id;
  }
  LOG(INFO) << "append log succ";
  // forward truncate
  ret = wal_stream.Truncate(1000);
  CHECK(ret == Ok()) << ret.ToString();
  last_log_id = 0;
  for (int64_t i = 1; i <= log_num; i++) {
    int64_t next_log_id = 0;
    ret = wal_stream.GetNextLogId(&next_log_id);
    CHECK(ret == Ok()) << ret.ToString();
    CHECK_GT(next_log_id, 0) << next_log_id;
    int64_t term = i;
    std::string log = std::to_string(i);
    ret = wal_stream.AppendLog(next_log_id, term, log);
    CHECK(ret == Ok()) << ret.ToString();
    last_log_id = next_log_id;
    ret = wal_stream.GetNextLogId(&next_log_id);
    CHECK(ret == Ok()) << ret.ToString();
    CHECK((last_log_id + 1) == next_log_id)
        << "last_log_id : " << last_log_id << " "
        << "next_log_id : " << next_log_id;
  }
  int64_t lower_bound = 0;
  int64_t upper_bound = 0;
  ret = wal_stream.GetFirstLogId(&lower_bound);
  CHECK(ret == Ok()) << ret.ToString();
  ret = wal_stream.GetLastLogId(&upper_bound);
  CHECK(ret == Ok()) << ret.ToString();
  CHECK(lower_bound == 1000);
  CHECK(upper_bound == 1000 + log_num - 1);
  // read log
  ret = wal_stream.GetFirstLogId(&lower_bound);
  CHECK(ret == Ok()) << ret.ToString();
  ret = wal_stream.GetLastLogId(&upper_bound);
  CHECK(ret == Ok()) << ret.ToString();
  for (int64_t i = lower_bound; i <= upper_bound; i++) {
    int64_t term = 0;
    std::string log;
    ret = wal_stream.GetLog(i, &term, &log);
    CHECK(ret == Ok()) << ret.ToString();
  }
  rocksdb::Env* env = rocksdb::Env::Default();
  std::vector<std::string> result;
  env->GetChildren(dir + "/" + stream_uuid, &result);
  for (auto& file : result) {
    if (file != "." && file != "..") {
      LOG(INFO) << "file : " << file;
      env->DeleteFile(dir + "/" + stream_uuid + "/" + file);
    }
  }
  env->DeleteDir(dir + "/" + stream_uuid);
}

TEST(WalStream, TestRelease) {
  std::string stream_uuid = "tablet_3";
  std::string dir = "./";
  WalStream wal_stream(stream_uuid, dir);
  auto ret = wal_stream.Init();
  CHECK(ret == Ok()) << ret.ToString();
  int64_t log_num = 100;
  // write log
  int64_t last_log_id = 0;
  for (int64_t i = 1; i <= log_num; i++) {
    int64_t next_log_id = 0;
    ret = wal_stream.GetNextLogId(&next_log_id);
    CHECK(next_log_id == i) << "next_log_id : "
        << next_log_id << "  i : " << i;
    CHECK(ret == Ok()) << ret.ToString();
    CHECK_GT(next_log_id, 0) << next_log_id;
    int64_t term = i;
    std::string log = std::to_string(i);
    ret = wal_stream.AppendLog(next_log_id, term, log);
    CHECK(ret == Ok()) << ret.ToString();
    last_log_id = next_log_id;
    ret = wal_stream.GetNextLogId(&next_log_id);
    CHECK(ret == Ok()) << ret.ToString();
    CHECK((last_log_id + 1) == next_log_id)
        << "last_log_id : " << last_log_id << " "
        << "next_log_id : " << next_log_id;
  }
  LOG(INFO) << "append log succ";
  // forward truncate
  ret = wal_stream.Release(50);
  CHECK(ret == Ok()) << ret.ToString();
  last_log_id = 0;
  for (int64_t i = 1; i <= log_num; i++) {
    int64_t next_log_id = 0;
    ret = wal_stream.GetNextLogId(&next_log_id);
    CHECK(ret == Ok()) << ret.ToString();
    CHECK_GT(next_log_id, 0) << next_log_id;
    int64_t term = i;
    std::string log = std::to_string(i);
    ret = wal_stream.AppendLog(next_log_id, term, log);
    CHECK(ret == Ok()) << ret.ToString();
    last_log_id = next_log_id;
    ret = wal_stream.GetNextLogId(&next_log_id);
    CHECK(ret == Ok()) << ret.ToString();
    CHECK((last_log_id + 1) == next_log_id)
        << "last_log_id : " << last_log_id << " "
        << "next_log_id : " << next_log_id;
  }
  int64_t lower_bound = 0;
  int64_t upper_bound = 0;
  ret = wal_stream.GetFirstLogId(&lower_bound);
  CHECK(ret == Ok()) << ret.ToString();
  ret = wal_stream.GetLastLogId(&upper_bound);
  CHECK(ret == Ok()) << ret.ToString();
  LOG(INFO) << "lower_bound : " << lower_bound;
  CHECK(lower_bound == 51);
  LOG(INFO) << "upper_bound : " << upper_bound;
  CHECK(upper_bound == 200);
  // read log
  ret = wal_stream.GetFirstLogId(&lower_bound);
  CHECK(ret == Ok()) << ret.ToString();
  ret = wal_stream.GetLastLogId(&upper_bound);
  CHECK(ret == Ok()) << ret.ToString();
  for (int64_t i = lower_bound; i <= upper_bound; i++) {
    int64_t term = 0;
    std::string log;
    ret = wal_stream.GetLog(i, &term, &log);
    CHECK(ret == Ok()) << ret.ToString();
  }
  rocksdb::Env* env = rocksdb::Env::Default();
  std::vector<std::string> result;
  env->GetChildren(dir + "/" + stream_uuid, &result);
  for (auto& file : result) {
    if (file != "." && file != "..") {
      LOG(INFO) << "file : " << file;
      env->DeleteFile(dir + "/" + stream_uuid + "/" + file);
    }
  }
  env->DeleteDir(dir + "/" + stream_uuid);
}

}  // mamespace wal
