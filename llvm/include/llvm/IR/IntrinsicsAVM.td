//===- IntrinsicsAVM.td - AVM intrinsics ------------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

def llvm_avm_progptr_ty : LLVMQualPointerType<1>;

let TargetPrefix = "avm" in {
  def int_avm_debug_putc : Intrinsic<[], [llvm_i8_ty],
      [IntrNoMem, IntrHasSideEffects, IntrWillReturn, IntrNoCallback,
       IntrNoFree]>;
  def int_avm_debug_break : Intrinsic<[], [],
      [IntrNoMem, IntrHasSideEffects, IntrWillReturn, IntrNoCallback,
       IntrNoFree]>;

  // The timer values are evolving implementation state.  IntrHasSideEffects
  // deliberately prevents CSE, speculation, duplication, and motion.
  def int_avm_millis : Intrinsic<[llvm_i16_ty], [],
      [IntrNoMem, IntrHasSideEffects, IntrWillReturn, IntrNoCallback,
       IntrNoFree]>;
  def int_avm_millis32 : Intrinsic<[llvm_i32_ty], [],
      [IntrNoMem, IntrHasSideEffects, IntrWillReturn, IntrNoCallback,
       IntrNoFree]>;

  class AVMUnaryMathIntrinsic
      : Intrinsic<[llvm_float_ty], [llvm_float_ty],
                  [IntrNoMem, IntrSpeculatable, IntrWillReturn, IntrNoCallback,
                   IntrNoFree]>;
  class AVMBinaryMathIntrinsic
      : Intrinsic<[llvm_float_ty], [llvm_float_ty, llvm_float_ty],
                  [IntrNoMem, IntrSpeculatable, IntrWillReturn, IntrNoCallback,
                   IntrNoFree]>;

  def int_avm_sinf : AVMUnaryMathIntrinsic;
  def int_avm_cosf : AVMUnaryMathIntrinsic;
  def int_avm_atan2f : AVMBinaryMathIntrinsic;
  def int_avm_tanf : AVMUnaryMathIntrinsic;
  def int_avm_expf : AVMUnaryMathIntrinsic;
  def int_avm_logf : AVMUnaryMathIntrinsic;
  def int_avm_log2f : AVMUnaryMathIntrinsic;
  def int_avm_log10f : AVMUnaryMathIntrinsic;
  def int_avm_powf : AVMBinaryMathIntrinsic;
  def int_avm_hypotf : AVMBinaryMathIntrinsic;
  def int_avm_fmodf : AVMBinaryMathIntrinsic;

  def int_avm_memcpy : Intrinsic<[llvm_ptr_ty],
      [llvm_ptr_ty, llvm_ptr_ty, llvm_i16_ty],
      [IntrArgMemOnly, IntrWillReturn, IntrNoFree, IntrNoCallback,
       NoCapture<ArgIndex<0>>, NoCapture<ArgIndex<1>>,
       NoAlias<ArgIndex<0>>, NoAlias<ArgIndex<1>>,
       WriteOnly<ArgIndex<0>>, ReadOnly<ArgIndex<1>>,
       Returned<ArgIndex<0>>]>;

  def int_avm_memset : Intrinsic<[llvm_ptr_ty],
      [llvm_ptr_ty, llvm_i16_ty, llvm_i16_ty],
      [IntrWriteMem, IntrArgMemOnly, IntrWillReturn, IntrNoFree, IntrNoCallback,
       NoCapture<ArgIndex<0>>, WriteOnly<ArgIndex<0>>,
       Returned<ArgIndex<0>>]>;

  def int_avm_memmove : Intrinsic<[llvm_ptr_ty],
      [llvm_ptr_ty, llvm_ptr_ty, llvm_i16_ty],
      [IntrArgMemOnly, IntrWillReturn, IntrNoFree, IntrNoCallback,
       NoCapture<ArgIndex<0>>, NoCapture<ArgIndex<1>>,
       WriteOnly<ArgIndex<0>>, ReadOnly<ArgIndex<1>>,
       Returned<ArgIndex<0>>]>;

  def int_avm_memcmp_p :
      Intrinsic<[llvm_i16_ty],
                [llvm_ptr_ty, llvm_avm_progptr_ty, llvm_i16_ty],
                [IntrReadMem, IntrArgMemOnly, IntrWillReturn,
                 NoCapture<ArgIndex<0>>, NoCapture<ArgIndex<1>>,
                 ReadOnly<ArgIndex<0>>, ReadOnly<ArgIndex<1>>]>;

  def int_avm_strcmp_p :
      Intrinsic<[llvm_i16_ty], [llvm_ptr_ty, llvm_avm_progptr_ty],
                [IntrReadMem, IntrArgMemOnly, IntrWillReturn,
                 NoCapture<ArgIndex<0>>, NoCapture<ArgIndex<1>>,
                 ReadOnly<ArgIndex<0>>, ReadOnly<ArgIndex<1>>]>;

  def int_avm_strlen_p :
      Intrinsic<[llvm_i16_ty], [llvm_avm_progptr_ty],
                [IntrReadMem, IntrArgMemOnly, IntrWillReturn,
                 NoCapture<ArgIndex<0>>, ReadOnly<ArgIndex<0>>]>;

  def int_avm_memcmp :
      Intrinsic<[llvm_i16_ty], [llvm_ptr_ty, llvm_ptr_ty, llvm_i16_ty],
                [IntrReadMem, IntrArgMemOnly, IntrWillReturn,
                 NoCapture<ArgIndex<0>>, NoCapture<ArgIndex<1>>,
                 ReadOnly<ArgIndex<0>>, ReadOnly<ArgIndex<1>>]>;

  def int_avm_strcmp :
      Intrinsic<[llvm_i16_ty], [llvm_ptr_ty, llvm_ptr_ty],
                [IntrReadMem, IntrArgMemOnly, IntrWillReturn,
                 NoCapture<ArgIndex<0>>, NoCapture<ArgIndex<1>>,
                 ReadOnly<ArgIndex<0>>, ReadOnly<ArgIndex<1>>]>;

  def int_avm_strlen :
      Intrinsic<[llvm_i16_ty], [llvm_ptr_ty],
                [IntrReadMem, IntrArgMemOnly, IntrWillReturn,
                 NoCapture<ArgIndex<0>>, ReadOnly<ArgIndex<0>>]>;

  def int_avm_strncpy_p :
      Intrinsic<[llvm_ptr_ty],
                [llvm_ptr_ty, llvm_avm_progptr_ty, llvm_i16_ty],
                [IntrArgMemOnly, IntrWillReturn,
                 NoCapture<ArgIndex<0>>, NoCapture<ArgIndex<1>>,
                 NoAlias<ArgIndex<0>>, NoAlias<ArgIndex<1>>,
                 Returned<ArgIndex<0>>, WriteOnly<ArgIndex<0>>,
                 ReadOnly<ArgIndex<1>>]>;

  def int_avm_strncpy :
      Intrinsic<[llvm_ptr_ty], [llvm_ptr_ty, llvm_ptr_ty, llvm_i16_ty],
                [IntrArgMemOnly, IntrWillReturn,
                 NoCapture<ArgIndex<0>>, NoCapture<ArgIndex<1>>,
                 NoAlias<ArgIndex<0>>, NoAlias<ArgIndex<1>>,
                 Returned<ArgIndex<0>>, WriteOnly<ArgIndex<0>>,
                 ReadOnly<ArgIndex<1>>]>;

  def int_avm_strncat_p :
      Intrinsic<[llvm_ptr_ty],
                [llvm_ptr_ty, llvm_avm_progptr_ty, llvm_i16_ty],
                [IntrArgMemOnly, IntrWillReturn,
                 NoCapture<ArgIndex<0>>, NoCapture<ArgIndex<1>>,
                 NoAlias<ArgIndex<0>>, NoAlias<ArgIndex<1>>,
                 Returned<ArgIndex<0>>, ReadOnly<ArgIndex<1>>]>;

  def int_avm_strncat :
      Intrinsic<[llvm_ptr_ty], [llvm_ptr_ty, llvm_ptr_ty, llvm_i16_ty],
                [IntrArgMemOnly, IntrWillReturn,
                 NoCapture<ArgIndex<0>>, NoCapture<ArgIndex<1>>,
                 NoAlias<ArgIndex<0>>, NoAlias<ArgIndex<1>>,
                 Returned<ArgIndex<0>>, ReadOnly<ArgIndex<1>>]>;
}
