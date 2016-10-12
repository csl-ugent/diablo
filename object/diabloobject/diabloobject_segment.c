/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diabloobject.h>

t_segment *
SegmentNew (t_const_string name)
{
  t_segment *ret = Calloc (1, sizeof (t_segment));

  SEGMENT_SET_NAME(ret, StringDup (name));
  SEGMENT_SET_RELOCATABLE_TYPE(ret, RT_SEGMENT);
  return ret;
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
