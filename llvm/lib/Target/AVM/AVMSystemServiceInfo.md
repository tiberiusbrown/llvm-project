# Adding an AVM system service

This guide describes the LLVM/Clang work required to add an AVM `SYS`
service after the architectural ABI, interpreter implementation, and public C
declaration have been decided.

The generic service infrastructure is intended to keep service-specific
information in a small number of declarative locations. For a normal service
using the existing value kinds and registers, no service-specific logic should
be added to the register-allocation or post-RA expansion passes.

## Running example

The examples below add this fictional, deterministic service:

```c
uint16_t __avm_example_mix(uint16_t value, uint16_t key);
```

Assumed architectural behavior:

| Property | Example value |
|---|---|
| SYS ID | `0x22` |
| Assembly name | `example_mix` |
| LLVM intrinsic | `llvm.avm.example.mix` |
| Machine pseudo | `SYS_EXAMPLE_MIX_PSEUDO` |
| Input 0 | `value`, passed in `r4` |
| Input 1 | `key`, passed in `r5` |
| Result | returned in `r4` |
| Input/output tie | result replaces input 0 |
| Memory effects | none |
| Other side effects | none |
| Representative latency | 48 interpreter cycles |

The ID and cycle count are illustrative. Before using the example literally,
choose an actually unused service ID and measure or otherwise establish the
real cost.

## Before modifying LLVM

The LLVM work should begin only after these details are stable:

1. The interpreter recognizes the service ID and implements the operation.
2. The architectural calling convention specifies every input and output
   register.
3. The service's memory behavior is known precisely.
4. The public C/C++ declaration and types are settled.
5. The service's observable side effects are known.

For the example, the interpreter-side behavior is conceptually:

```text
input:  r4 = value
        r5 = key
output: r4 = mixed result
preserve all other architectural registers and flags
```

The LLVM descriptor assumes that the service itself preserves registers not
listed as outputs. The generic expansion pass handles any temporary moves or
live-through preservation required to place operands in the ABI registers.

---

## Step 1: register the service identity and cost

### Files

Modify:

```text
llvm/lib/Target/AVM/AVMSystemCalls.inc
llvm/lib/Target/AVM/AVMCycleCosts.def
```

### 1.1 Add the X-macro entry

Add one `AVM_SYS_DEF` entry to `AVMSystemCalls.inc`:

```cpp
AVM_SYS_DEF(0x22, example_mix, AVM_SYS_PSEUDO,
            SYS_EXAMPLE_MIX_PSEUDO, AVM_SYS_INTRINSIC,
            avm_example_mix, AVM_SYS_FIXED_COST, SysExampleMix)
```

The fields are:

```text
AVM_SYS_DEF(
    numeric SYS ID,
    assembly spelling,
    pseudo selector and pseudo name,
    intrinsic selector and intrinsic stem,
    cost selector and cost name)
```

For this example:

- `0x22` must match the interpreter dispatch table.
- `example_mix` is printed by the assembler/disassembler after `sys`.
- `SYS_EXAMPLE_MIX_PSEUDO` must match the TableGen pseudo added in Step 4.
- `avm_example_mix` must match the intrinsic stem added in Step 2.
- `SysExampleMix` must match the cost entry added below.

`AVMSystemCalls.inc` is the authoritative LLVM-side mapping between the
numeric ID, assembly name, machine pseudo, intrinsic, and optional cost. It
does not define types, register operands, memory effects, or the service
implementation.

### 1.2 Add the cycle-model entry

Because the example has a fixed measured cost, add this near the other system
services in `AVMCycleCosts.def`:

```cpp
AVM_FIXED_COST(SysExampleMix, 48)
```

Use the selector in `AVMSystemCalls.inc` that matches the cost declaration:

| Cost behavior | `AVMSystemCalls.inc` selector | `AVMCycleCosts.def` entry |
|---|---|---|
| Fixed | `AVM_SYS_FIXED_COST` | `AVM_FIXED_COST(Name, Cycles)` |
| Variable/range | `AVM_SYS_RANGE_COST` | `AVM_RANGE_COST(Name, Typical, Minimum, Maximum)` |
| No dedicated TTI cost | `AVM_NO_COST` | no entry |

For a service without a dedicated cost, write:

```cpp
AVM_SYS_DEF(0x22, example_mix, AVM_SYS_PSEUDO,
            SYS_EXAMPLE_MIX_PSEUDO, AVM_SYS_INTRINSIC,
            avm_example_mix, AVM_NO_COST, AVM_NO_COST)
```

Do not invent a cost merely to satisfy the table. Use `AVM_NO_COST` until a
meaningful cost is available.

### What this step enables

The X-macro entry is consumed by multiple layers:

- MC recognizes and prints the service's numeric ID and assembly name.
- CodeGen associates the intrinsic with the semantic pseudo.
- The post-RA path obtains the final SYS ID.
- TTI can find the dedicated cost when one is present.

A mismatch in any stem normally becomes a build failure or a descriptor
assertion rather than silently creating a second mapping.

---

## Step 2: define the LLVM intrinsic

