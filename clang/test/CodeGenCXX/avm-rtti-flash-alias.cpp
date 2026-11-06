// RUN: %clang_cc1 -triple avm -std=c++23 -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,PLAIN
// RUN: %clang_cc1 -triple avm -std=c++23 -fdata-sections -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,DATA

// With RTTI, each AVM descriptor and type name goes to its own .avm.rtti section, so
// the linker can keep it in the flash window. Static typeid names the AS0 alias of
// the descriptor, which is its program address plus 0x0600. A marker symbol names
// each typeid target so that the linker can check its window placement.

namespace std {
class type_info {
public:
  virtual ~type_info();
  const char *name() const;
};
}

struct Alpha {};
struct Gamma : Alpha {};
struct Poly {
  virtual int value() const { return 1; }
};

// The alias of a constant initializer is a constant expression, so the data
// reference can name the descriptor directly.
const std::type_info &global_alpha = typeid(Alpha);

// Each function returns the same alias that the initializer above does.
const std::type_info &call_gamma() { return typeid(Gamma); }

int check_poly() { Poly p; return typeid(p).name() != 0; }

// CHECK-DAG: ".weak __avm_rtti_alias._ZTI5Alpha"
// CHECK-DAG: ".weak __avm_rtti_alias._ZTS5Alpha"
// CHECK-DAG: ".weak __avm_rtti_alias._ZTI5Gamma"

// PLAIN:  @_ZTI5Alpha = linkonce_odr {{.*}}section ".avm.rtti.type", comdat, align 1
// PLAIN:  @_ZTS5Alpha = linkonce_odr {{.*}}section ".avm.rtti.name", comdat, align 1
// DATA:   @_ZTI5Alpha = linkonce_odr {{.*}}section ".avm.rtti.type._ZTI5Alpha", comdat, align 1
// DATA:   @_ZTS5Alpha = linkonce_odr {{.*}}section ".avm.rtti.name._ZTS5Alpha", comdat, align 1

// CHECK: @global_alpha = constant ptr inttoptr (i16 trunc (i32 add (i32 ptrtoint (ptr addrspace(1) @_ZTI5Alpha to i32), i32 1536) to i16) to ptr), align 1

// The vtable names its descriptor as a program-space pointer, with no address-space cast.
// CHECK: @_ZTV4Poly = {{.*}}{ [3 x ptr addrspace(1)] } { [3 x ptr addrspace(1)] [ptr addrspace(1) null, ptr addrspace(1) @_ZTI4Poly,
// CHECK: ret ptr inttoptr (i16 trunc (i32 add (i32 ptrtoint (ptr addrspace(1) @_ZTI5Gamma to i32), i32 1536) to i16) to ptr)
