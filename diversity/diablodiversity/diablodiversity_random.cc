/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

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

#ifdef __cplusplus
extern "C" {
#endif
#include <diablodiversity_random.h>
#ifdef __cplusplus
}
#endif
/* A Linear Congruential Generator, with the parameters that glibc uses (according to Wikipedia) */

static unsigned int x_n = 1;
static unsigned int a   = 1103515245;
static unsigned int c   = 12345;

int diablodiversity_rand_next() {
	x_n = x_n * a + c;
    return x_n & DIABLO_RAND_MAX;
}

int diablodiversity_rand_next_range(int low, int high)  {
	return (diablo_rand_next() % (high - low + 1)) + low;
}

void diablodiversity_rand_seed(unsigned int seed) {
	x_n = seed;
}

int diablodiversity_rand_next_seed() {
  return x_n;
}
