//===-- AVMSystemServiceInfo.h - AVM system-service metadata ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AVM_AVMSYSTEMSERVICEINFO_H
#define LLVM_LIB_TARGET_AVM_AVMSYSTEMSERVICEINFO_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/MC/MCRegister.h"

#include <cstdint>

namespace llvm {

class TargetRegisterClass;

enum class AVMServiceValueKind {
  I16,
  I32,
  F32,
  ProgramPointer,
};

enum class AVMServicePointerPolicy {
  None,
  IgnorePadding,
  RequireNormalized,
};

struct AVMServiceInputInfo {
  unsigned LogicalArgumentIndex;
  AVMServiceValueKind Kind;
  MCPhysReg PhysReg;
  AVMServicePointerPolicy PointerPolicy;
  bool PassToMachine;
};

struct AVMServiceOutputInfo {
  unsigned ResultIndex;
  AVMServiceValueKind Kind;
  MCPhysReg PhysReg;
  int TiedLogicalInput;
};

enum class AVMServiceMemoryBaseKind {
  LogicalArgument,
  FixedGlobal,
  UnknownAddressSpace,
};

enum class AVMServiceMemorySizeKind {
  Constant,
  LogicalArgument,
  AfterPointer,
};

struct AVMServiceMemoryAccessInfo {
  AVMServiceMemoryBaseKind BaseKind;
  unsigned LogicalArgumentIndex;
  StringLiteral FixedGlobalName;
  unsigned AddressSpace;
  MachineMemOperand::Flags Flags;
  AVMServiceMemorySizeKind SizeKind;
  unsigned SizeLogicalArgumentIndex;
  uint64_t ConstantSize;
};

struct AVMSystemServiceInfo {
  unsigned Opcode;
  uint8_t ServiceID;
  StringLiteral AsmName;
  ArrayRef<AVMServiceInputInfo> Inputs;
  ArrayRef<AVMServiceOutputInfo> Outputs;
  ArrayRef<AVMServiceMemoryAccessInfo> MemoryAccesses;
};

const AVMSystemServiceInfo *getAVMSystemServiceInfo(unsigned Opcode);
const AVMSystemServiceInfo &getRequiredAVMSystemServiceInfo(unsigned Opcode);
bool isAVMSystemService(unsigned Opcode);
int getAVMSystemServiceID(unsigned Opcode);

const TargetRegisterClass *getAVMFixedRegisterClass(MCPhysReg Reg,
                                                    AVMServiceValueKind Kind);
const TargetRegisterClass *getAVMGeneralRegisterClass(AVMServiceValueKind Kind);
bool isAVMFixedServiceRegisterClass(const TargetRegisterClass *RC);

/// Return the explicit machine-operand index for an input. Outputs precede
/// inputs in a semantic service pseudo; hidden inputs have no operand.
int getAVMServiceInputOperandIndex(const AVMSystemServiceInfo &Info,
                                   unsigned LogicalArgumentIndex);

} // namespace llvm

#endif
