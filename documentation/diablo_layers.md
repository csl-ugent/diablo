# The Link Layer

At the link layer, represented by the `diabloobject` library, there are
five important data structures:

-  `t_object`: an object file (relocatable or executable)
-  `t_relocatable`: the base class for all relocatable entities
-  `t_section`: an object file section (code or data or ...). This is a
    relocatable entity and thus derived from `t_relocatable`.
-  `t_symbol`: a "label" attached to a relocatable entity
-  `t_reloc`: a *relocation*. This structure conveys information about
    the relations existing between different relocatable entities.

## Objects and sections

Each object file contains a number of sections. Each section has a type,
indicating the kind of information it carries:

-   **code sections** hold machine code instructions. They are typically
    read-only.
-   **rodata sections** hold constant (read-only) data.
-   **data sections** hold mutable data.
-   **bss sections** hold zero-initialized mutable data.
-   **note sections** contain some information needed by the OS loader
    to correctly load the program. They are not visible to the program
    code and their contents do not influence the execution of the
    program.

An object file can contain other section types as well (e.g. debug
sections), but they are not necessary for the correct running of the
program, and thus are ignored by Diablo. The `t_object` structure holds
pointers to all of the sections of the object file it represents. These
are stored in arrays per section type; you can access them through the
`OBJECT_{CODE|RODATA|DATA|BSS|NOTE}` getters. The `t_section` structure
contains all information to describe a section: it's type, it's size and
address, it's alignment constraints, etc. The `SECTION_DATA` field holds
a pointer to the section's contents. This is not true for bss sections:
the contents of these sections are not represented in Diablo as they
contain only zeroes anyway. For code sections, the `SECTION_DATA`
pointer points to different things depending on the state of the
section: it either points to the raw section contents, an array of
disassembled instructions or the control flow graph of the program.

## Sub- and parent objects and sections

The executable file to be rewritten by Diablo is called the *parent
object*. This executable is created by linking together a number of
relocatable object files and libraries. These object files are
represented in Diablo as well, and are called *subobjects*. Likewise,
the sections of the parent object are referred to as parent sections,
and the sections from the subobjects are subsections. All subsections
are mapped to a parent section. There are several ways to navigate this
hierarchy:

-   `OBJECT_FOREACH_SUBOBJECT`: for a parent object, iterate over all
    subobjects
-   `OBJECT_FOREACH_SECTION`: for a given object (either parent or
    subobject), iterate over all sections. It is also possible to
    iterate over sections of a specific type as well.
-   `SECTION_PARENT_SECTION`: returns the parent section of a subsection
-   There is no easy way to iterate over all subsections of a given
    parent section, but there is a function called
    `SectionGetSubsections()` that returns the list of all subsections
    for a parent section. You can then just iterate over this list.

For Diablo, a subsection is considered to be a fundamental unit of
information: the contents of a subsection cannot be divided into smaller
blocks. The only exception to this rule are code sections: they can be
subdivided into individual instructions, but this is done in the
flowgraph layer.

## Relocatable entities

A relocatable entity is, simply put, any program entity that has an
address. Examples are program sections, but also instructions and basic
blocks (these will be introduced in the section about the flowgraph
layer). The `t_relocatable` structure is the common base class for all
these relocatable entities. It contains some information that is common
to all types of relocatable entities: the current address, the original
address (its address in the input program), the size of the entity, a
list of relocations originating from this entity and a list of
relocations referring to this entity.

## Symbols

In order to identify (amongst others) functions and variables, object
files contain symbols that act as labels attached to relocatable
entities. In Diablo, these symbols are represented by a `t_symbol`
structure. This structure contains the name of the symbol and its type,
and indicates whether or not this is a global symbol.

## Relocations

