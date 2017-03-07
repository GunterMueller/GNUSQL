/*
 *  ordins.c  - Ordinary insertion
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

/* $Id: ordins.c,v 1.251 1998/09/29 21:25:41 kimelman Exp $ */

#include <assert.h>
#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"
#include "xmem.h"

extern struct ldesind **TAB_IFAM;
extern struct ADBL adlj;
extern i4_t ljmsize;
extern u2_t S_SC_S;

static u2_t wild_pn = (u2_t) ~0;

ARR_DECL(l_emp,u2_t,static); /* declaration of local dynamic stack/array 'l_emp'.*/
ARR_PROC_DECL(l_emp,u2_t,static); /* declaration of routines for it              */

static u2_t
along_leaf_pages (struct A *pg, u2_t necessary_size, char *pnt, u2_t n,
                  u2_t *pfms, u2_t *loc, char *key)
{
  u2_t pn, table_pn, ksz = size4b +1, sn;
  struct ind_page *indph;
  char *asp = NULL, *lastb;

  asp = pg->p_shm;
  sn = pg->p_sn;
  for (;; pnt += ksz)
    {
      indph = (struct ind_page *) asp;
      lastb = asp + indph->ind_off;
      for (; n != 0; n--)
        {
          table_pn = t2bunpack (pnt);
          pnt += size2b;
          *pfms = t2bunpack (pnt);
          pnt += size2b;
          if (*pfms >= necessary_size)
            {
              *loc = pnt - size2b - asp;
              return (table_pn);
            }
        }
      if (pnt < lastb)
        {
          putpg(pg, 'n');
          return (wild_pn);
        }
      pn = indph->ind_nextpn;
      putpg(pg, 'n'); /* make asp & indph etc undefined */
      if (pn == wild_pn)
        return wild_pn;
      while ((asp = getpg (pg, sn, pn, 's')) == NULL);
      pnt = asp + indphsize;
      n = t2bunpack (pnt);
      pnt += size2b;
      if ((t4bunpack (pnt + 1) - t4bunpack (key)) != 0)
        {
          putpg(pg, 'n');
          return (wild_pn);
        }
    }
}

static u2_t
find_suit_page (struct A *pg, u2_t sn, char *key, u2_t necessary_size,
                u2_t *pfms, u2_t *loc)
{
  char *aggr = NULL, *a, *lastb, *asp = NULL;
  u2_t pn, n = 0;
  i4_t ksz = size4b + 1, elsz = 2*size2b, l = 0;
  struct ind_page *indph;
  
  pn = S_SC_S;     /* rootpn for Allocation Memory Index */
  for (;;)
    {
      while ((asp = getpg (pg, sn, pn, 's')) == NULL);
      indph = (struct ind_page *) asp;
      lastb = asp + indph->ind_off;
      for (a = asp + indphsize; a < lastb; a += ksz + n * elsz)
        {
          aggr = a;
          n = t2bunpack (a);
          a += size2b;
          if ((l = t4bunpack (a + 1) - t4bunpack (key)) >= 0)
            break;
        }
      if (a == lastb)
        {
          putpg(pg, 'n');
          return (wild_pn);
        }
      if (indph->ind_wpage == LEAF)
        break;
      pn = t2bunpack (aggr + size2b + ksz + size2b);     /* pn down */
      putpg(pg, 'n');
    }
  if (l != 0)
    {
      putpg(pg, 'n');
      return (wild_pn);
    }
  a += ksz;
  return (along_leaf_pages (pg, necessary_size, a, n, pfms, loc, key));
}

static u2_t
cont_search_suit_page (struct A *pg, char *key, u2_t necessary_size,
                       u2_t *pfms, u2_t *loc)
{
  char *aggr = NULL, *a, *lastb, *asp;
  u2_t n = 0;
  i4_t ksz = size4b + 1, elsz = 2*size2b, l = 0;
  
  asp = pg->p_shm;
  lastb = asp + ((struct ind_page *) asp)->ind_off;
  for (a = asp + indphsize; a < lastb; a += ksz + n * elsz)
    {
      aggr = a;
      n = t2bunpack (a);
      a += size2b;
      if ((l = t4bunpack (a + 1) - t4bunpack (key)) >= 0)
        break;
    }
  assert (a != lastb);
  if (l != 0)
    {
      putpg(pg, 'n');
      return (wild_pn);
    }
  a = asp + *loc + size2b;
  n -= (a - (aggr + size2b + ksz)) / elsz; 
  return (along_leaf_pages (pg, necessary_size, a, n, pfms, loc, key));
}

static void
mod_free_size (struct A *pg, u2_t offloc, u2_t size)
{
  char *asp, *a;

  while (BUF_enforce (pg->p_sn, pg->p_pn) < 0);
  asp = pg->p_shm;
  a = asp + offloc;
  begmop (asp);
  recmjform (OLD, pg, offloc, size2b, a, 0);
  MJ_PUTBL ();
  t2bpack (size, a);
  putpg (pg, 'm');
}

