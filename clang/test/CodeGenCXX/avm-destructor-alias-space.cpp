// RUN: %clang_cc1 -triple avm -std=c++23 -O2 -mconstructor-aliases -emit-llvm-bc -o %t.bc %s
// RUN: llvm-dis %t.bc -o - | FileCheck %s

// A complete destructor that is an alias of its primary base's destructor lies in the
// aliasee's address space. AVM function addresses are program-space pointers, so the alias
// is in address space 1. The bitcode reader rejects an alias whose address space differs from
// its aliasee's, so the module must read back for LTO.

struct Shared {
  virtual ~Shared();
  virtual int value() const;
};

Shared::~Shared() = default;
int Shared::value() const { return 1; }

struct SharedDerived : Shared {
  ~SharedDerived() override;
  int value() const override;
};

SharedDerived::~SharedDerived() = default;
int SharedDerived::value() const { return 2; }

Shared &make_shared_derived() {
  static SharedDerived object;
  return object;
}

// CHECK-DAG: @_ZN6SharedD1Ev = unnamed_addr alias void (ptr), ptr addrspace(1) @_ZN6SharedD2Ev
// CHECK-DAG: @_ZN13SharedDerivedD2Ev = unnamed_addr alias void (ptr), ptr addrspace(1) @_ZN6SharedD2Ev
// CHECK-DAG: @_ZN13SharedDerivedD1Ev = unnamed_addr alias void (ptr), ptr addrspace(1) @_ZN13SharedDerivedD2Ev
