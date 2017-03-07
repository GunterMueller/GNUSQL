/*  cmpkey.c  - Comparision of keys
 *              Kernel of GNU SQL-server. Sorter   
 *
 * This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
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

/* $Id: cmpkey.c,v 1.249 1998/09/29 21:25:23 kimelman Exp $ */

#include "dessrt.h"
#include "pupsi.h"
#include "fdclsrt.h"
#include "fdcltrn.h"

extern char *nonsense;

int
cmp_key (struct des_sort *sdes, char *pk1, char *pk2)
{
  i4_t d, kk, k, v;
  char *k1, *kval1, *k2, *kval2, *drctn;
  u2_t *afn, kn, type;
  struct des_field *df;

  if (pk1 == nonsense)
    return (1);			/* The first is absent */
  if (pk2 == nonsense)
    return (-1);		/* The second is absent */
  pk1 += size2b + tidsize;
  pk2 += size2b + tidsize;
  k1 = kval1 = pk1 + scscal (pk1);
  k2 = kval2 = pk2 + scscal (pk2);
  drctn = sdes->s_drctn;
  if (*drctn == GROW)
    d = 1;
  else
    d = -1;  
  v = 0;
  kn = sdes->s_kn;
  df = sdes->s_df;
  afn = sdes->s_mfn;
  
  for (kk = 0, k = 0; kk < kn && pk1 < k1 && pk2 < k2; kk++, k++)
    {
      if (k == 7)
	{
	  k = 0;
	  pk1++;
	  if (pk1 >= k1)
	    break;
	  pk2++;
	  if (pk2 >= k2)
	    break;
	}
      if (*drctn++ == GROW)
	d = 1;
      else
	d = -1;
      if ((*pk1 & BITVL(k)) != 0)
	{
	  if ((*pk2 & BITVL(k)) != 0)
	    {			/* both are defined */
              type = (df + afn[kk])->field_type;
              if ((v = cmpval (kval1, kval2, type)) != 0)
                return (v * d);
              kval1 = proval (kval1, type);
              kval2 = proval (kval2, type);
	    }
	  else
	    return (-1);	/* The second not defined */
	}
      else if ((*pk2 & BITVL(k)) != 0)
	return (1);		/*The first isn't defined */
    }
  if (kk < kn)
    {
      if (pk1 == k1)
	{
	  if (pk2 == k2)
	    return (0);
	  else
	    return (1);
	}
      else
	return (-1);
    }
  return (0);
}
