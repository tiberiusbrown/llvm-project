// RUN: %clang --target=avm -O2 -flto=full -fno-rtti -fno-exceptions -ffreestanding -c %s -o %t.o
// RUN: ld.lld --avm-print-stack-usage --avm-print-stack-gaps %t.o -o %t.out 2>&1 | FileCheck %s --check-prefix=SAFE
// RUN: not ld.lld -e overflow %t.o -o %t.over 2>&1 | FileCheck %s --check-prefix=OVER
// RUN: %clang --target=avm -O2 -fno-rtti -fno-exceptions -ffreestanding -DAVM_NATIVE -c %s -o %t.native.o
// RUN: ld.lld -e mixed --avm-print-stack-usage --avm-print-stack-gaps %t.o %t.native.o -o %t.mixed 2>&1 | FileCheck %s --check-prefix=MIXED
// RUN: %clang --target=avm -O2 -flto=full -fno-whole-program-vtables -Xclang -fno-lto-unit -fno-rtti -fno-exceptions -ffreestanding -DAVM_NATIVE -c %s -o %t.untyped.o
// RUN: ld.lld -e mixed --avm-print-stack-usage --avm-print-stack-gaps %t.o %t.untyped.o -o %t.untyped 2>&1 | FileCheck %s --check-prefix=MIXED
// RUN: %clang --target=avm -O2 -flto=full -fvisibility=hidden -fno-rtti -fno-exceptions -ffreestanding -c %s -o %t.hidden.o
// RUN: ld.lld -e mixed --avm-print-stack-usage --avm-print-stack-gaps %t.hidden.o %t.native.o -o %t.hidden-native 2>&1 | FileCheck %s --check-prefix=MIXED
// RUN: ld.lld -e mixed --avm-print-stack-usage --avm-print-stack-gaps %t.hidden.o %t.untyped.o -o %t.hidden-untyped 2>&1 | FileCheck %s --check-prefix=MIXED
// RUN: ld.lld -e single --avm-print-stack-usage --avm-print-stack-gaps %t.o -o %t.single 2>&1 | FileCheck %s --check-prefix=SINGLE

// The dispatch remains virtual and must include both implementations. Separate
// entry points exercise a complete safe graph and an overflowing alternative.
// SAFE: complete bound: yes
// SAFE: Large::run(): frame
// SAFE-NOT: unresolved indirect
// OVER: error: AVM maximum provable stack usage is {{[0-9]+}} bytes; limit is 256 bytes
// OVER: Huge::run(): frame
// MIXED: complete bound: no
// MIXED: dispatch(Base*): unresolved indirect call target
// SINGLE: complete bound: yes
// SINGLE: single: frame
// SINGLE-NOT: unresolved indirect

#ifndef AVM_NATIVE
volatile unsigned char input;
volatile unsigned char output;
#else
extern volatile unsigned char output;
#endif

struct Base {
  virtual void run() = 0;
};
#ifdef AVM_NATIVE
struct Native : Base {
  void run() override { output = 3; }
};
static Native native;
static Base *volatile native_ptr = &native;
extern "C" Base *native_object() { return native_ptr; }
#else
struct Small : Base {
  __attribute__((noinline)) void run() override { output = 1; }
};
struct Large : Base {
  __attribute__((noinline)) void run() override {
    volatile unsigned char frame[100];
    frame[input % 100] = input;
    output = frame[0];
  }
};
struct Huge : Base {
  __attribute__((noinline)) void run() override {
    volatile unsigned char frame[240];
    frame[input % 240] = input;
    output = frame[0];
  }
};

__attribute__((noinline)) static void dispatch(Base *b) { b->run(); }
__attribute__((noinline)) static void member_dispatch(Base *b) {
  (b->*(&Base::run))();
}

extern "C" void _start() {
  Small s;
  Large l;
  dispatch(input ? static_cast<Base *>(&s) : &l);
  member_dispatch(input ? static_cast<Base *>(&s) : &l);
}

extern "C" void overflow() {
  Small s;
  Huge h;
  volatile unsigned char frame[40];
  frame[input % 40] = input;
  dispatch(input ? static_cast<Base *>(&s) : &h);
  output = frame[0];
}

extern "C" Base *native_object();
extern "C" void mixed() { dispatch(native_object()); }
extern "C" void single() {
  Small s;
  dispatch(&s);
}
#endif
