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


# Operand counts per opcode. Same-file opcodes resume from the retained line, and
# the short forms carry one operand each.
def operand_count(opcode):
    if opcode == 0x80:
        return 3
    if opcode == 0x81:
        return 2
    if opcode == 0x82 or 0x83 <= opcode <= 0x92:
        return 1
    return 0


# Walks the transition stream and returns the offset of the first opcode equal
# to wanted.
def find_opcode(data, base, wanted):
    stream, _ = section(data, base, 3)
    position = stream + 3
    for _ in range(u24(data, stream)):
        opcode = data[position]
        if opcode == wanted:
            return position
        position += 1
        for _ in range(operand_count(opcode)):
            position += uleb_length(data, position)
    raise SystemExit(f"opcode 0x{wanted:02x} not found")


def section(data, base, index):
    entry = base + 12 + 7 * index
    return base + u24(data, entry + 1), u24(data, entry + 4)


# Returns the offsets of the pool records, which start at the length prefix.
def pool_records(data, base):
    pool, size = section(data, base, 4)
    records = []
    cursor = pool
    while cursor < pool + size:
        records.append(cursor)
        length = 0
        prefix = 0
        while True:
            byte = data[cursor + prefix]
            length |= (byte & 0x7F) << (7 * prefix)
            prefix += 1
            if not byte & 0x80:
                break
        cursor += prefix + length + 1
    return records


def main(argv):
    kind, source, target = argv
    data = bytearray(open(source, "rb").read())
    base = u24(data, 0x0F)
    stream, stream_size = section(data, base, 3)
    stream += 3
    stream_end = stream + stream_size - 3
    functions, _ = section(data, base, 0)
    checkpoints, _ = section(data, base, 2)
    pool, _ = section(data, base, 4)
    if kind == "magic":
        data[base + 3] = ord("X")
    elif kind == "flags":
        data[base + 8] = 1
    elif kind == "section-order":
        data[base + 12] = 2
    elif kind == "section-offset":
        set24(data, base + 12 + 7 + 1, u24(data, base + 12 + 7 + 1) + 1)
    elif kind == "initial-same-file":
        # The first transition is absolute. A combined opcode before any mapping
        # has no line to resume from.
        data[stream] = 0x00
    elif kind == "zero-pc-delta":
        # A large same-file transition whose one-byte PC delta is zero.
        position = find_opcode(data, base, 0x81)
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
    elif kind == "truncated-short-large":
        # The last opcode becomes a 0x83-0x92 form with no operand after it.
        data[stream_end - 1] = 0x83
    elif kind == "truncated-extended-unmapped":
        # The last opcode becomes the extended unmapped form with no operand.
        data[stream_end - 1] = 0x82
    elif kind == "line-underflow":
        # The first absolute mapping names line 0 instead of its line.
        position = find_opcode(data, base, 0x80) + 1
        position += uleb_length(data, position)
        position += uleb_length(data, position)
        data[position] = 0x00
    elif kind == "line-overflow":
        # A PC delta of 2 with a large line delta. Its ZigZag operand becomes
        # 0x1FFFFFE, so the line exceeds 0xFFFFFF.
        position = find_opcode(data, base, 0x84) + 1
        assert data[position] == 0xFC and data[position + 3] == 0x0F
        data[position] = 0xFE
    elif kind == "invalid-file":
        # The first set mapping names file 127 in an image with one filename. Its
        # file operand follows the ULEB128 PC delta.
        position = stream + 1
        while data[position] & 0x80:
            position += 1
        data[position + 1] = 0x7F
    elif kind == "checkpoint-state":
        # Checkpoint 1 sits in a gap. Its retained line is changed, so the state
        # disagrees with the sequential decode.
        set24(data, checkpoints + 2 + 12 + 8, 0x55)
    elif kind == "checkpoint-offset":
        # Checkpoint 1 names an opcode offset that is not a transition start.
        set24(data, checkpoints + 2 + 12 + 3, 1)
    elif kind == "checkpoint-count":
        data[checkpoints] = 0
    elif kind == "string-overlong":
        # The first two-byte length prefix ends in a zero byte, so its value is
        # not canonical.
        records = pool_records(data, base)
        record = next(r for r in records if data[r] & 0x80 and not data[r + 1] & 0x80)
        data[record + 1] = 0x00
    elif kind == "string-overflow":
        # The last record claims 127 characters. Its characters and terminator
        # lie past the end of the pool.
        record = pool_records(data, base)[-1]
        assert not data[record] & 0x80, "expected a one-byte length prefix"
        data[record] = 0x7F
    elif kind == "string-missing-nul":
        # The first record's terminator is replaced by a character.
        record = pool_records(data, base)[0]
        length = data[record]
        data[record + 1 + length] = ord("X")
    elif kind == "string-interior-nul":
        # The first character of the first record becomes a NUL byte.
        record = pool_records(data, base)[0]
        data[record + 1] = 0x00
    elif kind == "string-offset":
        # The first function name points one byte into its string.
        set24(data, functions + 2 + 6, u24(data, functions + 2 + 6) + 1)
    elif kind == "string-outside":
        # The first function name points past the end of the string pool.
        set24(data, functions + 2 + 6, section(data, base, 4)[1] + 5)
    elif kind == "truncated":
        del data[-64:]
    else:
        raise SystemExit(f"unknown mutation {kind}")
    with open(target, "wb") as out:
        out.write(data)


if __name__ == "__main__":
    main(sys.argv[1:])
