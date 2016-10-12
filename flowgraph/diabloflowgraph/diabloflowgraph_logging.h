/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifdef DIABLOFLOWGRAPH_FUNCTIONS
#ifndef FLOWGRAPH_LOGGING
#define FLOWGRAPH_LOGGING

#include "diabloflowgraph.h"

/* Appends to the affected_code file of the current transformation */
void AddTransformedBblToLog(t_const_string transformation, t_bbl* bbl);
void LogFunctionTransformation(t_const_string prefix, t_function* fun); /* typically "before" or "after" */

#endif /* FLOWGRAPH_LOGGING */
#endif

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
