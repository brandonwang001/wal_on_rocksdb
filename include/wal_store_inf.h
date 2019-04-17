#pragma once

#include <string>

#include "common/error.h"
#include "wal_stream_inf.h"

namespace rocksdb_wal {

struct WalStoreInf {
 public:
  virtual ~WalStoreInf() = default;
  // Open a stream for stream_uuid. 
  virtual Error OpenStream(const std::string& stream_uuid,
      std::shared_ptr<WalStreamInf>* stream) = 0;
  // Remove the stream of stream_uuid.
  virtual Error RemoveStream(const std::string& stream_uuid) = 0; 
};

}  // namespace rocksdb_wal
