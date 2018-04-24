# Diablo Classes
Diablo provides a class mechanism which enables:

-   automatic generaton of getters and setters
-   inheritance

And in the case of a managed class:

-   [dynamic members](dynamic_members.md)

As such we make a distinction between a [basic class](#basic-class)
and a [managed class](#managed-class).

## Basic Class

We will introduce the concept of a basic class through an example:

~~~~
#include <diablosupport_class.h>
#ifndef CLASS
#define CLASS ins
#define ins_field_select_prefix INS
#define ins_function_prefix Ins
~~~~

We define a new class `ins`. This will result in a new type `t_ins`. The
ins_field_select_prefix will be used in the getters and setters. The
`ins_function_prefix` will be used in functions.

~~~~
DIABLO_CLASS_BEGIN
EXTENDS(t_relocatable)
~~~~

The class `ins` is an extension of the class `relocatable`. As such, it
will inherit the getters, setters and non-private functions of the class
`relocatable`. We will come back to this
[later](#inheritance).

~~~~
MEMBER(t_cfg *, cfg, CFG)
~~~~

A core members (as opposed to a [dynamic member](dynamic_members.md) for
a [managed class](#managed-class)) is defined using the `MEMBER`
mechanism. This will create a member of the type `t_cfg`. The second and
third argument indicate the name of the member in lowercase and
uppercase. This mechanism will automatically generate a getter and
setter, in this example: `t_cfg * INS_CFG(t_ins * ins)` and
`void INS_SET_CFG(t_ins * ins, t_cfg * cfg)`

~~~~
FUNCTION1(void, Kill, t_CLASS *){Free(ins);}
~~~~

This mechanism will create the function with signature
`void InsKill(t_ins *){Free(ins);}`. This function can be inherited if
other classes are extended from this class. To this end, t\_CLASS is
used instead of t\_ins. Similarly, functions with multiple arguments can
be defined by using `FUNCTION2(), FUNCTION3(), ...`

~~~~
PFUNCTION1(void, Print, t_CLASS *){VERBOSE((0,"@I",ins));}
~~~~

A PFUNCTION differs from a FUNCTION in that it can not be inherited (it
is Private).

~~~~
DIABLO_CLASS_END
#undef BASECLASS
#define BASECLASS relocatable
#include 
#undef BASECLASS
~~~~

### Inheritance
To enable the [inheritance]{#inheritance} mechanism, the definition of
the base class needs to be included. The getters, setters and
non-private functions of the base class will be inherited. If the base
class contains the following definition of a member:

~~~~
MEMBER(t_address, caddress, CADDRESS);
~~~~

then this will have o.a. resulted in the getter
`t_address RELOCATABLE_CADDRESS(t_relocatable * rel)`. This will be
inherited by the class `ins` and thus the getter
`t_address INS_CADDRESS(t_ins * ins)` will be available. The same
applies to setters and non-private functions.

## Managed Class

A managed class is a basic class with the additional feature that it
allows for [dynamic members](dynamic_members.md). We will limit this
discussion to the extensions to the basic class.

~~~~
#include <diablosupport_class.h>
#ifndef CLASS
#define CLASS ins
#define ins_field_select_prefix INS
#define ins_function_prefix Ins

#define MANAGER_TYPE t_cfg *
#define MANAGER_NAME cfg
#define MANAGER_FIELD ins_manager
~~~~

A managed class is, as it name suggests, managed by another class: the
manager class. The manager class is specified by MANAGER\_TYPE. It
should contain a member of type t\_manager with the (lowercase) name
specified by MANAGER\_FIELD. Thus, in our example, `cfg` should have the
following declaration: `MEMBER(t_manager, ins_manager, INS_MANAGER)` The
managed class should contain a member of type specified by MANAGER\_TYPE
with (lowercase) name specified by MANAGER\_NAME. Thus in our example:

~~~~
MEMBER(t_cfg *, cfg, CFG)
~~~~

Furthermore, a constructor, duplicator and destructor need to be
defined. These declarations will automatically generate functions needed
to manage the dynamic members. Note that these declarations are not
allowed in basic classes.

~~~~
CONSTRUCTOR({/*specified code*/})
DESTRUCTOR({/*specified code*/})
DUPLICATOR({/*specified code*/})
~~~~

The constructor will result in the function
`t_ins * InsInit(t_cfg * cfg)`, which executes the code specified in the
constructor and a function `t_ins * InsNew(t_cfg * cfg)`, which
allocates memory before it executes the specified code. The specified
code can access the instruction that will be returned through the
variable `ret`. Furthermore, the following functions will be created:

~~~~
static void InsCallbackInstall(t_cfg * cfg, t_manager_callback_type type, t_int32 prior, t_manager_callback_function fun, void * data)
static void InsCallbackUninstall(t_cfg * cfg, t_manager_callback_type type, t_int32 prior, t_manager_callback_function fun, void * data)
~~~~

They can be used to install extra callbacks when an instruction is
created (type CB\_NEW), duplicated (CB\_DUP) or freed (CB\_FREE).
`prior` indicates when this callback should be executed in the timeline.
For CB\_FREE, dynamic members are handled at time -10, the free itself
at time 0. For CB\_NEW and CB\_DUP, dynamic members are handled at time
10, while the new respectively dup are treated at time 0. The prior is
used to indicate when the callback should be called. The destructor
results in the function `void InsFree(t_ins * ins)` which will execute
the specified code and free the allocated memory. The specified code can
access the instruction that will be returned through the variable
`to_free`. The duplicator results in the function
`t_ins * InsDup(t_ins * ins)`. The specified code can access the
instruction that will be duplicated through the variable `to_dup` and
the instruction that will be returned through the variable `ret`.

~~~~
...
DIABLO_CLASS_END
#undef BASECLASS
#define BASECLASS relocatable
#include <diabloobject_relocatable.class.h>
#undef BASECLASS
~~~~
