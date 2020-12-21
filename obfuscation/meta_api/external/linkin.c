#include "linkin.h"

/* this function is used by Diablo to put the initialisation code in */
static void META_API_LINKIN_FUNCTION(init) () __attribute__((constructor));
static void META_API_LINKIN_FUNCTION(init) () {}

/* this function is here so we can easily refer to printf() from within Diablo,
 * and we don't have to manually create a PLT entry */
__attribute__((used))
void runme(int argc) {
  printf("runme %d\n", argc);
}
