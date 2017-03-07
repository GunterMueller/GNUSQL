/*  incrs.c  - Lock
 *             Kernel of GNU SQL-server. Synchronizer    
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

/* $Id: lock.c,v 1.248 1998/09/29 21:25:15 kimelman Exp $ */

#include "xmem.h"
#include <sys/types.h>
#include "dessnch.h"
#include "deftr.h"
#include "fdclsyn.h"
#include <assert.h>

struct des_rel *firstrel;
extern struct des_tran artran[];

static void
lform1 (struct des_lock *anl, struct des_tran *tr, struct des_rel *r, char lin)
{
  anl->tran = tr;
  anl->rel = r;
  anl->lockin = lin;
  anl->of = NULL;
  anl->ob = r->rob;
  anl->Ddown = NULL;
}

static void
lform2 (struct des_lock *a, i4_t n, u2_t els, char *con)
{
  struct des_rel *r;
  register i4_t i;

  r = a->rel;
  if (r->rof == NULL)
    r->rof = a;		/* place the lock into relation locks list */
  if (r->rob != NULL)
    r->rob->of = a;
  r->rob = a;
  if (a->lockin == 'm')
    assert(els==0);
  i = n + els;
  if ( i % 16 )
    i += 16  - i % 16 ;
  if (a->lockin != 'm')
    {
      char *c;
      c = (char *) a + i - els;
      bcopy (con, c, els);
    }
  assert (i % sizeof(i4_t) == 0);
  a->dls = i;
  a->tran->freelb -= i;
  a->tran->firstfree += i;
}

CPNM 
lock(u2_t trnum, struct id_rel rel, COST cost,
     char lin, i4_t totsize, char *lc)
{
  int i;
  char *lastb, *c;
  struct des_tran *tr;
  i4_t rn;
  u2_t arsize, els;
  struct des_rel *r;
  struct des_lock *anl, *fanl;

  for (i = 0; i < TRNUM && artran[i].idtr != trnum; i++);
  tr = artran + i;
  rn = rel.urn.obnum;
  for (r = firstrel; r != NULL; r = r->frellist)
    {
      if (r->idrel.obnum == rn)
	goto m1;
    }
  r = crtsrd (trnum, &rel.urn, rel.pagenum, rel.index);
m1:
  if (lin == 'm')
    {
      u2_t *a2b;
      if ((wlocksize + 2 * size2b) > tr->freelb)
	increase (tr);
      els = t2bunpack (lc);
      lc += size2b;
      els -= size2b;
      anl = (struct des_lock *) tr->firstfree;
      lform1 (anl, tr, r, lin);
      if ((i = shartest (anl, 0, lc, r->rof)) == 1)
	{			/* not shared */
	  lform2 (anl, wlocksize, 2 * size2b, lc);
	  a2b = (u2_t *) ((char *) anl + locksize);
	  *a2b++ = rel.pagenum;
	  *a2b = rel.index;
	}
      else if (i == 2)
	return (0);
      else
	{
	  lform2 (anl, locksize, 2 * size2b, lc);
	  a2b = (u2_t *) ((char *) anl + locksize);
	  *a2b++ = rel.pagenum;
	  *a2b = rel.index;
	  answer_opusk (trnum, (CPNM) 0);
	  return ((CPNM) 0);
	}
    }
  lastb = lc + totsize;
  for (arsize = wlsize, c = lc; c < lastb; c += els - size2b)
    {
      els = t2bunpack (c);
      c +=size2b;
      arsize += locksize + els - size2b;

      if (arsize > tr->freelb)
	increase (tr);
    }
  fanl = (struct des_lock *) tr->firstfree;
  for (; lc < lastb; lc += els)
    {
      els = t2bunpack (lc);
      lc += size2b;
      els -= size2b;
      anl = (struct des_lock *) tr->firstfree;
      lform1 (anl, tr, r, lin);
      if ((i = shartest (anl, els, lc, r->rof)) == 1)
	{  /* not shared, but this transaction isn't a rollback victim */
	  lform2 (anl, wlocksize, els, lc);	
	  tr->pwlock = anl;	
	  ((struct des_wlock *) anl)->newcost = cost;
	  for (lc += els; lc < lastb; lc += els)
	    {
	      els = t2bunpack (lc);
	      lc += size2b;
	      els -= size2b;
	      anl = (struct des_lock *) tr->firstfree;
	      lform1 (anl, tr, r, lin);
	      lform2 (anl, locksize, els, lc);
	    }
	  return ((CPNM) 0);
	}
      if (i == 2)
        return (0);
      lform2 (anl, locksize, els, lc);
    }				/* all elementary locks are satisfyed */
  if (lin == 'w')
    dlock (fanl);
  tr->trcost = cost;

  answer_opusk (trnum, (CPNM) 0);
  return ((CPNM) 0);
}

struct des_rel *
crtsrd ( u2_t trnum,  struct id_ob *udr, u2_t pn, u2_t ind)
{
  i4_t rn;
  u2_t fn, arsize;
  struct des_rel *r;
  char *a = NULL;

  rn = udr->obnum;
  if (rn == RDRNUM)
    fn = 7;
  else
    {
      char *asp;
      struct d_r_bd drbd;
      u2_t segn;
      unsigned char t;
      
      segn = udr->segnum;
      asp = getpage (trnum, segn, pn);
      a = asp + *((u2_t *) (asp + phsize) + ind);
      t = *a & MSKCORT;
      if (t == IND)
	{			/* indirect reference */
	  ind = t2bunpack (a + 1);
	  pn = t2bunpack (a + 1 + size2b);
	  putpage (trnum);
	  asp = getpage (trnum, segn, pn);
	  a = asp + *((u2_t *) (asp + phsize) + ind);
	}
      for (; (*a & EOSC) == 0; a++);
      a++;
      bcopy (a, (char *) &drbd, drbdsize);
      fn = drbd.fieldnum;
    }
  arsize = rfsize * fn;
  r = (struct des_rel *) xmalloc (relsize + arsize);
  r->idrel = *udr;
  r->frellist = firstrel;
  r->brellist = NULL;
  if (firstrel != NULL)
    firstrel->brellist = r;
  firstrel = r;
  r->rof = NULL;
  r->rob = NULL;
  r->rfn = fn;
  if (rn == RDRNUM)
    {
      struct des_field *df, *ldf;
      df = (struct des_field *) (r + 1);
      df->field_type = T4B;
      for (df++, ldf = df + 4; df < ldf; df++)
	df->field_type = T2B;
      for (ldf = df + 2; df < ldf; df++)
	{
	  df->field_type = TCH;
	  df->field_size = BD_PAGESIZE - phsize - size2b;
	}
    }
  else
    {
      bcopy (a + drbdsize, r + 1, arsize);
      putpage (trnum);
    }
  return (r);
}