### File

Modify:

```text
llvm/include/llvm/IR/IntrinsicsAVM.td
```

### Add the intrinsic declaration

Inside the existing:

```tablegen
let TargetPrefix = "avm" in {
  ...
}
```

block, add:

```tablegen
def int_avm_example_mix :
    Intrinsic<[llvm_i16_ty], [llvm_i16_ty, llvm_i16_ty],
              [IntrNoMem, IntrSpeculatable, IntrWillReturn,
               IntrNoCallback, IntrNoFree]>;
```

This declares:

```text
i16 @llvm.avm.example.mix(i16 value, i16 key)
```

The `int_avm_` TableGen name corresponds to the `avm_example_mix` stem used in
`AVMSystemCalls.inc`.

### Choose attributes from the service semantics

The attributes must describe the operation, not merely the current
interpreter implementation.

For the pure example:

- `IntrNoMem`: the service does not read or write memory.
- `IntrSpeculatable`: executing it early or on a path where its result is not
  used has no observable effect.
- `IntrWillReturn`: it returns normally.
- `IntrNoCallback`: it does not invoke arbitrary caller-visible callbacks.
- `IntrNoFree`: it does not free memory.

Do not use `IntrSpeculatable` for a timer, debug operation, display update,
random source, I/O operation, trap-like service, or anything whose number or
timing of executions is observable. Such services generally need
`IntrHasSideEffects`.

### Memory-using services

For a service that reads or writes through pointer arguments, express the
effects both here and later in the machine descriptor.

For example, a hypothetical read-only checksum:

```c
uint16_t __avm_checksum(void const *data, uint16_t size);
```

could use:

```tablegen
def int_avm_checksum :
    Intrinsic<[llvm_i16_ty], [llvm_ptr_ty, llvm_i16_ty],
              [IntrReadMem, IntrArgMemOnly, IntrWillReturn,
               IntrNoCallback, IntrNoFree,
               NoCapture<ArgIndex<0>>, ReadOnly<ArgIndex<0>>]>;
```

Important distinctions:

- `IntrNoMem` means no memory access at all.
- `IntrReadMem` means the intrinsic may read but not write.
- `IntrWriteMem` means it may write but not read.
- `IntrArgMemOnly` limits memory effects to memory reachable through pointer
  arguments.
- `ReadOnly<ArgIndex<N>>` and `WriteOnly<ArgIndex<N>>` describe a particular
  pointer argument.
- `Returned<ArgIndex<N>>` states that the return value is the same pointer as
  an input.
- `NoCapture<ArgIndex<N>>` states that the service does not retain the pointer.

Do not mark potentially overlapping source and destination pointers
`NoAlias`. For example, `memmove` must permit overlap.

---

## Step 3: optionally expose a Clang builtin

This step is required only when source code should call a Clang-recognized
builtin directly. A service reached only through LLVM IR, a library wrapper,
or another canonical intrinsic does not need a new builtin.

### Files

Modify:

```text
clang/include/clang/Basic/BuiltinsAVM.def
clang/lib/CodeGen/CGBuiltin.cpp
```

Also add or update the public declaration in the appropriate AVM sysroot
header outside `llvm-project`.

### 3.1 Declare the builtin

Add to `BuiltinsAVM.def`:

```cpp
BUILTIN(__avm_example_mix, "UiUiUi", "nc")
```

For the example, the builtin type string means:

```text
unsigned int (unsigned int, unsigned int)
```

On AVM, `unsigned int` is the 16-bit type used here. Verify the Clang builtin
type encoding against neighboring AVM builtins rather than guessing for
pointers, address spaces, qualifiers, or wider integer types.

The attribute string should match the operation. `"nc"` is appropriate for
the fictional pure, non-throwing example. A service with observable side
effects must not be declared const-like.

### 3.2 Map the builtin to the intrinsic

In `clang/lib/CodeGen/CGBuiltin.cpp`, find the:

```cpp
case llvm::Triple::avm:
```

target-builtin handling.

Add the builtin to the group that emits AVM intrinsics:

```cpp
case AVM::BI__avm_example_mix:
```

Then add its intrinsic mapping in the nested `switch (BuiltinID)`:

```cpp
case AVM::BI__avm_example_mix:
  ID = Intrinsic::avm_example_mix;
  break;
```

The generic argument-emission code can then emit:

```cpp
CGF->Builder.CreateCall(CGF->CGM.getIntrinsic(ID), Args)
```

No custom argument code is necessary for two ordinary scalar arguments.

### When custom Clang lowering is needed

Custom code in `CGBuiltin.cpp` is needed when the source-level operation does
more than pass its explicit arguments directly to one intrinsic, for example:

- converting a builtin into a generic LLVM memory intrinsic;
- adding a hidden framebuffer pointer;
- creating or referencing a special global;
- normalizing an address-space-qualified pointer;
- changing argument or result types;
- returning an input pointer after emitting a void IR operation.

Keep such source-language behavior in Clang. Do not encode it as an
architectural register convention.

### Public header

Declare the callable interface in the appropriate AVM SDK/sysroot header:

