# Adding an AVM system service

A system service is described in one place after its public declarations are
in place:

1. Add its ID, assembly name, pseudo, intrinsic, and cost mapping to
   `AVMSystemCalls.inc`.
2. Add or update the LLVM intrinsic with the correct IR memory attributes.
3. Add the optional Clang builtin and builtin-to-intrinsic mapping.
4. Define a semantic service pseudo using general register classes.
5. Add one `AVMSystemServiceInfo` descriptor containing its logical arguments,
   required physical input and output registers, ties, pointer policies, and
   memory accesses.
6. Add tests.

No modification should be needed in `AVMSystemServiceRegions.cpp`,
`AVMExpandSystemServices.cpp`, `AVMRegisterInfo.cpp`,
`AVMISelDAGToDAG.cpp` service switches, or `AVMMCInstLower.cpp`.
