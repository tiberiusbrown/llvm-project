import binascii
import struct
import sys

path, save_size, data_size = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
expected_pages = int(sys.argv[4]) if len(sys.argv) == 5 else None
image = open(path, "rb").read()
assert len(image) % 256 == 0
assert image[:4] == b"AVM\x01"
assert image[4] == 1
assert struct.unpack_from("<H", image, 8)[0] == data_size
assert struct.unpack_from("<H", image, 10)[0] == save_size
assert image[12:252] == bytes(240)
assert struct.unpack_from("<I", image, 252)[0] == binascii.crc32(image[:252])
changed = bytearray(image[:252])
changed[4] ^= 1
assert binascii.crc32(changed) != struct.unpack_from("<I", image, 252)[0]
assert image[-8:-4] == b"AVT\x01"
pages = struct.unpack_from("<H", image, len(image) - 4)[0]
assert pages == len(image) // 256
assert pages == (expected_pages if expected_pages is not None else pages)
assert image[-2:] == b"\0\0"
assert image[256:256 + save_size] == b"\x11"[:save_size]
assert image[256 + save_size:256 + data_size] == b"\0\x22"[:data_size-save_size]
if data_size:
    assert image[256 + data_size:512] == b"\xff" * (512 - 256 - data_size)
    entry = image[5] | image[6] << 8 | image[7] << 16
    assert image[entry:entry + 2] == b"\xaa\xbb"
