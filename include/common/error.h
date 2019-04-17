#pragma once

#include <string>

namespace wal {

enum class ErrorCode : int {
  #define ERROR_CODE_DEF(code, name, str) name = code
  #include "common/error_code_def.h"
  #undef ERROR_CODE_DEF
};

std::string ToErrorMessage(ErrorCode error_code);

class Error {
 public:
  Error(ErrorCode error_code)
      : error_code_(error_code) {
    error_msg_ = ToErrorMessage(error_code);
  }

  void Set(ErrorCode error_code) {
    error_code_ = error_code;
    error_msg_ = ToErrorMessage(error_code);
  }

  std::string ToString() {
    return "ret : " + std::to_string(int(error_code_))
      + " msg : " + error_msg_;
  }

  bool operator == (ErrorCode error_code) {
    return (error_code_ == error_code);
  }

  bool operator != (ErrorCode error_code) {
    return (error_code_ != error_code);
  }
 private:
  ErrorCode error_code_;
  std::string error_msg_;
};

#define RETURN_OK() \
do { \
  Error ret(ErrorCode::kOk); \
  return ret; \
} while (false)

#define RETURN_NOT_OK(code) \
do { \
  Error ret(code); \
  return ret; \
} while (false)

}  // namespace wal
