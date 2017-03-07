/*
 *  ind_ins.c  - Index Control Programm
 *               functions dealing with insertion into an index
 *               Kernel of GNU SQL-server 
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

/* $Id: ind_ins.c,v 1.253 1998/09/29 21:25:31 kimelman Exp $ */

#include "xmem.h"
#include <assert.h>
#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

struct des_field *d_f, *d_f2;
u2_t *afn;
u2_t k_n;
u2_t seg_n;
i4_t k2sz;
i4_t inf_size;
u2_t uniq_key;

ARR_DECL     (thread_s,u2_t,no_static) /* declaration of dynamic stack/array 'thread_s'. */
ARR_PROC_DECL(thread_s,u2_t,no_static) /* declaration of routines for it                 */

#define FIND_LAST_KEY(asp,elsz,lkey,lkey2,keysz)\
{\
  char *pnt, *lastb;\
  u2_t n;\
  lastb = asp + ((struct ind_page *) asp)->ind_off;\
  pnt = asp + indphsize;\
  assert(pnt < lastb);\
  while (pnt < lastb)\
    {\
      n = t2bunpack (pnt);\
      pnt += size2b;\
      lkey = pnt;\
      keysz = kszcal (lkey, afn, d_f);\
      pnt += keysz + n * elsz;\
    }\
  lkey2 = pnt - elsz;\
}

int
check_ind_page (char *asp)
{
  char *cur_key, *prev_key, *a, *lastb;
  u2_t n;
  i4_t ksz, elsz;
  struct ind_page *indph;

  indph = (struct ind_page *) asp;
  if (indph->ind_wpage == LEAF)
    elsz = k2sz + inf_size;
  else
    elsz = k2sz + size2b;
  lastb = asp + indph->ind_off;
  a = asp + indphsize;
  n = t2bunpack (a);
  a += size2b;
  prev_key = a;
  ksz = kszcal (prev_key, afn, d_f);
  a += ksz + n * elsz;
  while (a < lastb)
    {
      n = t2bunpack (a);
      a += size2b;
      cur_key = a;
      ksz = kszcal (cur_key, afn, d_f);
      if (cmpkeys (k_n, afn, d_f, cur_key, prev_key) <= 0)
        return -1;
      a += ksz + n * elsz;
      prev_key = cur_key;
    }
  return 0;
}

static int
insrep (char *asp, char *lastb, char *key, char *key2,
        i4_t elsz, char **bbeg, char **loc)
{
  char *a, *ckey;
  u2_t n;
  i4_t agsz = 0, keysz, ksz, l, l2;

  keysz = kszcal (key, afn, d_f);
  a = asp + indphsize;
  for (; a < lastb; a += agsz)
    {
      *bbeg = a;
      n = t2bunpack (a);
      assert (n < BD_PAGESIZE / 2);
      ckey = a + size2b;
      ksz = kszcal (ckey, afn, d_f);
      if ((l = cmpkeys (k_n, afn, d_f, ckey, key)) == 0)
	{
	  if (uniq_key == UNIQ)
	    return (0);
	  a += size2b + ksz;
	  for (; n != 0; n--, a += elsz)
            {
              if ((l2 = cmp2keys (d_f2->field_type, a, key2)) > 0)
                break;
              assert (l2 != 0);      /* Tne second key isn't unique */
            }
	  *loc = a;
	  return (elsz);
	}
      else if (l > 0)
	{
	  *loc = a;
	  return (size2b + keysz + elsz);
	}
      agsz = size2b + ksz + n * elsz;
    }
  *bbeg = a;
  *loc = a;
  return (size2b + keysz + elsz);
}

static void
icp_insrec (char *key, char *key2, char *inf, i4_t infsz, char *beg,
            char *loc, char *lastb, struct A *pg)
{
  char *src, *dst;
  u2_t fs;
  i4_t sz;

  fs = lastb - loc;
  if (beg == loc)
    {
      i4_t keysz;
      keysz = kszcal (key, afn, d_f);
      sz = size2b + keysz + k2sz + infsz;
      if (pg != NULL)
	recmjform (SHF, pg, loc - pg->p_shm, fs, NULL, -sz);
      for (src = lastb - 1, dst = src + sz; fs != 0; fs--)
	*dst-- = *src--;
      t2bpack (1, beg);
      loc += size2b;
      bcopy (key, loc, keysz);
      loc += keysz;
    }
  else
    {
      sz = k2sz + infsz;
      if (pg != NULL)
	recmjform (SHF, pg, loc - pg->p_shm, fs, NULL, -sz);
      for (src = lastb - 1, dst = src + sz; fs != 0; fs--)
	*dst-- = *src--;
      if (pg != NULL)
	recmjform (OLD, pg, beg - pg->p_shm, size2b, beg, 0);
      mod_nels (1, beg);
    }
  bcopy (key2, loc, k2sz);
  bcopy (inf, loc + k2sz, infsz);
}

static int
repet1 (char *asp, char *bbeg, char *a, i4_t elsz, char *middleb, char **lkey,
        char **lkey2, char **lnkey, char **lnkey2, i4_t *n1, u2_t *nels,
        i4_t *lbeg, i4_t *lnbeg, i4_t *pr, char *frkey)
{
  char *lastb, *ckey;
  i4_t ksz, agsz, n2;
  u2_t n;

  lastb = asp + ((struct ind_page *) asp)->ind_off;
  ckey = *lnkey;
  n = t2bunpack (bbeg);
  assert (n < BD_PAGESIZE / 2);
  ksz = kszcal (bbeg + size2b, afn, d_f);
  if (bbeg != a)
    {				/* not new element */
      char *b;
      b = bbeg;
      b += size2b + ksz;
      for (; b != a; n--)
	b += elsz;
      for (; n != 0; n--)
	if (a + elsz < middleb)
	  {
	    *lnkey2 = a;
	    a += elsz;
	  }
	else
	  goto m1;
    }
  for (; a < lastb;)
    {
      bbeg = a;
      n = t2bunpack (a);
      assert (n < BD_PAGESIZE / 2);
      ksz = kszcal (a + size2b, afn, d_f);
      agsz = size2b + ksz + n * elsz;
      if (a + agsz < middleb)
	{
	  *lnkey = a + size2b;
	  a += agsz;
	  *lnkey2 = a - elsz;
	}
      else
	break;
    }
  ckey = a + size2b;
  agsz = size2b + ksz + elsz;
  if (a + agsz < middleb)
    {
      *lnkey = a + size2b;
      a += agsz;
      *lnkey2 = a - elsz;
      for (n--; n != 0; n--)
	if (a + elsz < middleb)
	  {
	    *lnkey2 = a;
	    a += elsz;
	  }
	else
	  break;
    }
  else
    n = 0;
m1:
  n2 = lastb - a;
  *n1 = a - asp;
  *lnbeg = bbeg - asp;
  if (n != 0)
    {
      ksz = kszcal (*lnkey, afn, d_f);
      n2 += size2b + ksz;
      *nels = n;
      a += n * elsz;
    }
  else
    *nels = 0;
  for (; a < lastb; a += ksz + n * elsz)
    {
      bbeg = a;
      n = t2bunpack (a);
      assert (n < BD_PAGESIZE / 2);
      a += size2b;
      ckey = a;
      ksz = kszcal (a, afn, d_f);
    }
  *lbeg = a - bbeg;
  if (frkey != NULL) {
    if (cmpkeys (k_n, afn, d_f, ckey, frkey) == 0)
      {
	*pr = 0;
	n2 -= ksz + size2b;
      }
    else
      *pr = 1;
  }
  *lkey = ckey;
  *lkey2 = lastb - elsz;
  return (n2);
}

static int
repttn (char *mas, char *lastb, i4_t elsz, char *middleb, char **lkey,
        char **lkey2, char **lnkey, char **lnkey2, i4_t *n1,
        u2_t *nels, i4_t *lbeg, i4_t *lnbeg, i4_t *pr, char *frkey)
{
  char *a, *bbeg, *ckey;
  i4_t agsz = 0, ksz, n2;
  u2_t n;

  a = mas;
  do      
    {
      n = t2bunpack (a);
      assert (n < BD_PAGESIZE / 2);
      bbeg = a - agsz;
      ksz = kszcal (a + size2b, afn, d_f);
      agsz = size2b + ksz + n * elsz;
      if (a + agsz > middleb)
	break;
      a += agsz;
    }
  while( a < lastb);
  *lnkey = bbeg + size2b;
  *lnkey2 = a - elsz;
  *lnbeg = bbeg - mas;
  bbeg = a;
  agsz = size2b + ksz + elsz;
  if (a + agsz < middleb)
    {
      *lnkey = a + size2b;
      *lnbeg = a - mas;
      a += agsz;
      *lnkey2 = a - elsz;
      for (n--; n != 0; n--)
	if (a + elsz < middleb)
	  {
	    *lnkey2 = a;
	    a += elsz;
	  }
	else
	  break;
    }
  else
    n = 0;
  *n1 = a - mas;
  n2 = lastb - a;
  if (n != 0)
    {
      n2 += size2b + ksz;
      *nels = n;
      a += n * elsz;
    }
  else
    *nels = 0;
  for (; a < lastb; a += ksz + n * elsz)
    {
      bbeg = a;
      n = t2bunpack (a);
      assert (n < BD_PAGESIZE / 2);
      a += size2b;
      ksz = kszcal (a, afn, d_f);
    }
  ckey = bbeg + size2b;
  *lbeg = a - bbeg;
  if (frkey != NULL)
    {
      if (cmpkeys (k_n, afn, d_f, ckey, frkey) == 0)
	{
	  *pr = 0;
	  n2 -= ksz + size2b;
	}
      else
	*pr = 1;
    }
  *lkey = ckey;
  *lkey2 = lastb - elsz;
  return (n2);
}

