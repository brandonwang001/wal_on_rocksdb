#pragma once

#include <rocksdb/env.h>
#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>

#include <string>
#include <mutex>
#include <memory>

#include "common/error.h"
#include "wal_store_inf.h"
#include "idl/wal.pb.h"
#include "wal_stream_inf.h"
#include "wal_stream.h"

namespace rocksdb_wal {

struct WalStoreOptions {
 public:
  rocksdb::Env* env_;
  std::string dir_;
};

class WalStore : public WalStoreInf {
 public:
  const std::string kMetaKey = "RootMeta";
  ~WalStore() override {
  }

  WalStore(const WalStoreOptions& options)
      : options_(options) {
  }

  Error Init();

  Error OpenStream(const std::string& stream_uuid,
      std::shared_ptr<WalStreamInf>* stream) override;

  Error RemoveStream(const std::string& stream_uuid) override;
 
 private:
  Error InitRocksDB();

  Error ReadMeta(std::vector<std::string>* stream_uuids);

  Error UpdateMeta(const std::vector<
      std::string>& stream_uuids);

  std::mutex mutex_;
  WalStoreOptions options_;
  rocksdb::DB* db_;
  rocksdb::Options db_options_;
  std::vector<std::string> stream_uuids_;
  std::unordered_map<std::string, std::shared_ptr<WalStreamInf>> wal_streams_;
};

}  // namespace rocksdb_wal
