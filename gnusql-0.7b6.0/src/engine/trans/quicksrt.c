/*  quicksor.c - Quick sort
 *               Kernel of GNU SQL-server. Sorter     
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by  Vera Ponomarenko
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Contacts:   gss@ispras.ru
 *
 */

/* $Id: quicksrt.c,v 1.247 1997/08/19 19:18:40 kml Exp $ */

#include "dessrt.h"
#include "fdclsrt.h"

extern i4_t N;
extern char *nonsense;
extern char **regpkr;

void
quicksort(i4_t M, struct des_sort *sdes)
{
  i4_t bstek[STEKSZ];
  i4_t *stek, l, r, i, i1, j, v = 0, d1, d2;
  char **ptp, *curpk, prdbl;

  ptp = regpkr;
  stek = bstek;
  prdbl = sdes->s_prdbl;
  for (l = 0, r = N - 1;;)
    {
      if ((r - l) < M)
	{			/* The simple insertion sort */
	  if (prdbl == NODBL)
	    for (j = l; j <= r; j++)	/* nonsense reference deletion */
	      if (ptp[j] == nonsense)
		{
		  for (i = j; i < r; i++)
		    ptp[i] = ptp[i + 1];
		  ptp[r--] = nonsense;
		}
	  for (j = l + 1; j <= r; j++)
	    {
	      curpk = ptp[j];
	      for (i = j - 1; i >= l;)
		{
		  if ((v = cmp_key (sdes, curpk, ptp[i])) < 0)
		    {
		      ptp[i + 1] = ptp[i];
		      i--;
		    }
		  else if (v == 0 && prdbl == NODBL)
		    {
		      for (i1 = i; i1 > l; i1--)
			ptp[i1] = ptp[i1 - 1];
		      ptp[l++] = nonsense;
		    }
		  else
		    break;
		}
	      ptp[i + 1] = curpk;
	    }
	  if (stek == bstek)
	    break;
	  r = *(--stek);
	  l = *(--stek);
	  continue;
	}
      i = l;			/* quicksort */
      j = r;
      curpk = ptp[l];
    m1:
      for (; i < j; j--)
	if ((v = cmp_key (sdes, curpk, ptp[j])) >= 0)
	  break;
      if (v == 0 && i < j && prdbl == NODBL)
	{
	  ptp[j] = nonsense;
	  j--;
	  goto m1;
	}
      if (i >= j)
	{
	  ptp[i] = curpk;
	  goto m3;
	}
      ptp[i++] = ptp[j];
    m2:
      for (; i < j; i++)
	if ((v = cmp_key (sdes, ptp[i], curpk)) >= 0)
	  break;
      if (v == 0 && i < j && prdbl == NODBL)
	{
	  ptp[i] = nonsense;
	  i++;
	  goto m2;
	}
      if (i < j)
	{
	  ptp[j] = ptp[i];
	  j--;
	  goto m1;
	}
      ptp[j] = curpk;
      i = j;
    m3:
      d1 = i - l;
      d2 = r - i;
      if (d1 < 2)
	{
	  l += d1 + 1;
	  continue;
	}
      if (d2 < 2)
	{
	  r -= d2 + 1;
	  continue;
	}
      if (d1 <= d2)
	{
	  *stek++ = i + 1;
	  *stek++ = r;
	  r = i - 1;
	}
      else
	{
	  *stek++ = l;
	  *stek++ = i - 1;
	  l = i + 1;
	}
    }
}
