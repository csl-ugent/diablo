/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/** \file
 *
 * Linker maps are files that describe how and where input objects are placed
 * by a linker in the output file. They are needed to emulate a specific
 * linker. This file holds the typedefs, types, function, and defines needed
 * to handle these files.
 */

/* Parsemap Typedefs {{{ */
#ifndef DIABLOOBJECT_PARSEMAP_TYPEDEFS
#define DIABLOOBJECT_PARSEMAP_TYPEDEFS
/*! A map: maps are avl-trees */
typedef struct _t_map t_map;

/*! The type of a section in the map */
typedef enum
{ Code, Data, Dbug, TlsData, TlsBss, Zero, TypeOther } t_map_section_type;

/*! The attributes (Read Only, Read Write, ...) of a section in the map */
typedef enum
{ RO, RW, OT } t_map_section_attribute;
typedef struct _t_map_node t_map_node;
typedef struct _t_compressed_map_node t_compressed_map_node;
typedef struct _t_objects_ll t_objects_ll;
typedef struct _t_map_handler t_map_handler;
typedef struct _t_compressed_map_handler t_compressed_map_handler;
#endif /* }}} Parsemap Typedefs */
/* Parsemap Defines {{{ */
#ifndef DIABLOOBJECT_PARSEMAP_DEFINES
#define DIABLOOBJECT_PARSEMAP_DEFINES
#define MAP_FILE_EXTENSION ".map"
#endif /* }}} Parsemap Defines */
#include <diabloobject.h>
#ifdef DIABLOOBJECT_TYPES
/* Parsemap Types {{{ */
#ifndef DIABLOOBJECT_PARSEMAP_TYPES
#define DIABLOOBJECT_PARSEMAP_TYPES
/*!A linked list of objects */
struct _t_objects_ll
{
  t_object *first;
  t_object *last;
  /* necessary for ObjectGet */
  t_object *parent;
};

/*! represents a section as found in the map-file */
struct _t_map_node
{
  t_dlist_node node;
  t_address base_addr;
  t_address sec_size;
  t_string object;
  t_string sec_name;
  t_string mapped_sec_name;
  t_map_section_type type;
  t_map_section_attribute attr;
  t_int32 sec_idx;
  t_object *obj;
  t_bool builtin; /* indicates "builtin" object files, that are added automatically by the linker, e.g. the 
                     ovl_mgr code for spu binaries with overlays */
  t_bool has_sec_idx; /* indicates whether sec_idx contains the index of the section
                     in the original object file */
};

struct _t_compressed_map_node
{
  t_uint32 sec_name; /* Offset in diablo string table */
  t_uint32 obj_name; /* Offset in diablo string table */
  t_uint32 offset;   /* Offset in parent object file */
  t_uint32 type_attribute_size;
};

/*! Defines how a map can be identified and read */
struct _t_map_handler
{
  struct _t_map_handler *next_handler;
  struct _t_map_handler *prev_handler;
  /* How to identify an mapfile (see if this handler handles this type of maps) */
  t_bool (*ident) (FILE * fp); /* TYPE */
  /* Read the mapfile (constructor for this type of maps) */
  void (*read) (FILE * fp, t_map *); /* TYPE */
  t_const_string name;
  t_uint32 identifier;
};

/*! Defines how a compressed map can be identified and read */
struct _t_compressed_map_handler
{
  struct _t_compressed_map_handler *next_handler;
  struct _t_compressed_map_handler *prev_handler;
  /* How to identify an mapfile (see if this handler handles this type of maps) */
  t_bool (*ident) (const t_object *); /* TYPE */
  /* Read the mapfile (constructor for this type of maps) */
  void (*read) (const t_object *, t_map *); /* TYPE */
  t_const_string name;
  t_uint32 identifier;
};


/*! Holds the information found in the map file */
struct _t_map
{
  t_dlist list;
  t_map_handler *handler;
  t_object *obj;
};
#endif /* }}} Parsemap Types */
#endif
#ifdef DIABLOOBJECT_FUNCTIONS
/* Parsemap Functions {{{ */
#ifndef DIABLOOBJECT_PARSEMAP_FUNCTIONS
#define DIABLOOBJECT_PARSEMAP_FUNCTIONS
t_map_handler *MapHandlerAdd (t_const_string, t_bool (*)(FILE *), void (*)(FILE *, t_map *));
t_compressed_map_handler * CompressedMapHandlerAdd (t_const_string, t_bool (*) (const t_object *), void (*) (const t_object *, t_map *));

void MapReadF(t_map *, FILE *);
t_string MapNameF(const t_object* obj);
void MapReadCompressed(t_map *, const t_object *);
t_map *MapNew (void);
void MapFree (const t_map *);
void MapNodeFree (const t_map_node *);
t_map_node *MapGetSection (const t_map *, t_address);
void MapPrint (const t_map *);
t_objects_ll MapLoadObjects (const t_map *, t_object *, t_bool);
void MapInsertNode (t_map *, t_map_node *);
void MapMoveObjectsToParent (const t_map *, t_object *);
void MapTableFree ();
void CompressedMapTableFree ();
t_map_handler *MapHandlerGetByName (t_const_string name);


#endif /* }}} Parsemap Functions */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
