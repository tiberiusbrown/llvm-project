// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -std=c++23 -O0 -emit-llvm -o - %s | FileCheck %s --check-prefix=UNOPT
// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -std=c++23 -O2 -emit-llvm -o - %s | FileCheck %s --check-prefix=OPT

namespace std {
template <class T> class initializer_list {
  const T *data;
  decltype(sizeof(0)) count;

public:
  const T *begin() const { return data; }
  const T *end() const { return data + count; }
};
}

int sum() {
  int total = 0;
  for (int value : std::initializer_list<int>{1, 2, 3, 4})
    total += value;
  return total;
}

// UNOPT-LABEL: define {{.*}} i16 @_Z3sumv(
// UNOPT: [[COUNT:%.*]] = getelementptr {{.*}} %"class.std::initializer_list", ptr {{%.*}}, i32 0, i32 1
// UNOPT-NEXT: store i16 4, ptr [[COUNT]], align 1
// OPT-LABEL: define {{.*}} i16 @_Z3sumv(
// OPT: ret i16 10
