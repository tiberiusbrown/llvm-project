// RUN: %clang_cc1 -triple avm -emit-llvm -o - %s | FileCheck %s
// RUN: %clang --target=avm-unknown-arduboyfx -O2 -fomit-frame-pointer \
// RUN:   -S -emit-llvm %s -o - | FileCheck %s --check-prefix=DRIVER-IR
// RUN: %clang --target=avm-unknown-arduboyfx -O2 -fomit-frame-pointer \
// RUN:   -S %s -o - | FileCheck %s --check-prefix=DRIVER-ASM

#define AS1 __attribute__((address_space(1)))
typedef const char AS1 *prog_ptr;
typedef void (*function_ptr)(void);

const char *ordinary_pointer = "Hello";
const char ordinary_array[] = "Hello";
prog_ptr null_program = (prog_ptr)0;

char load_program(prog_ptr p) { return *p; }
const char *copy_data(const char *p) { return p; }
prog_ptr copy_program(prog_ptr p) { return p; }
unsigned long program_to_i32(prog_ptr p) { return (unsigned long)p; }
prog_ptr i32_to_program(unsigned long value) { return (prog_ptr)value; }
int equal_program(prog_ptr a, prog_ptr b) { return a == b; }
char indexed_program(prog_ptr base, long index) { return base[index]; }
extern void consume_program(prog_ptr);
prog_ptr computed_boundary(prog_ptr base, long index) {
  prog_ptr result = base + index;
  consume_program(result);
  return result;
}

struct pointer_record { char tag; prog_ptr pointer; };
prog_ptr program_array[2];

_Static_assert(sizeof(prog_ptr) == 3, "program pointer width");
_Static_assert(_Alignof(prog_ptr) == 1, "program pointer alignment");
_Static_assert(sizeof(struct pointer_record) == 4, "packed field");
_Static_assert(sizeof(program_array) == 6, "packed array");
_Static_assert(sizeof(function_ptr) == 3, "function pointer width");
_Static_assert(_Alignof(function_ptr) == 1, "function pointer alignment");

// CHECK: @.str = private unnamed_addr constant [6 x i8] c"Hello\00", align 1
// CHECK: @ordinary_pointer = global ptr @.str, align 1
// CHECK: @ordinary_array = constant [6 x i8] c"Hello\00", align 1
// CHECK: @null_program = global ptr addrspace(1) null, align 1
// CHECK: @program_array = global [2 x ptr addrspace(1)] zeroinitializer, align 1
// CHECK-LABEL: define{{.*}} i8 @load_program(ptr addrspace(1) noundef %p)
// CHECK: load i8, ptr addrspace(1)
// CHECK-LABEL: define{{.*}} ptr @copy_data(ptr noundef %p)
// CHECK-LABEL: define{{.*}} ptr addrspace(1) @copy_program(ptr addrspace(1) noundef %p)
// CHECK-LABEL: define{{.*}} i32 @program_to_i32(ptr addrspace(1) noundef %p)
// CHECK: ptrtoint ptr addrspace(1) {{.*}} to i24
// CHECK: zext i24 {{.*}} to i32
// CHECK-LABEL: define{{.*}} ptr addrspace(1) @i32_to_program(i32 noundef %value)
// CHECK: trunc i32 {{.*}} to i24
// CHECK: inttoptr i24 {{.*}} to ptr addrspace(1)
// CHECK-LABEL: define{{.*}} i16 @equal_program(ptr addrspace(1) noundef %a, ptr addrspace(1) noundef %b)
// CHECK: icmp eq ptr addrspace(1) {{.*}}, {{.*}}
// CHECK-NOT: addrspacecast

// DRIVER-IR: load i8, ptr addrspace(1)
// DRIVER-IR-NOT: addrspacecast
// DRIVER-ASM-LABEL: load_program:
// DRIVER-ASM:       ldp8u
// DRIVER-ASM-LABEL: program_to_i32:
// DRIVER-ASM:       and
// DRIVER-ASM-LABEL: equal_program:
// DRIVER-ASM-COUNT-2: zext8
// DRIVER-ASM:       cmp32
// DRIVER-ASM-LABEL: indexed_program:
// DRIVER-ASM:       add32
// DRIVER-ASM-NOT:   zext8
// DRIVER-ASM:       ldp8u
// DRIVER-ASM-LABEL: computed_boundary:
// DRIVER-ASM:       add32
// DRIVER-ASM:       zext8
// DRIVER-ASM:       call consume_program
