#pragma once

#include <string>

namespace wal {

enum class ErrorCode : int {
  #define ERROR_DEF(code, name, str) name = code
  #include "common/error_code_def.h"
};

}  // namespace wal