static char *
pereliv (char *asp, i4_t n, u2_t nels, char *key, char *mas, i4_t massz)
                /* n - a number of bytes from the first page */
     		/* nels - element number from the first page */
{
  char *a, *b, *c, *lastb;

  lastb = asp + ((struct ind_page *) asp)->ind_off;
  for (b = lastb - 1, a = b + n, c = asp + indphsize; b >= c;)
    *a-- = *b--;
  a = asp + indphsize;
  if (nels != 0)
    {
      i4_t ksz;
      t2bpack (nels, a);
      a += size2b;
      ksz = kszcal (key, afn, d_f);
      bcopy (key, a, ksz);
      a += ksz;
    }
  bcopy (mas, a, massz);
  return (a + massz);
}

static void
per22 (u2_t pn, u2_t rbrpn, i4_t n1, i4_t n2, u2_t nels, i4_t lbeg, i4_t lnbeg,
       i4_t pr, i4_t prpg, i4_t sz, char *key, char *key2, char *inf, char *lnkey,
       i4_t offbeg, i4_t offloc)         /* if pr==0 then lastkey==frkey */
{
  char *a, *lastb, *asp, *aspr, *end1;
  u2_t fnels;  /* element number of the first aggregate of a right brother */
  struct ind_page *indph, *indphr;
  i4_t idm, idmr;
  struct A pg, pgr;

  asp = getwl (&pg, seg_n, pn);	/* get pn without lock */
  aspr = getwl (&pgr, seg_n, rbrpn);	/* get pn without lock */
  indph = (struct ind_page *) asp;
  indphr = (struct ind_page *) aspr;
  idm = ++((struct p_head *) asp)->idmod;
  idmr = ++((struct p_head *) aspr)->idmod;
  if (pr == 0)
    recmjform (OLD, &pgr, indphsize, size2b, aspr + indphsize, 0);
  fnels = t2bunpack (aspr + indphsize);
  if (nels != 0)
    {
      recmjform (OLD, &pg, lnbeg, size2b, asp + lnbeg, 0);
      mod_nels ((-nels), asp + lnbeg);
    } 
  lastb = asp + indph->ind_off;
  end1 = asp + n1;
  recmjform (OLD, &pg, n1, lastb - end1, end1, 0);
  recmjform (SHF, &pgr, n2 + indphsize,
             indphr->ind_off - indphsize, NULL, -n2);
  a = pereliv (aspr, n2, nels, lnkey, end1, indph->ind_off - n1);
  if (pr == 0)
    {
      if (lbeg > n2)
	lbeg = n2;
      mod_nels (fnels, a - lbeg);
      if (lbeg == n2)
	{
          char *b;
          i4_t size;
	  b = a + size2b + kszcal (aspr + indphsize + size2b, afn, d_f);
          size = aspr + indphr->ind_off + n2 - b;
          bcopy (b, a, size);
	}
    }
  if (prpg == 1)
    {		/* An insertion in the first page */
      icp_insrec (key, key2, inf, inf_size, asp + offbeg,
                  asp + offloc, end1, &pg);
      n1 += sz;
    }
  else		/* An insertion in the right brother page */
    icp_insrec (key, key2, inf, inf_size, aspr + offbeg,
                aspr + offloc, a, &pgr);
  a = (char *) &indph->ind_off;
  recmjform (OLD, &pg, a - asp, size2b, a, 0);
  indph->ind_off = n1;
  a = (char *) &indphr->ind_off;
  recmjform (OLD, &pgr, a - aspr, size2b, a, 0);
  indphr->ind_off += n2;
  assert (check_ind_page (asp) == 0);
  putwul (&pg, 'm');		/* put page without unlock */
  assert (check_ind_page (aspr) == 0);
  putwul (&pgr, 'm');
}

static int
rep_2_3 (char *mas, i4_t massz, i4_t elsz, char *aspr, i4_t *n1, u2_t * nels,
         i4_t *lbeg, i4_t *lnbeg, char **nkey, char **nkey2, i4_t *pr)
{
  char *middleb, *frkey, *lkey, *lkey2, *lnkey, *lnkey2;
  i4_t off, n2, middle;

  off = ((struct ind_page *) aspr)->ind_off;
  middleb = mas + (massz + off - indphsize) / 3;
  frkey = aspr + indphsize + size2b;
  n2 = repttn (mas, mas + massz, elsz, middleb, &lkey, &lkey2, &lnkey,
               &lnkey2, n1, nels, lbeg, lnbeg, pr, frkey);
  middle = (n2 + off - indphsize) / 2;
  if (off - middle > indphsize)
    {
      char *a;
      u2_t n, agsz = 0;
      middleb = aspr + ((n2 + off) / 2 - n2);
      for (a = aspr + indphsize; a + agsz < middleb;)
	{
	  a += agsz;
	  n = t2bunpack (a);
          assert (n < BD_PAGESIZE / 2);
	  a += size2b;
	  agsz = kszcal (a, afn, d_f) + n * elsz;
	}
      *nkey = a;
      *nkey2 = a + agsz - elsz;
      n2 = a + agsz - aspr;
      if (n2 == indphsize)
	n2 = 0;
    }
  else
    n2 = 0;
  return (n2);
}

static int
crnlev (struct A *pg, char *mas, char *lastb, i4_t wpage)
{			/* a creation of a new level */
  char *a, *b, *middleb, *asp;
  char *lnkey, *lnkey2, *lkey, *lkey2, *inf1, *inf2;
  i4_t n1, lbeg, lnbeg, pr, ksz, size, elsz;
  u2_t newpn1, newpn2, nels, offn;
  struct A pgn;
  struct ind_page *indph;

  if (wpage == LEAF)
    elsz = k2sz + inf_size;
  else
    elsz = k2sz + size2b;
  middleb = mas + (lastb - mas) / 2;
  repttn (mas, lastb, elsz, middleb, &lkey, &lkey2, &lnkey,
          &lnkey2, &n1, &nels, &lbeg, &lnbeg, &pr, NULL);
  if (lenforce ()< 0)
    {
      putwul (pg, 'n');
      return (-1);
    }
  newpn1 = getempt (seg_n);
  asp = getnew (&pgn, seg_n, newpn1);	/* get new page */
  indph = (struct ind_page *) asp;
  bcopy (mas, asp + indphsize, n1);
  indph->ind_off = n1 + indphsize;
  if (nels != 0)
    mod_nels ((-nels), asp + lnbeg + indphsize);
  newpn2 = getempt (seg_n);
  indph->ind_nextpn = newpn2;
  indph->ind_wpage = wpage;
  offn = indph->ind_off;
  assert (check_ind_page (asp) == 0);
  putwul (&pgn, 'm');
  asp = getnew (&pgn, seg_n, newpn2);	/* get new page */
  indph = (struct ind_page *) asp;
  a = asp + indphsize;
  ksz = kszcal (lnkey, afn, d_f);
  if (nels != 0)
    {
      /*      write_key (nels, a, lnkey, ksz);*/
      t2bpack (nels, a);
      a += size2b;
      bcopy (lnkey, a, ksz);
      a += ksz;
    }
  b = mas + n1;
  size = lastb - b;
  bcopy (b, a, size);
  a += size;
  indph->ind_nextpn = (u2_t) ~ 0;
  indph->ind_off = a - asp;
  indph->ind_wpage = wpage;
  offn = indph->ind_off;
  assert (check_ind_page (asp) == 0);
  putwul (&pgn, 'm');
  inf1 = (char *) &newpn1;
  inf2 = (char *) &newpn2;
  asp = pg->p_shm;
  indph = (struct ind_page *) asp;
  ++((struct p_head *) asp)->idmod;
  a = asp + sizeof (struct p_head);
  b = asp + indph->ind_off;
  recmjform (OLD, pg, a - asp, b - a, a, 0);
  a = asp + indphsize + size2b;
  bcopy (lnkey, a, ksz);
  a += ksz;
  bcopy (lnkey2, a, k2sz);
  a += k2sz;
  t2bpack (newpn1, a);
  a += size2b;
  if (cmpkeys (k_n, afn, d_f, lkey, lnkey) == 0)
    nels = 2;
  else
    {
      nels = 1;
      t2bpack (nels, a);
      a += size2b;
      ksz = kszcal (lkey, afn, d_f);
      bcopy (lkey, a, ksz);
      a += ksz;
    }
  t2bpack (nels, asp + indphsize);
  bcopy (lkey2, a, k2sz);
  a += k2sz;
  t2bpack (newpn2, a);
  a += size2b;
  indph->ind_nextpn = (u2_t) ~ 0;
  indph->ind_off = a - asp;
  indph->ind_wpage = IROOT;
  assert (check_ind_page (asp) == 0);
  putwul (pg, 'm');
  return (0);
}

static i4_t rbrup (u2_t pn, u2_t rbrpn, char *mas, i4_t massz,
                  char *last_key, char *last_k2);

