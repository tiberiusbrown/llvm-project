add_llvm_component_group(AVM)

# The MC sublibraries include generated target descriptions from this target
# directory. `add_llvm_target` used to supply these inherited include paths;
# retain them now that AVM is intentionally not a code-generation target.
include_directories(BEFORE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR})

set(LLVM_TARGET_DEFINITIONS AVM.td)
llvm_tablegen(AVMGenRegisterInfo.inc -gen-register-info)
llvm_tablegen(AVMGenInstrInfo.inc -gen-instr-info)
llvm_tablegen(AVMGenSubtargetInfo.inc -gen-subtarget)
llvm_tablegen(AVMGenCallingConv.inc -gen-callingconv)
llvm_tablegen(AVMGenDAGISel.inc -gen-dag-isel)
add_public_tablegen_target(AVMCommonTableGen)

add_llvm_target(AVMCodeGen
  AVMAddressReuse.cpp
  AVMAggregateArguments.cpp
  AVMAtomicLibcalls.cpp
  AVMStackObjectAlignment.cpp
  AVMAggregateReturns.cpp
  AVMAsmPrinter.cpp
  AVMBranchPolarity.cpp
  AVMBytePHIs.cpp
  AVMCanonicalizeBooleans.cpp
  AVMConstantInitializers.cpp
  AVMCostModel.cpp
  AVMExpandSystemServices.cpp
  AVMExpandPseudoInsts.cpp
  AVMExitValues.cpp
  AVMFixedFloatRegs.cpp
  AVMFinalControlFlow.cpp
  AVMFrameLowering.cpp
  AVMISelDAGToDAG.cpp
  AVMISelLowering.cpp
  AVMInstrInfo.cpp
  AVMInlineAsmStack.cpp
  AVMLatePeephole.cpp
  AVMLoopStrengthReduce.cpp
  AVMMachineFunctionInfo.cpp
  AVMMCInstLower.cpp
  AVMProgramMemoryWidening.cpp
  AVMPrivateSpillCleanup.cpp
  AVMRegisterInfo.cpp
  AVMRegisterRange.cpp
  AVMSelectionDAGInfo.cpp
  AVMStackTargets.cpp
  AVMSystemServiceInfo.cpp
  AVMSystemServiceRegions.cpp
  AVMSubtarget.cpp
  AVMTailDuplication.cpp
  AVMTargetMachine.cpp
  AVMTargetObjectFile.cpp
  AVMTargetTransformInfo.cpp
  AVMWidenOddIntegers.cpp

  DEPENDS
  AVMCommonTableGen

  LINK_COMPONENTS
  Analysis
  AsmPrinter
  AVMDesc
  AVMInfo
  CodeGen
  CodeGenTypes
  Core
  InstCombine
  IPO
  MC
  MCParser
  SelectionDAG
  Support
  Target
  TargetParser
  TransformUtils

  ADD_TO_COMPONENT
  AVM
)

add_subdirectory(AsmParser)
add_subdirectory(Disassembler)
add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
