// RUN: %clang -### --target=avm -c %s 2>&1 | FileCheck %s --check-prefix=DEFAULT
// RUN: %clang -### --target=avm -mcpu=avm1 -c %s 2>&1 | FileCheck %s --check-prefix=MCpu
// RUN: %clang -### --target=avm -mtune=avm-interpreter-32u4-v1 -c %s 2>&1 | FileCheck %s --check-prefix=MTune
// RUN: not %clang --target=avm -mtune=unknown-avm-tune -c %s -o %t.o 2>&1 | FileCheck %s --check-prefix=INVALID

// DEFAULT: "-target-cpu" "avm1"
// DEFAULT: "-tune-cpu" "avm-interpreter-32u4-v1"
// MCpu: "-target-cpu" "avm1"
// MCpu: "-tune-cpu" "avm-interpreter-32u4-v1"
// MTune: "-target-cpu" "avm1"
// MTune: "-tune-cpu" "avm-interpreter-32u4-v1"
// INVALID: error: unknown target CPU 'unknown-avm-tune'
