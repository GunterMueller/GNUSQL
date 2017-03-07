/*
 *  keyfrm.c  - Key forming for some DB table index
 *              Kernel of GNU SQL-server  
 *
 *  This file is a part of GNU SQL Server
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

/* $Id: keyfrm.c,v 1.248 1998/09/29 21:25:35 kimelman Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

void
keyform (struct ldesind *desind, char *mas, char *cort)
{	/* mas - to, cort - from */
  u2_t kn, k, num_bit, fnk, sz, *afn;
  struct fun_desc_fields *desf;
  char *b;
  char *arrpnt[BD_PAGESIZE];
  u2_t arrsz[BD_PAGESIZE];  

  kn = desind->ldi.kifn & ~UNIQ & MSK21B;
  afn = (u2_t *) (desind + 1);
  desf = &(desind->dri->f_df_bt);
  tuple_break (cort, arrpnt, arrsz, desf);
  
  *mas = 0;  
  for (num_bit = k = 0; k < kn; k++, num_bit++)
    {
      if (num_bit == 7)
	{
	  num_bit = 0;
	  *(++mas) = 0;
	}
      if (arrpnt[afn[k]] != NULL)
	*mas |= BITVL(num_bit);		/* read 1 in key scale */
    }
  
  if ( *mas == 0)
    mas--;
  *mas |= EOSC;  
  mas++;
  for ( k = 0; k < kn; k++)
    {
      fnk = afn[k];
      if ( (b = arrpnt[fnk]) != NULL)
	{
	  if ( (sz = arrsz[fnk]) != 0 )
            {
              bcopy (b, mas, sz);
              mas += sz;
            }
	}
    }
}

char *
remval (char *aval, char **a, u2_t type)
{		/* a - to, aval - from */
  u2_t size = 0;
  
  size = get_length (aval, type);
  bcopy (aval, *a, size);
  *a += size;
  aval += size;
  return (aval);
}