```c
uint16_t __avm_example_mix(uint16_t value, uint16_t key);
```

The header declaration must agree with the builtin type and intrinsic
signature. Test C and C++ compilation if the declaration is exposed to both.

---

## Step 4: define the semantic machine pseudo

### File

Modify:

```text
llvm/lib/Target/AVM/AVMInstrInfo.td
```

### Add a pseudo using general register classes

Add:

```tablegen
def SYS_EXAMPLE_MIX_PSEUDO
    : AVMSystemServicePseudo<(outs GPR16:$result),
                             (ins GPR16:$value, GPR16:$key)> {
  let Constraints = "$result = $value";
}
```

The output is tied to the first input because both occupy `r4` and the service
replaces the input value with the result.

### Use semantic classes, not fixed singleton classes

The pseudo must describe the logical value types:

- `GPR16` for ordinary 16-bit values and data pointers.
- `GPR32` for ordinary 32-bit values and `float`.
- `ProgPtrGPR32` for program-space pointers.

Do **not** write:

```tablegen
(outs R4Only:$result)
(ins R4Only:$value, R5Only:$key)
```

Singleton classes are short ABI carriers created by the generic service-region
pass. Using them directly on semantic operands makes long-lived values
unnecessarily fixed to one physical register and can make allocation
impossible.

### Add effects to the pseudo

The pseudo's machine effects must agree with the intrinsic and descriptor:

```tablegen
let mayLoad = 1;
let mayStore = 1;
let hasSideEffects = 1;
```

Set only the flags that apply.

Examples:

- Pure arithmetic-like service: no `mayLoad`, `mayStore`, or
  `hasSideEffects`.
- Read-only memory service: `mayLoad = 1`.
- Write-only memory service: `mayStore = 1`.
- Copy-like service: both `mayLoad = 1` and `mayStore = 1`.
- Debug/I/O/timer service: usually `hasSideEffects = 1`, even when it has no
  memory access.

Memory flags alone do not describe volatile I/O or otherwise observable
execution. Use `hasSideEffects` when the operation must not be deleted or
freely reordered.

### Ties

A TableGen constraint such as:

```tablegen
let Constraints = "$result = $value";
```

is appropriate when:

1. The result and input occupy the same architectural register.
2. The input value is consumed/replaced by the service.
3. The descriptor in Step 5 uses the same logical-input tie.

Do not tie an independent output merely because the ABI happens to use the
same register as some unrelated service.

---

## Step 5: add the `AVMSystemServiceInfo` descriptor

### Files

Normally modify only:

```text
llvm/lib/Target/AVM/AVMSystemServiceInfo.cpp
```

Consult, but usually do not modify:

```text
llvm/lib/Target/AVM/AVMSystemServiceInfo.h
```

The header already defines the standard value kinds, pointer policies, and
memory-access shapes.

### 5.1 Add the input array

Using the local aliases already present in the file:

```cpp
using VK = AVMServiceValueKind;
using PP = AVMServicePointerPolicy;
```

add:

```cpp
static constexpr AVMServiceInputInfo ExampleMixInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
    {1, VK::I16, AVM::R5, PP::None, true},
};
```

Each field is:

```text
{
  logical argument index,
  value kind,
  required architectural register,
  program-pointer policy,
  pass as an explicit machine operand
}
```

For the example:

- Logical argument 0 is `value`.
- Logical argument 1 is `key`.
- Both are ordinary 16-bit values.
- Neither has a program-pointer normalization policy.
- Both appear explicitly on the semantic pseudo.

### 5.2 Add the output array

Add:

```cpp
static constexpr AVMServiceOutputInfo ExampleMixOutputs[] = {
    {0, VK::I16, AVM::R4, 0},
};
```

The fields are:

```text
{
  result index,
  value kind,
  architectural result register,
  tied logical input index or -1
}
```

The output is result 0, is a 16-bit value, is produced in `r4`, and replaces
logical input 0.

For an independent result, use `-1`:

```cpp
{0, VK::I16, AVM::R4, -1}
```

### 5.3 Describe memory accesses

The example has no memory accesses, so no memory array is needed.

For a memory-using service, add one entry for each independently modeled read
or write. A descriptor entry specifies:

```text
base kind
logical base argument
optional fixed-global name
address space
load/store flags
size kind
logical size argument
constant size
```

For a hypothetical checksum reading `size` bytes from data argument 0:

```cpp
static constexpr AVMServiceMemoryAccessInfo ChecksumMemory[] = {
    {MB::LogicalArgument, 0, "", 0, MachineMemOperand::MOLoad,
     MS::LogicalArgument, 1, 0},
};
```

For a null-terminated read whose size is not known until the terminator:

```cpp
{MB::LogicalArgument, 0, "", 0, MachineMemOperand::MOLoad,
 MS::AfterPointer, 0, 0}
```

For a fixed 1024-byte global buffer:

```cpp
{MB::FixedGlobal, 0, "__avm_framebuffer", 0,
 MachineMemOperand::MOLoad, MS::Constant, 0, 1024}
```

