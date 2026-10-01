// RUN: %clang -### --target=avm -c %s 2>&1 | FileCheck %s --check-prefix=DEFAULT
// RUN: %clang -### --target=avm -mcpu=avm1 -c %s 2>&1 | FileCheck %s --check-prefix=MCpu
// RUN: %clang -### --target=avm -mtune=avm-interpreter-32u4-v1 -c %s 2>&1 | FileCheck %s --check-prefix=MTune
// RUN: %clang -### --target=avm %s 2>&1 | FileCheck %s --check-prefix=LINK
// RUN: not %clang --target=avm -mtune=unknown-avm-tune -c %s -o %t.o 2>&1 | FileCheck %s --check-prefix=INVALID
// RUN: %clang -### --target=avm -g -c %s 2>&1 | FileCheck %s --check-prefix=DEBUG
// RUN: %clang -### --target=avm -g -gdwarf-5 -c %s 2>&1 | FileCheck %s --check-prefix=DWARF5

// DEFAULT: "-target-cpu" "avm1"
// DEFAULT: "-tune-cpu" "avm-interpreter-32u4-v1"
// MCpu: "-target-cpu" "avm1"
// MCpu: "-tune-cpu" "avm-interpreter-32u4-v1"
// MTune: "-target-cpu" "avm1"
// MTune: "-tune-cpu" "avm-interpreter-32u4-v1"
// LINK: avm-ld
// LINK: avm-image
// INVALID: error: unknown target CPU 'unknown-avm-tune'
// DEBUG: "-debug-info-kind=constructor" "-dwarf-version=4"
// DWARF5: "-debug-info-kind=constructor" "-dwarf-version=5"
