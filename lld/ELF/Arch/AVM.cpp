//===- AVM.cpp ------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "InputFiles.h"
#include "InputSection.h"
#include "OutputSections.h"
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
        ErrAlways(ctx) << file << ": AVM objects must be ELF32 little-endian";
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

  bool relaxOnce(int pass) const override;
  void finalizeRelax(int passes) const override;

  void checkAddressSpace(RelType type, const Symbol &sym) const {
    const Defined *defined = dyn_cast<Defined>(&sym);
    if (!defined || !defined->section)
      return;

    uint64_t required =
        type == R_AVM_DATA16 ? SHF_AVM_DATASPACE : SHF_AVM_PROGSPACE;
    uint64_t flags = defined->section->flags;
    if ((flags & (SHF_AVM_PROGSPACE | SHF_AVM_DATASPACE)) == 0 ||
        flags & required)
      return;
    Err(ctx) << "AVM relocation " << type << " against symbol '" << &sym
             << "' requires a " << (type == R_AVM_DATA16 ? "data" : "program")
             << "-space symbol";
  }

  RelExpr getRelExpr(RelType Type, const Symbol &S,
                     const uint8_t *) const override {
    switch (Type) {
    case R_AVM_NONE:
      return R_NONE;
    case R_AVM_RELAX:
      // Keep marker relocations in InputSection::relocations so the
      // relaxation pass can validate and consume them.
      return R_RELAX_HINT;
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

enum AVMRelaxKind : uint8_t {
  AVMRelaxNone,
  AVMRelaxJmp,
  AVMRelaxCall,
  AVMRelaxCond,
};

static AVMRelaxKind getRelaxKind(ArrayRef<uint8_t> content, uint64_t offset,
                                 uint64_t &size) {
  if (offset >= content.size())
    return AVMRelaxNone;
  switch (content[offset]) {
  case 0xe2:
    size = 4;
    return AVMRelaxJmp;
  case 0xe3:
    size = 4;
    return AVMRelaxCall;
  case 0xd0:
  case 0xd1:
  case 0xd2:
  case 0xd3:
  case 0xd8:
  case 0xd9:
    size = 6;
    return AVMRelaxCond;
  default:
    return AVMRelaxNone;
  }
}

static uint8_t directCondition(uint8_t inverse) {
  switch (inverse) {
  case 0xd1:
    return 0xd0; // br.ne -> br.eq
  case 0xd0:
    return 0xd1; // br.eq -> br.ne
  case 0xd8:
    return 0xd2; // br.uge -> br.ult
  case 0xd2:
    return 0xd8; // br.ult -> br.uge
  case 0xd9:
    return 0xd3; // br.sge -> br.slt
  case 0xd3:
    return 0xd9; // br.slt -> br.sge
  default:
    llvm_unreachable("invalid AVM inverse branch");
  }
}

static uint64_t findOriginalSymbolOffset(const RelaxAux &aux,
                                         const Defined *d) {
  for (const SymbolAnchor &a : aux.anchors)
    if (a.d == d && !a.end)
      return a.offset;
  return UINT64_MAX;
}

static void initAVMRelaxation(Ctx &ctx) {
  SmallVector<InputSection *, 0> storage;
  for (OutputSection *osec : ctx.outputSections) {
    for (InputSection *sec : getInputSections(*osec, storage)) {
      if (!llvm::any_of(sec->relocs(), [](const Relocation &r) {
            return r.type == R_AVM_RELAX;
          }))
        continue;
      sec->relaxAux = make<RelaxAux>();
      const size_t n = sec->relocs().size();
      sec->relaxAux->relocDeltas = std::make_unique<uint32_t[]>(n);
      sec->relaxAux->relocTypes = std::make_unique<RelType[]>(n);
      sec->relaxAux->relocPairs.assign(n, UINT32_MAX);
      sec->relaxAux->relocStates.assign(n, 0);
    }
  }

  auto addAnchor = [](Defined *d) {
    if (auto *sec = dyn_cast_or_null<InputSection>(d->section))
      if (sec->relaxAux) {
        sec->relaxAux->anchors.push_back({d->value, d, false});
        sec->relaxAux->anchors.push_back({d->value + d->size, d, true});
      }
  };
  for (InputFile *file : ctx.objectFiles)
    for (Symbol *sym : file->getSymbols())
      if (auto *d = dyn_cast<Defined>(sym))
        if (d->file == file || d->scriptDefined)
          addAnchor(d);

  for (OutputSection *osec : ctx.outputSections)
    for (InputSection *sec : getInputSections(*osec, storage))
      if (sec->relaxAux)
        llvm::sort(sec->relaxAux->anchors,
                   [](const SymbolAnchor &a, const SymbolAnchor &b) {
                     return std::make_pair(a.offset, a.end) <
                            std::make_pair(b.offset, b.end);
                   });
}

static bool validateAVMRelaxation(Ctx &ctx, InputSection &sec) {
  RelaxAux &aux = *sec.relaxAux;
  MutableArrayRef<Relocation> relocs = sec.relocs();
  ArrayRef<uint8_t> content = sec.content();
  SmallVector<std::pair<uint64_t, uint64_t>, 0> ranges;
  bool valid = true;
  for (auto [i, marker] : llvm::enumerate(relocs)) {
    if (marker.type != R_AVM_RELAX)
      continue;
    if (!marker.sym->getName().empty() || marker.addend != 0) {
      Err(ctx) << sec.getLocation(marker.offset)
               << ": R_AVM_RELAX must use symbol zero and addend zero";
      valid = false;
      continue;
    }
    uint64_t size = 0;
    AVMRelaxKind kind = getRelaxKind(content, marker.offset, size);
    if (kind == AVMRelaxNone) {
      Err(ctx) << sec.getLocation(marker.offset)
               << ": R_AVM_RELAX is not attached to a supported AVM sequence";
      valid = false;
      continue;
    }
    if (marker.offset + size > content.size()) {
      Err(ctx) << sec.getLocation(marker.offset)
               << ": truncated AVM relaxation sequence";
      valid = false;
      continue;
    }
    const uint64_t farOffset = marker.offset + (kind == AVMRelaxCond ? 3 : 1);
    if (kind == AVMRelaxCond && (content[marker.offset + 1] != 4 ||
                                 content[marker.offset + 2] != 0xe2)) {
      Err(ctx) << sec.getLocation(marker.offset)
               << ": malformed conditional AVM relaxation sequence";
      valid = false;
      continue;
    }
    unsigned farCount = 0;
    uint32_t farIndex = UINT32_MAX;
    for (auto [j, r] : llvm::enumerate(relocs)) {
      if (r.type != R_AVM_FAR24)
        continue;
      if (r.offset >= marker.offset && r.offset < marker.offset + size &&
          r.offset != farOffset) {
        Err(ctx) << sec.getLocation(r.offset)
                 << ": R_AVM_FAR24 is at the wrong AVM relaxation field";
        valid = false;
      }
      if (r.offset == farOffset) {
        ++farCount;
        farIndex = j;
      }
    }
    if (farCount != 1) {
      Err(ctx) << sec.getLocation(marker.offset)
               << ": R_AVM_RELAX is not paired with exactly one R_AVM_FAR24";
      valid = false;
      continue;
    }
    ranges.push_back({marker.offset, marker.offset + size});
    aux.relocPairs[i] = farIndex;
  }
  llvm::sort(ranges);
  for (size_t i = 1; i != ranges.size(); ++i)
    if (ranges[i].first < ranges[i - 1].second) {
      Err(ctx) << sec.getLocation(ranges[i].first)
               << ": overlapping AVM relaxation sequences";
      valid = false;
    }
  aux.relaxInvalid = !valid;
  return valid;
}

static uint8_t chooseAVMForm(Ctx &ctx, const InputSection &sec,
                             const RelaxAux &aux, const Relocation &marker,
                             const Relocation &far,
                             MutableArrayRef<Relocation> relocs,
                             AVMRelaxKind kind, uint64_t currentSize,
                             uint64_t p) {
  uint64_t target = far.sym->getVA(ctx, far.addend);
  const Defined *d = dyn_cast<Defined>(far.sym);
  const uint64_t original = d ? findOriginalSymbolOffset(aux, d) : UINT64_MAX;

  auto adjustedTarget = [&](uint64_t finalSize) {
    if (!d || d->section != &sec || original == UINT64_MAX)
      return target;

    const int64_t targetOffset = static_cast<int64_t>(original) + far.addend;
    if (targetOffset < 0)
      return target;

    uint64_t removed = 0;
    for (auto [i, r] : llvm::enumerate(relocs)) {
      if (r.type != R_AVM_RELAX ||
          r.offset >= static_cast<uint64_t>(targetOffset))
        continue;
      uint64_t size = 0;
      getRelaxKind(sec.content(), r.offset, size);
      uint64_t state = aux.relocStates[i] ? aux.relocStates[i] : size;
      if (r.offset == marker.offset)
        state = finalSize;
      removed += size - state;
    }
    return sec.getVA() + targetOffset - removed;
  };
  auto fits8 = [&](uint64_t finalSize) {
    return isInt<8>(static_cast<int64_t>(adjustedTarget(finalSize)) -
                    static_cast<int64_t>(p + 2));
  };
  auto fits16 = [&](uint64_t finalSize) {
    const uint64_t next = kind == AVMRelaxCond ? p + 5 : p + 3;
    return isInt<16>(static_cast<int64_t>(adjustedTarget(finalSize)) -
                     static_cast<int64_t>(next));
  };
  switch (kind) {
  case AVMRelaxJmp:
  case AVMRelaxCall:
    if (fits8(2))
      return 2;
    if (fits16(3))
      return 3;
    return 4;
  case AVMRelaxCond:
    if (fits8(2))
      return 2;
    if (fits16(5))
      return 5;
    return 6;
  default:
    llvm_unreachable("invalid AVM relaxation kind");
  }
}

bool AVM::relaxOnce(int pass) const {
  // LLD normally copies raw input relocations for --emit-relocs. Preserve the
  // maximal form in that mode rather than emit stale relocations for a
  // shortened instruction.
  if (!ctx.arg.relax || ctx.arg.emitRelocs)
    return false;
  if (pass == 0)
    initAVMRelaxation(ctx);

  SmallVector<InputSection *, 0> storage;
  bool changed = false;
  for (OutputSection *osec : ctx.outputSections) {
    for (InputSection *sec : getInputSections(*osec, storage)) {
      if (!sec->relaxAux)
        continue;
      if (pass == 0 && !validateAVMRelaxation(ctx, *sec))
        continue;
      RelaxAux &aux = *sec->relaxAux;
      if (aux.relaxInvalid)
        continue;
      MutableArrayRef<Relocation> relocs = sec->relocs();
      ArrayRef<SymbolAnchor> anchors = aux.anchors;
      uint64_t delta = 0;
      for (auto [i, marker] : llvm::enumerate(relocs)) {
        for (; !anchors.empty() && anchors.front().offset <= marker.offset;
             anchors = anchors.drop_front()) {
          const SymbolAnchor &a = anchors.front();
          if (a.end)
            a.d->size = a.offset - delta - a.d->value;
          else
            a.d->value = a.offset - delta;
        }
        uint32_t &cur = aux.relocDeltas[i];
        if (marker.type != R_AVM_RELAX || aux.relocPairs[i] == UINT32_MAX) {
          if (cur != delta) {
            cur = delta;
            changed = true;
          }
          continue;
        }
        uint64_t initialSize = 0;
        AVMRelaxKind kind =
            getRelaxKind(sec->content(), marker.offset, initialSize);
        const Relocation &far = relocs[aux.relocPairs[i]];
        const uint8_t selected = chooseAVMForm(
            ctx, *sec, aux, marker, far, relocs, kind, initialSize,
            sec->getVA() + marker.offset - delta);
        // A linker-script alignment change can make a formerly valid compact
        // form overflow. Re-evaluate from the current layout instead of
        // permanently committing to a shorter encoding.
        const uint8_t finalSize = selected;
        aux.relocStates[i] = finalSize;
        delta += initialSize - finalSize;
        if (cur != delta) {
          cur = delta;
          changed = true;
        }
      }
      for (const SymbolAnchor &a : anchors) {
        if (a.end)
          a.d->size = a.offset - delta - a.d->value;
        else
          a.d->value = a.offset - delta;
      }
      sec->bytesDropped = delta;
    }
  }
  return changed;
}

void AVM::finalizeRelax(int) const {
  if (!ctx.arg.relax)
    return;
  SmallVector<InputSection *, 0> storage;
  for (OutputSection *osec : ctx.outputSections) {
    for (InputSection *sec : getInputSections(*osec, storage)) {
      if (!sec->relaxAux)
        continue;
      RelaxAux &aux = *sec->relaxAux;
      if (aux.relaxInvalid)
        continue;
      MutableArrayRef<Relocation> relocs = sec->relocs();
      ArrayRef<uint8_t> old = sec->content();
      if (relocs.empty())
        continue;
      uint64_t total = aux.relocDeltas[relocs.size() - 1];
      uint8_t *out = ctx.bAlloc.Allocate<uint8_t>(old.size() - total);
      uint8_t *p = out;
      uint64_t offset = 0;
      for (auto [i, marker] : llvm::enumerate(relocs)) {
        if (!aux.relocStates[i])
          continue;
        uint64_t initialSize = 0;
        AVMRelaxKind kind = getRelaxKind(old, marker.offset, initialSize);
        memcpy(p, old.data() + offset, marker.offset - offset);
        p += marker.offset - offset;
        const uint8_t finalSize = aux.relocStates[i];
        switch (kind) {
        case AVMRelaxJmp:
          if (finalSize == 2) {
            p[0] = 0xd4;
            p[1] = 0;
          } else if (finalSize == 3) {
            p[0] = 0xe0;
            p[1] = p[2] = 0;
          } else {
            p[0] = 0xe2;
            p[1] = p[2] = p[3] = 0;
          }
          break;
        case AVMRelaxCall:
          if (finalSize == 2) {
            p[0] = 0xd5;
            p[1] = 0;
          } else if (finalSize == 3) {
            p[0] = 0xe1;
            p[1] = p[2] = 0;
          } else {
            p[0] = 0xe3;
            p[1] = p[2] = p[3] = 0;
          }
          break;
        case AVMRelaxCond:
          if (finalSize == 2) {
            p[0] = directCondition(old[marker.offset]);
            p[1] = 0;
          } else if (finalSize == 5) {
            p[0] = old[marker.offset];
            p[1] = 3;
            p[2] = 0xe0;
            p[3] = p[4] = 0;
          } else {
            memcpy(p, old.data() + marker.offset, 6);
          }
          break;
        default:
          llvm_unreachable("invalid AVM relaxation kind");
        }
        p += finalSize;
        offset = marker.offset + initialSize;
        const uint32_t farIndex = aux.relocPairs[i];
        aux.relocTypes[i] = R_AVM_NONE;
        aux.relocTypes[farIndex] =
            finalSize == initialSize
                ? R_AVM_FAR24
                : (finalSize == 2 ? R_AVM_PCREL8 : R_AVM_PCREL16);
        relocs[farIndex].expr = finalSize == initialSize ? R_ABS : R_PC;
      }
      memcpy(p, old.data() + offset, old.size() - offset);
      sec->content_ = out;
      sec->size = old.size() - total;
      sec->bytesDropped = 0;

      auto removedBefore = [&](uint64_t targetOffset) {
        uint64_t removed = 0;
        for (auto [i, marker] : llvm::enumerate(relocs)) {
          if (!aux.relocStates[i] || marker.offset >= targetOffset)
            continue;
          uint64_t initialSize = 0;
          getRelaxKind(old, marker.offset, initialSize);
          removed += initialSize - aux.relocStates[i];
        }
        return removed;
      };
      for (Relocation &r : relocs) {
        const Defined *d = dyn_cast<Defined>(r.sym);
        if (!d || !d->isSection() || d->section != sec)
          continue;
        const int64_t targetOffset = static_cast<int64_t>(d->value) + r.addend;
        if (targetOffset >= 0)
          r.addend -= removedBefore(targetOffset);
      }

      SmallVector<uint8_t, 0> fieldOffsets(relocs.size(), 0);
      for (auto [i, marker] : llvm::enumerate(relocs)) {
        if (!aux.relocStates[i])
          continue;
        uint64_t initialSize = 0;
        AVMRelaxKind kind = getRelaxKind(old, marker.offset, initialSize);
        const uint8_t finalSize = aux.relocStates[i];
        fieldOffsets[aux.relocPairs[i]] =
            finalSize == 2 ? 1 : (kind == AVMRelaxCond ? 3 : 1);
      }

      uint64_t delta = 0;
      for (size_t i = 0, e = relocs.size(); i != e; ++i) {
        relocs[i].offset -= delta;
        if (aux.relocTypes[i] != R_AVM_NONE || relocs[i].type == R_AVM_RELAX)
          relocs[i].type = aux.relocTypes[i];
        delta = aux.relocDeltas[i];
      }
      for (auto [i, marker] : llvm::enumerate(relocs))
        if (aux.relocStates[i])
          relocs[aux.relocPairs[i]].offset =
              marker.offset + fieldOffsets[aux.relocPairs[i]];
    }
  }
}
} // namespace

void elf::setAVMTargetInfo(Ctx &Ctx) { Ctx.target.reset(new AVM(Ctx)); }
