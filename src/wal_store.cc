#include "wal_store.h"

#include <string>

#include "common/error.h"

namespace wal {

Error WalStore::Init() {
  RETURN_OK();
}

Error WalStore::OpenStream(
    const std::string& stream_uuid,
    std::shared_ptr<WalStreamInf>* stream) {
  std::lock_guard<std::mutex> lock(mutex_);
  RETURN_OK();
}

Error WalStore::RemoveStream(
    const std::string& stream_uuid) {
  std::lock_guard<std::mutex> lock(mutex_);
  RETURN_OK();
}

Error WalStore::InitRocksDB() {
  RETURN_OK();
}

Error WalStore::ReadMeta(
    std::vector<std::string>* stream_uuids) {
  RETURN_OK();
}

Error WalStore::UpdateMeta(const std::vector<
    std::string>& stream_uuids) {
  RETURN_OK();
}

}  // namespace wal
