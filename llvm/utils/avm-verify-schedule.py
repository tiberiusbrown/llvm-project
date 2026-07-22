#!/usr/bin/env python3
"""Compare fixed-cost AVM interpreter measurements with llvm-mca.

The benchmark output intentionally includes data-dependent instruction ranges.
Those rows are not part of this fixed-cost comparison.
"""

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys


FIXED_CASES = {
    "SYS debug_putc": "sys debug_putc",
    "ADD r4,r5 (upper one-byte)": "add r4, r5",
    "ADD r0,r1 (full F2)": "add r0, r1",
    "LD8U r4,[r6] (upper one-byte)": "ld8u r4, [r6]",
    "LD16 r0,[r6] (dense F5)": "ld16 r0, [r6]",
    "LD16 r0,[r1-5] (displaced ED)": "ld16 r0, [r1-5]",
    "LDP8U r0,[q3]": "ldp8u r0, [q3]",
    "BREQ8 (not taken)": "breq8 0",
}


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--bench-root",
        default=os.environ.get("AVM_BENCH_ROOT"),
        help="AVM checkout containing bench/cycles_instruction.txt",
    )
    parser.add_argument(
        "--llvm-mca",
        default=os.environ.get("LLVM_MCA") or shutil.which("llvm-mca"),
        help="Debug-built llvm-mca executable",
    )
    return parser.parse_args()


def read_measurements(path):
    measurements = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        match = re.match(r"^\s*(\d+)\s+(.*?)\s*$", line)
        if match:
            measurements[match.group(2)] = int(match.group(1))
    return measurements


def schedule_cycles(llvm_mca, instruction):
    command = [
        llvm_mca,
        "-mtriple=avm",
        "-mcpu=avm1",
        "-mattr=+v1",
        "-iterations=1",
    ]
    result = subprocess.run(
        command,
        input=".text\n" + instruction + "\n",
        text=True,
        capture_output=True,
        check=False,
    )
    if result.returncode:
        raise RuntimeError(result.stderr.strip() or result.stdout.strip())
    match = re.search(r"Block RThroughput:\s*([0-9.]+)", result.stdout)
    if not match:
        raise RuntimeError("llvm-mca did not report Block RThroughput")
    return float(match.group(1))


def main():
    args = parse_args()
    if not args.bench_root:
        raise SystemExit("set AVM_BENCH_ROOT or pass --bench-root")
    if not args.llvm_mca:
        raise SystemExit("pass --llvm-mca or put llvm-mca on PATH")

    measurements_path = Path(args.bench_root) / "bench" / "cycles_instruction.txt"
    measurements = read_measurements(measurements_path)
    failures = []
    for label, instruction in FIXED_CASES.items():
        if label not in measurements:
            failures.append(f"missing benchmark row: {label}")
            continue
        measured = measurements[label]
        modeled = schedule_cycles(args.llvm_mca, instruction)
        difference = abs(measured - modeled)
        print(f"{label}: measured={measured}, modeled={modeled:g}, delta={difference:g}")
        if difference > 2:
            failures.append(
                f"{label}: measured {measured}, modeled {modeled:g} (delta {difference:g})"
            )

    if failures:
        print("schedule validation failed:", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
