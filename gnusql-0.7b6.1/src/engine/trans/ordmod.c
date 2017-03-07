/*
 *  ordmod.c  - Ordinary Modification
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

/* $Id: ordmod.c,v 1.250 1998/09/29 21:25:41 kimelman Exp $ */

#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"
#include "xmem.h"

extern struct d_r_t *firstrel;

void
ordmod (struct full_des_tuple *dtuple, struct des_tid *ref_tid,
        u2_t oldsize, char *nc, u2_t newsize)
{
  char *asp = NULL;
  i2_t delta;
  u2_t sn, pn;
  struct A pg;
  struct id_ob fullrn;

  sn = dtuple->sn_fdt;
  fullrn.segnum = sn;
  fullrn.obnum = dtuple->rn_fdt;
  delta = oldsize - newsize;
  if (delta >= 0)
    {
      u2_t *ai, ind;

      if (ref_tid->tpn != (u2_t) ~ 0)
        dtuple->tid_fdt = *ref_tid;
      pn = dtuple->tid_fdt.tpn;
      while ((asp = getpg (&pg, sn, pn, 'x')) == NULL);
      begmop (asp);
      ind = dtuple->tid_fdt.tindex;
      ai = (u2_t *) (asp + phsize) + ind;
      if (delta > 0)
        compress (&pg, ind, newsize);
      else
        recmjform (OLD, &pg, *ai, oldsize, asp + *ai, 0);
      bcopy (nc, asp + *ai, newsize);
      MJ_PUTBL ();
      putpg (&pg, 'm');
      if (delta > 0)
        modrec (&fullrn, pn, delta);
    }
  else
    {
      pn = dtuple->tid_fdt.tpn;
      if (ref_tid->tpn == (u2_t) ~ 0)
	{
	  if (nordins (dtuple, CORT, oldsize, newsize, nc) != 0)
	    {
	      doindir (dtuple, oldsize, newsize, nc);
	      modrec (&fullrn, pn, oldsize - MIN_TUPLE_LENGTH);
	    }
	}
      else
	{
          struct full_des_tuple dtuple_ref;

          dtuple_ref = *dtuple;
          dtuple_ref.tid_fdt = *ref_tid;
	  if (nordins (dtuple, CORT, MIN_TUPLE_LENGTH, newsize, nc) == 0)
            {
              pn = ref_tid->tpn;
              while ((asp = getpg (&pg, sn, pn, 'x')) == NULL);
              begmop (asp);
              compress (&pg, ref_tid->tindex, 0);
              MJ_PUTBL ();
              putpg (&pg, 'm');
              modrec (&fullrn, pn, oldsize + size2b);
            }
	  else if (nordins (&dtuple_ref, CREM, oldsize, newsize, nc) != 0)
	    {
	      doindir (dtuple, MIN_TUPLE_LENGTH, newsize, nc);
              pn = ref_tid->tpn;
              while ((asp = getpg (&pg, sn, pn, 'x')) == NULL);
              begmop (asp);
              compress (&pg, ref_tid->tindex, 0);
              MJ_PUTBL ();
              putpg (&pg, 'm');
              modrec (&fullrn, pn, oldsize + size2b);
	    }
	}
    }
}

static int
analoc (u2_t sn, u2_t pn, u2_t delta, struct A *pg, u2_t pfms)
{
  u2_t lind, fs;
  char *asp = NULL;
  
  while ((asp = getpg (pg, sn, pn, 'x')) == NULL);
  lind = ((struct page_head *) asp)->lastin;
  fs = *((u2_t *) (asp + phsize) + lind) - (phsize + size2b * (lind + 1));
  if (testfree (asp, fs, delta) == -1)
    {
      putpg (pg, 'n');
      return (-1);
    }
  begmop (asp);
  if (fs < pfms)
    rempbd (pg);
  return (0);
}

