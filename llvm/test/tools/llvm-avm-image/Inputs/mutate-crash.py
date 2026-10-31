# Writes one corrupted copy of an AVD1 image, for the rejection tests. Each kind
# changes a single field the decoder must reject. Usage: mutate-crash.py KIND
# IMAGE OUT
import sys


def u24(data, offset):
    return data[offset] | data[offset + 1] << 8 | data[offset + 2] << 16


def set24(data, offset, value):
    data[offset] = value & 0xFF
    data[offset + 1] = (value >> 8) & 0xFF
    data[offset + 2] = (value >> 16) & 0xFF


def uleb_length(data, offset):
    length = 1
    while data[offset + length - 1] & 0x80:
        length += 1
    return length


# Walks the transition stream and returns the offset of the first opcode equal
# to wanted. Operand counts per opcode: combined 0, set mapping 3, large 2,
# unmapped 1.
def find_opcode(data, base, wanted):
    stream, _ = section(data, base, 3)
    position = stream + 3
    for _ in range(u24(data, stream)):
        opcode = data[position]
        if opcode == wanted:
            return position
        operands = {0x80: 3, 0x81: 2, 0x82: 1}.get(opcode, 0)
        position += 1
        for _ in range(operands):
            position += uleb_length(data, position)
    raise SystemExit(f"opcode 0x{wanted:02x} not found")


def section(data, base, index):
    entry = base + 12 + 7 * index
    return base + u24(data, entry + 1), u24(data, entry + 4)


def main(argv):
    kind, source, target = argv
    data = bytearray(open(source, "rb").read())
    base = u24(data, 0x0F)
    stream, _ = section(data, base, 3)
    stream += 3
    if kind == "magic":
        data[base + 3] = ord("X")
    elif kind == "flags":
        data[base + 8] = 1
    elif kind == "section-order":
        data[base + 12] = 2
    elif kind == "section-offset":
        set24(data, base + 12 + 7 + 1, u24(data, base + 12 + 7 + 1) + 1)
    elif kind == "unknown-opcode":
        data[stream] = 0x83
    elif kind == "zero-pc-delta":
        # An unmapped transition whose one-byte PC delta is zero.
        position = find_opcode(data, base, 0x82)
        assert data[position + 1] & 0x80 == 0, "expected a one-byte PC delta"
        data[position + 1] = 0x00
    elif kind == "overlong-uleb":
        # An unmapped transition whose PC delta is 0x80 0x00 instead of 0x00.
        data[stream] = 0x82
        data[stream + 1] = 0x80
        data[stream + 2] = 0x00
    elif kind == "long-uleb":
        # An operand with continuation bits on all four bytes has no end.
        data[stream] = 0x82
        data[stream + 1:stream + 5] = b"\x80\x80\x80\x80"
    elif kind == "invalid-file":
        # The first set mapping names file 127 in an image with one filename. Its
        # file operand follows the ULEB128 PC delta.
        position = stream + 1
        while data[position] & 0x80:
            position += 1
        data[position + 1] = 0x7F
    elif kind == "checkpoint-offset":
        # Checkpoint 1 names an opcode offset that is not a transition start.
        checkpoints, _ = section(data, base, 2)
        set24(data, checkpoints + 2 + 12 + 3, 1)
    elif kind == "checkpoint-count":
        checkpoints, _ = section(data, base, 2)
        data[checkpoints] = 0
    elif kind == "truncated":
        del data[-64:]
    else:
        raise SystemExit(f"unknown mutation {kind}")
    with open(target, "wb") as out:
        out.write(data)


if __name__ == "__main__":
    main(sys.argv[1:])
