//===- AVM.cpp ------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Symbols.h"
#include "Target.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Support/Endian.h"

using namespace llvm;
using namespace llvm::ELF;
using namespace llvm::support::endian;
using namespace lld;
using namespace lld::elf;

namespace {
class AVM final : public TargetInfo {
public:
  explicit AVM(Ctx &Ctx) : TargetInfo(Ctx) {
    defaultMaxPageSize = 0x100;
    defaultCommonPageSize = 0x100;
  }

  RelExpr getRelExpr(RelType Type, const Symbol &,
                     const uint8_t *) const override {
    switch (Type) {
    case R_AVM_NONE:
    case R_AVM_RELAX:
      return R_NONE;
    case R_AVM_DATA16:
    case R_AVM_PROG24:
    case R_AVM_PROG_LO16:
    case R_AVM_PROG_HI8:
    case R_AVM_FAR24:
      return R_ABS;
    case R_AVM_PCREL8:
    case R_AVM_PCREL16:
      return R_PC;
    default:
      Err(ctx) << "unknown AVM relocation (" << Type << ')';
      return R_NONE;
    }
  }

  void relocate(uint8_t *Loc, const Relocation &Rel,
                uint64_t Val) const override {
    switch (Rel.type) {
    case R_AVM_NONE:
    case R_AVM_RELAX:
      return;
    case R_AVM_DATA16:
      checkUInt(ctx, Loc, Val, 16, Rel);
      write16le(Loc, Val);
      return;
    case R_AVM_PROG24:
      checkUInt(ctx, Loc, Val, 24, Rel);
      Loc[0] = Val;
      Loc[1] = Val >> 8;
      Loc[2] = Val >> 16;
      return;
    case R_AVM_PROG_LO16:
      checkUInt(ctx, Loc, Val, 24, Rel);
      write16le(Loc, Val & 0xffff);
      return;
    case R_AVM_PROG_HI8:
      checkUInt(ctx, Loc, Val, 24, Rel);
      Loc[0] = Val >> 16;
      return;
    case R_AVM_FAR24:
      checkUInt(ctx, Loc, Val, 24, Rel);
      checkAlignment(ctx, Loc, Val, 2, Rel);
      // Bit zero of the encoded low byte is the CALL/JMP link bit.
      Loc[0] = (Val & 0xfe) | (Loc[0] & 1);
      Loc[1] = Val >> 8;
      Loc[2] = Val >> 16;
      return;
    case R_AVM_PCREL8:
      checkInt(ctx, Loc, Val, 8, Rel);
      Loc[0] = Val;
      return;
    case R_AVM_PCREL16:
      checkInt(ctx, Loc, Val, 16, Rel);
      write16le(Loc, Val);
      return;
    default:
      llvm_unreachable("unknown AVM relocation");
    }
  }
};
} // namespace

void elf::setAVMTargetInfo(Ctx &Ctx) { Ctx.target.reset(new AVM(Ctx)); }
