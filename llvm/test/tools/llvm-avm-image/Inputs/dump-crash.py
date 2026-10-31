import sys

# Validates the AVD1 crash block referenced by header offset 0x0F and prints
# its records for FileCheck. An image without metadata prints "crash: none".
image = open(sys.argv[1], "rb").read()


def u16(data, offset):
    return data[offset] | data[offset + 1] << 8


def u24(data, offset):
    return data[offset] | data[offset + 1] << 8 | data[offset + 2] << 16


# --development appends a raw erased save sector after the flat image.
tail = len(image) - 8
if image[tail:tail + 4] != b"AVT\x01":
    tail = len(image) - 4096 - 8
assert image[tail:tail + 4] == b"AVT\x01", "missing AVM tail"
crash = u24(image, 0x0F)
if crash == 0:
    print("crash: none")
    sys.exit(0)

assert crash % 4 == 0, "AVD1 block is not 4-byte aligned"
assert crash + 24 <= tail, "AVD1 header overlaps the tail"
block = image[crash:tail]
assert block[:4] == b"AVD1", "missing AVD1 magic"
size = u24(block, 4)
assert block[7] == 0, "nonzero AVD1 flags"
function_count = u16(block, 8)
line_count = u16(block, 10)
function_table = u24(block, 12)
line_table = u24(block, 15)
string_table = u24(block, 18)
assert u24(block, 21) == 0, "nonzero reserved AVD1 field"
assert function_table == 24, "function table is not directly after the header"
assert line_table == function_table + 9 * function_count, "line table misplaced"
assert string_table == line_table + 12 * line_count, "string table misplaced"
assert size >= string_table and size <= len(block), "AVD1 size out of range"
# Everything between the block and the tail is erased flash.
assert image[crash + size:tail] == b"\xff" * (tail - crash - size), \
    "bytes after AVD1 are not erased"

strings = block[string_table:size]
if strings:
    assert strings[-1] == 0, "string table is not NUL terminated"
    pieces = strings[:-1].split(b"\0")
    assert len(pieces) == len(set(pieces)), "string table has duplicates"
    assert all(c == 0 or 0x20 <= c < 0x7F for c in strings), "nonprintable string"


def string(offset):
    assert string_table <= offset < size, "string offset outside table"
    end = block.index(b"\0", offset, size)
    return block[offset:end].decode("ascii")


def check_ranges(name, starts, ends):
    previous_end = 0
    for start, end in zip(starts, ends):
        assert start < end, f"empty {name} range"
        assert end <= 0xFFFFFF, f"{name} range exceeds 24 bits"
        assert start >= previous_end, f"{name} ranges overlap or are unsorted"
        previous_end = end


functions = []
for i in range(function_count):
    record = function_table + 9 * i
    start = u24(block, record)
    end = u24(block, record + 3)
    name = string(u24(block, record + 6))
    assert len(name) <= 80, "function name exceeds 80 bytes"
    functions.append((start, end, name))
check_ranges("function", [f[0] for f in functions], [f[1] for f in functions])

lines = []
for i in range(line_count):
    record = line_table + 12 * i
    start = u24(block, record)
    end = u24(block, record + 3)
    filename = string(u24(block, record + 6))
    line = u24(block, record + 9)
    assert len(filename) <= 48, "source filename exceeds 48 bytes"
    assert "/" not in filename, "source filename is not a basename"
    lines.append((start, end, filename, line))
check_ranges("line", [l[0] for l in lines], [l[1] for l in lines])

print(f"crash: 0x{crash:x} size {size} functions {function_count} "
      f"lines {line_count}")
for start, end, name in functions:
    print(f"FN {start:#08x}-{end:#08x} {name}")
for start, end, filename, line in lines:
    print(f"LN {start:#08x}-{end:#08x} {filename}:{line}")