The task of a linker is to create a working program out of all the
different object files and libraries supplied by the developer. To do
this, the linker has to resolve the dependencies between the input
objects: if code in one object file calls a function in another object
file, the linker has to look up this function and write the correct
address for the function in the call instruction. The object files
represent their dependencies as *relocations*: these structures identify
the source of the reference, what is referenced, and how the address
that needs to be filled in is to be computed (e.g. as an absolute
address, as a pc-relative offset, ...) This is a very important data
structure, as it models the dependencies between different relocatable
entities. If a relocatable entity is not referenced by any relocation,
it is in effect unreachable and can be removed from the program. This is
because no other part of the program can produce the address of this
relocatable entity if there is no relocation point ing to it. If the
address is never produced, the contents of the entity can never be used
in the program. The `t_reloc` structure in Diablo represents such a
relocation. It has a `FROM` field pointing to the relocatable entity
that contains the reference and a `TO_RELOCATABLE` field that points to
the referenced relocatable entity. Diablo uses a special stack-based
language to describe how the relocation should be computed, the details
of this language can be found [here](relocations.md). Each relocatable
entity has a list of relocations that refer to this entity
(`RELOCATABLE_REFED_BY`), and a list of relocations coming from this
entity (`RELOCATABLE_REFERS_TO`). These are singly-linked lists of
`t_reloc_ref` structures. The actual relocation can then be accessed
through the `->rel` field of this structure. The following example code
iterates over all relocations referring to a given relocatable entity:

~~~~
t_relocatable *r = ...;
t_reloc_ref *rr;
for (rr = RELOCATABLE_REFED_BY(r); rr; rr = rr->next)
{
  t_reloc *rel = rr->rel;
  // do something with rel
}
~~~~

**Note:** Initially, just like in a real linker, a relocation in Diablo
actually points to a *symbol* instead of a relocatable entity. However,
after the symbol resolution phase this layer of indirection is removed
for convenience. Symbols stay attached to the relocatable entities
however, as they can be used later on to look up the location of
functions and data structures in the program.

# The Flowgraph Layer

On top of the `diabloobject` library, Diablo also offers the
`diabloflowgraph` library. This library offers a more detailed view of
the program code sections by splitting them in individual instructions
and constructing a control flow graph. This control flow graph, combined
with the data dependency graph from the link layer, forms the *Augmented
Whole-Program Control Flow Graph (AWPCFG)*, which is Diablo's most
detailed representation of the program that is being rewritten. The
basic data structures in this layer are:

-  `t_cfg`: the control flow graph.
-  `t_ins`: a program instruction.
-  `t_bbl`: a basic block. This is a group of instructions that is
    guaranteed to be executed together. These blocks are the nodes of
    the control flow graph.
-  `t_cfg_edge`: a control flow graph edge. These edges model the
    possible control flow between the basic blocks.
-  `t_function`: a function is a *single-entry* group of basic blocks,
    roughly corresponding to the original program procedures.

## Instructions and basic blocks

The `t_ins` structure represents the architecture-independent
information about an instruction. This type is derived from
`t_relocatable`. It contains some information about the registers that
are used and defined by this instruction, the instruction type, some
architecture-independent instruction flags, etc. Each architecture
backend derives an architecture-specific `t_arch_ins` structure from
`t_ins` (e.g. `t_arm_ins, t_i386_ins`). This structure represents the
architecture-specific information about the instruction: opcode,
operands, etc. A basic block is defined as *"A sequence of instructions
with a single entry point, single exit point, and no internal
branches"*. As a consequence, once control flow enters the first
instruction of a basic block, all other instructions in the basic block
are guaranteed to be executed without interruption. This property makes
the basic block ideal as node in the control flow graph. The `t_bbl`
structure is, like the `t_ins` structure, derived from `t_relocatable`.
This means basic blocks have addresses (typically the address of their
first instruction) and a size (the sum of the sizes of all the block's
instructions). As a rule, relocations reference basic blocks and not
instructions. Relocations to code imply that the referenced code can be
called or jumped to. Even if the instruction at the referenced address
is changed or deleted, the reference should keep existing (and move to
the next instruction). The easiest way to accomplish this is to let the
relocation reference the basic block instead of the instruction: no
matter how many instructions are removed from the block, the relocation
will always point to the first instruction of the remaining block. On
the other hand, relocations always come from instructions and never from
basic blocks. The fact that some relocatable entity is referenced
depends very much on the referencing instruction. If the instruction is
removed, the reference should be removed as well. Hence, the relocation
comes from the instruction itself and not from the containing basic
block. **Note:** On some architectures(e.g. ARM), there is data
interspersed with the instructions in the code section. This data is
represented in so-called *data instructions*, which are just dummy
instructions that hold the data. These data instructions are grouped in
*data blocks*, which are also represented as `t_bbl` structures.

