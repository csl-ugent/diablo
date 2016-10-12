/* {{{ Copyright
 * Copyright 2012 Bart Coppens, Ghent University
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
/* }}} */
#ifndef DIABLODIVERSITY_RANDOM_H
#define DIABLODIVERSITY_RANDOM_H

int diablo_rand_next();
/** Generate a random number in the range [low, high] (inclusive) */
int diablo_rand_next_range(int low, int high);
void diablo_rand_seed(unsigned int seed);
int diablo_rand_next_seed();

#define DIABLO_RAND_MAX 0x3FFFFFFF

#endif /* DIABLODIVERSITY_RANDOM_H */
