//===- AVM.h - AVM stack ABI metadata -----------------------------*- C++ -*-===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef LLVM_BINARYFORMAT_AVM_H
#define LLVM_BINARYFORMAT_AVM_H

#include <cstdint>

namespace llvm::AVM {
inline constexpr uint64_t StackLimit = 256;
inline constexpr uint64_t ReturnAddressSize = 3;

// .avm.stackcalls consists of 9-byte records, with no padding:
// caller (3-byte relocated text address), callee (3-byte relocated text address
// or zero for an indirect call), outgoing bytes (uint16 LE), flags (uint8).
// Addresses are ELF section offsets, not packed interpreter program pointers.
// R_AVM_DEBUG24 relocations permit ordinary ELF symbol resolution. Each section
// has SHF_LINK_ORDER and the same COMDAT group as its associated text section.
inline constexpr unsigned StackCallRecordSize = 9;
inline constexpr unsigned StackCallIndirect = 1;
inline constexpr unsigned StackCallTail = 2;
// A function marker, not a call: dynamic allocation or opaque inline assembly
// prevents a complete bound. Caller is relocated; the remaining fields are 0.
inline constexpr unsigned StackCallIncomplete = 4;
// A function marker certifying that its machine calls have been recorded.
// Even leaf functions need this to distinguish legacy .stack_sizes-only
// objects from a complete callsite list. May be combined with Incomplete.
inline constexpr unsigned StackCallFunction = 8;
} // namespace llvm::AVM

#endif
