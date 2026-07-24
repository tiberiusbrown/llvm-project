//===- AVMSystemServiceInfoTest.cpp --------------------------------------===//

#include "AVMSystemServiceInfo.h"
#include "AVM.h"
#include "gtest/gtest.h"

#include <vector>

using namespace llvm;

namespace {

struct ExpectedService {
  unsigned Opcode;
  uint8_t ID;
  std::vector<MCPhysReg> Inputs;
  std::vector<MCPhysReg> Outputs;
  std::vector<int> Ties;
  std::vector<AVMServicePointerPolicy> Policies;
  std::vector<MachineMemOperand::Flags> MemoryFlags;
  std::vector<unsigned> AddressSpaces;
};

TEST(AVMSystemServiceInfoTest, EveryCurrentServiceDescriptor) {
  using PP = AVMServicePointerPolicy;
  using MF = MachineMemOperand;
  const std::vector<ExpectedService> Expected = {
      {AVM::SYS_DEBUG_PUTC_PSEUDO, 0x00, {AVM::R4}, {}, {}, {PP::None}, {}, {}},
      {AVM::SYS_DEBUG_BREAK_PSEUDO, 0x01, {}, {}, {}, {}, {}, {}},
      {AVM::SYS_MILLIS_PSEUDO, 0x02, {}, {AVM::R4}, {-1}, {}, {}, {}},
      {AVM::SYS_MILLIS32_PSEUDO, 0x03, {}, {AVM::R4R5}, {-1}, {}, {}, {}},
      {AVM::SYS_SINF_PSEUDO,
       0x04,
       {AVM::R4R5},
       {AVM::R4R5},
       {0},
       {PP::None},
       {},
       {}},
      {AVM::SYS_COSF_PSEUDO,
       0x05,
       {AVM::R4R5},
       {AVM::R4R5},
       {0},
       {PP::None},
       {},
       {}},
      {AVM::SYS_ATAN2F_PSEUDO,
       0x06,
       {AVM::R4R5, AVM::R6R7},
       {AVM::R4R5},
       {0},
       {PP::None, PP::None},
       {},
       {}},
      {AVM::SYS_TANF_PSEUDO,
       0x07,
       {AVM::R4R5},
       {AVM::R4R5},
       {0},
       {PP::None},
       {},
       {}},
      {AVM::SYS_EXPF_PSEUDO,
       0x08,
       {AVM::R4R5},
       {AVM::R4R5},
       {0},
       {PP::None},
       {},
       {}},
      {AVM::SYS_LOGF_PSEUDO,
       0x09,
       {AVM::R4R5},
       {AVM::R4R5},
       {0},
       {PP::None},
       {},
       {}},
      {AVM::SYS_LOG2F_PSEUDO,
       0x0a,
       {AVM::R4R5},
       {AVM::R4R5},
       {0},
       {PP::None},
       {},
       {}},
      {AVM::SYS_LOG10F_PSEUDO,
       0x0b,
       {AVM::R4R5},
       {AVM::R4R5},
       {0},
       {PP::None},
       {},
       {}},
      {AVM::SYS_POWF_PSEUDO,
       0x0c,
       {AVM::R4R5, AVM::R6R7},
       {AVM::R4R5},
       {0},
       {PP::None, PP::None},
       {},
       {}},
      {AVM::SYS_HYPOTF_PSEUDO,
       0x0d,
       {AVM::R4R5, AVM::R6R7},
       {AVM::R4R5},
       {0},
       {PP::None, PP::None},
       {},
       {}},
      {AVM::SYS_FMODF_PSEUDO,
       0x0e,
       {AVM::R4R5, AVM::R6R7},
       {AVM::R4R5},
       {0},
       {PP::None, PP::None},
       {},
       {}},
      {AVM::SYS_MEMCPY_PSEUDO,
       0x0f,
       {AVM::R4, AVM::R5, AVM::R6},
       {AVM::R4},
       {0},
       {PP::None, PP::None, PP::None},
       {MF::MOStore, MF::MOLoad},
       {0, 0}},
      {AVM::SYS_MEMCPY_P_PSEUDO,
       0x10,
       {AVM::R4, AVM::R6R7, AVM::R5},
       {AVM::R4},
       {0},
       {PP::None, PP::IgnorePadding, PP::None},
       {MF::MOStore, MF::MOLoad},
       {0, 1}},
      {AVM::SYS_MEMSET_PSEUDO,
       0x11,
       {AVM::R4, AVM::R5, AVM::R6},
       {AVM::R4},
       {0},
       {PP::None, PP::None, PP::None},
       {MF::MOStore},
       {0}},
      {AVM::SYS_MEMMOVE_PSEUDO,
       0x12,
       {AVM::R4, AVM::R5, AVM::R6},
       {AVM::R4},
       {0},
       {PP::None, PP::None, PP::None},
       {MF::MOStore, MF::MOLoad},
       {0, 0}},
      {AVM::SYS_MEMCMP_P_PSEUDO,
       0x13,
       {AVM::R4, AVM::R6R7, AVM::R5},
       {AVM::R4},
       {0},
       {PP::None, PP::RequireNormalized, PP::None},
       {MF::MOLoad, MF::MOLoad},
       {0, 1}},
      {AVM::SYS_STRCMP_P_PSEUDO,
       0x14,
       {AVM::R4, AVM::R6R7},
       {AVM::R4},
       {0},
       {PP::None, PP::RequireNormalized},
       {MF::MOLoad, MF::MOLoad},
       {0, 1}},
      {AVM::SYS_STRLEN_P_PSEUDO,
       0x15,
       {AVM::R6R7},
       {AVM::R4},
       {-1},
       {PP::RequireNormalized},
       {MF::MOLoad},
       {1}},
      {AVM::SYS_STRNCPY_P_PSEUDO,
       0x16,
       {AVM::R4, AVM::R6R7, AVM::R5},
       {AVM::R4},
       {0},
       {PP::None, PP::RequireNormalized, PP::None},
       {MF::MOStore, MF::MOLoad},
       {0, 1}},
      {AVM::SYS_STRNCAT_P_PSEUDO,
       0x17,
       {AVM::R4, AVM::R6R7, AVM::R5},
       {AVM::R4},
       {0},
       {PP::None, PP::RequireNormalized, PP::None},
       {MF::MOLoad, MF::MOStore, MF::MOLoad},
       {0, 0, 1}},
      {AVM::SYS_MEMCMP_PSEUDO,
       0x18,
       {AVM::R4, AVM::R5, AVM::R6},
       {AVM::R4},
       {0},
       {PP::None, PP::None, PP::None},
       {MF::MOLoad, MF::MOLoad},
       {0, 0}},
      {AVM::SYS_STRCMP_PSEUDO,
       0x19,
       {AVM::R4, AVM::R5},
       {AVM::R4},
       {0},
       {PP::None, PP::None},
       {MF::MOLoad, MF::MOLoad},
       {0, 0}},
      {AVM::SYS_STRLEN_PSEUDO,
       0x1a,
       {AVM::R4},
       {AVM::R4},
       {0},
       {PP::None},
       {MF::MOLoad},
       {0}},
      {AVM::SYS_STRNCPY_PSEUDO,
       0x1b,
       {AVM::R4, AVM::R5, AVM::R6},
       {AVM::R4},
       {0},
       {PP::None, PP::None, PP::None},
       {MF::MOStore, MF::MOLoad},
       {0, 0}},
      {AVM::SYS_STRNCAT_PSEUDO,
       0x1c,
       {AVM::R4, AVM::R5, AVM::R6},
       {AVM::R4},
       {0},
       {PP::None, PP::None, PP::None},
       {MF::MOLoad, MF::MOStore, MF::MOLoad},
       {0, 0, 0}},
      {AVM::SYS_DISPLAY_PSEUDO, 0x1d, {}, {}, {}, {}, {MF::MOLoad}, {0}},
      {AVM::SYS_DRAW_SPRITE_OVERWRITE_PSEUDO,
       0x1e,
       {AVM::R4, AVM::R5, AVM::R6R7, AVM::R0, MCPhysReg()},
       {},
       {},
       {PP::None, PP::None, PP::IgnorePadding, PP::None, PP::None},
       {MF::MOLoad, MF::MOStore, MF::MOLoad},
       {0, 0, 1}},
      {AVM::SYS_DRAW_SPRITE_PLUS_MASK_PSEUDO,
       0x1f,
       {AVM::R4, AVM::R5, AVM::R6R7, AVM::R0, MCPhysReg()},
       {},
       {},
       {PP::None, PP::None, PP::IgnorePadding, PP::None, PP::None},
       {MF::MOLoad, MF::MOStore, MF::MOLoad},
       {0, 0, 1}},
      {AVM::SYS_DRAW_SPRITE_SELF_MASKED_PSEUDO,
       0x20,
       {AVM::R4, AVM::R5, AVM::R6R7, AVM::R0, MCPhysReg()},
       {},
       {},
       {PP::None, PP::None, PP::IgnorePadding, PP::None, PP::None},
       {MF::MOLoad, MF::MOStore, MF::MOLoad},
       {0, 0, 1}},
      {AVM::SYS_DRAW_SPRITE_ERASE_PSEUDO,
       0x21,
       {AVM::R4, AVM::R5, AVM::R6R7, AVM::R0, MCPhysReg()},
       {},
       {},
       {PP::None, PP::None, PP::IgnorePadding, PP::None, PP::None},
       {MF::MOLoad, MF::MOStore, MF::MOLoad},
       {0, 0, 1}},
  };

  for (const ExpectedService &E : Expected) {
    SCOPED_TRACE(E.ID);
    const AVMSystemServiceInfo &Info =
        getRequiredAVMSystemServiceInfo(E.Opcode);
    EXPECT_EQ(Info.ServiceID, E.ID);
    ASSERT_EQ(Info.Inputs.size(), E.Inputs.size());
    ASSERT_EQ(Info.Inputs.size(), E.Policies.size());
    for (unsigned I = 0; I != Info.Inputs.size(); ++I) {
      EXPECT_EQ(Info.Inputs[I].PhysReg, E.Inputs[I]);
      EXPECT_EQ(Info.Inputs[I].PointerPolicy, E.Policies[I]);
    }
    ASSERT_EQ(Info.Outputs.size(), E.Outputs.size());
    ASSERT_EQ(Info.Outputs.size(), E.Ties.size());
    for (unsigned I = 0; I != Info.Outputs.size(); ++I) {
      EXPECT_EQ(Info.Outputs[I].PhysReg, E.Outputs[I]);
      EXPECT_EQ(Info.Outputs[I].TiedLogicalInput, E.Ties[I]);
    }
    ASSERT_EQ(Info.MemoryAccesses.size(), E.MemoryFlags.size());
    ASSERT_EQ(Info.MemoryAccesses.size(), E.AddressSpaces.size());
    for (unsigned I = 0; I != Info.MemoryAccesses.size(); ++I) {
      EXPECT_EQ(Info.MemoryAccesses[I].Flags, E.MemoryFlags[I]);
      EXPECT_EQ(Info.MemoryAccesses[I].AddressSpace, E.AddressSpaces[I]);
    }
    EXPECT_TRUE(isAVMSystemService(E.Opcode));
    EXPECT_EQ(getAVMSystemServiceID(E.Opcode), E.ID);
  }
}

struct ExpectedMemoryAccess {
  AVMServiceMemoryBaseKind BaseKind;
  unsigned LogicalArgumentIndex;
  const char *FixedGlobalName;
  AVMServiceMemorySizeKind SizeKind;
  unsigned SizeLogicalArgumentIndex;
  uint64_t ConstantSize;
};

TEST(AVMSystemServiceInfoTest, EveryMemoryAddressAndSizeContract) {
  using MB = AVMServiceMemoryBaseKind;
  using MS = AVMServiceMemorySizeKind;
  const ExpectedMemoryAccess Copy[] = {
      {MB::LogicalArgument, 0, "", MS::LogicalArgument, 2, 0},
      {MB::LogicalArgument, 1, "", MS::LogicalArgument, 2, 0},
  };
  const ExpectedMemoryAccess Set[] = {
      {MB::LogicalArgument, 0, "", MS::LogicalArgument, 2, 0},
  };
  const ExpectedMemoryAccess Compare[] = {
      {MB::LogicalArgument, 0, "", MS::LogicalArgument, 2, 0},
      {MB::LogicalArgument, 1, "", MS::LogicalArgument, 2, 0},
  };
  const ExpectedMemoryAccess Strcmp[] = {
      {MB::LogicalArgument, 0, "", MS::AfterPointer, 0, 0},
      {MB::LogicalArgument, 1, "", MS::AfterPointer, 0, 0},
  };
  const ExpectedMemoryAccess Strlen[] = {
      {MB::LogicalArgument, 0, "", MS::AfterPointer, 0, 0},
  };
  const ExpectedMemoryAccess Strncpy[] = {
      {MB::LogicalArgument, 0, "", MS::LogicalArgument, 2, 0},
      {MB::LogicalArgument, 1, "", MS::LogicalArgument, 2, 0},
  };
  const ExpectedMemoryAccess Strncat[] = {
      {MB::LogicalArgument, 0, "", MS::AfterPointer, 0, 0},
      {MB::LogicalArgument, 0, "", MS::AfterPointer, 0, 0},
      {MB::LogicalArgument, 1, "", MS::LogicalArgument, 2, 0},
  };
  const ExpectedMemoryAccess Display[] = {
      {MB::FixedGlobal, 0, "__avm_framebuffer", MS::Constant, 0, 1024},
  };
  const ExpectedMemoryAccess Sprite[] = {
      {MB::LogicalArgument, 4, "__avm_framebuffer", MS::Constant, 0, 1024},
      {MB::LogicalArgument, 4, "__avm_framebuffer", MS::Constant, 0, 1024},
      {MB::LogicalArgument, 2, "", MS::AfterPointer, 0, 0},
  };

  auto Check = [](unsigned Opcode, ArrayRef<ExpectedMemoryAccess> Expected) {
    const AVMSystemServiceInfo &Info = getRequiredAVMSystemServiceInfo(Opcode);
    ASSERT_EQ(Info.MemoryAccesses.size(), Expected.size());
    for (unsigned I = 0; I != Expected.size(); ++I) {
      EXPECT_EQ(Info.MemoryAccesses[I].BaseKind, Expected[I].BaseKind);
      EXPECT_EQ(Info.MemoryAccesses[I].LogicalArgumentIndex,
                Expected[I].LogicalArgumentIndex);
      EXPECT_EQ(Info.MemoryAccesses[I].FixedGlobalName,
                Expected[I].FixedGlobalName);
      EXPECT_EQ(Info.MemoryAccesses[I].SizeKind, Expected[I].SizeKind);
      EXPECT_EQ(Info.MemoryAccesses[I].SizeLogicalArgumentIndex,
                Expected[I].SizeLogicalArgumentIndex);
      EXPECT_EQ(Info.MemoryAccesses[I].ConstantSize, Expected[I].ConstantSize);
    }
  };

  Check(AVM::SYS_MEMCPY_PSEUDO, Copy);
  Check(AVM::SYS_MEMSET_PSEUDO, Set);
  Check(AVM::SYS_MEMMOVE_PSEUDO, Copy);
  Check(AVM::SYS_MEMCPY_P_PSEUDO, Copy);
  Check(AVM::SYS_MEMCMP_P_PSEUDO, Compare);
  Check(AVM::SYS_STRCMP_P_PSEUDO, Strcmp);
  Check(AVM::SYS_STRLEN_P_PSEUDO, Strlen);
  Check(AVM::SYS_STRNCPY_P_PSEUDO, Strncpy);
  Check(AVM::SYS_STRNCAT_P_PSEUDO, Strncat);
  Check(AVM::SYS_MEMCMP_PSEUDO, Compare);
  Check(AVM::SYS_STRCMP_PSEUDO, Strcmp);
  Check(AVM::SYS_STRLEN_PSEUDO, Strlen);
  Check(AVM::SYS_STRNCPY_PSEUDO, Strncpy);
  Check(AVM::SYS_STRNCAT_PSEUDO, Strncat);
  Check(AVM::SYS_DISPLAY_PSEUDO, Display);
  Check(AVM::SYS_DRAW_SPRITE_OVERWRITE_PSEUDO, Sprite);
  Check(AVM::SYS_DRAW_SPRITE_PLUS_MASK_PSEUDO, Sprite);
  Check(AVM::SYS_DRAW_SPRITE_SELF_MASKED_PSEUDO, Sprite);
  Check(AVM::SYS_DRAW_SPRITE_ERASE_PSEUDO, Sprite);
}

} // namespace
