#include "common/error.h"

#include <unordered_map>

namespace wal {

std::unordered_map<int, std::string> err_msg_map({
  #define ERROR_CODE_DEF(code, name, str) {code, str}
  #include "common/error_code_def.h"
  #undef ERROR_CODE_DEF
});

std::string ToErrorMessage(ErrorCode error_code) {
  if (err_msg_map.find(int(error_code)) ==
      err_msg_map.end()) {
    return "";
  }
  return err_msg_map[int(error_code)];
}

ErrorCode Ok() {
  return ErrorCode::kOk;
}

}  // namespace wal
