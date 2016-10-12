#ifndef DIABLOTAR_H
#define DIABLOTAR_H
#include <diabloobject.h>

t_bool IsArchTar(FILE * fp);
void ArchTarOpen(FILE * fp, t_archive * ar);
void ArchTarClose(t_archive * ar);
void DiabloTarInit();
#endif
