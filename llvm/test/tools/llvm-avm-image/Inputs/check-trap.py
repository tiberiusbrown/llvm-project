import binascii
import struct
import sys

path, expected = sys.argv[1], sys.argv[2]
image = open(path, "rb").read()
assert image[4] == 4
trap = image[12] | image[13] << 8 | image[14] << 16
if expected == "none":
    assert trap == 0, trap
else:
    assert trap == int(expected, 0), hex(trap)
assert image[0x12:252] == bytes(0xfc - 0x12)
assert struct.unpack_from("<I", image, 252)[0] == binascii.crc32(image[:252])
