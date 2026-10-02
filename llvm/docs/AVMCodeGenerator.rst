AVM Code Generator
==================

Runtime names and address spaces
---------------------------------

AVM's ordinary data pointers use address space 0; program-memory pointers use
address space 1. Clang keeps distinct internal builtins for these operations,
for example __avm_memcpy and __avm_memcpy_P, with
__builtin_avm_memcpy_p as another internal spelling. Public names such as
memcpy_P are ordinary runtime functions, not target builtin identifiers.
This lets the C runtime define the stable ELF symbol directly.

C uses the explicit program-memory suffix. C++ runtime headers overload the
ordinary spelling by pointer address space and bind the program-memory
overload to the existing _P ABI symbol. Direct header-dispatched calls
lower to the corresponding AVM intrinsic and system service. Address-taking
and parenthesized calls name the addressable runtime function.

Optional schedule validation
----------------------------

LLVM's tests do not require an external AVM checkout.  To compare the
serial-interpreter scheduling model with locally measured fixed-cost
instructions, set ``AVM_BENCH_ROOT`` to an AVM checkout, build and run its
generated benchmark corpus, and run the comparison script with the Debug-built
``llvm-mca``::

  cmake --build "$AVM_BENCH_ROOT/build" --parallel --target avm_bench_run
  python llvm/utils/avm-verify-schedule.py \
    --bench-root "$AVM_BENCH_ROOT" --llvm-mca /path/to/debug/llvm-mca

In PowerShell, the equivalent commands are::

  cmake --build "$env:AVM_BENCH_ROOT/build" --parallel --target avm_bench_run
  python llvm/utils/avm-verify-schedule.py `
    --bench-root "$env:AVM_BENCH_ROOT" --llvm-mca C:\path\to\Debug\llvm-mca.exe

For a multi-configuration build, add ``--config Debug`` to the build command.
The comparison fails when any fixed-cost measurement differs from the schedule
model by more than two cycles.  Data-dependent divide, floating-point, and
other range-cost measurements are deliberately excluded from that threshold.
