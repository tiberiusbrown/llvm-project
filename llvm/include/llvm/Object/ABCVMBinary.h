//===-- ABCVMBinary.h - ABC VM final binary parsing ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_OBJECT_ABCVMBINARY_H
#define LLVM_OBJECT_ABCVMBINARY_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include <algorithm>
#include <cstdint>
#include <system_error>

namespace llvm::object {

struct ABCVMBinaryInfo {
  static constexpr size_t HeaderSize = 256;
  static constexpr size_t TrailerSize = 6;

  uint64_t FileSize = 0;
  uint16_t SaveSize = 0;
  uint8_t FileTableLength = 0;
  uint32_t FileTableOffset = 0;
  uint32_t LineTableOffset = 0;
  uint8_t Shades = 0;
  uint32_t GlobInitAddress = 0;
  uint32_t MainAddress = 0;
  uint8_t StartupReturnSlotSize = 0;
  uint16_t DevDataPage = 0;

  uint64_t ProgramDataOffset = HeaderSize;
  uint64_t CodeOffset = HeaderSize;
  uint64_t FileTableSize = 0;
  uint64_t LineTableSize = 0;
  uint64_t ProgramDataSize = 0;
  uint64_t CodeSize = 0;
  uint64_t PayloadEndOffset = 0;
  uint64_t TrailerOffset = 0;
  uint64_t PaddingSize = 0;
};

inline uint16_t readABCLittle16(StringRef Buffer, size_t Offset) {
  return uint16_t(uint8_t(Buffer[Offset])) |
         (uint16_t(uint8_t(Buffer[Offset + 1])) << 8);
}

inline uint32_t readABCLittle24(StringRef Buffer, size_t Offset) {
  return uint32_t(uint8_t(Buffer[Offset])) |
         (uint32_t(uint8_t(Buffer[Offset + 1])) << 8) |
         (uint32_t(uint8_t(Buffer[Offset + 2])) << 16);
}

inline bool isABCVMBinary(StringRef Buffer) {
  static constexpr char Signature[] = {
      char(0xAB), char(0xC0), char(0x0A), char(0xBC)};
  return Buffer.size() >= 4 && Buffer.substr(0, 4) == StringRef(Signature, 4);
}

inline Expected<ABCVMBinaryInfo> parseABCVMBinary(StringRef Buffer) {
  static constexpr char Signature[] = {
      char(0xAB), char(0xC0), char(0x0A), char(0xBC)};
  static constexpr char EndSignature[] = {
      char(0xAB), char(0xCE), char(0xEA), char(0xBC)};
  static constexpr uint8_t OpAlloc = 0x4A;
  static constexpr uint8_t OpCall = 0xB5;

  if (Buffer.size() < ABCVMBinaryInfo::HeaderSize + ABCVMBinaryInfo::TrailerSize)
    return createStringError(std::errc::invalid_argument,
                             "ABC VM binary is truncated");
  if (Buffer.substr(0, 4) != StringRef(Signature, 4))
    return createStringError(std::errc::invalid_argument,
                             "missing ABC VM header signature");
  if (Buffer.substr(Buffer.size() - 4, 4) != StringRef(EndSignature, 4))
    return createStringError(std::errc::invalid_argument,
                             "missing ABC VM trailer signature");

  ABCVMBinaryInfo Info;
  Info.FileSize = Buffer.size();
  Info.SaveSize = readABCLittle16(Buffer, 0x0A);
  Info.FileTableLength = uint8_t(Buffer[0x0C]);
  Info.FileTableOffset = readABCLittle24(Buffer, 0x0D);
  Info.LineTableOffset = readABCLittle24(Buffer, 0x10);
  Info.Shades = uint8_t(Buffer[0x13]);
  if (uint8_t(Buffer[0x14]) != OpCall)
    return createStringError(std::errc::invalid_argument,
                             "startup does not begin with CALL $globinit");
  Info.GlobInitAddress = readABCLittle24(Buffer, 0x15);

  size_t MainCallOffset = 0;
  if (uint8_t(Buffer[0x18]) == OpAlloc) {
    Info.StartupReturnSlotSize = uint8_t(Buffer[0x19]);
    MainCallOffset = 0x1A;
  } else {
    Info.StartupReturnSlotSize = 0;
    MainCallOffset = 0x18;
  }
  if (uint8_t(Buffer[MainCallOffset]) != OpCall)
    return createStringError(std::errc::invalid_argument,
                             "startup does not contain CALL main");
  Info.MainAddress = readABCLittle24(Buffer, MainCallOffset + 1);
  Info.TrailerOffset = Buffer.size() - ABCVMBinaryInfo::TrailerSize;
  Info.PayloadEndOffset = Info.TrailerOffset;
  Info.DevDataPage = readABCLittle16(Buffer, Info.TrailerOffset);

  if (Info.FileTableOffset > Info.PayloadEndOffset)
    return createStringError(std::errc::invalid_argument,
                             "file table offset is outside the binary");
  if (Info.LineTableOffset > Info.PayloadEndOffset)
    return createStringError(std::errc::invalid_argument,
                             "line table offset is outside the binary");
  if (Info.LineTableOffset < Info.FileTableOffset)
    return createStringError(std::errc::invalid_argument,
                             "line table offset precedes file table offset");

  uint64_t CodeOffset = std::min<uint32_t>(Info.GlobInitAddress, Info.MainAddress);
  if (CodeOffset < ABCVMBinaryInfo::HeaderSize || CodeOffset > Info.FileTableOffset)
    CodeOffset = Info.FileTableOffset;
  Info.CodeOffset = CodeOffset;
  Info.ProgramDataSize = Info.CodeOffset - Info.ProgramDataOffset;
  Info.CodeSize = Info.FileTableOffset - Info.CodeOffset;
  Info.FileTableSize = Info.LineTableOffset - Info.FileTableOffset;
  Info.LineTableSize = Info.PayloadEndOffset - Info.LineTableOffset;

  uint64_t UsedBeforeTrailer = Info.LineTableOffset + Info.LineTableSize;
  if (UsedBeforeTrailer > Info.TrailerOffset)
    return createStringError(std::errc::invalid_argument,
                             "debug tables extend into the trailer");
  Info.PaddingSize = Info.TrailerOffset - UsedBeforeTrailer;
  return Info;
}

} // namespace llvm::object

#endif // LLVM_OBJECT_ABCVMBINARY_H