Keep the descriptor consistent with the intrinsic:

- A descriptor load requires corresponding read effects in the intrinsic and
  pseudo.
- A descriptor store requires corresponding write effects.
- The address space must match the pointer's actual address space.
- The size argument index is a **logical argument index**, not a machine
  operand number.

### 5.4 Register the descriptor

Near the existing `SERVICE(...)` declarations, add:

```cpp
SERVICE(SYS_EXAMPLE_MIX_PSEUDO,
        ExampleMixInputs,
        ExampleMixOutputs,
        {});
```

The `SERVICE` macro obtains the numeric ID and assembly name from
`AVMSystemCalls.inc`; do not duplicate `0x22` or `"example_mix"` here.

### Pointer policies

`AVMServicePointerPolicy` applies to `ProgramPointer` inputs:

| Policy | Meaning |
|---|---|
| `None` | No special program-pointer handling |
| `IgnorePadding` | Unused high/padding bits do not affect the service |
| `RequireNormalized` | The program pointer must be canonicalized before use |

Use `VK::ProgramPointer` with a pair register such as `AVM::R6R7` when the ABI
expects `q3`. Do not model an ordinary data-space pointer as
`ProgramPointer`; data pointers are 16-bit `I16` values in the current ABI.

### Hidden arguments

A logical argument may exist for memory modeling without being passed as an
explicit machine operand. Set:

```cpp
PassToMachine = false
```

and use an empty physical register. Sprite services use this pattern for the
hidden framebuffer argument. The logical argument numbering must still match
the intrinsic-level argument list after any Clang-added hidden argument.

---


## Special case: services that read or write the framebuffer

The AVM framebuffer is a fixed 1024-byte data-space object named:

```text
__avm_framebuffer
```

A framebuffer service needs more than `mayLoad` or `mayStore`. The compiler
must also know which memory object is accessed, how much of it may be touched,
and whether the framebuffer pointer is a real machine operand or only an
IR-level alias-analysis aid.

There are two supported modeling patterns.

### Pattern A: implicit access to the fixed framebuffer global

Use this pattern when the service always accesses `__avm_framebuffer` and does
not need a framebuffer pointer as either a source-level argument or a machine
operand.

`SYS_DISPLAY_PSEUDO` is the existing model for this kind of service: the
source-level operation has no framebuffer argument, but the descriptor states
that the service reads the fixed global.

#### LLVM intrinsic

A display-like service can have no pointer argument:

```tablegen
def int_avm_present_framebuffer :
    Intrinsic<[], [],
              [IntrReadMem, IntrHasSideEffects, IntrWillReturn,
               IntrNoCallback, IntrNoFree]>;
```

Because there is no pointer argument, do not use `IntrArgMemOnly`. The
intrinsic-level attributes state only the broad memory behavior. The
machine-level descriptor supplies the precise fixed-global identity.

For a service that writes but does not read the framebuffer, use the
appropriate write-memory attributes. For a service that both reads and writes,
model both effects. Retain `IntrHasSideEffects` when invocation itself is
observable, such as presenting the display, synchronizing with hardware, or
performing I/O.

#### Semantic pseudo

The pseudo has no framebuffer operand:

```tablegen
def SYS_PRESENT_FRAMEBUFFER_PSEUDO
    : AVMSystemServicePseudo<(outs), (ins)> {
  let hasSideEffects = 1;
  let mayLoad = 1;
}
```

Set:

- `mayLoad = 1` when the service reads framebuffer bytes;
- `mayStore = 1` when it writes framebuffer bytes;
- both flags when it can do both;
- `hasSideEffects = 1` when executing the service is observably different
  from merely reading or writing ordinary memory.

#### Descriptor

Use `MB::FixedGlobal`:

```cpp
static constexpr AVMServiceMemoryAccessInfo PresentFramebufferMemory[] = {
    {MB::FixedGlobal, 0, "__avm_framebuffer", 0,
     MachineMemOperand::MOLoad,
     MS::Constant, 0, 1024},
};

SERVICE(SYS_PRESENT_FRAMEBUFFER_PSEUDO,
        {},
        {},
        PresentFramebufferMemory);
```

For a write-only service:

```cpp
static constexpr AVMServiceMemoryAccessInfo ClearFramebufferMemory[] = {
    {MB::FixedGlobal, 0, "__avm_framebuffer", 0,
     MachineMemOperand::MOStore,
     MS::Constant, 0, 1024},
};
```

For a read-modify-write service, add separate load and store records:

```cpp
static constexpr AVMServiceMemoryAccessInfo TransformFramebufferMemory[] = {
    {MB::FixedGlobal, 0, "__avm_framebuffer", 0,
     MachineMemOperand::MOLoad,
     MS::Constant, 0, 1024},
    {MB::FixedGlobal, 0, "__avm_framebuffer", 0,
     MachineMemOperand::MOStore,
     MS::Constant, 0, 1024},
};
```

Use the whole 1024-byte extent when the compiler cannot precisely describe
the subset touched by the service. An over-conservative whole-buffer access
may inhibit some reordering, but an understated access can cause incorrect
optimization.

