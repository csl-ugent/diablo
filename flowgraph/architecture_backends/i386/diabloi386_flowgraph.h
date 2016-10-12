/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/*
 * Copyright 2001,2002: Bruno De Bus
 *                      Bjorn De Sutter
 *                      Dominique Chanet
 *                      Ludo Van Put
 *
 *
 * This file is part of Diablo (Diablo is a better link-time optimizer)
 *
 * Diablo is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Diablo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Diablo; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <diabloobject.h>
#ifndef I386_FLOWGRAPH_H
#define I386_FLOWGRAPH_H
t_bool I386IsPcThunk(t_bbl* target);
void I386AddCallFromBblToBbl (t_object* obj, t_bbl* from, t_bbl* to);
void I386AddInstrumentationToBbl(t_object* obj, t_bbl* bbl, t_section* profiling_sec, t_address offset);
void I386Flowgraph(t_object *obj);
void I386MakeAddressProducers(t_cfg *cfg);
void I386StucknessAnalysis(t_object * obj);
#endif
/* vim: set shiftwidth=2: */
