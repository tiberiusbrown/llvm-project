//===-- AVMCallingConv.td - AVM calling conventions ---------*- tablegen -*-===//

def CC_AVM : CallingConv<[
  CCIfType<[i16], CCAssignToReg<[R4, R5, R6, R7]>>
]>;

def RetCC_AVM : CallingConv<[
  CCIfType<[i16], CCAssignToReg<[R4]>>
]>;
