import struct
import sys

# Copies a little-endian ELF32 file and gives its first .debug_line unit an
# unsupported DWARF version, so the line tables cannot be parsed.
source, destination = sys.argv[1], sys.argv[2]
data = bytearray(open(source, "rb").read())
shoff, = struct.unpack_from("<I", data, 0x20)
shentsize, shnum, shstrndx = struct.unpack_from("<HHH", data, 0x2E)


def section(index):
    base = shoff + index * shentsize
    return struct.unpack_from("<IIIIIIIIII", data, base)


strtab_offset = section(shstrndx)[4]
for index in range(shnum):
    fields = section(index)
    name_offset = strtab_offset + fields[0]
    name = data[name_offset:data.index(b"\0", name_offset)]
    if name == b".debug_line":
        offset = fields[4]
        struct.pack_into("<H", data, offset + 4, 0xFFFF)
        break
else:
    raise SystemExit("no .debug_line section")

open(destination, "wb").write(data)
