//===-- AVMCallingConv.td - AVM calling conventions ---------*- tablegen -*-===//

// AVM's no-split, close-on-first-stack-argument allocator is implemented in
// AVMISelLowering.cpp.  It cannot be expressed faithfully as a declarative
// sequence of independent TableGen assignments.