static char *
findkey (char *asp, char *key, char *key2, i4_t elsz, char **loc)
{
  char *a, *lastb, *bbeg;
  u2_t n;
  i4_t ksz;

  lastb = asp + ((struct ind_page *) asp)->ind_off;
  for (a = asp + indphsize; a < lastb; a += ksz + n * elsz)
    {
      bbeg = a;
      n = t2bunpack (a);
      a += size2b;
      ksz = kszcal (a, afn, d_f);
      if (cmpkeys (k_n, afn, d_f, a, key) == 0)
	{
	  a += ksz;
	  for (; n != 0; n--, a += elsz)
	    if (cmp2keys (d_f2->field_type, a, key2) == 0)
	      {
		*loc = a;
		return (bbeg);
	      }
	}
    }
  return (NULL);
}

static int
div_upmod (char *lkey, char *lkey2, char *lnkey,
           char *lnkey2, u2_t pn, u2_t * newpn)
{
  char *asp, *a, *inf, *lastb;
  char *beg, *loc;
  i4_t elsz, insz, off;
  struct ind_page *indph;
  struct A pg;
  char ninf[size2b];

  t2bpack (pn, ninf);
  pn = *(--thread_s.u);
  asp = getwl (&pg, seg_n, pn);	/* get pn without lock */
  indph = (struct ind_page *) asp;
  elsz = k2sz + size2b;
  beg = findkey (asp, lkey, lkey2, elsz, &loc);
  inf = loc + k2sz;
  off = indph->ind_off;
  lastb = asp + off;
  insz = insrep (asp, lastb, lnkey, lnkey2, elsz, &beg, &loc);
  assert (insz != 0);
  if (off + insz > BD_PAGESIZE)
    {
      char *mas, *d;
      i4_t massz, ans;
      struct A psevdo_page;

      psevdo_page.p_shm = NULL;
      massz = off + insz - indphsize;
      mas = (char *) xmalloc (massz);
      d = asp + indphsize;
      bcopy (d, mas, off - indphsize);
      *newpn = getempt (seg_n);
      t2bpack (*newpn, mas + (inf - d));
      beg = mas + (beg - d);
      loc = mas + (loc - d);
      lastb = mas + massz - insz;
      icp_insrec (lnkey, lnkey2, ninf, size2b, beg, loc, lastb, &psevdo_page);
      if (indph->ind_wpage != IROOT)
	{
          char last_key[BD_PAGESIZE];
          char last_k2[BD_PAGESIZE];
          u2_t rbrpn, keysz = 0 ;
          
          FIND_LAST_KEY (asp, elsz, lkey, lkey2, keysz);
          assert (keysz != 0 );
          bcopy (lkey, last_key, keysz);
          bcopy (lkey2, last_k2, k2sz);
          rbrpn = indph->ind_nextpn;
	  putwul (&pg, 'n');
	  ans = rbrup (pn, rbrpn, mas, massz, last_key, last_k2);
	}
      else
        ans = crnlev (&pg, mas, lastb + insz, BTWN);
      if (ans < 0)
	emptypg (seg_n, *newpn, 'n');
      xfree ((void *) mas);
      return (ans);
    }
  upunlock ();
  if (lenforce ()< 0)
    {
      putwul (&pg, 'n');
      return (-1);
    }
  *newpn = getempt (seg_n);
  ++((struct p_head *) asp)->idmod;
  recmjform (OLD, &pg, inf - asp, size2b, inf, 0);
  t2bpack (*newpn, inf);
  icp_insrec (lnkey, lnkey2, ninf, size2b, beg, loc, lastb, &pg);
  a = (char *) &indph->ind_off;
  recmjform (OLD, &pg, a - asp, size2b, a, 0);
  indph->ind_off += insz;
  assert (check_ind_page (asp) == 0);
  putwul (&pg, 'm');
  return (0);
}

static int
divsn (u2_t pn, char *mas, i4_t massz, u2_t nextpn,
       i4_t wpage, char *last_key, char *last_key2)
{
  char *lastb, *a, *b, *middleb, *asp;
  i4_t n1, lbeg, lnbeg, pr, elsz;
  char *lkey, *lkey2, *lnkey, *lnkey2;
  u2_t nels, newpn;
  struct ind_page *indph;
  struct A pg;

  lastb = mas + massz;
  middleb = mas + massz / 2;
  if (wpage == LEAF)
    elsz = k2sz + inf_size;
  else
    elsz = k2sz + size2b;
  repttn (mas, lastb, elsz, middleb, &lkey, &lkey2, &lnkey, &lnkey2,
          &n1, &nels, &lbeg, &lnbeg, &pr, (char *) NULL);
  if (div_upmod (last_key, last_key2, lnkey, lnkey2, pn, &newpn) < 0)
    return (-1);
  asp = getwl (&pg, seg_n, pn);	/* get pn without lock */
  indph = (struct ind_page *) asp;
  ++((struct p_head *) asp)->idmod;
  a = asp + sizeof (struct p_head);
  b = asp + indph->ind_off;
  recmjform (OLD, &pg, a - asp, b - a, a, 0);
  bcopy (mas, asp + indphsize, n1);
  if (nels != 0) 
    mod_nels ((-nels), asp + lnbeg);
  indph->ind_off = n1 + indphsize;
  indph->ind_nextpn = newpn;
  assert (check_ind_page (asp) == 0);
  putwul (&pg, 'm');
  asp = getnew (&pg, seg_n, newpn);	/* get newpn without lock */
  indph = (struct ind_page *) asp;
  lkey = lastb - lbeg + size2b;
  indph->ind_off = indphsize;
  b = pereliv (asp, 0, nels, lkey, mas + n1, massz - n1);
  indph->ind_off = b - asp;
  indph->ind_nextpn = nextpn;
  indph->ind_wpage = wpage;
  assert (check_ind_page (asp) == 0);
  putwul (&pg, 'm');		/* put page without unlock */
  return (0);
}

static int
mlrep1 (char *asp, char *key, char *key2, char *nkey, i4_t *orbeg,
        i4_t *orloc, i4_t *oibeg, i4_t *oiloc, i4_t *sz)
{
  i4_t remsz, elsz, offbef;
  char *rbeg, *rloc;

  elsz = k2sz + size2b;
  remsz = remrep (asp, key, key2, elsz, &rbeg, &rloc, &offbef);
  *orbeg = rbeg - asp;
  *orloc = rloc - asp;
  if (cmpkeys (k_n, afn, d_f, key, nkey) == 0)
    {
      *oibeg = *orbeg;
      *oiloc = *orloc;
      *sz = remsz;
    }
  else
    {
      char *a;
      u2_t n;
      a = asp + offbef;
      *oibeg = offbef;
      n = t2bunpack (a);
      assert (n < BD_PAGESIZE / 2);
      a += size2b;
      *oiloc = *oibeg;
      if (cmpkeys (k_n, afn, d_f, a, nkey) == 0)
        {
          *oiloc += size2b + kszcal (nkey, afn, d_f) + n * elsz;
          *sz = elsz;
        }
      else
        {
          *oibeg = *oiloc = *orbeg;
          *sz = size2b + kszcal (nkey, afn, d_f) + elsz;
        }
    }
  return (remsz);
}

static int
upmod_2_3 (char *lkey, char *lkey2, char *lnkey, char *lnkey2,
           char *nkey, char *nkey2, u2_t * newpn)
{ /* a repetition of a modification of an uplevel for a division 2 on 3 */
  char *a, *asp, *lastb;
  u2_t uppn;
  struct ind_page *indph;
  struct A pg;
  i4_t rbeg, rloc, ibeg1, iloc1, ibeg2, iloc2;
  i4_t remsz, sz1, sz2, elsz, ksz_lnkey, ksz_nkey, off, dopsz;
  char inf1[size2b], inf2[size2b], *beg, *loc;

  uppn = *(--thread_s.u);
  asp = getwl (&pg, seg_n, uppn);	/* get pn without lock */
  indph = (struct ind_page *) asp;
  elsz = k2sz + size2b;
  remsz = mlrep1 (asp, lkey, lkey2, lnkey, &rbeg, &rloc, &ibeg1, &iloc1, &sz1);
  ksz_nkey = kszcal (nkey, afn, d_f);
  ksz_lnkey = kszcal (lnkey, afn, d_f);
  if (cmpkeys (k_n, afn, d_f, lnkey, nkey) == 0)
    {
      sz2 = elsz;
      ibeg2 = ibeg1;
      if (ibeg1 == iloc1)
        iloc2 = ibeg1 + sz1 + elsz;
      else
        iloc2 = iloc1 + elsz;
    }
  else
    {
      sz2 = size2b + ksz_nkey + elsz;
      ibeg2 = iloc2 = iloc1 + sz1;
    }
  bcopy (asp + rloc + k2sz, inf1, size2b);
  off = indph->ind_off;
  dopsz = sz1 + sz2 - remsz;
  if (off + dopsz > BD_PAGESIZE)
    {
      char *mas;
      i4_t massz, ans, size;

      lastb = asp + off;
      massz = off + dopsz - indphsize;
      mas = (char *) xmalloc (massz);
      size = off - indphsize;
      bcopy (asp + indphsize, mas, size);
      lastb = mas + size;
      beg = mas + rbeg - indphsize;
      loc = mas + rloc - indphsize; 
      icp_remrec (beg, loc, remsz, lastb, elsz, NULL);
      lastb -= remsz;
      beg = mas + ibeg1 - indphsize;
      loc = mas + iloc1 - indphsize;
      icp_insrec (lnkey, lnkey2, inf1, size2b, beg, loc, lastb, NULL);
      lastb += sz1;
      *newpn = getempt (seg_n);
      beg = mas + ibeg2 - indphsize;
      loc = mas + iloc2 - indphsize;
      t2bpack (*newpn, inf2);
      icp_insrec (nkey, nkey2, inf2, size2b, beg, loc, lastb, NULL);
      if (indph->ind_wpage != IROOT)
	{
          char last_key[BD_PAGESIZE];
          char last_k2[BD_PAGESIZE];
          u2_t rbrpn, keysz = 0 ;
          
          if (rloc + elsz != off)     /* lkey isn't the last key in pn */
            {
              FIND_LAST_KEY (asp, elsz, lkey, lkey2, keysz);
            }
          else
            keysz = kszcal (lkey, afn, d_f);
          assert ( keysz != 0 ) ;
          bcopy (lkey, last_key, keysz);
          bcopy (lkey2, last_k2, k2sz);
	  rbrpn = indph->ind_nextpn;
	  putwul (&pg, 'n');
	  ans = rbrup (uppn, rbrpn, mas, massz, last_key, last_k2);
	}
      else
        ans = crnlev (&pg, mas, mas + massz, BTWN);
      if (ans < 0)
	emptypg (seg_n, *newpn, 'n');
      xfree ((void *) mas);
      return (ans);
    }
  else if (rloc + elsz == off && indph->ind_wpage != IROOT)
    {
      putwul (&pg, 'n');
      if (mlreddi (lkey, lkey2, nkey, nkey2, uppn) < 0)
        return (-1);
      asp = getwl (&pg, seg_n, uppn);
    }
  else
    {
      upunlock ();
      if (lenforce ()< 0)
	{
	  putwul (&pg, 'n');
	  return (-1);
	}
    }
  *newpn = getempt (seg_n);
  ++((struct p_head *) asp)->idmod;
  lastb = asp + off;
  icp_remrec (asp + rbeg, asp + rloc, remsz, lastb, elsz, &pg);
  lastb -= remsz;
  beg = asp + ibeg1;
  loc = asp + iloc1;
  icp_insrec (lnkey, lnkey2, inf1, size2b, beg, loc, lastb, &pg);
  lastb += sz1;
  insrep (asp, lastb, nkey, nkey2, elsz, &beg, &loc);
  t2bpack (*newpn, inf2);
  icp_insrec (nkey, nkey2, inf2, size2b, beg, loc, lastb, &pg);
  a = (char *) &indph->ind_off;
  recmjform (OLD, &pg, a - asp, size2b, a, 0);
  indph->ind_off += dopsz;
  assert (check_ind_page (asp) == 0);
  putwul (&pg, 'm');
  return (0);
}

