//===- AVM.cpp ------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "InputFiles.h"
#include "Symbols.h"
#include "Target.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Support/Endian.h"

using namespace llvm;
using namespace llvm::ELF;
using namespace llvm::object;
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

  uint32_t calcEFlags() const override {
    assert(!ctx.objectFiles.empty());
    for (InputFile *file : ctx.objectFiles) {
      if (file->ekind != ELF32LEKind) {
        ErrAlways(ctx) << file
                       << ": AVM objects must be ELF32 little-endian";
        continue;
      }
      uint32_t flags =
          cast<ObjFile<ELF32LE>>(file)->getObj().getHeader().e_flags;
      // Existing AVM MC objects predate the ABI flag. Treat zero as the
      // legacy spelling of ABI v1 while rejecting every other ABI value.
      if (flags != 0 && flags != EF_AVM_ABI_V1)
        ErrAlways(ctx) << file << ": incompatible AVM ABI flags: 0x"
                       << utohexstr(flags);
    }
    return EF_AVM_ABI_V1;
  }

  unsigned getRelocSize(RelType type) const override {
    switch (type) {
    case R_AVM_NONE:
    case R_AVM_RELAX:
      return 0;
    case R_AVM_PROG_HI8:
    case R_AVM_PCREL8:
      return 1;
    case R_AVM_DATA16:
    case R_AVM_PROG_LO16:
    case R_AVM_PCREL16:
      return 2;
    case R_AVM_PROG24:
    case R_AVM_FAR24:
      return 3;
    default:
      return 0;
    }
  }

  void checkAddressSpace(RelType type, const Symbol &sym) const {
    const Defined *defined = dyn_cast<Defined>(&sym);
    if (!defined || !defined->section)
      return;

    uint64_t required = type == R_AVM_DATA16 ? SHF_AVM_DATASPACE
                                              : SHF_AVM_PROGSPACE;
    uint64_t flags = defined->section->flags;
    if ((flags & (SHF_AVM_PROGSPACE | SHF_AVM_DATASPACE)) == 0 ||
        flags & required)
      return;
    Err(ctx) << "AVM relocation " << type << " against symbol '" << &sym
             << "' requires a "
             << (type == R_AVM_DATA16 ? "data" : "program")
             << "-space symbol";
  }

  RelExpr getRelExpr(RelType Type, const Symbol &S,
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
      checkAddressSpace(Type, S);
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
      Loc[0] = Val;
      Loc[1] = Val >> 8;
      Loc[2] = Val >> 16;
      return;
    case R_AVM_PCREL8:
      // R_PC is based on the relocated operand at P + 1. AVM defines P as
      // the primary opcode address and the next PC as P + 2.
      Val -= 1;
      checkInt(ctx, Loc, Val, 8, Rel);
      Loc[0] = Val;
      return;
    case R_AVM_PCREL16:
      // The rel16 operand is also at P + 1, while its next PC is P + 3.
      Val -= 2;
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
