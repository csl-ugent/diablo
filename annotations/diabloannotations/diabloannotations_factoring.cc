#include <diabloannotations.h>

t_bool BblIsInFactoringRegion(t_bbl *bbl)
{
  t_bool result = FALSE;

  Region *region;
  FactoringAnnotationInfo *info;
  BBL_FOREACH_FACTORING_REGION(bbl, region, info)
  {
  	result = TRUE;
  	break;
  }

  return result;
}
