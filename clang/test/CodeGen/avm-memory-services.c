// RUN: %clang --target=avm-unknown-arduboyfx -O2 -fomit-frame-pointer \
// RUN:   -S -emit-llvm %s -o - \
// RUN:   | FileCheck %s --check-prefix=IR
// RUN: %clang --target=avm-unknown-arduboyfx -O2 -fomit-frame-pointer \
// RUN:   -S %s -o - \
// RUN:   | FileCheck %s --check-prefix=ASM
// RUN: %clang --target=avm-unknown-arduboyfx -x c++ -std=c++17 -O2 \
// RUN:   -fomit-frame-pointer -S -emit-llvm %s -o - \
// RUN:   | FileCheck %s --check-prefix=IR
// RUN: %clang --target=avm-unknown-arduboyfx -x c++ -std=c++17 -O2 \
// RUN:   -fomit-frame-pointer -S %s -o - | FileCheck %s --check-prefix=ASM

typedef unsigned int size_t;

#ifdef __cplusplus
extern "C" {
#endif
void *memcpy(void *, const void *, size_t);
void *memset(void *, int, size_t);
void *memmove(void *, const void *, size_t);
void *__avm_memcpy(void *, const void *, unsigned int);
void *__avm_memset(void *, int, unsigned int);
void *__avm_memmove(void *, const void *, unsigned int);

void *ordinary_fill(void *dst, int value, size_t size) {
  return memset(dst, value, size);
}

void *builtin_fill(void *dst, int value, size_t size) {
  return __builtin_memset(dst, value, size);
}

void *target_fill(void *dst, int value, unsigned int size) {
  return __avm_memset(dst, value, size);
}

void *ordinary_move(void *dst, const void *src, size_t size) {
  return memmove(dst, src, size);
}

void *builtin_move(void *dst, const void *src, size_t size) {
  return __builtin_memmove(dst, src, size);
}

void *target_move(void *dst, const void *src, unsigned int size) {
  return __avm_memmove(dst, src, size);
}

void *target_copy(void *dst, const void *src, unsigned int size) {
  return __avm_memcpy(dst, src, size);
}

void small_fill(void *dst, int value) { __builtin_memset(dst, value, 2); }
void small_move(void *dst, const void *src) {
  __builtin_memmove(dst, src, 2);
}

void *(*memset_address)(void *, int, size_t) = &memset;
void *(*avm_memset_address)(void *, int, unsigned int) = &__avm_memset;
void *(*memmove_address)(void *, const void *, size_t) = &memmove;
void *(*avm_memmove_address)(void *, const void *, unsigned int) =
    &__avm_memmove;

#ifdef __cplusplus
}
#endif

// IR: @memset_address = {{.*}}global ptr addrspace(1) @memset
// IR: @avm_memset_address = {{.*}}global ptr addrspace(1) @__avm_memset
// IR: @memmove_address = {{.*}}global ptr addrspace(1) @memmove
// IR: @avm_memmove_address = {{.*}}global ptr addrspace(1) @__avm_memmove
// IR-LABEL: define{{.*}} ptr @ordinary_fill
// IR: call{{.*}} void @llvm.memset.p0.i16
// IR-LABEL: define{{.*}} ptr @builtin_fill
// IR: call{{.*}} void @llvm.memset.p0.i16
// IR-LABEL: define{{.*}} ptr @target_fill
// IR: call{{.*}} ptr @llvm.avm.memset
// IR-LABEL: define{{.*}} ptr @ordinary_move
// IR: call{{.*}} void @llvm.memmove.p0.p0.i16
// IR-LABEL: define{{.*}} ptr @builtin_move
// IR: call{{.*}} void @llvm.memmove.p0.p0.i16
// IR-LABEL: define{{.*}} ptr @target_move
// IR: call{{.*}} ptr @llvm.avm.memmove
// IR-LABEL: define{{.*}} ptr @target_copy
// IR: call{{.*}} ptr @llvm.avm.memcpy

// ASM-LABEL: ordinary_fill:
// ASM:       sys memset
// ASM-NEXT:  ret
// ASM-LABEL: builtin_fill:
// ASM:       sys memset
// ASM-NEXT:  ret
// ASM-LABEL: target_fill:
// ASM:       sys memset
// ASM-NEXT:  ret
// ASM-LABEL: ordinary_move:
// ASM:       sys memmove
// ASM-NEXT:  ret
// ASM-LABEL: builtin_move:
// ASM:       sys memmove
// ASM-NEXT:  ret
// ASM-LABEL: target_move:
// ASM:       sys memmove
// ASM-NEXT:  ret
// ASM-LABEL: target_copy:
// ASM:       sys memcpy
// ASM-NEXT:  ret
// ASM-LABEL: small_fill:
// ASM-NOT:   sys memset
// ASM:       st8
// ASM-LABEL: small_move:
// ASM-NOT:   sys memmove
// ASM:       ld{{8u|16}}
// ASM:       st{{8|16}}