### Pattern B: add a hidden framebuffer pointer to the intrinsic

Use this pattern when the source-level builtin should not expose a framebuffer
parameter, but LLVM IR should carry an explicit pointer to
`__avm_framebuffer`.

The sprite services use this design. Their source-level builtins have only the
visible sprite arguments, while the LLVM intrinsic includes an additional
data-space framebuffer pointer. Clang creates or references the framebuffer
global and appends that pointer to the intrinsic call.

This pattern gives IR-level alias analysis a concrete memory base while
keeping the architectural SYS ABI unchanged.

#### LLVM intrinsic

Add the framebuffer pointer as the final intrinsic argument. For example:

```tablegen
def int_avm_draw_example :
    Intrinsic<[],
              [llvm_i16_ty, llvm_i16_ty, llvm_avm_progptr_ty,
               llvm_i16_ty, llvm_ptr_ty],
              [IntrArgMemOnly, IntrHasSideEffects, IntrWillReturn,
               IntrNoCallback, IntrNoFree,
               NoCapture<ArgIndex<2>>,
               ReadOnly<ArgIndex<2>>,
               NoCapture<ArgIndex<4>>]>;
```

The argument order is conceptually:

```text
0: x
1: y
2: program-space sprite pointer
3: frame/index value
4: hidden data-space framebuffer pointer
```

For framebuffer access attributes:

- read-only framebuffer: add `ReadOnly<ArgIndex<4>>`;
- write-only framebuffer: add `WriteOnly<ArgIndex<4>>`;
- read/write framebuffer: do not mark argument 4 read-only or write-only;
- always add `NoCapture<ArgIndex<4>>` unless the service retains the pointer.

`IntrArgMemOnly` is appropriate when every memory access is through one of the
intrinsic's pointer arguments. If the service also accesses unrelated global
state or device memory, do not claim `IntrArgMemOnly`.

#### Clang builtin declaration

The source-level builtin does **not** include the hidden argument:

```cpp
BUILTIN(__avm_draw_example, "viivC*1Ui", "n")
```

Its type should describe only the arguments visible to the programmer.

#### Clang builtin lowering

In `clang/lib/CodeGen/CGBuiltin.cpp`, add the builtin-to-intrinsic mapping as
usual, then append the hidden framebuffer pointer before creating the
intrinsic call.

Follow the existing sprite-service logic:

```cpp
constexpr llvm::StringLiteral FramebufferName("__avm_framebuffer");
llvm::Module &M = CGF->CGM.getModule();
llvm::ArrayType *FramebufferTy =
    llvm::ArrayType::get(CGF->Int8Ty, 1024);

GlobalVariable *Framebuffer =
    M.getGlobalVariable(FramebufferName, /*AllowInternal=*/true);

if (!Framebuffer)
  Framebuffer = new GlobalVariable(
      M, FramebufferTy,
      /*isConstant=*/false,
      GlobalValue::ExternalLinkage,
      /*Initializer=*/nullptr,
      FramebufferName,
      /*InsertBefore=*/nullptr,
      GlobalVariable::NotThreadLocal,
      /*AddressSpace=*/0);

Framebuffer->setAlignment(Align(1));

if (!StringRef(M.getModuleInlineAsm())
         .contains(".globl __avm_framebuffer"))
  M.appendModuleInlineAsm(".globl __avm_framebuffer");

Args.push_back(
    CGF->Builder.CreateConstInBoundsGEP2_32(
        FramebufferTy, Framebuffer, 0, 0));
```

Then emit the intrinsic call with the augmented `Args` list.

This does four things:

1. Declares the framebuffer as an external 1024-byte data-space object.
2. Gives it byte alignment.
3. Ensures the symbol is externally visible to the AVM link/runtime setup.
4. Passes an `i8*`-like pointer to element zero as the hidden final argument.

If several framebuffer builtins share this code, add the new builtin to the
existing sprite/framebuffer case group rather than duplicating the global
creation logic.

#### Semantic pseudo

Do not add the hidden framebuffer pointer to the machine pseudo:

```tablegen
class AVMDrawExampleServicePseudo
    : AVMSystemServicePseudo<
          (outs),
          (ins GPR16:$x, GPR16:$y,
               ProgPtrGPR32:$sprite, GPR16:$frame)> {
  let hasSideEffects = 1;
  let mayLoad = 1;
  let mayStore = 1;
}
```

The architectural SYS operation does not consume a framebuffer register. It
implicitly knows the framebuffer location.

#### Descriptor input

The descriptor must still include the hidden logical argument so memory
access records can refer to it:

```cpp
static constexpr AVMServiceInputInfo DrawExampleInputs[] = {
    {0, VK::I16, AVM::R4, PP::None, true},
    {1, VK::I16, AVM::R5, PP::None, true},
    {2, VK::ProgramPointer, AVM::R6R7, PP::IgnorePadding, true},
    {3, VK::I16, AVM::R0, PP::None, true},

    // Hidden IR-level framebuffer argument. It participates in memory
    // modeling but is not passed to the architectural SYS instruction.
    {4, VK::I16, MCPhysReg(), PP::None, false},
};
```

