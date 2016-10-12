#ifndef MY_ARCH_H
#define MY_ARCH_H
#define ARMAG   "!<arch>\n"
#define SARMAG  8
#define ARFMAG  "`\n"   
#define ARFZMAG  "Z\n"   
#define COMPRESSED_ALPHA 1
#include <diabloobject.h>

/*! typedef for struct _t_ar_archive_header. */

typedef struct _t_ar_archive_header t_ar_archive_header;

/*! The header for an ar archive.
 *
 * Field names are given as in GNU/BSD ar file format. However, we also use
 * this header to process CodeComposer libraries, where these fields have
 * alternate meanings */

struct _t_ar_archive_header
{
  char ar_name[16];
  char ar_date[12];
  /* In normal CodeComposer lib files abuse this field. For these files it will contain
   * an offset in the filename string table (called <filenames>) */ 
  char ar_uid[6];
  char ar_gid[6];
  char ar_mode[8];
  char ar_size[10];
  char ar_fmag[2];
};

t_bool IsArchAr(FILE * fp);
t_bool IsArchAlphaAr(FILE * fp);
t_bool IsArchMSAr(FILE * fp);
void ArchArOpen(FILE * fp, t_archive * ar);
void ArchArClose(t_archive * ar);
void ArchAlphaArOpen(FILE * fp, t_archive * ar);
void ArchAlphaArClose(t_archive * ar);
void ArchMSArOpen(FILE * fp, t_archive * ar);
void ArchMSArClose(t_archive * ar);
void DiabloArInit(int argc, char ** argv);
void DiabloArFini();
#endif