static void
per_2_3 (u2_t pn, u2_t newpn, u2_t rbrpn, char *mas, i4_t massz, i4_t n1,
         i4_t n2, u2_t nels, i4_t pr, i4_t lbeg, i4_t lnbeg, i4_t wpage)
{
  char *a, *b, *b1, *asp, *aspn, *lkey;
  struct ind_page *indph, *indphn;
  struct A pg, pgn;
  i4_t size;

  asp = getwl (&pg, seg_n, pn);	/* get pn without lock */
  indph = (struct ind_page *) asp;
  ++((struct p_head *) asp)->idmod;
  a = asp + sizeof (struct p_head);
  b = asp + indph->ind_off;
  recmjform (OLD, &pg, a - asp, b - a, a, 0);
  bcopy (mas, asp + indphsize, n1);
  indph->ind_off = n1 + indphsize;
  if (nels != 0)
    mod_nels ((-nels), asp + lnbeg);
  indph->ind_nextpn = newpn;
  assert (check_ind_page (asp) == 0);
  putwul (&pg, 'm');
  aspn = getnew (&pgn, seg_n, newpn);	/* get new page */
  indphn = (struct ind_page *) aspn;
  indphn->ind_off = indphsize;
  lkey = mas + lnbeg + size2b;
  b = pereliv (aspn, 0, nels, lkey, mas + n1, massz - n1);
  asp = getwl (&pg, seg_n, rbrpn);	/* get rbrpn without lock */
  indph = (struct ind_page *) asp;
  ++((struct p_head *) asp)->idmod;
  a = asp + sizeof (struct p_head);
  b1 = asp + indph->ind_off;
  recmjform (OLD, &pg, a - asp, b1 - a, a, 0);
  a = asp + indphsize;
  if (pr == 0)
    {
      u2_t fnels;
      fnels = t2bunpack (a);
      a += size2b;
      a += kszcal (a, afn, d_f);
      mod_nels (fnels, b - lbeg);
    }
  size = asp + n2 - a;
  bcopy (a, b, size);
  b += size;
  bcopy (asp + n2, asp + indphsize, indph->ind_off - n2);
  indphn->ind_off = b - aspn;
  indphn->ind_nextpn = rbrpn;
  indphn->ind_wpage = wpage;
  assert (check_ind_page (aspn) == 0);
  putwul (&pgn, 'm');
  indph->ind_off -= n2 - indphsize;
  assert (check_ind_page (asp) == 0);
  putwul (&pg, 'm');
}

static int
rbrnup (u2_t pn, struct A *pgr, char *mas, i4_t massz,
        char *last_key, char *last_k2)
{  /* need rbrpn and a new page for a deletion and an insertion */
  i4_t elsz, n1, n2, pr, lbeg, lnbeg;
  char *lnkey, *lnkey2, *nkey, *nkey2, *aspr;
  u2_t newpn, nels;
  char newkey[BD_PAGESIZE];
  char newk2[BD_PAGESIZE];  

  aspr = pgr->p_shm;
  elsz = k2sz + size2b;
  n2 = rep_2_3 (mas, massz, elsz, aspr, &n1, &nels,
                &lbeg, &lnbeg, &nkey, &nkey2, &pr);
  if (n2 == 0)
    {
      putwul (pgr, 'n');
      return (divsn (pn, mas, massz, pgr->p_pn, BTWN, last_key, last_k2));
    }
  bcopy ( nkey, newkey, kszcal (nkey, afn, d_f));
  bcopy ( nkey2, newk2, k2sz);
  putwul (pgr, 'n');
  lnkey = mas + lnbeg + size2b;
  lnkey2 = mas + n1 - elsz;
  if (upmod_2_3 (last_key, last_k2, lnkey, lnkey2, newkey, newk2, &newpn) < 0)
    return (-1);
  per_2_3 (pn, newpn, pgr->p_pn, mas, massz, n1, n2, nels, pr, lbeg, lnbeg, BTWN);
  return (0);
}

static i4_t
rbrup (u2_t pn, u2_t rbrpn, char *mas, i4_t massz,
       char *last_key, char *last_k2)
	/* rbrobr with a deletion and an insertion */
{
  i4_t off2, dopsz, n1, lbeg, lnbeg, pr, rfreesz, n2;
  char *frkey, *middleb, *lkey, *lkey2, *lnkey, *lnkey2, *asp, *a, *b;
  u2_t nels, fnels;
  struct A pg;
  struct ind_page *indph;

  if (rbrpn == (u2_t) ~ 0)
    return (divsn (pn, mas, massz, (u2_t) ~ 0, BTWN, last_key, last_k2));
  if ((asp = getpg (&pg, seg_n, rbrpn, 's')) == NULL)
    return ( -1);
  thread_s_put (rbrpn);
  frkey = asp + indphsize + size2b;
  off2 = ((struct ind_page *) asp)->ind_off;
  dopsz = massz + indphsize - BD_PAGESIZE;
  rfreesz = BD_PAGESIZE - off2;
  if (rfreesz < dopsz)
    return (rbrnup (pn, &pg, mas, massz, last_key, last_k2));
  /* A location is enough in the right brother */
  middleb = mas + (massz + off2 - indphsize) / 2;
  n2 = repttn (mas, mas + massz, k2sz + size2b, middleb, &lkey, &lkey2,
               &lnkey, &lnkey2, &n1, &nels, &lbeg, &lnbeg, &pr, frkey);
  if (n2 > rfreesz)
    return (rbrnup (pn, &pg, mas, massz, last_key, last_k2));
  putwul (&pg, 'n');
  if (mlreddi (last_key, last_k2, lnkey, lnkey2, pn) < 0)
    return (-1);
  asp = getwl (&pg, seg_n, pn);   /* get pn without lock */
  indph = (struct ind_page *) asp;
  ++((struct p_head *) asp)->idmod;
  a = asp + sizeof (struct p_head);
  b = asp + indph->ind_off;
  recmjform (OLD, &pg, a - asp, b - a, a, 0);
  bcopy (mas, asp + indphsize, n1);
  indph->ind_off = indphsize + n1;
  if (nels != 0)
    mod_nels ((-nels), asp + lnbeg);
  assert (check_ind_page (asp) == 0);
  putwul (&pg, 'm');
  
  asp = getwl (&pg, seg_n, rbrpn);/* get rbrpn without lock */
  indph = (struct ind_page *) asp;
  ++((struct p_head *) asp)->idmod;
  a = asp + sizeof (struct p_head);
  b = asp + indph->ind_off;
  recmjform (OLD, &pg, a - asp, b - a, a, 0);
  fnels = t2bunpack (asp + indphsize);
  lkey = mas + lnbeg + size2b;
  a = pereliv (asp, n2, nels, lkey, mas + n1, massz - n1);
  if (pr == 0)
    mod_nels (fnels, a - lbeg);
  indph->ind_off += n2;
  assert (check_ind_page (asp) == 0);
  putwul (&pg, 'm');		/* put page without unlock */
  return (0);
}

