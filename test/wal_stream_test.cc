#include <gtest/gtest.h>
#include <glog/logging.h>

#include "common/error.h"
#include "wal_stream.h"

namespace wal {

TEST(WalStream, TestOpen) {
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
}

}  // mamespace wal
