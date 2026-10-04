// RUN: %clang_cc1 -triple avm -emit-llvm -o - %s | FileCheck %s

typedef void (*callback)(void);
// CHECK-LABEL: define{{.*}} @dispatch
// CHECK: call addrspace(1) void %{{.*}}(){{.*}}, !avm.stack.dispatch ![[INIT:[0-9]+]]
// CHECK: call addrspace(1) void %{{.*}}(){{.*}}, !avm.stack.dispatch ![[FINI:[0-9]+]]
// CHECK: call addrspace(1) void %{{.*}}(){{.*}}, !avm.stack.dispatch ![[LOCAL:[0-9]+]]
// CHECK: icmp ne ptr addrspace(1)
// CHECK: call addrspace(1) void %{{.*}}(){{.*}}, !avm.stack.dispatch ![[WEAK:[0-9]+]]
void dispatch(callback f) {
  __builtin_avm_stack_dispatch(f, 0);
  __builtin_avm_stack_dispatch(f, 1);
  __builtin_avm_stack_dispatch(f, 2);
  __builtin_avm_stack_dispatch(f, 3);
}
// CHECK: ![[INIT]] = !{i32 16}
// CHECK: ![[FINI]] = !{i32 32}
// CHECK: ![[LOCAL]] = !{i32 64}
// CHECK: ![[WEAK]] = !{i32 128}
