/*
 * addflds.c  - add fields to specific table
 *              Kernel of GNU SQL-server 
 *
 * This file is a part of GNU SQL Server
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

/* $Id: addflds.c,v 1.251 1998/05/20 05:52:42 kml Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

extern i4_t ljmsize;
extern struct des_nseg desnseg;
extern struct ADBL adlj;
extern char *pbuflj;

i4_t
addflds (struct id_rel *pidrel, i4_t fn, struct des_field *afn)
{
  struct des_field *df, *bfn;
  u2_t sn, size, n;

  for (df = afn + fn, bfn = afn; bfn < df; bfn++)
    if ((n = bfn->field_type) == T1B || n == T2B || n == T4B || n == TCH ||
	n == TFL || n == TFLOAT)
      continue;
    else
      return (-ER_NCF);
  sn = pidrel->urn.segnum;
  if (sn == NRSNUM)
    {
      char **t;
      struct des_trel *destrel;
      u2_t kn;
      
      t = desnseg.tobtab + pidrel->urn.obnum;
      destrel = (struct des_trel *) * t;
      if (destrel->tobtr.prdt.prob != TREL)
	return (-ER_NDR);
      n = destrel->f_df_tt.f_fn;
      kn = destrel->keysntr;
      destrel = (struct des_trel *) xrealloc (*t, dtrsize + (n + fn) * rfsize + kn * size2b);
      *t = (char *) destrel;
      df = destrel->f_df_tt.df_pnt + n;
      if (kn != 0)
        {
          u2_t *pnt_key_to, *pnt_key_from;
          
          size = (kn - 1) * size2b;
          pnt_key_to = (u2_t *)((char *)(df + fn)+ size);
          pnt_key_from = (u2_t *)((char *)df + size);
          for (; kn != 0; kn--)
            *pnt_key_to-- = *pnt_key_from--;
        }
      destrel->f_df_tt.f_fn += fn;
    }
  else
    {
      struct d_r_t *desrel;
      struct d_r_bd *drbd;
      char *cort, *nc, *pnt_new, *pnt_old;
      struct des_tid tid, ref_tid;
      struct id_rel idr;
      struct full_des_tuple dtuple;
      u2_t scsize, rcorsize, corsize, size1, newsize;
      CPNM cpn;
      
      cort = pbuflj + ljmsize;
      if ((desrel = getrd (pidrel, &ref_tid, cort, &corsize)) == NULL)
	return (-ER_NDR);
      size = fn * rfsize;
      if (((rcorsize = getrc (&desrel->desrbd, cort)) + size) > (BD_PAGESIZE - phsize - size2b))
	return (-ER_NCF);
      drbd = &desrel->desrbd;
      n = drbd->fieldnum;
      cpn = sn_lock (pidrel, 'm', NULL, 0);
      if (cpn != 0)
	{
	  rllbck (cpn, adlj);	/* adlj - external var */
	  return (cpn);
	}
      nc = cort + corsize;
      drbd->fieldnum += fn;
      scsize = scscal (cort);
      bcopy (cort, nc, scsize);
      pnt_old = cort + scsize;
      pnt_new = nc + scsize;
      bcopy ((char *) drbd, pnt_new, drbdsize);
      pnt_new += drbdsize;
      pnt_old += drbdsize;
      size1 = n * rfsize;
      bcopy (pnt_old, pnt_new, size1);     /* old fields descriptors */
      pnt_new += size1;
      pnt_old += size1;
      bcopy ((char *) afn, pnt_new, size); /* add new fields descriptors */
      pnt_new += size;
      size1 = rcorsize - (pnt_old - cort);
      bcopy (pnt_old, pnt_new, size1);     /* another descriptors */
      pnt_new += size1;
      if ((newsize = pnt_new - nc) < corsize)
	newsize = corsize;
      tid.tpn = pidrel->pagenum;
      tid.tindex = pidrel->index;
      modmes ();
      idr = *pidrel;
      idr.urn.obnum = RDRNUM;
      wmlj (ADFLJ, ljmsize + corsize + newsize, &adlj, &idr, &tid, 0);
      dtuple.sn_fdt = idr.urn.segnum;
      dtuple.rn_fdt = RDRNUM;
      dtuple.tid_fdt = tid;
      ordmod (&dtuple, &ref_tid, corsize, nc, newsize);
      BUF_endop ();
      desrel = (struct d_r_t *) xrealloc ((char *) desrel, newsize);
      desrel->f_df_bt.f_fn += fn;
      df = desrel->f_df_bt.df_pnt + n;
    }
  for (; fn != 0; fn--)
    *df++ = *afn++;
  return (OK);
}
