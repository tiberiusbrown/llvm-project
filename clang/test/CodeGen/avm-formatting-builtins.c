// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -target-cpu avm1 \
// RUN:   -tune-cpu avm-interpreter-32u4-v1 -ffreestanding -O0 -emit-llvm \
// RUN:   -o - %s | FileCheck %s --check-prefix=IR
// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -target-cpu avm1 \
// RUN:   -tune-cpu avm-interpreter-32u4-v1 -ffreestanding -O2 -S \
// RUN:   -mllvm -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=ASM

typedef unsigned int size_t;
typedef const char __attribute__((address_space(1))) *progmem_char_ptr;

int builtin_vsnprintf(char *dst, size_t size, const char *format,
                      __builtin_va_list ap) {
  return __builtin_avm_vsnprintf(dst, size, format, ap);
}

int avm_vsnprintf(char *dst, size_t size, const char *format,
                  __builtin_va_list ap) {
  return __avm_vsnprintf(dst, size, format, ap);
}

int builtin_vsnprintf_p(char *dst, size_t size, progmem_char_ptr format,
                        __builtin_va_list ap) {
  return __builtin_avm_vsnprintf_p(dst, size, format, ap);
}

int avm_vsnprintf_P(char *dst, size_t size, progmem_char_ptr format,
                    __builtin_va_list ap) {
  return __avm_vsnprintf_P(dst, size, format, ap);
}

int public_vsnprintf_P(char *dst, size_t size, progmem_char_ptr format,
                       __builtin_va_list ap) {
  return vsnprintf_P(dst, size, format, ap);
}

// IR-LABEL: define{{.*}} i16 @builtin_vsnprintf
// IR: call i16 @llvm.avm.vsnprintf(ptr {{.*}}, i16 {{.*}}, ptr {{.*}}, ptr {{.*}})
// IR-LABEL: define{{.*}} i16 @avm_vsnprintf
// IR: call i16 @llvm.avm.vsnprintf(ptr {{.*}}, i16 {{.*}}, ptr {{.*}}, ptr {{.*}})
// IR-LABEL: define{{.*}} i16 @builtin_vsnprintf_p
// IR: call i16 @llvm.avm.vsnprintf.p(ptr {{.*}}, i16 {{.*}}, ptr addrspace(1) {{.*}}, ptr {{.*}})
// IR-LABEL: define{{.*}} i16 @avm_vsnprintf_P
// IR: call i16 @llvm.avm.vsnprintf.p(ptr {{.*}}, i16 {{.*}}, ptr addrspace(1) {{.*}}, ptr {{.*}})
// IR-LABEL: define{{.*}} i16 @public_vsnprintf_P
// IR: call i16 @llvm.avm.vsnprintf.p(ptr {{.*}}, i16 {{.*}}, ptr addrspace(1) {{.*}}, ptr {{.*}})

// IR-NOT: call{{.*}} @__avm_vsnprintf
// IR-NOT: call{{.*}} @__avm_vsnprintf_P
// IR-NOT: call{{.*}} @vsnprintf_P

// ASM-LABEL: builtin_vsnprintf:
// ASM: sys vsnprintf
// ASM-LABEL: avm_vsnprintf:
// ASM: sys vsnprintf
// ASM-LABEL: builtin_vsnprintf_p:
// ASM: sys vsnprintf_p
// ASM-LABEL: avm_vsnprintf_P:
// ASM: sys vsnprintf_p
// ASM-LABEL: public_vsnprintf_P:
// ASM: sys vsnprintf_p

// The standard RAM-space vsnprintf symbol remains a runtime veneer.  Its
// implementation calls __avm_vsnprintf so address-taking and indirect calls
// retain ordinary C function semantics.
