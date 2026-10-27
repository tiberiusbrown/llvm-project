// RUN: %clang_cc1 -triple avm -std=c++17 -fno-use-cxa-atexit -emit-llvm -o - %s | FileCheck %s
// Global hooks identify their own fini stub with an unmodified code pointer.
// The destructor's hook result guards the actual C++ destructor, allowing an
// exit during initialization to skip objects that never completed construction.
// CHECK: call addrspace(1) void @__avm_note_global_ctor(ptr addrspace(1) @__dtor_{{.*}})
// CHECK: call addrspace(1) i16 @__avm_before_global_dtor(ptr addrspace(1) @__dtor_{{.*}})
// CHECK: icmp ne i16 {{.*}}, 0
// CHECK: br i1 {{.*}}, label %avm.call.global.dtor, label %avm.skip.global.dtor
// CHECK: avm.skip.global.dtor:
// CHECK: avm.call.global.dtor:
// CHECK: call addrspace(1) void @_ZN1GD1Ev
struct G { G(); ~G(); } global;
