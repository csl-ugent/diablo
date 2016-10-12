/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloobject.h>

/* Object Typedefs {{{ */
#ifndef DIABLOOBJECT_OBJECT_TYPEDEFS
#define DIABLOOBJECT_OBJECT_TYPEDEFS
typedef struct _t_section_descriptor t_section_descriptor;
typedef struct _t_overlay t_overlay;
typedef struct _t_overlay_sec t_overlay_sec;
typedef enum
{
  NOHANDLER,
  GROUPHANDLER,
  OBJECTHANDLER
} t_handler_type;
typedef struct _t_object_handler t_object_handler;
typedef struct _t_builtin_object t_builtin_object;

/*! The different objectfile formats */
/*! Describes the size of a certain objectfile format */
typedef enum
{
  S_NOSIZE, /* Depends on subtype */
  S_32BIT, /* 32 bit risc */
  S_64BIT, /* 64 bit risc */
  S_VARIABLE /* Cisc */
} t_instruction_size;

typedef enum
{
  /* not set */
  OBJTYP_UNDEFINED = 0,
  /* relocatable object file (*.o, *.obj) */
  OBJTYP_RELOCATABLE,
  /* linked binary */
  OBJTYP_EXECUTABLE,
  /* linked PIC binary */
  OBJTYP_EXECUTABLE_PIC,
  /* dynamic/shared library (no PIC) */
  OBJTYP_SHARED_LIBRARY,
  /* dynamic/shared library (PIC) */
  OBJTYP_SHARED_LIBRARY_PIC
} t_object_type;
#endif /* Object Typedefs }}} */
/* Object Defines {{{ */
#ifndef DIABLOOBJECT_OBJECT_DEFINES
#define DIABLOOBJECT_OBJECT_DEFINES
#ifdef GENERICADDRSUPPORT
/*! Get a new t_address initialized in the same address format as the
 * object */
#define AddressNewForObject(obj,a) ((OBJECT_ADDRESS_SIZE(obj)==ADDRSIZE32)?AddressNew32(((t_uint32)(a))):AddressNew64((t_uint64)(a)))
#else
#ifdef BIT64ADDRSUPPORT
#define AddressNewForObject(obj,a) AddressNew64((t_uint64)(a))
#else
#define AddressNewForObject(obj,a) AddressNew32((t_uint32)(a))
#endif
#endif
#define AddressNullForObject(obj)       AddressNewForObject(obj,0)
/*! Standard cast to object */
#define T_OBJECT(x) ((t_object *) (x))
#endif /* }}} Object Defines }}} */
#ifdef DIABLOOBJECT_TYPES
/* Object Types {{{ */
#ifndef DIABLOOBJECT_OBJECT_TYPES
#define DIABLOOBJECT_OBJECT_TYPES

struct _t_overlay_sec 
{
  t_string name;
  struct _t_overlay_sec *next;
  void *chains; /* this should be t_chain_holder *, but that is only defined in diabloflowgraph */
  t_section *section;
};

struct _t_overlay
{
  struct _t_overlay_sec *sec;
  struct _t_overlay *next;
};

/*! a helper structure for describing the original code sections (can be used
 * after merging to determine which basic blocks originally belonged to which
 * code section).*/

struct _t_section_descriptor
{
  t_string name;
  t_address base;
  t_address size;
  t_address obase;
  t_address osize;
  char type;
};

/* Describes a built-in object (e.g. the SPU overlay manager, which is provided
 * by the linker itself) */
struct _t_builtin_object {
  t_string fileformat;
  t_string arch;
  t_string id;
  t_string object_name;
  struct _t_builtin_object *next;
};

/*! Describes how an object can be handled (identified/read/written) */
struct _t_object_handler
{
  struct _t_object_handler *next_group;
  struct _t_object_handler *prev_group;
  struct _t_object_handler *next_handler;
  struct _t_object_handler *prev_handler;
  /*! Handler type: can be either a group handler (a handler used to identify a
   * group of objects, typically a type of object files (e.g. ELF). This type of
   * handler cannot be used to read or write the objects, you need a normal
   * handler for that) or a normal handler (a handler used to identify, read and
   * write a specific object file format for a specific architecture) */
  t_handler_type htype;
  t_string (*getmapname)  (const t_object*);
  /*! callback function used to check if an opened object file can be handled by
   * this object handler */
  t_bool (*ident) (FILE *);
  /*! Read the objectfile and return a new t_object (constructor for this type
   * of objects). If the third parameter is TRUE, the object loader is
   * instructed to load debug information, if it is false, debug information is
   * skipped. */
  void (*read) (FILE *, t_object *, t_bool);
  /*! Write the objectfile */
  void (*write) (FILE *, t_object *);
  
  t_uint64 (*sizeofheaders) (t_object *, const t_layout_script *);
  t_uint64 (*linkbaseaddress) (const t_object *, const t_layout_script *);
  t_uint64 (*alignstartofrelro) (t_object *, long long currpos);
  t_uint64 (*aligngotafterrelro) (t_object *, long long currpos);
  t_uint64 (*aligndataafterrelro) (t_object *, long long currpos);