static int
rbrnew (struct A *pg, struct A *pgr, i4_t sz, char *key,
        char *key2, char *inf, i4_t ibeg, i4_t iloc)
     /* a division 2 on 3 with an insertion at the last level */
{
  char *mas, *lastb, *beg, *loc;
  i4_t massz, n1, n2, lbeg, lnbeg, pr, elsz, ans, size;
  char *lkey, *lkey2, *nkey, *nkey2, *asp, *aspr;
  u2_t newpn, nels;
  
  asp = pg->p_shm;
  aspr = pgr->p_shm;
  size = ((struct ind_page *) asp)->ind_off - indphsize;
  massz = size + sz;
  mas = (char *) xmalloc (massz);
  bcopy (asp + indphsize, mas, size);
  beg = mas + ibeg - indphsize;
  loc = mas + iloc - indphsize;
  lastb = mas + size;
  icp_insrec (key, key2, inf, inf_size, beg, loc, lastb, NULL);
  elsz = k2sz + inf_size;
  putwul (pg, 'n');
  n2 = rep_2_3 (mas, massz, elsz, aspr, &n1, &nels, &lbeg, &lnbeg, &nkey, &nkey2, &pr);
  lastb = mas + massz;
  lkey = lastb - lbeg + size2b;
  lkey2 = lastb - elsz;
  if (n2 == 0)
    {
      putwul (pgr, 'n');
      ans = divsn (pg->p_pn, mas, massz, pgr->p_pn, LEAF, lkey, lkey2);
    }
  else
    {
      char *lnkey, *lnkey2;
      char newkey[BD_PAGESIZE];
      char newk2[BD_PAGESIZE];

      lnkey = mas + lnbeg + size2b;
      lnkey2 = mas + n1 - elsz;
      bcopy (nkey, newkey, kszcal (nkey, afn, d_f));
      bcopy (nkey2, newk2, k2sz);
      putwul (pgr, 'n');
      ans = upmod_2_3 (lkey, lkey2, lnkey, lnkey2, newkey, newk2, &newpn);
      if (ans >= 0)
        per_2_3 (pg->p_pn, newpn, pgr->p_pn, mas, massz,
                 n1, n2, nels, pr, lbeg, lnbeg, LEAF);
    }
  xfree ((void *) mas);
  return (ans);
}

static int
div12 (struct A *pg, i4_t sz, char *key, char *key2,
       char *inf, char *beg, char *loc)
{
  char *a, *b, *asp, *aspn, *lastb, *middleb, *lastb2;
  i4_t elsz, pr, off, n1, lbeg, lnbeg, middle, n2, prpg, ans;
  struct ind_page *indph, *indphn;
  struct A pgn;
  char *lnkey, *lnkey2, *lkey, *lkey2;
  u2_t nels, newpn, pn;
  
  asp = pg->p_shm;
  indph = (struct ind_page *) asp;
  elsz = k2sz + inf_size;
  off = indph->ind_off;
  lastb = asp + off;
  middle = (off + sz + indphsize) / 2;
  if (loc - asp + sz <= middle)
    {			/* An insertion in the first page */
      middleb = asp + middle - sz;
      lnkey = key;
      lnkey2 = key2;
      n2 = repet1 (asp, beg, loc, elsz, middleb, &lkey, &lkey2, &lnkey,
                   &lnkey2, &n1, &nels, &lbeg, &lnbeg, &pr, NULL);
      prpg = 1;
    }
  else
    {				/* An insertion in the second page */
      middleb = asp + middle;
      n2 = repttn (asp + indphsize, lastb, elsz, middleb, &lkey, &lkey2,
                   &lnkey, &lnkey2, &n1, &nels, &lbeg, &lnbeg, &pr, NULL);
      n1 += indphsize;
      prpg = 2;
    }
  pn = pg->p_pn;
  ans = (div_upmod (lkey, lkey2, lnkey, lnkey2, pn, &newpn) < 0);
  if (ans == -1)
    {
      putwul (pg, 'n');
      return (-1);
    }
  ++((struct p_head *) asp)->idmod;
  a = asp + sizeof (struct p_head);
  b = asp + indph->ind_off;
  recmjform (OLD, pg, a - asp, b - a, a, 0);
  if (nels != 0)
    mod_nels ((-nels), asp + lnbeg);
  lastb = asp + n1;
  aspn = getnew (&pgn, seg_n, newpn);	/* get newpn */
  indphn = (struct ind_page *) aspn;
  indphn->ind_off = indphsize;
  lastb2 = pereliv (aspn, 0, nels, lnkey, lastb, off - n1);
  if (prpg == 1)
    {				/* An insertion in the first page */
      icp_insrec (key, key2, inf, inf_size, beg, loc, lastb, NULL);
      n1 += sz;
    }
  else
    {
      sz = insrep (aspn, lastb2, key, key2, elsz, &beg, &loc);
      icp_insrec (key, key2, inf, inf_size, beg, loc, lastb2, NULL);
      n2 += sz; 
    }
  indph->ind_off = n1;
  indph->ind_nextpn = newpn;
  assert (check_ind_page (asp) == 0);
  putwul (pg, 'm');		/* put page without unlock */
  indphn->ind_off = n2 + indphsize;
  indphn->ind_nextpn = (u2_t) ~ 0;
  indphn->ind_wpage = LEAF;
  assert (check_ind_page (aspn) == 0);
  putwul (&pgn, 'm');		/* put new page */
  return (0);
}

static int
rbrobr (struct A *pg, i4_t sz, char *key, char *key2,
        char *inf, char *beg, char *loc)
{	/* right brother request for an insertion */
  char *aspr, *frkey, *asp;
  char *middleb, *lnkey, *lnkey2, *lkey, *lkey2;
  i4_t elsz, n1, n2, rfreesz, lbeg, ans, off1, off2;
  i4_t lnbeg, prpg, pr, ksz, offbeg, offloc, middle;
  u2_t nels, rbrpn, pn;
  struct A pgr;
  char oldkey[BD_PAGESIZE];
  char oldk2[BD_PAGESIZE];  
  char nkey[BD_PAGESIZE];
  char nkey2[BD_PAGESIZE];

  asp = pg->p_shm;
  rbrpn = ((struct ind_page *) asp)->ind_nextpn;
  if (rbrpn == (u2_t) ~ 0)
    return (div12 (pg, sz, key, key2, inf, beg, loc));
  if ((aspr = getpg (&pgr, seg_n, rbrpn, 's')) == NULL)
    return ( -1);
  thread_s_put (rbrpn);
  off2 = ((struct ind_page *) aspr)->ind_off;
  rfreesz = BD_PAGESIZE - off2;
  offbeg = beg - asp;
  offloc = loc - asp;
  if (rfreesz < sz)
    return (rbrnew (pg, &pgr, sz, key, key2, inf, offbeg, offloc));
  /* A location is enough in the right brother */
  elsz = k2sz + inf_size;
  off1 = ((struct ind_page *) asp)->ind_off;
  middle = (off1 + off2 + sz) / 2;
  frkey = aspr + indphsize + size2b;
  if (offloc + sz <= middle)
    {				/* An insertion in the first page */
      middleb = asp + middle - sz;
      lnkey = key;
      lnkey2 = key2;
      n2 = repet1 (asp, beg, loc, elsz, middleb, &lkey, &lkey2, &lnkey, &lnkey2,
		   &n1, &nels, &lbeg, &lnbeg, &pr, frkey);
      if (n2 > rfreesz)
	return (rbrnew (pg, &pgr, sz, key, key2, inf, offbeg, offloc));
      ksz = kszcal (lnkey, afn, d_f);
      prpg = 1;
    }
  else
    {				/* An insertion in the right brother page */
      char * mas, *a, *b, *lastb;
      i4_t massz, size;
      middleb = asp + middle;
      lastb = asp + off1;
      n2 = repttn (asp + indphsize, lastb, elsz, middleb, &lkey, &lkey2, &lnkey,
		   &lnkey2, &n1, &nels, &lbeg, &lnbeg, &pr, frkey);
      ksz = kszcal (lnkey, afn, d_f);
      n1 += indphsize;
      lnbeg += indphsize;
      massz = off1 - n1 + size2b + ksz + elsz + indphsize;
      mas = (char *) xmalloc (massz);
      a = mas + indphsize;
      if (nels != 0)
	{
	  t2bpack (nels, a);
	  a += size2b;
          bcopy (lnkey, a, ksz);
          a += ksz;
	}
      b = asp + n1;
      size = lastb - b;
      bcopy (b, a, size);
      sz = insrep (mas, mas + massz, key, key2, elsz, &beg, &loc);
      n2 += sz;
      if (n2 > rfreesz)
	{
	  xfree ((void *) mas);
	  return (rbrnew (pg, &pgr, sz, key, key2, inf, offbeg, offloc));
	}
      offbeg = beg - mas;
      offloc = loc - mas;
      xfree ((void *) mas);
      prpg = 2;
    }
  bcopy (lkey, oldkey, kszcal (lkey, afn, d_f));
  bcopy (lkey2, oldk2, k2sz);
  bcopy (lnkey, nkey, ksz);
  bcopy (lnkey2, nkey2, k2sz);
  putwul (pg, 'n');		/* put pn without unlock */
  putwul (&pgr, 'n');		/* put rbrpn without unlock */
  pn = pg->p_pn;
  ans = mlreddi (oldkey, oldk2, nkey, nkey2, pn);
  if (ans == -1)
    return (-1);
  per22 (pn, rbrpn, n1, n2, nels, lbeg, lnbeg, pr, prpg,
         sz, key, key2, inf, nkey, offbeg, offloc);
  return (0);
}

