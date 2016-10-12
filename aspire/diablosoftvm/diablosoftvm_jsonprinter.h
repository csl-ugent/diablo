/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOSOFTVM_JSONPRINTER_H
#define DIABLOSOFTVM_JSONPRINTER_H

#include "diablosoftvm.h"

struct json_t;

json_t *ChunkArray2Json(t_cfg *cfg, t_ptr_array * chunks, t_bool emit_symbol_address_information, t_bool limit, int max);
json_t *SymbolArray2Json(t_cfg *cfg, t_ptr_array *symbols);

#endif
