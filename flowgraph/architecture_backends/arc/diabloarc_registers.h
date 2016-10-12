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

#ifndef ARC_REGISTERS_H
#define ARC_REGISTERS_H

#include <diabloflowgraph.h> 

#define ARC_REG_INVALID   254
#define ARC_REG_NONE      255

/* general purpose registers */
#define ARC_REG_R0		0

/* function prototypes */
t_string ArcRegisterName(t_reg reg);

/* macros */
#endif