static int
inroot (struct A *pg, i4_t insz, char *key, char *key2,
        char *inf, char *bbeg, char *loc, i4_t wpage)
{
  char *a, *b, *lastb;
  i4_t off, size, ans, infsz, ibeg, iloc;
  char *mas;
  char *asp;

  if (wpage == LEAF)
    infsz = inf_size;
  else
    infsz = size2b;
  asp = pg->p_shm;
  ibeg = bbeg - asp;
  iloc = loc - asp;
  off = ((struct ind_page *) asp)->ind_off;
  size = off - indphsize;
  mas = (char *) xmalloc (size + insz);
  bcopy (asp + indphsize, mas, size);
  lastb = mas + size;
  a = mas + ibeg - indphsize;
  b = mas + iloc - indphsize;
  icp_insrec (key, key2, inf, infsz, a, b, lastb, NULL);
  ans = crnlev (pg, mas, lastb + insz, wpage);
  xfree ((void *) mas);
  return (ans);
}

static int
largest_key (struct A *pg, char *key, char *key2, char *inf, i4_t wpage);

static int
addlast (char *key, char *key2, u2_t * newpn)
{
  u2_t uppn;
  struct A pg;
  char *asp;
  char inf[size2b];

  uppn = *(--thread_s.u);
  asp = getwl (&pg, seg_n, uppn);	/* get uppn without lock */
  *newpn = getempt (seg_n);
  t2bpack (*newpn, inf);
  if (largest_key (&pg, key, key2, inf, BTWN) < 0)
    {
      emptypg (seg_n, *newpn, 'n');
      return (-1);
    }
  return (0);
}

static int
largest_key (struct A *pg, char *key, char *key2, char *inf, i4_t wpage)
{
  char *lastb, *bbeg, *loc, *asp;
  i4_t elsz, sz, off, infsz;
  u2_t pn;
  struct ind_page *indph;

  asp = pg->p_shm;
  indph = (struct ind_page *) asp;
  if (wpage == LEAF)
    infsz = inf_size;
  else
    infsz = size2b;
  elsz = k2sz + infsz;
  off = indph->ind_off;
  lastb = asp + off;
  sz = insrep (asp, lastb, key, key2, elsz, &bbeg, &loc);
  if (sz == 0 )
    return (1);
  pn = pg->p_pn;
  if (sz + off > BD_PAGESIZE)
    {
      if (indph->ind_wpage == IROOT)
        {
          return (inroot (pg, sz, key, key2, inf, bbeg, loc, wpage));
        }
      else
	{
          u2_t newpn, keysz;
	  putwul (pg, 'n');
	  if (addlast (key, key2, &newpn) < 0)
	    return (-1);
	  asp = getnew (pg, seg_n, newpn);	/* get pn without lock */
	  loc = asp + indphsize;
	  t2bpack (1, loc);
          loc += size2b;
          keysz = kszcal (key, afn, d_f);
          bcopy (key, loc, keysz);
          loc += keysz;
          bcopy (key2, loc, k2sz);
          loc += k2sz;
          bcopy (inf, loc, infsz);
	  indph = (struct ind_page *) asp;
	  indph->ind_nextpn = (u2_t) ~ 0;
	  indph->ind_off = indphsize + size2b + keysz + k2sz + infsz;
	  indph->ind_wpage = wpage;
          assert (check_ind_page (asp) == 0);
	  putwul (pg, 'm');
	  asp = getwl (pg, seg_n, pn);	/* get pn without lock */
	  indph = (struct ind_page *) asp;
	  ++((struct p_head *) asp)->idmod;
	  loc = (char *) &indph->ind_nextpn;
	  recmjform (OLD, pg, loc - asp, size2b, loc, 0);
	  indph->ind_nextpn = newpn;
	}
    }
  else
    {
      if (indph->ind_wpage != IROOT)
	{			/* pn is not root */
          char *oldkey;
          char old_key[BD_PAGESIZE];
          char oldk2[BD_PAGESIZE];
          i4_t offbeg, offloc;
	  offbeg = bbeg - asp;
	  offloc = loc - asp;
	  if (sz != elsz)
	    {
              char *a;
              u2_t keysz = 0 , n;
              for (a = asp + indphsize; a < lastb; a += keysz + n * elsz)
                {
                  n = t2bunpack (a);
                  a += size2b;
                  loc = a;
                  keysz = kszcal (a, afn, d_f);
                }
	      oldkey = old_key;
              assert ( keysz != 0 );
              bcopy (loc, oldkey, keysz);
	    }
	  else
	    oldkey = key;
          bcopy (lastb - elsz, oldk2, k2sz);
	  putwul (pg, 'n');
	  if (modlast (oldkey, oldk2, key, key2, pn) < 0)
	    return (-1);
	  asp = getwl (pg, seg_n, pn);	/* get pn without lock */
	  indph = (struct ind_page *) asp;
	  bbeg = offbeg + asp;
	  loc = offloc + asp;
          lastb = asp + indph->ind_off;
	}
      else
	{
	  upunlock ();
	  if (lenforce ()< 0)
	    {
	      putwul (pg, 'n');
	      return (-1);
	    }
	}
      ++((struct p_head *) asp)->idmod;
      icp_insrec (key, key2, inf, infsz, bbeg, loc, lastb, pg);
      loc = (char *) &indph->ind_off;
      recmjform (OLD, pg, loc - asp, size2b, loc, 0);
      indph->ind_off += sz;
    }
  assert (check_ind_page (asp) == 0);
  putwul (pg, 'm');
  return (0);
}

static char *
find_largest_key (struct A *pg)
{
  char  *a, *asp;
  u2_t  pn;
  struct ind_page *indph;

  asp = pg->p_shm;
  indph = (struct ind_page *) asp;
  do
    {
      a = asp + indph->ind_off - size2b;
      pn = t2bunpack (a);
      putwul (pg, 'n');
      thread_s_put (pn);
      if ((asp = getpg (pg, seg_n, pn, 's')) == NULL)
	return (NULL);		/* Lock and request */
      indph = (struct ind_page *) asp;
    }
  while (indph->ind_wpage != LEAF);
  thread_s.u = thread_s.d - 1;
  return (asp);
}

static int
divsn_modlast (char *key, char *key2, u2_t pn, char *mas, i4_t massz);

static int
divup_modlast (char *key, char *key2, char *nkey, char *nkey2,
               char *lnkey, char *lnkey2, u2_t * newpn)
{
  char *a, *asp, *lastb;
  u2_t uppn;
  struct ind_page *indph;
  struct A pg;
  i4_t rbeg, rloc, ibeg1, iloc1, ibeg2, iloc2;
  i4_t remsz, sz1, sz2, elsz, off, dopsz;
  char inf1[size2b], inf2[size2b], *beg, *loc;

  uppn = *(--thread_s.u);
  asp = getwl (&pg, seg_n, uppn);	/* get pn without lock */
  indph = (struct ind_page *) asp;
  elsz = k2sz + size2b;
  remsz = mlrep1 (asp, key, key2, nkey, &rbeg, &rloc, &ibeg1, &iloc1, &sz1);
  if (cmpkeys (k_n, afn, d_f, lnkey, nkey) == 0)
    {
      sz2 = elsz;
      ibeg2 = ibeg1;
      if (ibeg1 == iloc1)
        iloc2 = ibeg1 + sz1 +elsz;
      else
        iloc2 = iloc1 + elsz;
    }
  else
    {
      sz2 = size2b + kszcal (lnkey, afn, d_f) + elsz;
      ibeg2 = iloc2 = iloc1 + sz1;
    }
  bcopy (asp + rloc + k2sz, inf1, size2b);
  off = indph->ind_off;
  dopsz = sz1 + sz2 - remsz;
  if ((off + dopsz) > BD_PAGESIZE)
    {
      char *mas;
      i4_t massz, ans, size;
      lastb = asp + off;
      massz = off + dopsz - indphsize;
      mas = (char *) xmalloc (massz);
      size = off - indphsize;
      bcopy (asp + indphsize, mas, size);
      lastb = mas + size;
      beg = mas + rbeg - indphsize;
      loc = mas + rloc - indphsize; 
      icp_remrec (beg, loc, remsz, lastb, elsz, NULL);
      lastb -= remsz;
      beg = mas + ibeg1 - indphsize;
      loc = mas + iloc1 - indphsize;
      icp_insrec (nkey, nkey2, inf1, size2b, beg, loc, lastb, NULL);
      lastb += sz1;
      *newpn = getempt (seg_n);
      beg = mas + ibeg2 - indphsize;
      loc = mas + iloc2 - indphsize;
      t2bpack (*newpn, inf2);
      icp_insrec (nkey, nkey2, inf2, size2b, beg, loc, lastb, NULL);
      if (indph->ind_wpage != IROOT)
        {
          putwul (&pg, 'n');
          ans = divsn_modlast (key, key2, uppn, mas, massz);
        }
      else
        ans = crnlev (&pg, mas, mas + massz, BTWN);
      if (ans < 0)
	emptypg (seg_n, *newpn, 'n');
      xfree ((void *) mas);
      return (ans);
    }
  else
    {
      if (indph->ind_wpage != IROOT)
	{			/* pn is not root */
	  putwul (&pg, 'n');
	  if (modlast (key, key2, nkey, nkey2, uppn) < 0)
	    return (-1);
	  asp = getwl (&pg, seg_n, uppn);	/* get pn without lock */
	  indph = (struct ind_page *) asp;
	}
      else
	{
	  upunlock ();
	  if (lenforce ()< 0)
	    {
	      putwul (&pg, 'n');
	      return (-1);
	    }
	}
    }
  *newpn = getempt (seg_n);
  ++((struct p_head *) asp)->idmod;
  lastb = asp + off;
  icp_remrec (asp + rbeg, asp + rloc, remsz, lastb, elsz, &pg);
  lastb -= remsz;
  beg = asp + ibeg1;
  loc = asp + iloc1;
  icp_insrec (lnkey, lnkey2, inf1, size2b, beg, loc, lastb, &pg);
  lastb += sz1;
  insrep (asp, lastb, nkey, nkey2, elsz, &beg, &loc);
  t2bpack (*newpn, inf2);
  icp_insrec (nkey, nkey2, inf2, size2b, beg, loc, lastb, &pg);
  a = (char *) &indph->ind_off;
  recmjform (OLD, &pg, a - asp, size2b, a, 0);
  indph->ind_off += dopsz;
  assert (check_ind_page (asp) == 0);
  putwul (&pg, 'm');
  return (0);
}