The important fields of the final entry are:

```text
LogicalArgumentIndex = 4
PhysReg              = no register
PassToMachine        = false
```

`VK::I16` is used because an ordinary AVM data-space pointer is represented as
a 16-bit value at the machine level. It is not a program pointer and should
not use `VK::ProgramPointer`.

Because `PassToMachine` is false:

- the semantic pseudo has no corresponding explicit operand;
- the service-region pass creates no fixed register carrier for it;
- the post-RA expander does not try to pass it to the interpreter;
- the memory descriptor can still identify the IR-level pointer argument.

#### Descriptor memory records

For a service that may read and write the framebuffer:

```cpp
static constexpr AVMServiceMemoryAccessInfo DrawExampleMemory[] = {
    {MB::LogicalArgument, 4, "__avm_framebuffer", 0,
     MachineMemOperand::MOLoad,
     MS::Constant, 0, 1024},

    {MB::LogicalArgument, 4, "__avm_framebuffer", 0,
     MachineMemOperand::MOStore,
     MS::Constant, 0, 1024},

    // Other accesses, such as reading the program-space sprite.
    {MB::LogicalArgument, 2, "", 1,
     MachineMemOperand::MOLoad,
     MS::AfterPointer, 0, 0},
};

SERVICE(SYS_DRAW_EXAMPLE_PSEUDO,
        DrawExampleInputs,
        {},
        DrawExampleMemory);
```

Use logical argument 4 as the base of the framebuffer accesses. The
`"__avm_framebuffer"` name records the known fixed object associated with that
hidden argument.

For read-only or write-only operations, include only the applicable memory
record and make the intrinsic and pseudo flags agree.

### Choosing between the two patterns

Use `MB::FixedGlobal` without an intrinsic pointer when:

- the operation has no source-level framebuffer argument;
- IR does not need a concrete pointer operand for the operation;
- broad intrinsic memory effects are sufficient;
- the machine descriptor can model the fixed object precisely.

Use a hidden intrinsic pointer plus `MB::LogicalArgument` when:

- the source builtin should remain convenient and omit the framebuffer;
- IR-level aliasing and memory dependence should be tied to an actual pointer;
- the service has other explicit pointer arguments;
- several calls should visibly refer to the same framebuffer object in IR.

Do not add a physical framebuffer register merely to improve memory modeling.
The hidden pointer is an IR/compiler construct, not part of the architectural
SYS ABI.

### Framebuffer-specific tests

Add checks at several levels.

#### Clang IR test

Verify that the source builtin gains the hidden final argument:

```c
void test_draw(int x, int y, __attribute__((address_space(1))) const void *p,
               unsigned frame) {
  __avm_draw_example(x, y, p, frame);
}

// CHECK: @__avm_framebuffer = external global [1024 x i8], align 1
// CHECK: call void @llvm.avm.draw.example(
// CHECK-SAME: i16 {{.*}}, i16 {{.*}}, ptr addrspace(1) {{.*}}, i16 {{.*}},
// CHECK-SAME: ptr @__avm_framebuffer
```

Match the exact pointer/GEP spelling produced by the current LLVM version
rather than assuming opaque-pointer formatting.

For Pattern A, verify that the intrinsic call has no hidden pointer argument.

#### Intrinsic and MIR test

Verify all of the following:

- the intrinsic has the correct read/write attributes;
- the semantic pseudo contains only architectural operands;
- the hidden logical input is not emitted as a pseudo operand;
- the resulting machine instruction has a 1024-byte framebuffer load/store
  memory operand;
- a read/write service receives both load and store memory operands.

#### Optimization/aliasing test

Add a test with ordinary loads or stores to `__avm_framebuffer` around the
service. Verify that LLVM does not incorrectly move conflicting accesses
across the service.

Useful cases include:

```text
store framebuffer byte
call framebuffer-reading service
```

and:

```text
call framebuffer-writing service
load framebuffer byte
```

A service modeled as touching the whole buffer should conflict with accesses
to any byte of the buffer.

#### Runtime test

Initialize the framebuffer with a known pattern, call the service, and verify:

- bytes that should change;
- bytes that should remain unchanged;
- source program data remains intact;
- repeated calls behave correctly;
- any display or I/O side effect occurs exactly once.

For a display-only reader, verify that the framebuffer itself remains
unchanged.

### Framebuffer consistency checklist

For every framebuffer service, verify:

- [ ] The object is named exactly `__avm_framebuffer`.
- [ ] The framebuffer is modeled in data address space 0.
- [ ] The modeled extent is 1024 bytes unless a smaller precise extent is
      valid and representable.
- [ ] Read services have intrinsic read effects, pseudo `mayLoad`, and an
      `MOLoad` descriptor record.
- [ ] Write services have intrinsic write effects, pseudo `mayStore`, and an
      `MOStore` descriptor record.
- [ ] Read/write services have both descriptor records.
- [ ] Observable display or I/O behavior uses `IntrHasSideEffects` and
      `hasSideEffects`.
