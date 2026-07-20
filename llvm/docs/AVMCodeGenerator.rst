AVM Code Generator
==================

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