static int
divsn_modlast (char *key, char *key2, u2_t pn, char *mas, i4_t massz)
{
  char *lastb, *a, *b, *middleb, *asp;
  i4_t n1, lbeg, lnbeg, pr;
  char *nkey, *nkey2, *lnkey, *lnkey2;
  u2_t nels, newpn;
  struct ind_page *indph;
  struct A pg;

  lastb = mas + massz;
  middleb = mas + massz / 2;
  repttn (mas, lastb, k2sz + size2b, middleb, &nkey, &nkey2, &lnkey,
          &lnkey2, &n1, &nels, &lbeg, &lnbeg, &pr, (char *) NULL);
  if (divup_modlast (key, key2, nkey, nkey2, lnkey, lnkey2, &newpn) < 0)
    return (-1);
  asp = getwl (&pg, seg_n, pn);	/* get pn without lock */
  indph = (struct ind_page *) asp;
  ++((struct p_head *) asp)->idmod;
  a = asp + sizeof (struct p_head);
  b = asp + indph->ind_off;
  recmjform (OLD, &pg, a - asp, b - a, a, 0);
  bcopy (mas, asp + indphsize, n1);
  if (nels != 0)
    mod_nels ((-nels), asp + lnbeg);
  indph->ind_off = n1 + indphsize;
  indph->ind_nextpn = newpn;
  assert (check_ind_page (asp) == 0);
  putwul (&pg, 'm');
  asp = getnew (&pg, seg_n, newpn);	/* get newpn without lock */
  indph = (struct ind_page *) asp;
  nkey = lastb - lbeg + size2b;
  indph->ind_off = indphsize;
  b = pereliv (asp, 0, nels, nkey, mas + n1, massz - n1);
  indph->ind_off = b - asp;
  indph->ind_nextpn = (u2_t) ~0;
  indph->ind_wpage = BTWN;
  assert (check_ind_page (asp) == 0);
  putwul (&pg, 'm');		/* put page without unlock */
  return (0);
}

i4_t
icp_insrtn (struct ldesind *desind, char *key, char *key2,
            char *inf, i4_t infsz)
{
  i4_t lagelsz, agelsz, sz, off, ans, ans_lk;
  u2_t rootpn, d_fn;
  char *asp = NULL, *lastb, *bbeg, *loc;
  struct ind_page *indph;
  struct A pg;

  thread_s_ini_check();
  
  seg_n = desind->i_segn;
  afn = (u2_t *) (desind + 1);
  k_n = desind->ldi.kifn & ~UNIQ & MSK21B;
  uniq_key = UNIQ & desind->ldi.kifn;
  d_f = desind->pdf;
  d_fn = k_n;
  if ((k_n % 2) != 0)
    d_fn += 1;
  d_f2 = (struct des_field *) (afn + d_fn);
  k2sz = d_f2->field_size;
  inf_size = infsz;
  lagelsz = k2sz + infsz;
  agelsz = k2sz + size2b;
  rootpn = desind->ldi.rootpn;
  beg_mop ();
try_again:
  while ((asp = getpg (&pg, seg_n, rootpn, 's')) == NULL); /* Lock and request */
  
  thread_s_ini();
  thread_s_put (rootpn);
  indph = (struct ind_page *) asp;
  if (indph->ind_wpage == LEAF)
    {				/* root-leaf */
      off = indph->ind_off;
      lastb = asp + off;
      sz = insrep (asp, lastb, key, key2, lagelsz, &bbeg, &loc);
      if (sz == 0)
	{
	  putpg (&pg, 'n');
          thread_s_ini();
	  return (-ER_NU);
	}
      if (BD_PAGESIZE - off < sz)
	{
	  if (inroot (&pg, sz, key, key2, inf, bbeg, loc, LEAF) < 0)
	    goto try_again;
	}
      else
	{
	  if (lenforce ()< 0)
	    {
	      all_unlock ();
	      goto try_again;
	    }
	  ++((struct p_head *) asp)->idmod;
	  icp_insrec (key, key2, inf, infsz, bbeg, loc, lastb, &pg);
	  loc = (char *) &indph->ind_off;
	  recmjform (OLD, &pg, loc - asp, size2b, loc, 0);
	  indph->ind_off += sz;
          assert (check_ind_page (asp) == 0);
	  putwul (&pg, 'm');
	}
    }
  else
    {
      if ((ans = icp_spusk (&pg, agelsz, key, key2)) == -1)
	{
	  all_unlock ();
	  goto try_again;
	}
      else if (ans == 1)
	{
	  if ((ans_lk = largest_key (&pg, key, key2, inf, LEAF)) < 0)
            {
              all_unlock ();
              goto try_again;
            }
          else
            if (ans_lk == 1)
              {
                putpg (&pg, 'n');
                all_unlock ();
                thread_s_ini();
                return (-ER_NU);
              }
	}
      else
	{
	  /* for the leaf */
          asp = pg.p_shm;
	  indph = (struct ind_page *) asp;
	  off = indph->ind_off;
	  lastb = asp + off;
	  sz = insrep (asp, lastb, key, key2, lagelsz, &bbeg, &loc);
	  if (sz == 0)
	    {
              putpg (&pg, 'n');
	      all_unlock ();
              thread_s_ini();
	      return (-ER_NU);
	    }
	  if (BD_PAGESIZE - off < sz)
	    {
	      if (rbrobr (&pg, sz, key, key2, inf, bbeg, loc) < 0)
		goto try_again;
	    }
          else
            {
              assert (loc != lastb);
              upunlock ();
              if (lenforce ()< 0)
                {
                  all_unlock ();
                  goto try_again;
                }
              ++((struct p_head *) asp)->idmod;
	      icp_insrec (key, key2, inf, infsz, bbeg, loc, lastb, &pg);
	      loc = (char *) &indph->ind_off;
	      recmjform (OLD, &pg, loc - asp, size2b, loc, 0);
	      indph->ind_off += sz;
              assert (check_ind_page (asp) == 0);
	      putwul (&pg, 'm');
	    }
	}
    }
  MJ_PUTBL ();
  downunlock ();
  
  thread_s_ini();
  
  return (OK);
}

i4_t
icp_spusk (struct A *pg, i4_t elsz, char *key, char *key2)
{
  char *a, *lastb, *asp;
  u2_t n, pn;
  i4_t l, l2=0, ksz, i;
  struct ind_page *indph;

  asp = pg->p_shm;
  indph = (struct ind_page *)asp;
  lastb = asp + indph->ind_off;
  a = asp + indphsize;
  for (i = 0; indph->ind_wpage != LEAF; i++)
    {
      lastb = asp + indph->ind_off;
      for (a = asp + indphsize; a < lastb; )
	{
	  n = t2bunpack (a);
          assert (n < BD_PAGESIZE / 2);
	  a += size2b;
	  ksz = kszcal (a, afn, d_f);
	  if ((l = cmpkeys (k_n, afn, d_f, a, key)) == 0)
	    {
	      a += ksz;
              if (uniq_key == UNIQ)
                break;
	      for (; n != 0; n--, a += elsz)
                if ((l2 = cmp2keys (d_f2->field_type, a, key2)) >= 0)
                  break;
              if (n == 0 && l2 < 0)
                {
                  if (a == lastb)
                    assert (indph->ind_nextpn == (u2_t) ~0);
                  continue;
                }
              if (n == 0)
                a -= elsz;
	      break;
	    }
	  else if (l > 0)
	    {
	      a += ksz;
	      break;
	    }
          a += ksz + n * elsz;
	}
      if (a == lastb)
	{
          assert (i==0);
	  if ((asp = find_largest_key (pg)) == NULL)
	    return (-1);
	  return (1);
	}
      a += k2sz;
      pn = t2bunpack (a);
      putwul (pg, 'n');
      if ((asp = getpg (pg, seg_n, pn, 's')) == NULL)
	return (-1);
      thread_s_put (pn);
      indph = (struct ind_page *) asp;
    }
  thread_s.u = thread_s.d - 1;
  return (0);
}

