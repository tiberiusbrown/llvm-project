// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding \
// RUN:   -fomit-frame-pointer -O0 -S %s -o %t.O0.s
// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding \
// RUN:   -fomit-frame-pointer -O2 -S %s -o - | FileCheck %s --check-prefix=O2
// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding \
// RUN:   -fomit-frame-pointer -Os -S %s -o %t.Os.s
// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding \
// RUN:   -fomit-frame-pointer -Oz -S %s -o %t.Oz.s
// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding -O0 -c %s \
// RUN:   -o %t.O0.o
// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding -O2 -c %s \
// RUN:   -o %t.O2.o
// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding -Os -c %s \
// RUN:   -o %t.Os.o
// RUN: %clang --target=avm-unknown-arduboyfx -ffreestanding -Oz -c %s \
// RUN:   -o %t.Oz.o
// RUN: ld.lld -e add %t.O2.o -o %t.elf
// RUN: llvm-readobj --file-headers --sections %t.elf \
// RUN:   | FileCheck %s --check-prefix=ELF
// RUN: llvm-objdump -d %t.elf | FileCheck %s --check-prefix=LINKED

typedef unsigned char u8;
typedef unsigned int u16;
typedef unsigned long u32;
typedef const u8 __attribute__((address_space(1))) program_byte;

volatile u16 branch_sink;
u8 global_byte;
program_byte program_data[] = {1, 2, 3};

// O2-LABEL: add:
// O2:       add r4, r5
// O2-NEXT:  ret
u16 add(u16 left, u16 right) { return left + right; }

// O2-LABEL: increment:
// O2:       inc16 r4
// O2-NEXT:  ret
u16 increment(u16 value) { return value + 1; }

// O2-LABEL: load_byte:
// O2:       ld8u r4, [r4]
// O2-NEXT:  ret
u8 load_byte(const u8 *pointer) { return *pointer; }

// O2-LABEL: add32:
// O2:       add32 q2, q3
// O2-NEXT:  ret
u32 add32(u32 left, u32 right) { return left + right; }

// O2-LABEL: add_float:
// O2:       fadd
// O2-NEXT:  ret
float add_float(float left, float right) { return left + right; }

// O2-LABEL: compare_branch:
// O2:       cmp
// O2:       bruge
u16 compare_branch(u16 left, u16 right) {
  if (left < right)
    branch_sink = left;
  return right;
}

// O2-LABEL: scalar_select:
// O2:       cmp
// O2-NEXT:  cmov.ult
u16 scalar_select(u16 left, u16 right, u16 yes, u16 no) {
  return left < right ? yes : no;
}

__attribute__((noinline)) u16 callee(u16 value) { return value + 2; }

// O2-LABEL: direct_call:
// O2:       call callee
u16 direct_call(u16 value) { return callee(value) + 1; }

typedef u16 (*callback)(u16);

// O2-LABEL: indirect_call:
// O2-NOT:   zext8
// O2:       callp
u16 indirect_call(callback function, u16 value) {
  return function(value) + 1;
}

// The second i32 argument arrives in q3 and returns in q2. MOV32 expands into
// two compact upper-register MOV instructions.
// O2-LABEL: second32:
// O2-COUNT-2: mov r{{[45]}}, r{{[67]}}
// O2-NEXT:  ret
u32 second32(u32 first, u32 second) {
  (void)first;
  return second;
}

// O2-LABEL: load_global_byte:
// O2:       ldm8u r4, [global_byte]
u8 load_global_byte(void) { return global_byte; }

// O2-LABEL: postincrement_program:
// O2:       ldp8u {{.*}}+
// O2:       st16
// O2:       st8
u8 postincrement_program(program_byte *pointer, program_byte **updated) {
  u8 value = *pointer++;
  *updated = pointer;
  return value;
}

// ELF: Format: elf32-avm
// ELF: Arch: avm
// ELF: Name: .text
// LINKED-LABEL: <add>:
// LINKED: add r4, r5
// LINKED: ret
