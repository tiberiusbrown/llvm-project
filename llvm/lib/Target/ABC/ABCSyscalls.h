//===-- ABCSyscalls.h - ABC syscall table helpers -------------------------===//

#ifndef LLVM_LIB_TARGET_ABC_ABCSYSCALLS_H
#define LLVM_LIB_TARGET_ABC_ABCSYSCALLS_H

#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/StringRef.h"
#include <optional>

namespace llvm {

inline std::optional<unsigned> getABCSyscallNumber(StringRef Label) {
  return StringSwitch<std::optional<unsigned>>(Label)
#define ABC_SYSCALL(Name, Label, Number) .Case(Label, Number)
#include "ABCSyscalls.def"
#undef ABC_SYSCALL
      .Default(std::nullopt);
}

inline std::optional<unsigned> getABCSyscallImmediate(StringRef Name) {
  return StringSwitch<std::optional<unsigned>>(Name)
#define ABC_SYSCALL(Name, Label, Number) .Case(#Name, Number * 2)
#include "ABCSyscalls.def"
#undef ABC_SYSCALL
      .Default(std::nullopt);
}

inline StringRef getABCSyscallName(unsigned Immediate) {
  switch (Immediate) {
#define ABC_SYSCALL(Name, Label, Number) \
  case Number * 2: return #Name;
#include "ABCSyscalls.def"
#undef ABC_SYSCALL
  default:
    return {};
  }
}

} // namespace llvm

#endif // LLVM_LIB_TARGET_ABC_ABCSYSCALLS_H
