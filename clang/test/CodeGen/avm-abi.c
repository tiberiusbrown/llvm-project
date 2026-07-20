// RUN: %clang_cc1 -triple avm -emit-llvm -o - %s | FileCheck %s

#define AS1 __attribute__((address_space(1)))

struct s0 {};
struct s1 { char x[1]; };
struct s2 { char x[2]; };
struct s3 { char x[3]; };
struct s4 { char x[4]; };
struct s5 { char x[5]; };
struct s6 { char x[6]; };
struct s7 { char x[7]; };
struct s8 { char x[8]; };

struct s0 r0(struct s0 x) { return x; }
struct s1 r1(struct s1 x) { return x; }
struct s2 r2(struct s2 x) { return x; }
struct s3 r3(struct s3 x) { return x; }
struct s4 r4(struct s4 x) { return x; }
struct s5 r5(struct s5 x) { return x; }
struct s6 r6(struct s6 x) { return x; }
struct s7 r7(struct s7 x) { return x; }
struct s8 r8(struct s8 x) { return x; }

signed char narrow_s(signed char x) { return x; }
unsigned char narrow_u(unsigned char x) { return x; }
_Bool narrow_b(_Bool x) { return x; }
long long pass_i64(long long x) { return x; }
const char AS1 *pass_program(const char AS1 *p) { return p; }
extern const char AS1 *external_program_identity(const char AS1 *p);
const char AS1 *computed_program(const char AS1 *base, unsigned short offset) {
  return external_program_identity(base + offset);
}
typedef void (*function_ptr)(void);
function_ptr pass_function(function_ptr p) { return p; }

unsigned short variadic(unsigned int named, ...) {
  __builtin_va_list ap;
  __builtin_va_start(ap, named);
  unsigned short a = __builtin_va_arg(ap, unsigned int);
  unsigned long b = __builtin_va_arg(ap, unsigned long);
  __builtin_va_list copy;
  __builtin_va_copy(copy, ap);
  __builtin_va_end(copy);
  __builtin_va_end(ap);
  return named + a + (unsigned short)b;
}

extern unsigned int consume_variadic(unsigned int named, ...);
unsigned int call_variadic(signed char s, unsigned char u, _Bool b) {
  return consume_variadic(1, s, u, b);
}

// CHECK: define{{.*}} void @r0()
// CHECK: define{{.*}} i8 @r1(i8 %{{.*}})
// CHECK: define{{.*}} i16 @r2(i16 %{{.*}})
// CHECK: define{{.*}} i24 @r3(i24 %{{.*}})
// CHECK: define{{.*}} i32 @r4(i32 %{{.*}})
// CHECK: define{{.*}} void @r5(ptr{{.*}}sret(%struct.s5) align 1 %{{.*}}, ptr noundef byval(%struct.s5) align 1 %{{.*}})
// CHECK: define{{.*}} void @r6(ptr{{.*}}sret(%struct.s6) align 1 %{{.*}}, ptr noundef byval(%struct.s6) align 1 %{{.*}})
// CHECK: define{{.*}} void @r7(ptr{{.*}}sret(%struct.s7) align 1 %{{.*}}, ptr noundef byval(%struct.s7) align 1 %{{.*}})
// CHECK: define{{.*}} void @r8(ptr{{.*}}sret(%struct.s8) align 1 %{{.*}}, ptr noundef byval(%struct.s8) align 1 %{{.*}})
// CHECK: define{{.*}} i8 @narrow_s(i8 noundef signext %x)
// CHECK: define{{.*}} i8 @narrow_u(i8 noundef zeroext %x)
// CHECK: define{{.*}} i1 @narrow_b(i1 noundef zeroext %x)
// CHECK: define{{.*}} i64 @pass_i64(i64 noundef %x)
// CHECK: define{{.*}} ptr addrspace(1) @pass_program(ptr addrspace(1) noundef %p)
// CHECK-LABEL: define{{.*}} ptr addrspace(1) @computed_program(ptr addrspace(1) noundef %base, i16 noundef zeroext %offset)
// CHECK: zext i16 {{.*}} to i24
// CHECK: getelementptr inbounds{{.*}} i8, ptr addrspace(1) {{.*}}, i24 {{.*}}
// CHECK: call addrspace(1) ptr addrspace(1) @external_program_identity(ptr addrspace(1) noundef {{.*}})
// CHECK: define{{.*}} ptr addrspace(1) @pass_function(ptr addrspace(1) noundef %p)
// CHECK: define{{.*}} i16 @variadic(i16 noundef %named, ...)
// CHECK-LABEL: define{{.*}} i16 @call_variadic(i8 noundef signext %s, i8 noundef zeroext %u, i1 noundef zeroext %b)
// CHECK: call addrspace(1) i16 (i16, ...) @consume_variadic(i16 noundef 1, i16 noundef {{.*}}, i16 noundef {{.*}}, i16 noundef {{.*}})