- [ ] Pattern A uses `MB::FixedGlobal`.
- [ ] Pattern B appends a hidden intrinsic argument in Clang.
- [ ] The hidden logical argument has no physical register and
      `PassToMachine = false`.
- [ ] The hidden pointer is not present on the semantic machine pseudo.
- [ ] The intrinsic argument index, descriptor logical index, and Clang
      appended-argument position agree.
- [ ] Tests verify aliasing against ordinary framebuffer loads and stores.

---

## Step 6: add tests at every layer

A new service should not be considered complete with only an end-to-end C
test. Add focused tests that isolate each mapping.

### 6.1 Clang builtin-to-intrinsic test

Modify an existing AVM builtin CodeGen test, or add:

```text
clang/test/CodeGen/AVM/system-service-builtins.c
```

Example:

```c
// RUN: %clang_cc1 -triple avm-unknown-arduboyfx -emit-llvm -o - %s \
// RUN:   | FileCheck %s

unsigned int test_example_mix(unsigned int a, unsigned int b) {
  return __avm_example_mix(a, b);
}

// CHECK-LABEL: define{{.*}} i16 @test_example_mix
// CHECK: call i16 @llvm.avm.example.mix(i16 {{.*}}, i16 {{.*}})
```

Also check the intrinsic declaration/attributes when that is important:

```text
CHECK: declare i16 @llvm.avm.example.mix(i16, i16)
```

For memory services, test the emitted pointer address spaces and attributes.

### 6.2 Intrinsic-to-semantic-pseudo test

Modify an existing AVM service-lowering test, or add:

```text
llvm/test/CodeGen/AVM/system-service-intrinsics.ll
```

Example:

```llvm
; RUN: llc -mtriple=avm-unknown-arduboyfx -stop-after=finalize-isel %s -o - \
; RUN:   | FileCheck %s

declare i16 @llvm.avm.example.mix(i16, i16)

define i16 @example_mix(i16 %a, i16 %b) {
entry:
  %r = call i16 @llvm.avm.example.mix(i16 %a, i16 %b)
  ret i16 %r
}

; CHECK-LABEL: name: example_mix
; CHECK: {{%[0-9]+}}:gpr16 =
; CHECK-SAME: SYS_EXAMPLE_MIX_PSEUDO
```

This verifies that the X-macro mapping selects the pseudo without adding a
service-specific selector switch.

### 6.3 Fixed-carrier/region test

Modify:

```text
llvm/test/CodeGen/AVM/system-service-regions.mir
```

Add a function showing that the semantic general operands become short fixed
carriers:

```yaml
---
name: example_mix_carriers
tracksRegLiveness: true
body: |
  bb.0:
    %0:gpr16 = IMPLICIT_DEF
    %1:gpr16 = IMPLICIT_DEF
    %2:gpr16 = SYS_EXAMPLE_MIX_PSEUDO %0, %1
    RET_PSEUDO implicit-def dead $sp, implicit $sp

# CHECK-LABEL: name: example_mix_carriers
# CHECK: [[R4IN:%[0-9]+]]:r4only = COPY %0
# CHECK: [[R5IN:%[0-9]+]]:r5only = COPY %1
# CHECK: [[R4OUT:%[0-9]+]]:r4only =
# CHECK-SAME: SYS_EXAMPLE_MIX_PSEUDO [[R4IN]], [[R5IN]]
# CHECK: %2:gpr16 = nomerge COPY [[R4OUT]]
...
```

Exact checks should follow the current pass output. The important properties
are:

- semantic inputs begin in general classes;
- the ABI carriers use `r4only` and `r5only`;
- the result is produced in `r4only`;
- the escaping result returns to a general class;
- the tied input and output agree with the descriptor.

### 6.4 Post-RA expansion test

Modify the existing AVM service-expansion MIR test, or add:

```text
llvm/test/CodeGen/AVM/system-service-expansion.mir
```

Verify that a physically allocated semantic service becomes the expected
generic SYS operation with the correct service ID and physical operands.

Also include a live-through case when useful:

```text
r4 contains a live unrelated value
example_mix needs r4/r5
the expander preserves the live value
the final SYS ID is 0x22
```

This catches descriptor/register mistakes and parallel-copy problems.

### 6.5 MC assembler/disassembler test

Modify an existing AVM SYS MC test, or add:

```text
llvm/test/MC/AVM/system-services.s
```

Test both spelling and encoding:

```asm
# RUN: llvm-mc -triple=avm-unknown-arduboyfx -show-encoding %s \
# RUN:   | FileCheck %s

sys example_mix

# CHECK: sys example_mix
# CHECK-SAME: encoding: [0xd7,0x22]
```

Also disassemble the bytes and verify that `llvm-objdump` prints
`sys example_mix`.

### 6.6 End-to-end runtime test

In the AVM runtime/test repository, add a small C test that:

1. Calls the public function with known inputs.
2. Checks the returned value.
3. Places unrelated live values around the call.
4. Uses the result after other operations.
5. Includes edge values such as `0`, `0xffff`, and values that exercise carries.

