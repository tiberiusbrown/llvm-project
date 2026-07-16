====================
llvm-avm-image
====================

Synopsis
--------

``llvm-avm-image input.elf -o output.bin``

Description
-----------

``llvm-avm-image`` validates a final AVM ``ET_EXEC`` ELF file and packages it
as one Version 1 AVM flat image. It is a packer, not a linker: it neither
assigns section addresses nor resolves relocations, and it rejects inputs that
still need either operation.

The tool synthesizes the 256-byte runtime header from the linked ELF entry and
section layout. It writes the contiguous ``.saved`` then ``.data`` initializer
at offset ``0x100``, places program-space sections at their linked logical
addresses, fills unused image bytes with ``0xFF``, computes the header
CRC-32/ISO-HDLC, and appends the eight-byte tail.

The tail stores an unsigned 16-bit count of 256-byte pages. Consequently the
largest Version 1 output is ``0xFFFF00`` bytes (``0xFFFF`` pages); ELF payloads
that are valid in the architectural 24-bit program space but cannot fit this
container are diagnosed. Invalid ELF or AVM layouts produce a nonzero exit
status and no committed output file.

Unlike ``llvm-objcopy -O binary``, this tool understands AVM data and program
address spaces and creates the AVM runtime header, padding, CRC, and tail.

Options
-------

``-o <file>``, ``--output <file>``
  Write the packaged image to *file*.

``--help``
  Display available options.

``--version``
  Display the LLVM version.
