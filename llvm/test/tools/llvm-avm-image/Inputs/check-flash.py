import struct
import sys

# Usage: check-flash.py IMAGE DATA_SIZE ENTRY OFFSET FLASH_HEX
# The flash bytes must be stored at OFFSET exactly once in the flat image.
path, data_size, entry, offset = (sys.argv[1], int(sys.argv[2]),
                                  int(sys.argv[3], 0), int(sys.argv[4], 0))
flash = bytes.fromhex(sys.argv[5])
image = open(path, "rb").read()
assert len(image) % 256 == 0
assert image[:4] == b"AVM\x01"
assert struct.unpack_from("<H", image, 8)[0] == data_size, "DataSize changed"
assert struct.unpack_from("<H", image, 10)[0] == 0, "SaveSize changed"
assert image[5] | image[6] << 8 | image[7] << 16 == entry, "entry moved"
assert image[offset:offset + len(flash)] == flash, "flash bytes misplaced"
assert image.count(flash) == 1, "flash bytes are not stored exactly once"
assert image[-8:-4] == b"AVT\x01"
assert struct.unpack_from("<H", image, len(image) - 4)[0] == len(image) // 256