This test validates the interpreter implementation and ABI, while the LLVM
tests above localize compiler failures.

### 6.7 Cost-model test

When the service has a dedicated TTI cost, add or update an AVM cost-model test
to verify the expected relative or normalized cost. Do not rely only on the
presence of the enum: a mapping can compile while returning an unintended
fallback cost.

---

## Step 7: build and run focused validation

From the LLVM build directory, rebuild the affected tools:

```bash
ninja clang llc llvm-mc llvm-objdump FileCheck
```

Run focused tests first:

```bash
python ../llvm/utils/lit/lit.py -sv \
  ../clang/test/CodeGen/AVM \
  ../llvm/test/CodeGen/AVM \
  ../llvm/test/MC/AVM
```

Then rebuild the AVM project against the updated compiler and run its complete
runtime and benchmark suites.

Inspect generated MIR or assembly when debugging:

```bash
llc -mtriple=avm-unknown-arduboyfx \
  -stop-after=finalize-isel input.ll -o -

llc -mtriple=avm-unknown-arduboyfx \
  -stop-after=avm-system-service-regions input.ll -o -

llc -mtriple=avm-unknown-arduboyfx \
  -stop-after=greedy input.ll -o -

llc -mtriple=avm-unknown-arduboyfx input.ll -o -
```

The expected progression for the example is:

```text
LLVM intrinsic call
    ↓
SYS_EXAMPLE_MIX_PSEUDO with general operands
    ↓
short r4/r5 fixed carriers around the pseudo
    ↓
physical post-RA service operation
    ↓
d7 22 / sys example_mix
```

---

## Files normally changed

For the complete example with a Clang builtin and dedicated cost:

```text
llvm/lib/Target/AVM/AVMSystemCalls.inc
llvm/lib/Target/AVM/AVMCycleCosts.def
llvm/include/llvm/IR/IntrinsicsAVM.td
clang/include/clang/Basic/BuiltinsAVM.def
clang/lib/CodeGen/CGBuiltin.cpp
llvm/lib/Target/AVM/AVMInstrInfo.td
llvm/lib/Target/AVM/AVMSystemServiceInfo.cpp
relevant Clang, CodeGen, MIR, MC, and runtime test files
```

A service without a Clang builtin omits the two Clang files. A service without
a dedicated cost omits `AVMCycleCosts.def` and uses `AVM_NO_COST`.

## Files normally not changed

For a service expressible using the existing value kinds, physical registers,
pointer policies, and memory-access forms, no service-specific modification
should be needed in:

```text
llvm/lib/Target/AVM/AVMSystemServiceRegions.cpp
llvm/lib/Target/AVM/AVMExpandSystemServices.cpp
llvm/lib/Target/AVM/AVMRegisterInfo.cpp
llvm/lib/Target/AVM/AVMISelDAGToDAG.cpp
llvm/lib/Target/AVM/AVMMCInstLower.cpp
llvm/lib/Target/AVM/AVMSystemServiceInfo.h
```

These components consume the generic pseudo and descriptor.

Needing to add a service name to a switch in one of these files is usually a
sign that information is missing from `AVMSystemCalls.inc` or
`AVMSystemServiceInfo.cpp`.

An exception is a genuinely new generic capability, such as:

- a new service value width or register representation;
- a new pointer-normalization policy;
- a new kind of memory extent;
- a source-language transformation that cannot be represented as a direct
  intrinsic call;
- a new cost category shared by multiple operations.

In that case, extend the generic abstraction and add tests for the abstraction
itself rather than inserting a one-off service check.

---

## Consistency checklist

Before considering the service complete, verify all of the following:

- [ ] The numeric ID matches the interpreter.
- [ ] The assembly spelling is unique.
- [ ] The pseudo stem matches between TableGen and `AVMSystemCalls.inc`.
- [ ] The intrinsic stem matches between TableGen and `AVMSystemCalls.inc`.
- [ ] The intrinsic types match the public declaration.
- [ ] Clang builtin types match the intrinsic types.
- [ ] Intrinsic memory attributes match the operation.
- [ ] Pseudo `mayLoad`, `mayStore`, and `hasSideEffects` flags match.
- [ ] Descriptor memory accesses match the intrinsic and pseudo.
- [ ] A framebuffer service uses either `MB::FixedGlobal` or a hidden
      logical framebuffer argument intentionally.
- [ ] A hidden framebuffer argument has `PassToMachine = false` and no
      physical register.
- [ ] Framebuffer read/write effects and the 1024-byte extent are modeled
      consistently at the intrinsic, pseudo, and descriptor layers.
- [ ] Logical argument indexes match the intrinsic argument order.
- [ ] Physical input/output registers match the architecture.
- [ ] TableGen ties and descriptor ties agree.
- [ ] Program pointers use the correct value kind and pointer policy.
- [ ] Semantic pseudo operands use general register classes.
- [ ] The MC encoding and disassembly use the expected ID and name.
- [ ] The dedicated cost, when present, is tested.
- [ ] Focused compiler tests and end-to-end runtime tests pass.
