# Writes assembly whose .loc rows fix exact source transitions, and the
# FileCheck directives the packed image must satisfy. Each row is a one-byte ret
# placed with .org, so its address is exact. A row with line 0 is an unmapped
# gap, and the intervals below are a hand model of the packer's rules.
#
# Usage: gen-line-rows.py CASE BASE ASM CHECKS [COUNT]
#   CASE    deltas | lines | gaps | files | limits | count
#   BASE    program address the .text section is linked at (decimal or 0x)
#   ASM     output assembly
#   CHECKS  output prefix. Writes CHECKS (records), CHECKS.probes (boundary
#           lookups), and CHECKS.summary (transition and checkpoint counts).
#   COUNT   for the count case, the number of source transitions to produce
import sys

FILES = {1: "a.c", 2: "b.h"}
CHECKPOINT_INTERVAL = 64


def rows_for(case, count):
    """Returns (offset, file, line) rows in address order."""
    if case == "deltas":
        # PC deltas 1, 16, 17, 127, 128, 129, and 1 again, with line steps of +1
        # and one -1. Deltas above 16 need the large same-file opcode.
        return [(0, 1, 10), (1, 1, 11), (17, 1, 12), (34, 1, 13), (161, 1, 14),
                (289, 1, 15), (418, 1, 16), (419, 1, 15)]
    if case == "lines":
        # Line deltas -5, -4, -1, +1, +3, +4, +1000, -1000, a file change at
        # equal line, +1, a drop to line 1, the maximum line, and a step back.
        return [(0, 1, 100), (2, 1, 95), (4, 1, 91), (6, 1, 90), (8, 1, 91),
                (10, 1, 94), (12, 1, 98), (14, 1, 1098), (16, 1, 98),
                (18, 2, 98), (20, 2, 99), (22, 2, 1), (24, 1, 0xffffff),
                (26, 1, 0xfffffe)]
    if case == "gaps":
        # Gaps at the start, between runs, and at the end, from line-0 rows.
        return [(3, 1, 4), (4, 1, 0), (6, 1, 6), (7, 1, 0), (8, 1, 0),
                (10, 2, 7), (11, 2, 0)]
    if case == "files":
        # Every file change is an absolute mapping. Equal neighbours coalesce.
        return [(0, 1, 1), (1, 2, 1), (2, 1, 1), (3, 2, 2), (4, 2, 2),
                (5, 1, 2), (6, 2, 3)]
    if case == "limits":
        return [(0, 1, 1), (2, 1, 0xffffff), (4, 1, 1)]
    if case == "count":
        # COUNT - 1 contiguous rows with distinct lines. The unmapped transition
        # after the last row makes COUNT transitions in all.
        return [(2 * i, 1, 10 + i) for i in range(count - 1)]
    raise SystemExit(f"unknown case {case}")


def intervals(rows, end):
    """Expected source intervals: each row runs to the next row, line 0 is
    skipped, and equal neighbours join."""
    out = []
    for index, (offset, file, line) in enumerate(rows):
        if line == 0:
            continue
        nxt = rows[index + 1][0] if index + 1 < len(rows) else end
        if nxt <= offset:
            continue
        if out and out[-1][1] == offset and out[-1][2] == file and out[-1][3] == line:
            out[-1][1] = nxt
        else:
            out.append([offset, nxt, file, line])
    return out


def transition_count(found):
    """One mapped transition per interval, plus one unmapped transition wherever
    an interval is followed by a gap or ends the sequence."""
    count = len(found)
    for index, (_, stop, _, _) in enumerate(found):
        if index + 1 == len(found) or found[index + 1][0] != stop:
            count += 1
    return count


def write_asm(path, rows, end):
    with open(path, "w") as asm:
        asm.write(".text\n.globl _start\n_start:\n")
        for number, name in FILES.items():
            asm.write(f'.file {number} "{name}"\n')
        for offset, file, line in rows:
            asm.write(f"  .org {offset:#x}\n")
            asm.write(f"  .loc {file} {line}\n")
            asm.write("  ret\n")
        asm.write(f"  .org {end:#x}\n")
        # .loc rows alone produce no compile unit, and the packer reads lines
        # through compile units. This is a minimal unit whose statement list is
        # the only line table in the object.
        asm.write(".section .debug_abbrev,\"\",@progbits\n")
        asm.write("  .byte 1, 0x11, 0\n  .byte 0x10, 0x17\n  .byte 0, 0\n  .byte 0\n")
        asm.write(".section .debug_info,\"\",@progbits\n")
        asm.write("  .long cu_end - cu_start\ncu_start:\n  .short 4\n  .long 0\n")
        asm.write("  .byte 3\n  .byte 1\n  .long 0\ncu_end:\n")


def write_checks(prefix, base, found, transitions):
    with open(prefix, "w") as out:
        out.write("CHECK-LABEL: crash:\n")
        for index, (start, stop, file, line) in enumerate(found):
            directive = "CHECK" if index == 0 else "CHECK-NEXT"
            out.write(f"{directive}: LN 0x{base + start:06x}-0x{base + stop:06x} "
                      f"{FILES[file]}:{line}\n")
        out.write("CHECK-NOT: LN\n")

    # The probes are the byte before each interval, its first and last byte, and
    # the byte after it. dump-crash.py --boundaries samples the same set.
    probes = set()
    for start, stop, _, _ in found:
        for pc in (base + start - 1, base + start, base + stop - 1, base + stop):
            if pc >= 0:
                probes.add(pc)
    with open(prefix + ".probes", "w") as out:
        for index, pc in enumerate(sorted(probes)):
            location = "?:0"
            for start, stop, file, line in found:
                if base + start <= pc < base + stop:
                    location = f"{FILES[file]}:{line}"
            directive = "CHECK" if index == 0 else "CHECK-NEXT"
            out.write(f"{directive}: LOOKUP 0x{pc:06x} function=? source={location}\n")

    with open(prefix + ".summary", "w") as out:
        out.write(f"CHECK: Source transitions: {transitions}{{{{$}}}}\n")
        out.write(f"CHECK: Checkpoints: {1 + transitions // CHECKPOINT_INTERVAL}{{{{$}}}}\n")


def main(argv):
    case, base_text, asm_path, prefix = argv[:4]
    count = int(argv[4]) if len(argv) > 4 else 0
    base = int(base_text, 0)
    rows = rows_for(case, count)
    end = rows[-1][0] + 1
    found = intervals(rows, end)
    write_asm(asm_path, rows, end)
    write_checks(prefix, base, found, transition_count(found))


if __name__ == "__main__":
    main(sys.argv[1:])
