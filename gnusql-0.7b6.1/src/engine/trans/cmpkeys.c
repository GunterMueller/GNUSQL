/*
 * cmpkeys.c  - keys comparision
 *              Kernel of GNU SQL-server  
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

/* $Id: cmpkeys.c,v 1.250 1998/09/29 21:25:23 kimelman Exp $ */

#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

i4_t
cmpkeys (u2_t kn, u2_t * afn, struct des_field *df, char *pk1, char *pk2)
{
  u2_t kk, k, type;
  char *k1, *kval1, *k2, *kval2;
  i4_t v;

  if (pk1 == NULL)
    return (1);			/* The first is absent */
  if (pk2 == NULL)
    return (-1);		/* The second is absent */
  k1 = kval1 = pk1 + scscal (pk1);
  k2 = kval2 = pk2 + scscal (pk2);
  for (kk = 0; kk < kn && pk1 < k1 && pk2 < k2; pk1++, pk2++)
    for (k = 0; k < 7 && kk < kn; kk++, k++)
      if ((*pk1 & BITVL(k)) != 0)
	{
	  if ((*pk2 & BITVL(k)) != 0)
	    {			/* both are defined */
              type = (df + afn[kk])->field_type;
              if ((v = cmpval (kval1, kval2, type)) != 0)
                return (v);
              kval1 = proval (kval1, type);
              kval2 = proval (kval2, type);
	    }
	  else
	    return (-1);	/* The second isn't defined */
	}
      else if ((*pk2 & BITVL(k)) != 0)
	return (1);		/* The first isn't defined */
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

i4_t
cmp2keys (u2_t type, char *pk1, char *pk2)
{
  switch (type)
    {
    case T2B:
      return (t2bunpack (pk1) - t2bunpack (pk2));
    case T4B:
      return (t4bunpack (pk1) - t4bunpack (pk2));
    default:
      printf ("TRN.cmp2keys: This data type is incorrect");
      break;
    }
  return (0);
}