int
nordins (struct full_des_tuple *dtuple, i4_t type,
         u2_t oldsize, u2_t newsize, char *nc)
{
  struct A pg;
  u2_t size, offloc, sn, pn;
  i2_t delta;
  struct des_tid *tid;
  struct id_ob fullrn;

  delta = newsize - oldsize;
  sn = dtuple->sn_fdt;
  fullrn.segnum = sn;
  fullrn.obnum = dtuple->rn_fdt;
  tid = &dtuple->tid_fdt;
  pn = tid->tpn;
  if ((size = getrec (&fullrn, pn, &pg, &offloc)) >= (u2_t) delta)
    {
      char *a, *asp;
      u2_t *ai, pnifam, ind;
      struct A pg_table;
      
      if (analoc (sn, pn, delta, &pg_table, size) != 0)
	{
	  BUF_unlock (sn, 1, &pg.p_pn);
	  return (-1);
	}
      *nc = type;
      ind = tid->tindex;
      ai = (u2_t *) (pg_table.p_shm + phsize) + ind;
      if (*ai == 0)
        inscort (&pg_table, ind, nc, newsize);
      else
        exspind (&pg_table, ind, oldsize, newsize, nc);
      MJ_PUTBL ();
      putpg (&pg_table, 'm');
      pnifam = pg.p_pn;
      while (BUF_enforce (sn, pnifam) < 0);
      asp = getwl (&pg, sn, pnifam);
      a = asp + offloc;
      begmop (asp);
      recmjform (OLD, &pg, offloc, size2b, a, 0);
      MJ_PUTBL ();
      t2bpack (size - delta, a);
      putpg (&pg, 'm');
      return (0);
    }
  else
    {
      BUF_unlock (sn, 1, &pg.p_pn);
      return (-1);
    }
}

void
doindir (struct full_des_tuple *dtuple,
         u2_t oldsize, u2_t newsize, char *nc)
{
  i4_t rep;
  char *a, *asp = NULL;
  u2_t cpn, pfms, *ai, sn, pn, ind;
  struct d_sc_i *scind;
  struct ldesscan *disc;
  i2_t num;
  struct A pg;
  struct des_tid ref_tid;
  i4_t rn;
  struct id_rel idr;

  sn = dtuple->sn_fdt;
  idr.urn.segnum = sn;
  rn = idr.urn.obnum = dtuple->rn_fdt;
  *nc = CREM;
  if (rn != RDRNUM)
    {
      struct d_r_t *desrel;
      for (desrel = firstrel; desrel != NULL; desrel = desrel->drlist)
        if (desrel->desrbd.relnum == rn)
          {
            idr.pagenum = desrel->pn_r;
            idr.index = desrel->ind_r;
            break;
          }
    }
  scind = rel_scan (&idr.urn, NULL, &num, 0, NULL, NULL, 0, 0, NULL);
  disc = &scind->dessc;
  pn = cpn = dtuple->tid_fdt.tpn;
  rep = fgetnext (disc, &cpn, &pfms, FASTSCAN);
  while (rep != EOI && cpn != pn)
    rep = getnext (disc, &cpn, &pfms, FASTSCAN);
  if (rep != EOI)
    rep = getnext (disc, &cpn, &pfms, FASTSCAN);
  if (rep != EOI && pfms >= newsize)
    {
      if (analoc (sn, cpn, (i2_t) newsize, &pg, pfms) == 0)
        {
          u2_t nind = 0, lind;

          ai = (u2_t *) (pg.p_shm + phsize);
          lind = ((struct page_head *) pg.p_shm)->lastin;
          while ((nind <= lind) && (*ai != 0))
            {
              CHECK_PG_ENTRY(ai);
              ai++;
              nind++;
            }
          ref_tid.tpn = cpn;
          ref_tid.tindex = nind;
          inscort (&pg, nind, nc, newsize);
          MJ_PUTBL ();
          putpg (&pg, 'm');
          modcur (disc, pfms - newsize - size2b);
	}
      else
        {
          BUF_unlock (sn, 1, &disc->curlpn);
          ref_tid = ordins (&idr, nc, newsize, 'n');
        }
    }
  else
    {
      if (rep != EOI)
        BUF_unlock (sn, 1, &disc->curlpn);
      ref_tid = ordins (&idr, nc, newsize, 'n');
    }
  delscan (num);
  while ((asp = getpg (&pg, sn, pn, 'x')) == NULL);
  begmop (asp);
  ind = dtuple->tid_fdt.tindex;
  if (oldsize > MIN_TUPLE_LENGTH)
    compress (&pg, ind, MIN_TUPLE_LENGTH);
  ai = (u2_t *) (asp + phsize) + ind;
  a = asp + *ai;
  *a++ = IND;
  t2bpack (ref_tid.tindex, a);
  t2bpack (ref_tid.tpn, a + size2b);
  MJ_PUTBL ();
  putpg (&pg, 'm');
}