## CFG Edges

The control flow graph edges model all possible control flow between the
basic blocks. The possible edge types are:

-   `ET_FALLTHROUGH`: control falls through to the next block, either
    because there is no control flow instruction or the instruction is
    conditional and this edge models the "not-taken" path.
-   `ET_JUMP`: control flows because of a (possibly conditional) jump
    instruction.
-   `ET_CALL`: function call
-   `ET_SWI`: system call (software interrupt)
-   `ET_SWITCH`: the basic block at the head of the edge implements a
    `switch` statement. `ET_SWITCH` edges connect all possible cases to
    the switch block.
-   `ET_RETURN`: this edge models a function return.
-   `ET_COMPENSATING`: this edge compensates for non-call
    interprocedural control flow edges. This will be explained in the
    next section.
-   `ET_IPJUMP`, `ET_IPFALLTHRU`, `ET_IPSWITCH`: interprocedural
    variants of the regular edges. This will also be explained in the
    next section.

Interprocedural edges are divided in two categories: call edges and
interprocedural jump, switch or fallthrough edges are *forward
interprocedural edges*, return and compensating edges are *backward
interprocedural edges*. The interprocedural edges are paired: call edges
have corresponding return edges and all other types of forward
interprocedural edges have corresponding compensating edges. If a
function does not return, an incoming forward interprocedural edge need
not have a corresponding edge. However, every backward interprocedural
edge should be paired to a forward interprocedural edge: it is
impossible to return from a function that hasn't been entered.

## Functions

Diablo's control flow graph is partitioned into *functions*. These
correspond roughly to the procedures from the original program, but not
entirely. Functions in Diablo are single-entry and single-exit regions.
The single-entry requirement is met by starting a new function with
every block that has incoming call edges, and all blocks that have
incoming edges from two different functions. The single-exit requirement
is met by introducing a *dummy return block* at the end of each
function. All basic blocks that end in a return instruction are
connected to this return block by `ET_JUMP` edges. The actual
`ET_RETURN` edges then flow from the dummy return block to the function
return site. If the function never returns (e.g. because it calls
`exit()` on every code path), there will be no return block. Unlike in
source code procedures, it is very much possible for control flow to
enter a function by other means than a function call. Hand-written
assembler code (which can be found in any C library) need not follow the
conventions compiler-generated code follows, and control flow can go in
all kinds of unconventional ways. Optimizing compilers can perform
tail-call optimization, thus creating interprocedural jumps between
procedures. All of this is compounded by the single-entry requirement
Diablo imposes on functions: once some interprocedural edges exist, new
functions need to be created starting from their entry point, leading to
more interprocedural edges, and so on. Hence the need for the
interprocedural edge types introduced in the previous section. However,
the presence of interprocedural control flow poses a new problem: if
control flows from function A to function B without going through a
call/return pair, how can it return to A's caller upon return of B? For
this we would need to add extra return edges from B's return block to
all possible return sites of function A. This would be very complicated.
Fortunately, a simpler solution exists: for every forward
interprocedural (non-call) edge from A to B a corresponding
*compensating* edge is added from B's return block to A's return block.
When B returns, the control can flow back to A's return block and from
there on to the return sites of function A.

## Unknown control flow