  void * (*rawread) (FILE *);
  void (*rawwrite) (void *, FILE * fp);

  t_address (*rawaddrtofile) (void *, t_address);
  t_address (*rawfiletoaddr) (void *, t_address);

  char * (*rawaddsec)(void *, t_const_string, t_address);
  char * (*rawaddrtosecname) (void *, t_address);

  t_const_string main_name;
  t_const_string sub_name;
  t_uint32 identifier;
};
#endif /* }}} Object Types */
#endif
#ifdef DIABLOOBJECT_FUNCTIONS
/* Object Functions {{{ */
#ifndef DIABLOOBJECT_OBJECT_FUNCTIONS
#define DIABLOOBJECT_OBJECT_FUNCTIONS
t_object *ObjectReadWithFakeName (t_const_string, t_const_string, t_object *, t_bool);
t_object *ObjectGetFromStream (t_const_string, FILE *, t_object *, t_bool);
t_object *ObjectGetFromCache (t_const_string, const t_object *);
t_object *ObjectGetBuiltin(t_const_string name, t_object * parent, t_bool read_debug);
t_object *ObjectNewCached (t_const_string, t_object *);
void ObjectWrite (t_object *, t_const_string);
void ObjectBuildUnifiedSymbolTable (t_object *);
void ObjectReadMap (t_object *);
void ObjectReadObjectInMap (t_object *, t_bool);
void ObjectMapPatchAlpha (t_object *);
void ObjectRemoveLinkerRemovedSections (t_object *);

/*!
 * \param obj the object
 * \param type char type (C,D,or B)
 * \param ro a bool to say if it's read only
 *
 * Add's a section from an objectfile.
 */
t_section *ObjectAddSectionFromFile (t_object *, char, t_bool, FILE *, t_uint32, t_address, t_address, t_address, t_const_string, t_int32);
t_uint32 ObjectGetData32 (const t_object *, t_const_string, t_address);
t_uint64 ObjectGetData64 (const t_object * obj, t_const_string name, t_address address);
void ObjectDisassemble (t_object * obj);
void ObjectAssemble (t_object * obj);
void ObjectDeflowgraph (t_object * obj);
void ObjectMergeCodeSections (t_object * obj);
void ObjectMergeRODataSections (t_object * obj);
void ObjectMergeDataSections (t_object * obj);
void ObjectMoveBssToDataSections (t_object * obj);
void ObjectRelocateSubObjects (t_object *, t_bool);
void ObjectAppendSubObject (t_object *, t_object *);
void ObjectRebuildSectionsFromSubsections (t_object *);

void ObjectCreateDataOrCodeTable (t_object *);
void ObjectOrderCodeSectionsContiguously (t_object *);
void ObjectBuildRelocTableFromSubObjects (t_object *);

t_layout_script *ObjectGetAndParseLayoutScript (t_object *);

t_segment *ObjectGetSegmentByName (const t_object *, t_const_string);
void ObjectAddSegment (t_object *, t_segment *);
t_object *ObjectGetLinkerSubObject (t_object *);
t_section *ObjectGetSectionContainingAddress (const t_object * obj, t_address addr);

t_uint32 ObjectHandlerAdd (t_const_string handler_main, t_const_string handler_sub, t_bool (*ident) (FILE *), void (*read) (FILE *, t_object *, t_bool), void (*write) (FILE *, t_object *), t_uint64 (*sizeofheaders) (t_object *, const t_layout_script *), t_uint64 (*linkbaseaddress) (const t_object *, const t_layout_script *), t_uint64 (*alignstartofrelro) (t_object *, long long),  t_uint64 (*aligngotpastrelro) (t_object *, long long),  t_uint64 (*aligndatapastrelro) (t_object *, long long), void * (*rawread) (FILE *), void (*rawwrite) (void *, FILE * fp), t_address (*rawaddrtofile) (void *, t_address), t_address (*rawfiletoaddr) (void *, t_address), char * (*rawaddsec)(void *, t_const_string, t_address), char * (*rawaddrtosecname)(void *, t_address), char * (*getmapname)(const t_object*));
void ObjectRegisterBuiltinObject(t_const_string fileformat, t_const_string arch, t_const_string id, t_const_string oname);
void ObjectDestroyBuiltinObject(t_const_string fileformat, t_const_string arch, t_const_string id, t_const_string oname);

t_symbol *ObjectGetSymbolByNameAndContext(const t_object *parentobj, const t_object *secobj, t_const_string sname);
t_symbol *ObjectGetBinutilsLocalLabelSymbol(const t_object *secobj, t_const_string sname);

void ObjectAddComdatSectionGroup(t_object *obj, t_section_group *group);
t_section_group *ObjectFindComdatSectionGroupBySignature(const t_object *obj, t_const_string signature);

void ObjectTableFree ();
void ObjectHashElementFree (const void *tf, void *data);
void ObjectConstructFinalSymbolTable(t_object *obj, t_const_string const *strip_symbol_masks);
t_bool ObjectAdaptSpaceForLEBRelocs(t_object * obj);
extern t_object_handler *object_table;
extern t_builtin_object *builtin_object_table;
#endif /* }}} Object Functions }}} */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