i4_t
remrep (char *asp, char *key, char *key2, i4_t elsz,
        char ** rbeg, char **rloc, i4_t *offbef)
{
  char *a, *lastb, *ckey, *bbeg;
  i4_t i, keysz, agsz = 0;
  u2_t n;

  lastb = asp + ((struct ind_page *) asp)->ind_off;
  for (a = asp + indphsize; a < lastb; a += agsz)
    {
      bbeg = a;
      n = t2bunpack (a);
      assert (n < BD_PAGESIZE / 2);
      ckey = a + size2b;
      keysz = kszcal (ckey, afn, d_f);
      if (cmpkeys (k_n, afn, d_f, ckey, key) == 0)
	{
	  a += size2b + keysz;
	  for (i = 0; i != n; i++, a += elsz)
	    if (cmp2keys (d_f2->field_type, a, key2) == 0)
	      {
		*rbeg = bbeg;
		*offbef = bbeg - asp - agsz;
		*rloc = a;
		if (n == 1)
		  return (size2b + keysz + elsz);
		else
		  return (elsz);
	      }
          assert (i != n);
	}
      agsz = size2b + keysz + n * elsz;
      assert (agsz < BD_PAGESIZE);
    }
  assert (a < lastb);
  return (0);
}

void
icp_remrec (char *beg, char *loc, i4_t sz, char *lastb,
            i4_t elsz, struct A *pg)
{
  char *b;
  i4_t size;

  if (sz == elsz)
    {
      b = loc + sz;
      size = lastb - b;
      if (pg != NULL)
	recmjform (COMBL, pg, loc - pg->p_shm, size, loc, sz);
      bcopy (b, loc, size);
      if (pg != NULL)
	recmjform (OLD, pg, beg - pg->p_shm, size2b, beg, 0);
      mod_nels (-1, beg);
    }
  else
    {
      b = beg + sz;
      size = lastb - b;
      if (pg != NULL)
	recmjform (COMBL, pg, beg - pg->p_shm, size, beg, sz);
      bcopy (b, beg, size);
    }
}

i4_t
mlreddi (char *lkey, char *lkey2, char *lnkey, char *lnkey2, u2_t pn)
{
  char *asp, *pnt;
  i4_t sz, remsz, insz, off, orbeg, orloc, oibeg, oiloc, elsz, dopsz;
  struct ind_page *indph;
  struct A pg;
  char ninf[size2b];

  t2bpack (pn, ninf);
  pn = *(--thread_s.u);
  asp = getwl (&pg, seg_n, pn);	/* get pn without lock */
  remsz = mlrep1 (asp, lkey, lkey2, lnkey, &orbeg, &orloc, &oibeg, &oiloc, &insz);
  indph = (struct ind_page *) asp;
  off = indph->ind_off;
  dopsz = insz - remsz;
  sz = off + dopsz;
  elsz = k2sz + size2b;
  if (sz > BD_PAGESIZE)
    {
      char *mas, *b, *beg, *loc, *lastb;
      i4_t massz, ans, size;
      massz = sz - indphsize;
      mas = (char *) xmalloc (massz);
      size = off - indphsize;
      bcopy (asp + indphsize, mas, size);
      b = mas + size;
      beg = mas + orbeg - indphsize;
      loc = mas + orloc - indphsize;
      icp_remrec (beg, loc, remsz, b, elsz, NULL);
      lastb = b - remsz;
      beg = mas + oibeg - indphsize;
      loc = mas + oiloc - indphsize;
      icp_insrec (lnkey, lnkey2, ninf, size2b, beg, loc, lastb, NULL);
      if (indph->ind_wpage == IROOT)
	{			/* uppn is root */
	  ans = crnlev (&pg, mas, mas + massz, BTWN);
	}
      else
	{
          char last_key[BD_PAGESIZE];
          char last_k2[BD_PAGESIZE];
          u2_t rbrpn, keysz = 0 ;
          
          if (orloc + elsz != off)     /* lkey isn't the last key in pn */
            {
              FIND_LAST_KEY (asp, elsz, lkey, lkey2, keysz);
            }
          else
            keysz = kszcal (lkey, afn, d_f);
          assert ( keysz != 0 );
          bcopy (lkey, last_key, keysz);
          bcopy (lkey2, last_k2, k2sz);
	  rbrpn = indph->ind_nextpn;
	  putwul (&pg, 'n');
	  ans = rbrup (pn, rbrpn, mas, massz, last_key, last_k2);
	}
      xfree ((void *) mas);
      return (ans);
    }
  if (orloc + elsz == off && indph->ind_wpage != IROOT)
    {
      putwul (&pg, 'n');
      if (mlreddi (lkey, lkey2, lnkey, lnkey2, pn) < 0)
	return (-1);
      asp = getwl (&pg, seg_n, pn);
    }
  else
    {
      upunlock ();
      if (lenforce ()< 0)
	{
	  putwul (&pg, 'n');
	  return (-1);
	}
    }
  ++((struct p_head *) asp)->idmod;
  indph = (struct ind_page *) asp;
  if (orloc == oiloc)
    {				/* keys match */
      pnt = asp + orloc;
      recmjform (OLD, &pg, orloc, k2sz, pnt, 0);
      bcopy (lnkey2, pnt, k2sz);
    }
  else
    {
      pnt = asp + indph->ind_off;
      icp_remrec (asp + orbeg, asp + orloc, remsz, pnt, elsz, &pg);
      icp_insrec (lnkey, lnkey2, ninf, size2b, asp + oibeg,
		  asp + oiloc, pnt - remsz, &pg);
    }
  if (dopsz != 0)
    {
      pnt = (char *) &indph->ind_off;
      recmjform (OLD, &pg, pnt - asp, size2b, pnt, 0);
      indph->ind_off += dopsz;
    }
  assert (check_ind_page (asp) == 0);
  putwul (&pg, 'm');		/* put page without unlock */
  return (0);
}

i4_t
modlast (char *key, char *key2, char *nkey, char *nkey2, u2_t pn)
{
  char *lastb, *asp, *beg, *loc;
  struct ind_page *indph;
  i4_t sz = 0, remsz = 0, elsz, off;
  i4_t orbeg, orloc, oibeg, oiloc;
  struct A pg;
  char ninf[size2b];

  t2bpack (pn, ninf);
  pn = *(--thread_s.u);
  asp = getwl (&pg, seg_n, pn);	/* get uppn without lock */
  indph = (struct ind_page *) asp;
  off = indph->ind_off;
  elsz = k2sz + size2b;
  if (key != nkey)
    {
      if (cmpkeys (k_n, afn, d_f, key, nkey) == 0)
        nkey = key;
      else
        {
          i4_t insz;
          remsz = mlrep1 (asp, key, key2, nkey, &orbeg, &orloc, &oibeg, &oiloc, &insz);
          assert (insz != 0);
          sz = insz - remsz;
          if (sz + off > BD_PAGESIZE)
            {
              char *mas;
              i4_t massz, ans;
              massz = off + insz - indphsize;
              mas = (char *) xmalloc (massz);
              sz = off - indphsize;
              bcopy (asp + indphsize, mas, sz);
              lastb = mas + sz;
              icp_remrec (mas + orbeg, mas + orloc, remsz, lastb, elsz, NULL);
              lastb -= remsz;
              icp_insrec (nkey, nkey2, ninf, size2b, mas + oibeg, mas + oiloc,
                          lastb, NULL);
              if (indph->ind_wpage != IROOT)
                {
                  putwul (&pg, 'n');
                  ans = divsn_modlast (key, key2, pn, mas, massz);
                }
              else
                ans = crnlev (&pg, mas, mas + massz, BTWN);
              xfree ((void *) mas);
              return (ans);
            }
        }
    }
  if (indph->ind_wpage != IROOT)
    {			/* pn is not root */
      putwul (&pg, 'n');
      if (modlast (key, key2, nkey, nkey2, pn) < 0)
        return (-1);
      asp = getwl (&pg, seg_n, pn);	/* get pn without lock */
      indph = (struct ind_page *) asp;
    }
  else
    {
      upunlock ();
      if (lenforce ()< 0)
        {
          putwul (&pg, 'n');
          return (-1);
        }
    }
  ++((struct p_head *) asp)->idmod;
  lastb = asp + off;
  if (key == nkey)
    {
      loc = lastb - elsz;
      recmjform (OLD, &pg, loc - asp, size2b, loc, 0);
      bcopy (nkey2, loc, k2sz);
    }
  else
    {
      beg = asp + orbeg;
      loc = asp + orloc;
      icp_remrec (beg, loc, remsz, lastb, elsz, &pg);
      lastb -= remsz;
      beg = asp + oibeg;
      loc = asp + oiloc;
      icp_insrec (nkey, nkey2, ninf, size2b, beg, loc, lastb, &pg);
    }
  if (sz != 0)
    {
      char *a;
      a = (char *) &indph->ind_off;
      recmjform (OLD, &pg, a - asp, size2b, a, 0);
      indph->ind_off += sz;
    }
  assert (check_ind_page (asp) == 0);
  putwul (&pg, 'm');
  return (0);
}

void
all_unlock ()
{
  BUF_unlock (seg_n, thread_s.count, thread_s.arr);
}

void
upunlock ()
{
  i4_t i;
  i = thread_s.u - thread_s.arr;
  if (i)
    BUF_unlock (seg_n, i, thread_s.arr);
}

void
downunlock ()
{
  i4_t i;
  i = thread_s.d - thread_s.u;
  if (i)
    BUF_unlock (seg_n, i, thread_s.u);
}

i4_t
lenforce ()
{
  u2_t *a;

  for (a = thread_s.u; a < thread_s.d; a++)
    if (BUF_enforce (seg_n, *a) < 0)
      {
	downunlock ();
	return (-1);
      }
  return (0);
}