Unfortunately, it is usually impossible to build a completely accurate
control flow graph. In each program there are a number of indirect
control flow transfers. If Diablo cannot determine the possible targets
of these transfers, it should conservatively assume that these transfers
can go to any instruction in the program. Of course, this would make it
nearly impossible to do any program rewriting as the control flow graph
would be too conservative. Fortunately, we can be more precise: an
indirect control transfer can only target some piece of code if the
address of this piece of code is stored somewhere in the program. As all
stored program addresses are represented by relocations, indirect
control flow transfers can only target basic block referenced by
relocations. Still, if there are `n` indirect control transfers and `m`
possible targets, this would mean there would be `n*m` unknown control
flow edges. As both `n` and `m` are typically large values, this would
be very impractical. Therefore, the *unknown node* (AKA hell node) is
introduced. This is a dummy control flow graph node that has incoming
edges from all indirect control flow transfers and outgoing edges to all
possible indirect control flow targets. This way, only `n+m` edges are
needed to represent all possible indirect control flow. For analyses, it
suffices to make worst-case assumptions for the unknown node. This will
guarantee conservative treatment of all indirect control flow. In
practice, Diablo distinguishes different unknown nodes, for different
types of unknown control flow:

-   the *hell node* models general unknown control flow
-   the *call hell node* models indirect procedure calls
-   the *swi hell node* models system calls: its incoming edges are
    `ET_SWI` edges
-   the *longjmp hell node* models longjmp-type control flow

As mentioned before, all possible targets of indirect control flow are
marked by relocations. These relocations contain a reference to their
corresponding unknown control flow edge. If the relocation is removed
from the program, the unknown control flow edge is automatically removed
as well.

## Navigating the hierarchy

This section summarizes the ways in which the hierarchy can be
navigated.

### from `t_cfg`

The `t_cfg` structure keeps unordered lists of all basic blocks, edges
and functions. These can be accessed through the
`CFG_FOREACH_{BBL|EDGE|FUNCTION}` iterators. A CFG keeps a pointer to
the code section it was derived from in the `CFG_SECTION` field.

### from `t_function`

The `t_function` structure keeps a list of all basic blocks associated
with this function. This list is in no particular order, except for the
first block, which is guaranteed to be the entry block of the function,
and the last block, which is the function's dummy return block (if there
is any). The function's entry can always be accessed through
`FUNCTION_BBL_FIRST`. The exit block can be accessed with the
`FunctionGetExitBlock()` function, which returns `NULL` if the function
has no exit block, and otherwise returns `FUNCTION_BBL_LAST`. The list
can be iterated with the `FUNCTION_FOREACH_BBL` iterator. A function
keeps a pointer to its CFG in the `FUNCTION_CFG` field. **Note:** the
following code fragments have different results:

~~~~
CFG_FOREACH_FUNCTION(cfg,fun)
{
  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    //do something
  }
}
~~~~

and

~~~~
CFG_FOREACH_BBL(cfg,bbl)
{
  //do something
}
~~~~

The reason is that functions never contain data blocks. Directly
iterating over all basic blocks in the control flow graph includes the
data blocks, iterating over all basic blocks in all functions includes
only the code blocks.

### from `t_bbl`

The list of instructions in the basic block can be iterated over with
the `BBL_FOREACH_INS` iterator. This list is of course ordered. It is
also possible to iterate over the predecessor and successor edges of a
basic block with the `BBL_FOREACH_{PRED|SUCC}_EDGE`. These lists are
unordered. A basic block keeps pointers to its CFG and its function in
the `BBL_CFG` and `BBL_FUNCTION` fields.

### from `t_cfg_edge`

This structure keeps pointers to the head of the edge (`CFG_EDGE_HEAD`),
its tail (`CFG_EDGE_TAIL`) and, for interprocedural edges, its
corresponding edge (`CFG_EDGE_CORR`).

### from `t_ins`

This structure keeps a pointer to its basic block (`INS_BBL`) and to the
next and previous instructions in the basic block (`INS_INEXT` and
`INS_IPREV`).
