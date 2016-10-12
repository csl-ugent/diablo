/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/** \file
 *
 * An archive in diabloobject is a file that holds a collection of object
 * files. Examples are the UNIX .a libraries, MVC++ .lib files and Java's jar
 * files. This file holds the typedefs, types, function, and defines related
 * to archives. */

/* Archive Typedefs {{{ */
#ifndef DIABLOOBJECT_ARCHIVE_TYPEDEFS
#define DIABLOOBJECT_ARCHIVE_TYPEDEFS
/*! typedef for struct _t_archive */
typedef struct _t_archive t_archive;

/*! typedef for struct _t_archive_hash_table_node */
typedef struct _t_archive_hash_table_node t_archive_hash_table_node;

/*! typedef for struct _t_archive_object_hash_table_node */
typedef struct _t_archive_object_hash_table_node t_archive_object_hash_table_node;
/*! typedef for struct _t_archive_symbol_hash_table_node */
typedef struct _t_archive_symbol_hash_table_node t_archive_symbol_hash_table_node;

/*! typedef for struct _t_archive_handler */
typedef struct _t_archive_handler t_archive_handler;
#endif /* Archive Typedefs }}} */
#include <diabloobject.h>
#ifdef DIABLOOBJECT_TYPES
/* Archive Types {{{ */
#ifndef DIABLOOBJECT_ARCHIVE_TYPES
#define DIABLOOBJECT_ARCHIVE_TYPES
/*!
 *
 * \todo add handler and document */
struct _t_archive
{
  t_uint32 type;
  t_const_string name;
  FILE *open_fp;
  t_hash_table *objects;
  t_hash_table *symbols;
  t_archive_handler *handler;
  void * data;
};

/*! Structure used to store archives in a hash table */
struct _t_archive_hash_table_node
{
  t_hash_table_node node;
  t_archive *arch;
};

/*! Stores the position of an object in the archive */
struct _t_archive_object_hash_table_node
{
  t_hash_table_node node;
  int flags;
  int fpos;
  t_uint32 size; /* 10 ^ 10 < 2 ^ 32 */
};

/*! Stores the position of an (object containing a) symbol in the archive */
struct _t_archive_symbol_hash_table_node
{
  t_hash_table_node node;
  t_const_string name;
  int fpos;
  t_uint32 size; /* 10 ^ 10 < 2 ^ 32 */
};

/*! Specifies how to handle (read/identify) an archive */
struct _t_archive_handler
{
  struct _t_archive_handler *next_handler;
  struct _t_archive_handler *prev_handler;
  int identifier;
  t_bool (*ident) (FILE *); /* TYPE */
  void (*read) (FILE *, t_archive *); /* TYPE */
  t_object *(*get_object) (const t_archive *, t_const_string name, t_object *, t_bool read_debug);
  t_object * (*get_object_by_symbol) (const t_archive *, t_const_string, t_object *, t_bool);
  void (*close) (t_archive *);
  t_const_string name;
};
#endif /* }}} Archive Types */
#endif
#ifdef DIABLOOBJECT_FUNCTIONS
/* Archive Functions {{{ */
#ifndef DIABLOOBJECT_ARCHIVE_FUNCTIONS
#define DIABLOOBJECT_ARCHIVE_FUNCTIONS
/*! Global constructor */
void ArchivesInit (void);

/*! Global destructor */
void ArchivesFini (void);
void ArchiveFree (const t_archive *);

/*! Cached constructor */
t_archive *ArchiveOpenCached (t_const_string name);
t_object *ArchiveGetObject (const t_archive *, t_const_string, t_object *, t_bool);
t_object *ArchiveGetObjectBySymbol (const t_archive *, t_const_string, t_object *, t_bool);
t_archive_handler *ArchiveHandlerAdd (t_const_string, t_bool (*)(FILE *), void (*)(FILE *, t_archive *), t_object * (*)(const t_archive *, t_const_string, t_object *, t_bool), t_object * (*)(const t_archive *, t_const_string, t_object *, t_bool), void (*)(t_archive *));
void ArchiveObjectHashTableNodeFree (const void *, void *);
void ArchiveSymbolHashTableNodeFree (const void *, void *);
#endif /* Archive Functions }}} */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
