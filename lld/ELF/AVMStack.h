//===- AVMStack.h - Post-codegen AVM stack analysis
//------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef LLD_ELF_AVMSTACK_H
#define LLD_ELF_AVMSTACK_H
namespace lld::elf {
struct Ctx;
void analyzeAVMStack(Ctx &ctx);
} // namespace lld::elf
#endif
