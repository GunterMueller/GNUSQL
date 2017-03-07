/*
 *  crtfrm.c  - forming of a tuple
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

/* $Id: crtfrm.c,v 1.249 1998/09/29 21:25:25 kimelman Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "sctp.h"
#include "strml.h"
#include "fdcltrn.h"

u2_t
cortform (struct fun_desc_fields *desf, Colval colval, u2_t *lenval,
          char *cort, char *buf, u2_t mod_count, u2_t *mfn)
{
  u2_t k, fn, n, scsize, fdf, fields_n;
  struct des_field *df;
  char *arrpnt[BD_PAGESIZE];
  u2_t arrsz[BD_PAGESIZE];
  char *val, *endsc, *a, *sc;
  i4_t size;
  
  tuple_break (cort, arrpnt, arrsz, desf);  
  scsize = scscal (cort);

  for (endsc = buf, a = buf + scsize; endsc < a;)
    *endsc++ = *cort++ & ~EOSC;
  fdf = desf->f_fdf;
  fields_n = desf->f_fn;
  df = desf->df_pnt;
  for (n = 0; n < mod_count; n++)
    {
      fn = mfn[n];
      if (colval[n] != NULL)
	{
          arrsz[fn] = lenval[n];
	  arrpnt[fn] = colval[n];
        }
      else
	arrpnt[fn] = NULL;
      if (fn < fdf)
	continue;
      k = fn - fdf;
      sc = buf + 1;
      if (k >= 7)
	{
          sc += k / 7;
	  k = k % 7;
	}
      while (endsc <= sc)
	*endsc++ = 0;
      if (colval[n] != NULL)
	*sc |= BITVL(k);	/* write 1 in a new tuple scale */
      else
	*sc &= ~BITVL(k);
    }
  while (*(--endsc) == 0);
  *endsc++ |= EOSC;
  val = endsc;
  for (fn = 0; fn < fields_n; fn++)
    if ((a = arrpnt[fn]) != NULL)
      {
        size = arrsz[fn];
        bcopy (a, val, size);
        val += size;
      }
  return (val - buf);
}

