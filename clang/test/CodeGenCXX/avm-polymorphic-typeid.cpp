// RUN: %clang_cc1 -triple avm -std=c++23 -emit-llvm -o /dev/null -verify %s

// A typeid that needs run-time lookup through a vptr is not supported on AVM yet. It
// is rejected instead of naming the wrong descriptor. Static typeid of a polymorphic
// class still works.

namespace std {
class type_info {
public:
  virtual ~type_info();
};
}

struct Base {
  virtual ~Base() = default;
};

struct Derived : Base {};

const std::type_info &inspect(Base &value) {
  // expected-error@+1 {{cannot compile this polymorphic typeid on AVM yet}}
  return typeid(value);
}

const std::type_info &keep_base() { return typeid(Base); }
const std::type_info &keep_derived() { return typeid(Derived); }
