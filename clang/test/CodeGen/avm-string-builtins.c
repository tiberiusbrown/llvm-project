// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -target-cpu avm1 \
// RUN:   -tune-cpu avm-interpreter-32u4-v1 -ffreestanding -O0 -emit-llvm \
// RUN:   -o - %s | FileCheck %s
// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -target-cpu avm1 \
// RUN:   -tune-cpu avm-interpreter-32u4-v1 -ffreestanding -O0 -emit-llvm \
// RUN:   -o - %s | FileCheck %s --check-prefix=ATTR

typedef unsigned int uint16_t;
typedef const void __attribute__((address_space(1))) *progmem_void_ptr;
typedef const char __attribute__((address_space(1))) *progmem_char_ptr;

int builtin_memcmp_p(const void *lhs, progmem_void_ptr rhs, uint16_t n) {
  return __builtin_avm_memcmp_p(lhs, rhs, n);
}
int avm_memcmp_P(const void *lhs, progmem_void_ptr rhs, uint16_t n) {
  return __avm_memcmp_P(lhs, rhs, n);
}
int public_memcmp_P(const void *lhs, progmem_void_ptr rhs, uint16_t n) {
  return memcmp_P(lhs, rhs, n);
}

int builtin_strcmp_p(const char *lhs, progmem_char_ptr rhs) {
  return __builtin_avm_strcmp_p(lhs, rhs);
}
int avm_strcmp_P(const char *lhs, progmem_char_ptr rhs) {
  return __avm_strcmp_P(lhs, rhs);
}
int public_strcmp_P(const char *lhs, progmem_char_ptr rhs) {
  return strcmp_P(lhs, rhs);
}

uint16_t builtin_strlen_p(progmem_char_ptr src) {
  return __builtin_avm_strlen_p(src);
}
uint16_t avm_strlen_P(progmem_char_ptr src) { return __avm_strlen_P(src); }
uint16_t public_strlen_P(progmem_char_ptr src) { return strlen_P(src); }

char *builtin_strncpy_p(char *dst, progmem_char_ptr src, uint16_t n) {
  return __builtin_avm_strncpy_p(dst, src, n);
}
char *avm_strncpy_P(char *dst, progmem_char_ptr src, uint16_t n) {
  return __avm_strncpy_P(dst, src, n);
}
char *public_strncpy_P(char *dst, progmem_char_ptr src, uint16_t n) {
  return strncpy_P(dst, src, n);
}

char *builtin_strncat_p(char *dst, progmem_char_ptr src, uint16_t n) {
  return __builtin_avm_strncat_p(dst, src, n);
}
char *avm_strncat_P(char *dst, progmem_char_ptr src, uint16_t n) {
  return __avm_strncat_P(dst, src, n);
}
char *public_strncat_P(char *dst, progmem_char_ptr src, uint16_t n) {
  return strncat_P(dst, src, n);
}

int ram_memcmp(const void *lhs, const void *rhs, uint16_t n) {
  return __avm_memcmp(lhs, rhs, n);
}
int ram_strcmp(const char *lhs, const char *rhs) {
  return __avm_strcmp(lhs, rhs);
}
uint16_t ram_strlen(const char *src) { return __avm_strlen(src); }
char *ram_strncpy(char *dst, const char *src, uint16_t n) {
  return __avm_strncpy(dst, src, n);
}
char *ram_strncat(char *dst, const char *src, uint16_t n) {
  return __avm_strncat(dst, src, n);
}

// CHECK-COUNT-3: call{{.*}} i16 @llvm.avm.memcmp.p(ptr {{.*}}, ptr addrspace(1) {{.*}}, i16 {{.*}})
// CHECK-COUNT-3: call{{.*}} i16 @llvm.avm.strcmp.p(ptr {{.*}}, ptr addrspace(1) {{.*}})
// CHECK-COUNT-3: call{{.*}} i16 @llvm.avm.strlen.p(ptr addrspace(1) {{.*}})
// CHECK-COUNT-3: call{{.*}} ptr @llvm.avm.strncpy.p(ptr {{.*}}, ptr addrspace(1) {{.*}}, i16 {{.*}})
// CHECK-COUNT-3: call{{.*}} ptr @llvm.avm.strncat.p(ptr {{.*}}, ptr addrspace(1) {{.*}}, i16 {{.*}})
// CHECK: call{{.*}} i16 @llvm.avm.memcmp(ptr {{.*}}, ptr {{.*}}, i16 {{.*}})
// CHECK: call{{.*}} i16 @llvm.avm.strcmp(ptr {{.*}}, ptr {{.*}})
// CHECK: call{{.*}} i16 @llvm.avm.strlen(ptr {{.*}})
// CHECK: [[CPY:%.*]] = call{{.*}} ptr @llvm.avm.strncpy(ptr {{.*}}, ptr {{.*}}, i16 {{.*}})
// CHECK: ret ptr [[CPY]]
// CHECK: [[CAT:%.*]] = call{{.*}} ptr @llvm.avm.strncat(ptr {{.*}}, ptr {{.*}}, i16 {{.*}})
// CHECK: ret ptr [[CAT]]
// CHECK-NOT: call{{.*}} @memcmp_P
// CHECK-NOT: call{{.*}} @strcmp_P
// CHECK-NOT: call{{.*}} @strlen_P
// CHECK-NOT: call{{.*}} @strncpy_P
// CHECK-NOT: call{{.*}} @strncat_P

// ATTR-DAG: declare i16 @llvm.avm.memcmp.p(ptr readonly captures(none), ptr addrspace(1) readonly captures(none), i16) addrspace(1) #[[RO:[0-9]+]]
// ATTR-DAG: declare i16 @llvm.avm.strcmp(ptr readonly captures(none), ptr readonly captures(none)) addrspace(1) #[[RO]]
// ATTR-DAG: declare i16 @llvm.avm.strlen.p(ptr addrspace(1) readonly captures(none)) addrspace(1) #[[RO]]
// ATTR-DAG: declare ptr @llvm.avm.strncpy.p(ptr noalias returned writeonly captures(none), ptr addrspace(1) noalias readonly captures(none), i16) addrspace(1) #[[RW:[0-9]+]]
// ATTR-DAG: declare ptr @llvm.avm.strncpy(ptr noalias returned writeonly captures(none), ptr noalias readonly captures(none), i16) addrspace(1) #[[RW]]
// ATTR-DAG: declare ptr @llvm.avm.strncat.p(ptr noalias returned captures(none), ptr addrspace(1) noalias readonly captures(none), i16) addrspace(1) #[[RW]]
// ATTR-DAG: declare ptr @llvm.avm.strncat(ptr noalias returned captures(none), ptr noalias readonly captures(none), i16) addrspace(1) #[[RW]]
// ATTR-DAG: attributes #[[RO]] = { nounwind willreturn memory(argmem: read) }
// ATTR-DAG: attributes #[[RW]] = { nounwind willreturn memory(argmem: readwrite) }
