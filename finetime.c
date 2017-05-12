/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * @file   finetime.c
 * @brief  Fine timing management based on rdtsc.
 * @author Mejbah<mohammad.alam@utsa.edu>
 */

#include <time.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "finetime.h"

double cpu_freq = 2000000; //KHz

void get_tsc( struct timeinfo *ti )
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
	ti->low  = lo;
	ti->high = hi;
}


unsigned long get_elapsed_cycle( struct timeinfo *start, struct timeinfo *stop)
{
	
	unsigned long begin = ( (unsigned long )start->low)|( ((unsigned long )start->high)<<32 );
	unsigned long  end = ( (unsigned long )stop->low)|( ((unsigned long )stop->high)<<32 );
	if( stop->high < start->high)
	{
			return (TSC_MAX - begin)+end;
	}
	else {
		return end - begin;
	}
}

/**
 * TODO: not right way to count time, but works for fine for performance compare purpose
 */
double get_elapsed2ms( struct timeinfo *start, struct timeinfo *stop)
{
	if(stop==NULL){
		struct timeinfo end;
		get_tsc(&end);
		return (double)get_elapsed_cycle(start,&end)/ cpu_freq;
	}
	else {
		get_tsc(stop);
		return (double)get_elapsed_cycle(start,stop) / cpu_freq;
	}
}

void start(struct timeinfo *ti)
{
	/* Clear the start_ti and stop_ti */
	get_tsc(ti);
	return;
}

