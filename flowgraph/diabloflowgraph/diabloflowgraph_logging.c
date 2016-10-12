/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diabloflowgraph.h>

/* Appends to the affected_code file of the current transformation */
void AddTransformedBblToLog(t_const_string transformation, t_bbl* bbl)
{
  if (!diablosupport_options.enable_transformation_dumps)
    return;
  t_ins* ins;
  t_string filename = StringIo("%s/affected_code", GetCurrentTransformationDirectory());
  FILE* affected_code = fopen(filename, "a");
  t_function* fun = BBL_FUNCTION(bbl);

  BBL_FOREACH_INS(bbl, ins) {
    LOG(affected_code, "%s,%s,0x%x,%s:%i\n", transformation, fun ? FUNCTION_NAME(fun) : "(null)", INS_CADDRESS(ins), INS_SRC_FILE(ins), INS_SRC_LINE(ins));
  }

  Free(filename);
  fclose(affected_code);
}

void LogFunctionTransformation(t_const_string prefix, t_function* fun) {
  if (!diablosupport_options.enable_transformation_dumps)
    return;
  t_string filename = StringIo("%s/%s-%s.dot", GetCurrentTransformationDirectory(), prefix, FUNCTION_NAME(fun));
  FunctionDrawGraph(fun, filename);
  Free(filename);
}
