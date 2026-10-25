//===-- AVMRegisterRange.h - Allocated register ranges -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AVM_AVMREGISTERRANGE_H
#define LLVM_LIB_TARGET_AVM_AVMREGISTERRANGE_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/Register.h"
#include <cstdint>
#include <optional>
#include <utility>

namespace llvm {
class AVMInstrInfo;
class MachineBasicBlock;
class MachineFunction;
class MachineInstr;

// An unsigned interval describes the actual allocated word, including its
// upper byte. It is not the range of a truncated IR value. Widening thresholds
// at comparison boundaries allow bounded induction ranges to converge without
// assuming that an unproven byte counter cannot wrap.
class AVMRegisterRange {
public:
  struct Interval {
    unsigned Lo = 0, Hi = UINT16_MAX;
    bool operator==(const Interval &R) const {
      return Lo == R.Lo && Hi == R.Hi;
    }
  };
  struct State {
    bool Reachable = false;
    SmallVector<Interval, 16> Values;
    SmallVector<uint64_t, 16> Equal;
    struct Comparison {
      bool Valid = false;
      uint64_t Equal = 0;
      Interval Value;
      unsigned Constant = 0;
      bool Reversed = false;
    } CC;
    explicit State(unsigned Size = 8, bool Reachable = false);
  };

  AVMRegisterRange(MachineFunction &MF, const AVMInstrInfo &TII);
  State getEntry(const MachineBasicBlock &MBB) const;
  Interval get(const State &S, Register R) const;
  void step(State &S, const MachineInstr &MI) const;

private:
  MachineFunction &MF;
  const AVMInstrInfo &TII;
  DenseMap<const MachineBasicBlock *, State> Entries;
  DenseMap<int64_t, unsigned> Slots;
  SmallVector<unsigned, 16> Thresholds;
  unsigned Size = 8;

  unsigned index(Register R) const;
  std::optional<std::pair<int64_t, uint64_t>>
  privateAccess(const MachineInstr &MI) const;
  void refine(State &S, unsigned Condition) const;
  bool join(State &Dest, const State &Source) const;
};
} // namespace llvm
#endif
