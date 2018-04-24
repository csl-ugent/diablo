# I/O
Diablo provides a number of wrappers to facilitate I/O: The functions
described hereafter print a message which can contain conversion
specifiers, each of which results in fetching zero or more subsequent
arguments. These conversion specifiers will be discussed later.

## Wrappers

~~~~
VERBOSE((t_int32 verboselevel), (t_string message, ...));
~~~~

This function will print the message if diablosupport\_options.verbose
is larger than or equal to verboselevel.

~~~~
FATAL((t_string message, ...));
~~~~

This function can be used to print out a message before terminating the
program.

~~~~
ASSERT((scalar expression),(t_string message, ...));
~~~~

This function will check whether the expression evaluates to a non-zero
value. If it does evaluate to zero, the program will terminate after
writing the message.

~~~~
WARNING((t_string message, ...));
~~~~

This function will print out the message as a warning

~~~~
DEBUG((t_string message, ...));
~~~~

This function will print out the message as debug information

~~~~
STATUS((START,t_string message, ...));
STATUS((STOP,t_string message, ...));
~~~~

This STATUS function is used to indicate the boundaries of some step in
the program rewriting.

## Conversion Specifiers

There are two types of conversion specifiers. The first type is introduced by a
%, followed by the conversion specifier. These are the traditional conversion
specifiers which are also used in standard I/O. The second type is introduced
by an @, followed by the conversion specifier. These are diablo-specific
conversion specifiers.

*Supported Diablo Conversion Specifiers*
| Conversion Specifier | Purpose | Modifier |
|----------|-------------|------|
| @ | allows you to print @ | |
| B | Basic Block (`t_bbl *`) | &lt;none&gt;: short info <br> e: print edges <br> i: print instructions |
| I | Instruction (`t_ins *`)|  |
| E | Edge (`t_cfg_edge *`)   |  |
| R | Relocation (`t_reloc *`)|  |
| S | Symbol (`t_symbol *`)   |  |
| P | Path (`t_path *`)       |  |
| G | Address (`t_address`)   |  |
| X | Regset (`t_regset`)     |  |

**Example:** Print the instruction (`t_ins * ins`) and the used registers:

~~~~ 
VERBOSE(0, ("Instruction:@I Used Regs:@X\n", ins, CPREGSET(BBL_CFG(INS_BBL(ins)), INS_REGS_USE(ins))));
~~~~ 
