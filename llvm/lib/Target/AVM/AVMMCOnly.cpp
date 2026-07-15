//===-- AVMMCOnly.cpp ----------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// AVM intentionally exposes an MC-only target aggregate.  Keeping this
// translation unit separate from the assembler/disassembler libraries lets
// generic LLVM tools link the AVM target without registering a TargetMachine.
//
//===----------------------------------------------------------------------===//

namespace llvm {
void llvmAVMMCOnlyAnchor() {}
}

// LLVM's generated InitializeAllTargets() table calls this symbol for every
// configured target.  AVM deliberately registers no TargetMachine here: MC
// initialization is provided by LLVMInitializeAVMTargetMC instead.
extern "C" void LLVMInitializeAVMTarget() {}
