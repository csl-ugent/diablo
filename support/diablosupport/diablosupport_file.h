/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/* File support
 *
 * Interface: TODO
 */

#include <diablosupport.h>
#include <stdio.h>

/** Platform independent paths */
#ifdef _MSC_VER
  #define PATH_DELIMITER "?"
  #define PATH_SEPARATOR_STR "\\"
  #define PATH_SEPARATOR_CHR '\\'
  #define CUR_DIR ".\\"  
  #define CHR_ISA_PATH_SEPARATOR(a) ((a) == '\\' || (a) == '/')
  #define CHR_ISA_RELATIVE_PATH_SPECIFIER(a) ((a) == '.')
#else
  #define PATH_DELIMITER ":"
  #define PATH_SEPARATOR_STR "/"
  #define PATH_SEPARATOR_CHR '/'
  #define CUR_DIR "./"
  #define CHR_ISA_PATH_SEPARATOR(a) ((a) == '/')
  #define CHR_ISA_RELATIVE_PATH_SPECIFIER(a) ((a) == '.')
#endif

/* File Typedefs {{{ */
#ifndef DIABLOSUPPORT_FILE_TYPEDEFS
#define DIABLOSUPPORT_FILE_TYPEDEFS
typedef struct _t_path t_path;
typedef struct _t_file_region t_file_region;
typedef struct _t_file_regions t_file_regions;
#endif /* }}} File Typedefs */
/* File Types {{{ */
#ifndef DIABLOSUPPORT_FILE_TYPES
#define DIABLOSUPPORT_FILE_TYPES
/*! \brief A file/search path
 *
 * The path type. This type is used to specify search paths. It holds a list
 * to directories */
struct _t_path
{
  t_uint32 nelems;
  char **dirs;
};

/*! \brief A region of a file
 *
 * The file region type. This type is used to describe a portion of a file */

struct _t_file_region
{
  t_uint32 offset;
  t_uint32 size;
  char * data;
  char * id;
};

/*! \brief A region of a file
 *
 * The file regions type. This type is used to describe a file build from
 * different regions. */

struct _t_file_regions
{
  t_file_region * region_array;
  t_uint32 nregions;
};
#endif /* }}} File Types */
#ifdef DIABLOSUPPORT_FUNCTIONS
/* File Functions {{{ */
#ifndef DIABLOSUPPORT_FILE_FUNCTIONS
#define DIABLOSUPPORT_FILE_FUNCTIONS
/* Paths */
/*! Add a string to an existing path */
void PathAddDirectory (t_path *, t_const_string);

/*! Global constructor for paths */
void PathInit (void);

/*! Global destructor for paths */
void PathFini (void);

/*! Path constructor */
#define PathNew(x,y) RealPathNew(FORWARD_MALLOC_DEFINE x,y)
t_path *RealPathNew (FORWARD_MALLOC_PROTOTYPE t_const_string, t_const_string);

/*! Path destructor */
void PathFree (const t_path *);
t_string IoModifierPath (t_const_string, va_list *);

/* Utilities */
t_string FileNameBase (t_const_string);

#define FileNameNormalize(file) RealFileNameNormalize(FORWARD_MALLOC_DEFINE file)
t_string RealFileNameNormalize (FORWARD_MALLOC_PROTOTYPE t_const_string);

/* Directories */
void DirMake (t_const_string, t_bool);
void DirDel (t_const_string);

/* Files */
t_string FileFind (const t_path *, t_const_string);
void FileCopy (t_const_string, t_const_string);
t_bool FileExists (t_const_string);

#define FileGetLine(x) RealFileGetLine(FORWARD_MALLOC_DEFINE x)
#define FileGetString(x,y) RealFileGetString(FORWARD_MALLOC_DEFINE x,y)
t_string RealFileGetLine (FORWARD_MALLOC_PROTOTYPE FILE *);
t_string RealFileGetString (FORWARD_MALLOC_PROTOTYPE FILE *, char);

/* Tar */
void Tar (t_const_string, t_const_string);
void Untar (t_const_string);

/* File Regions */
t_file_regions * FileRegionsNew();
void FileRegionsFree(const t_file_regions *);
void FileRegionsAddFileRegion(t_file_regions *, t_uint32, t_uint32, char *, char *);
void FileRegionsWrite(const t_file_regions *, FILE *);
t_uint32 FileRegionsFindGap(const t_file_regions *, t_uint32);
void FileRegionsSort(t_file_regions *);
#endif /* }}} File Functions */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
