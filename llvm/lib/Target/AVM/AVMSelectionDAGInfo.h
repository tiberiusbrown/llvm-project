//===-- AVMSelectionDAGInfo.h - AVM SelectionDAG information -*- C++ -*-===//

#ifndef LLVM_LIB_TARGET_AVM_AVMSELECTIONDAGINFO_H
#define LLVM_LIB_TARGET_AVM_AVMSELECTIONDAGINFO_H

#include "llvm/CodeGen/SelectionDAGTargetInfo.h"

namespace llvm {
class AVMSelectionDAGInfo final : public SelectionDAGTargetInfo {
public:
  bool shouldDeferMemIntrinsics() const override { return true; }

  SDValue EmitTargetCodeForMemcpyWithAA(SelectionDAG &DAG, const SDLoc &DL,
                                        SDValue Chain, SDValue Dst, SDValue Src,
                                        SDValue Size, Align Alignment,
                                        bool IsVolatile, bool AlwaysInline,
                                        MachinePointerInfo DstPtrInfo,
                                        MachinePointerInfo SrcPtrInfo,
                                        const AAMDNodes &AAInfo) const override;

  SDValue EmitTargetCodeForMemmoveWithAA(
      SelectionDAG &DAG, const SDLoc &DL, SDValue Chain, SDValue Dst,
      SDValue Src, SDValue Size, Align Alignment, bool IsVolatile,
      MachinePointerInfo DstPtrInfo, MachinePointerInfo SrcPtrInfo,
      const AAMDNodes &AAInfo) const override;

  SDValue EmitTargetCodeForMemsetWithAA(SelectionDAG &DAG, const SDLoc &DL,
                                        SDValue Chain, SDValue Dst,
                                        SDValue Value, SDValue Size,
                                        Align Alignment, bool IsVolatile,
                                        bool AlwaysInline,
                                        MachinePointerInfo DstPtrInfo,
                                        const AAMDNodes &AAInfo) const override;
};
} // namespace llvm

#endif