static void
free_pages (u2_t sn, i4_t rn, i4_t i)
{
  u2_t empn;
  i4_t i1;
  char *a;
  char key[size4b + 1], key2[size2b];

  a = key;
  *a++ = BITVL(0) | EOSC;
  t4bpack (rn, a);
  for (i1 = i; i1 < l_emp.count; i1++)
    {
      empn = l_emp.arr[i1];
      t2bpack (empn, key2);
      icp_rem (TAB_IFAM[sn], key, key2, size2b);
      /*      delrec (TAB_IFAM[sn], rn, empn);*/
      emptypg (sn, empn, 'f');
    }
  BUF_unlock (sn, l_emp.count - i , l_emp.arr + i);
  l_emp_ini();
}

struct des_tid
ordins (struct id_rel *pidrel, char *cort, u2_t corsize, char type)
{
  struct page_head *ph;
  u2_t pn, pfms, offloc, sn;
  i2_t delta;
  i4_t rn;
  struct des_tid tid;
  char *asp = NULL;
  struct A pg, pg_ifam;
  char key[size4b];
  
  sn = pidrel->urn.segnum;
  rn = pidrel->urn.obnum;
  delta = corsize + size2b;
  tab_difam (sn);
  l_emp_ini_check();
  t4bpack (rn, key);
  pn = find_suit_page (&pg_ifam, sn, key, delta, &pfms, &offloc);
m1:
  if (pn != wild_pn)
    {
      u2_t lind, *ai, ind, fs;

      while ((asp = getpg (&pg, sn, pn, 'x')) == NULL);
      lind = ((struct page_head *) asp)->lastin;
      fs = *((u2_t *) (asp + phsize) + lind) - (phsize + size2b * (lind + 1));
      assert (fs <= BD_PAGESIZE - phsize);
      if (fs < pfms)
        {
          i4_t i;
          if ((i = testfree (asp, fs, delta)) == 1)
            {			/* this page is empty */
              putwul (&pg, 'n');
              l_emp_put (pn);
              pn = cont_search_suit_page (&pg_ifam, key, delta, &pfms, &offloc);
              goto m1;
            }
          else if (i == -1)    /* no allocation */
            {
              putpg (&pg, 'n');
              pn = cont_search_suit_page (&pg_ifam, key, delta, &pfms, &offloc);
              goto m1;
            }
        }
      begmop (asp);
      if (fs < pfms)
        rempbd (&pg);
      ai = (u2_t *) (asp + phsize);
      for (ind = 0; ind <= lind && *ai != 0; ai++, ind++);
      tid.tindex = ind;
      tid.tpn = pn;
      if (type == 'w')
        {
          if (rn == RDRNUM)
            {
              struct id_rel idr;
              idr = *pidrel;
              idr.pagenum = pn;
              idr.index = ind;
              pidrel = &idr;
            }
          wmlj (INSLJ, ljmsize + corsize, &adlj, pidrel, &tid, 0);
        }
      inscort (&pg, ind, cort, corsize);
      MJ_PUTBL ();
      putpg (&pg, 'm');
      mod_free_size (&pg_ifam, offloc, pfms - corsize - size2b);
      if (l_emp.count != 0)
        free_pages (sn, rn, 0);
    }
  else
    {
      u2_t size;
      if (l_emp.count != 0)
        {
          pn = l_emp.arr[0];
          asp = getwl (&pg, sn, pn);
        }
      else
        {
          pn = getempt (sn);
          asp = getnew (&pg, sn, pn);
        }
      tid.tpn = pn;
      tid.tindex = 0;
      if (type == 'w')
        {
          if (rn == RDRNUM)
            {
              struct id_rel idr;
              idr = *pidrel;
              idr.pagenum = pn;
              idr.index = 0;
              pidrel = &idr;
            }
          wmlj (INSLJ, ljmsize + corsize, &adlj, pidrel, &tid, 0);
        }
      size = BD_PAGESIZE - corsize;
      bcopy (cort, asp + size, corsize);
      ph = (struct page_head *) asp;
      ph->lastin = 0;
      t2bpack (size, asp + phsize);
      ++ph->ph_ph.idmod;
      if (l_emp.count != 0)
        {
          struct id_ob fullrn;
          putpg (&pg, 'm');
          fullrn.segnum = sn;
          fullrn.obnum = rn;
          modrec (&fullrn, pn, -(corsize + size2b));
          free_pages (sn, rn, 1);
        }
      else
        {
          putwul (&pg, 'm');
          insrec (TAB_IFAM[sn], rn, pn, size - phsize - size2b);
        }
    }
  l_emp_ini();
  return (tid);
}
