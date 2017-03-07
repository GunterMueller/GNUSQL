/*
 *  ind_rem.c  - Index Control Programm
 *               functions dealing with key deletion from an index
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

/* $Id: ind_rem.c,v 1.252 1998/09/29 21:25:32 kimelman Exp $ */

#include "xmem.h"
#include <assert.h>
#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

extern struct des_field *d_f, *d_f2;
extern u2_t *afn;
extern u2_t k_n;
extern u2_t seg_n;
extern i4_t k2sz;
extern i4_t inf_size;
extern char uniq_key;

i4_t  check_ind_page (char *asp);


ARR_DECL(thread_s,u2_t,extern) /* declaration of dynamic stack/array 'thread_s'. */
ARR_PROC_PROTO(thread_s,u2_t)  /* declaration of routines for it               */

ARR_DECL(l_emp,u2_t,static)    /* declaration of local dynamic stack/array 'l_emp'. */
ARR_PROC_DECL(l_emp,u2_t,static) /* declaration of routines for it               */

static void
alter_pn (i4_t remsz, i4_t elsz, char *rbeg, char *rloc, struct A *pg,
          char *inf)   /* after removing */
{
  char *a;
  u2_t n;
  i4_t agsz, ksz;

  n = t2bunpack (rbeg);
  ksz = kszcal (rbeg + size2b, afn, d_f); 
  agsz = size2b + ksz + n * elsz;
  if (remsz == elsz && rloc != rbeg + agsz)
    /* the key was not last in the aggregate */
    a = rloc + k2sz;
  else
    a = rbeg + size2b + ksz + k2sz;
  recmjform (OLD, pg, a - pg->p_shm, size2b, a, 0);
  bcopy (inf, a, size2b);
}

static void
remove_level (struct A *pg_root, struct A *pg_down) 
{
  char *pnt, *asp_down, *asp;
  u2_t off_down;
  struct ind_page *indph, *indph_down;

  asp_down = pg_down->p_shm;
  asp = pg_root->p_shm;
  ++((struct p_head *) asp)->idmod;
  indph = (struct ind_page *) asp;
  pnt = asp + indphsize; 
  recmjform (OLD, pg_root, indphsize, indph->ind_off - indphsize, pnt, 0);
  indph_down = (struct ind_page *) asp_down;
  if (indph_down->ind_wpage == LEAF)
    {
      pnt = (char *) &indph->ind_wpage;
      recmjform (OLD, pg_root, pnt - asp, size2b, pnt, 0);
      indph->ind_wpage = LEAF;
    }
  off_down = indph_down->ind_off;
  pnt = asp + indphsize;
  bcopy (asp_down + indphsize, pnt, off_down - indphsize);
  pnt = (char *) &indph->ind_off;
  recmjform (OLD, pg_root, pnt - asp, size2b, pnt, 0);
  indph->ind_off = off_down;
  putwul (pg_down, 'n');
  l_emp_put (pg_down->p_pn);
}

static int
rem_rec (struct A *pg, char *key, char *key2, i4_t infsz, char *newinf);

static int
rem_page (char *key, char *key2, u2_t *uppn, u2_t pn)
{   /* remove a record about page from a middle level */
  char *asp, inf[size2b];
  struct A pg;
  i4_t ans = 0;
  struct ind_page *indph;

  *uppn = *(--thread_s.u);
  asp = getwl (&pg, seg_n, *uppn);	/* get uppn without lock */
  indph = (struct ind_page *) asp;
  t2bpack (pn, inf);
  if (indph->ind_wpage == IROOT)
    {
      char *a, *lastb, *rbeg, *rloc;
      u2_t n, elsz;
      i4_t remsz, offbef;
      
      elsz = k2sz + size2b;
      remsz = remrep (asp, key, key2, elsz, &rbeg, &rloc, &offbef);
      if (lenforce ()< 0)
	{
	  putwul (&pg, 'n');
	  return (-1);
	}
      lastb = asp + indph->ind_off;
      ++((struct p_head *) asp)->idmod;
      icp_remrec (rbeg, rloc, remsz, lastb, elsz, &pg);
      alter_pn (remsz, elsz, rbeg, rloc, &pg, inf);
      lastb -= remsz;
      a = asp + indphsize;
      n = t2bunpack (a);
      a += size2b;
      a += kszcal (a, afn, d_f) + n * elsz;
      rloc = (char *) &indph->ind_off;
      recmjform (OLD, &pg, rloc - asp, size2b, rloc, 0);
      indph->ind_off -= remsz;
      if (a == lastb && n == 1)	/* remove one level */
        ans = 1;
      assert (check_ind_page (asp) == 0);
      putwul (&pg, 'm');
    }
  else
    ans = rem_rec (&pg, key, key2, size2b, inf);
  return (ans);
}

static int
rem_last_page (char *key, char *key2, u2_t *prevpn_array, i4_t lev_num)
	/* remove a record about last page */
{
  char *a, *asp;
  char *rbeg, *rloc, *newkey;
  i4_t remsz, offbef, elsz, off, offbeg, offloc, i;
  u2_t uppn, prevpn, n, prevpn_down;
  struct ind_page *indph;
  struct A pg;
  char new_key[BD_PAGESIZE];
  char newk2[BD_PAGESIZE];

  uppn = *(--thread_s.u);
  asp = getwl (&pg, seg_n, uppn);	/* get uppn without lock */
  indph = (struct ind_page *) asp;
  elsz = k2sz + size2b;
  remsz = remrep (asp, key, key2, elsz, &rbeg, &rloc, &offbef);
  off = indph->ind_off;
  if (off - remsz == indphsize && indph->ind_wpage != IROOT)
    {
      putwul (&pg, 'n');
      return (rem_last_page (key, key2, prevpn_array, lev_num + 1));
    }
  a = (remsz!=elsz ? rbeg : rloc) - size2b;
  prevpn = t2bunpack (a);
  for (prevpn_down = prevpn, i = lev_num; i >= 0; i--)
    {
      struct A pg_down;
      char *asp_down;

      thread_s_put (prevpn_down);
      prevpn_array[i] = prevpn_down;
      if ((asp_down = getpg (&pg_down, seg_n, prevpn_down, 's')) == NULL)
        return ( -1);
      if (i > 0 )
        prevpn_down = t2bunpack (asp_down + ((struct ind_page *) asp_down)->ind_off - size2b);
      putwul (&pg_down, 'n');
    }
  if (indph->ind_wpage == IROOT)
    {
      if (lenforce ()< 0)
        {
          putwul (&pg, 'n');
          return (-1);
        }
      ++((struct p_head *) asp)->idmod;
      icp_remrec (rbeg, rloc, remsz, asp + off, elsz, &pg);
      a = (char *) &indph->ind_off;
      recmjform (OLD, &pg, a - asp, size2b, a, 0);
      indph->ind_off -= remsz;
      a = asp + indphsize;
      n = t2bunpack (a);
      a += size2b;
      a += kszcal (a, afn, d_f) + n * elsz;
      if (a == asp + indph->ind_off && n == 1)	/* remove one level */
        {
          struct A pg_down;
          getwl (&pg_down, seg_n, prevpn);
          remove_level (&pg, &pg_down);
        }
    }
  else
    {
      offbeg = rbeg - asp;
      offloc = rloc - asp;
      if (remsz != elsz)
        {
          a = asp + offbef + size2b;
          newkey = new_key;
          bcopy (a, newkey, kszcal (a, afn, d_f));
          a = rbeg - elsz;
        }
      else
        {
          newkey = key;
          a = asp + off - 2 * elsz;
        }
      bcopy (a, newk2, k2sz);
      putwul (&pg, 'n');
      if (modlast (key, key2, newkey, newk2, uppn) < 0)
        return (-1);
      asp = getwl (&pg, seg_n, uppn);	/* get pn without lock */
      indph = (struct ind_page *) asp;
      rbeg = offbeg + asp;
      rloc = offloc + asp;
      ++((struct p_head *) asp)->idmod;
      icp_remrec (rbeg, rloc, remsz, asp + off, elsz, &pg);
      a = (char *) &indph->ind_off;
      recmjform (OLD, &pg, a - asp, size2b, a, 0);
      indph->ind_off -= remsz;
    }
  assert (check_ind_page (asp) == 0);
  putwul (&pg, 'm');
  for (; lev_num >= 0; lev_num--)
    {
      u2_t pn;
      prevpn = prevpn_array[lev_num];
      asp = getwl (&pg, seg_n, prevpn);
      indph = (struct ind_page *) asp;
      ++((struct p_head *) asp)->idmod;
      a = (char *) &indph->ind_nextpn;
      recmjform (OLD, &pg, a - asp, size2b, a, 0);
      pn = indph->ind_nextpn;
      indph->ind_nextpn = (u2_t) ~ 0;
      l_emp_put (pn);
      assert (check_ind_page (asp) == 0);
      putwul (&pg, 'm');
    }
  return (0);
}

static int
rem_rec (struct A *pg, char *key, char *key2, i4_t infsz, char *newinf)
{
  char *a, *lastb, *asp;
  char *rbeg, *rloc, *lkey, *lkey2;
  char *aspr = NULL, *bbeg;
  i4_t remsz, offbef, off, n1, elsz, off2 = 0;
  i4_t n2 = 0, ans = 0, lbeg = 0; 
  struct ind_page *indph, *indphr = NULL;
  u2_t rbrpn, pn, uppn;
  i4_t pr_merge = 0, off_rbr = 0;
  struct A pgr;

  asp = pg->p_shm;
  elsz = k2sz + infsz;
  remsz = remrep (asp, key, key2, elsz, &rbeg, &rloc, &offbef);
  indph = (struct ind_page *) asp;
  off = indph->ind_off;
  lastb = asp + off;
  rbrpn = indph->ind_nextpn;
  n1 = off - remsz;
  pn = pg->p_pn;
  if (n1 < BD_PAGESIZE / 2 && rbrpn != (u2_t) ~ 0)
    {			/* merging or transfusion */
      u2_t keysz, n;
      a = rbeg;
      do
        {
          lbeg = lastb - a;
          n = t2bunpack (a);
          a += size2b;
          lkey = a;
          keysz = kszcal (lkey, afn, d_f);
          a += keysz + n * elsz;
        }
      while(a < lastb);
      lkey2 = lastb - elsz;
      if ((aspr = getpg (&pgr, seg_n, rbrpn, 's')) == NULL) /* Lock and request */
        {
          putwul (pg, 'n');
          return (-1);
        }
      indphr = (struct ind_page *) aspr;
      off2 = indphr->ind_off;
      if (off2 > (2 * BD_PAGESIZE / 3))
        {			/*transfusion from rigth brother*/
          char *middleb;
          i4_t agsz;
          char *nkey, *nkey2;

          middleb = aspr + ((off + off2 - indphsize) / 2 - off);
          agsz = 0;
          for (bbeg = a = aspr + indphsize; a + agsz < middleb;)
            {
              a += agsz;
              n = t2bunpack (a);
              agsz = size2b + kszcal (a + size2b, afn, d_f) + n * elsz;
            }
          nkey = a + size2b;
          nkey2 = a + agsz - elsz;
          n2 = a + agsz - aspr - indphsize;
          if (n2 + n1 > BD_PAGESIZE)
            n2 = 0;
          if (n2 != 0)
            {
              thread_s_put (rbrpn);
              
              if (mlreddi (lkey, lkey2, nkey, nkey2, pn) < 0)
                return (-1);
             }
          else
            putpg (&pgr, 'n');
        }
      else if (n1 + off2 - indphsize < BD_PAGESIZE)
        {			/* merging with r_br*/
          thread_s_put (rbrpn);
          n2 = off2 - indphsize;
          ans = rem_page (lkey, lkey2, &uppn, pn);
          if (ans < 0)
            return (-1);
          pr_merge = 1;
        }
      else
        putpg (&pgr, 'n');
    }
  if (n2 == 0)		/* don't touch right brother */
    {
      if (rloc + elsz == lastb )
        {
          char *newkey, *newk2;
          if (newinf != NULL && rbrpn != (u2_t) ~ 0)
            {
              if (BUF_lockpage (seg_n, rbrpn, 's') == -1) /* Lock rbrpn */
                {
                  putwul (pg, 'n');
                  return (-1);
                }
              thread_s_put (rbrpn);
            }
          if ( rbeg == asp + indphsize && t2bunpack (rbeg) == 1 && indph->ind_wpage != IROOT)
            {
              u2_t *prevpn_array;
              prevpn_array = (u2_t *) xmalloc(thread_s.count * size2b);
              putwul (pg, 'n');
              ans = rem_last_page(key, key2, prevpn_array, 0);
              xfree(prevpn_array);
              return (ans);
            }
          if (remsz == elsz)
            {
              newkey = key;
              newk2 = key2 - elsz;
            }
          else
            {
              newkey = asp + offbef + size2b;
              newk2 = rbeg - elsz;
            }
          if (rbrpn != (u2_t) ~ 0)
            {
              if (mlreddi (key, key2, newkey, newk2, pn) < 0)
                return (-1);
            }
          else
            {
              if (modlast (key, key2, newkey, newk2, pn) < 0)
                return (-1);
            }
        }
      else
        {
          upunlock ();
          if (lenforce ()< 0)
            return (-1);
        }
    }
  ++((struct p_head *) asp)->idmod;
  icp_remrec (rbeg, rloc, remsz, lastb, elsz, pg);
  if (n2 != 0)
    {
      if (remsz != elsz && rbeg - remsz == lastb)
	bbeg = asp + offbef;
      else
	bbeg = lastb - lbeg;
      lkey = bbeg + size2b;
      a = aspr + indphsize;
      if (cmpkeys (k_n, afn, d_f, lkey, a + size2b) == 0 && n1 != indphsize)
	{
          u2_t keysz;
	  recmjform (OLD, pg, bbeg - asp, size2b, bbeg, 0);
          mod_nels (t2bunpack (a), bbeg);
	  keysz = kszcal (a + size2b, afn, d_f);
          off_rbr = keysz + size2b;
          /*	  n2 -= keysz + size2b;*/
	}
      lastb -= remsz;
      bcopy (a + off_rbr, lastb, n2 - off_rbr);
      if (newinf != NULL)
        alter_pn (remsz, elsz, rbeg, rloc, pg, newinf);
      if (pr_merge == 1)
	{
	  a = (char *) &indph->ind_nextpn;
	  recmjform (OLD, pg, a - asp, size2b, a, 0);
          indph->ind_nextpn = indphr->ind_nextpn;
          putwul (&pgr, 'n');
          l_emp_put (rbrpn);
          if (ans == 1)
            {			/* remove one level */
              struct A pg1;
              char *asp1; 
              a = (char *) &indph->ind_off;
              recmjform (OLD, pg, a - asp, size2b, a, 0);
              indph->ind_off += n2 - remsz;
              asp1 = getwl (&pg1, seg_n, uppn);
              remove_level (&pg1, pg);
              assert (check_ind_page (asp1) == 0);
              putwul (&pg1, 'm');
              return (0);
            }
	}
      else
	{
          i4_t size;
	  a = aspr + indphsize;
          ++((struct p_head *) aspr)->idmod;
          size = off2 - indphsize - n2;
	  recmjform (COMBL, &pgr, indphsize, size, a, n2);
          bcopy (a + n2, a, size);
	  a = (char *) &indphr->ind_off;
	  recmjform (OLD, &pgr, a - aspr, size2b, a, 0);
	  indphr->ind_off -= n2;
          assert (check_ind_page (aspr) == 0);
          putwul (&pgr, 'm');
	}
    }
  else if (newinf != NULL)
    {
      if (rloc + elsz == lastb)
	{
	  aspr = getwl (&pgr, seg_n, rbrpn);	/* get rbrpn without lock */
	  ++((struct p_head *) aspr)->idmod;
	  a = aspr + indphsize + size2b;
	  a += kszcal (a, afn, d_f);
	  a += k2sz;
	  recmjform (OLD, &pgr, a - aspr, size2b, a, 0);
          bcopy (newinf, a, size2b);
          assert (check_ind_page (aspr) == 0);
	  putwul (&pgr, 'm');
	}
      else
        alter_pn (remsz, elsz, rbeg, rloc, pg, newinf);
    }
  a = (char *) &indph->ind_off;
  recmjform (OLD, pg, a - asp, size2b, a, 0);
  indph->ind_off += n2 - off_rbr - remsz;
  assert (check_ind_page (asp) == 0);
  putwul (pg, 'm');
  return (0);
}

int
icp_rem (struct ldesind *desind, char *key, char *key2, i4_t infsz)
{
  char *asp = NULL;
  u2_t rootpn, d_fn;
  struct ind_page *indph;
  struct A pg;
  
  thread_s_ini_check();
  l_emp_ini_check();
  
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
  rootpn = desind->ldi.rootpn;
  beg_mop ();
try_again:
  while ((asp = getpg (&pg, seg_n, rootpn, 's')) == NULL);/* Lock and request */
  
  thread_s_ini();
  thread_s_put (rootpn);
  
  indph = (struct ind_page *) asp;
  if (indph->ind_wpage == LEAF)
    {
      i4_t lagelsz, remsz, offbef;
      char  *rbeg, *rloc;
      if (BUF_enforce (seg_n, rootpn) < 0)
	{
	  putwul (&pg, 'n');
	  downunlock ();
	  goto try_again;
	}
      lagelsz = k2sz + infsz;
      remsz = remrep (asp, key, key2, lagelsz, &rbeg, &rloc, &offbef);
      ++((struct p_head *) asp)->idmod;
      icp_remrec (rbeg, rloc, remsz, asp + indph->ind_off, lagelsz, &pg);
      rloc = (char *) &indph->ind_off;
      recmjform (OLD, &pg, rloc - asp, size2b, rloc, 0);
      indph->ind_off -= remsz;
      assert (check_ind_page (asp) == 0);
      putwul (&pg, 'm');
    }
  else
    {
      if (icp_spusk (&pg, k2sz + size2b, key, key2) == -1)
	{
	  all_unlock ();
	  goto try_again;
	}
      else
	{
	  if (rem_rec (&pg, key, key2, infsz, NULL) < 0)
	    {
	      all_unlock ();
              l_emp_ini();
	      goto try_again;
	    }
	}
    }
  MJ_PUTBL ();
  {
    i4_t  i;
    for (i = 0; i < l_emp.count; i++)
      emptypg (seg_n, l_emp.arr[i], 'f');
    l_emp_ini();
  }
  downunlock ();
  thread_s_ini();
  return 0;
}

int
kszcal (char *key, u2_t * mfn, struct des_field *ad_f)
{
  int k, keysz;
  char *a, *aval;
  u2_t nk;

  keysz = scscal (key);
  a = aval = key + keysz;
  for (nk = 0, k = 0; nk < k_n && key < aval; nk++, k++)
    {
      if (k == 7)
	{
	  k = 0;
	  key++;
	}
      if ((*key & BITVL(k)) != 0)
	a = proval (a, (ad_f + mfn[nk])->field_type);
    }
  keysz += a - aval;
  assert (keysz < BD_PAGESIZE);  
  return (keysz);
}

static void
roll_level (u2_t nextpn)
{
  char *asp;
  struct A pg;

  while (nextpn != (u2_t) ~ 0)
    {
      l_emp_put (nextpn);
      thread_s_put (nextpn);
      asp = getpg (&pg, seg_n, nextpn, 's');
      nextpn = ((struct ind_page *) asp)->ind_nextpn;
      putwul (&pg, 'n');
    }
}

int
killind (struct ldesind *desind)
{
  char *asp, *a;
  u2_t pn, nextpn, d_fn;
  struct ind_page *indph;
  struct A pg;

  l_emp_ini_check();
  thread_s_ini_check();
  
  seg_n = desind->i_segn;
  afn = (u2_t *) (desind + 1);
  k_n = desind->ldi.kifn & ~UNIQ & MSK21B;
  d_f = desind->pdf;
  d_fn = k_n;
  if ((k_n % 2) != 0)
    d_fn += 1;
  d_f2 = (struct des_field *) (afn + d_fn);
  k2sz = d_f2->field_size;
  pn = desind->ldi.rootpn;
  for (;;)
    {
      l_emp_put (pn);
      asp = getpg (&pg, seg_n, pn, 's');
      thread_s_put (pn);
      indph = (struct ind_page *) asp;
      if (indph->ind_wpage == LEAF)
	break;
      a = asp + indphsize + size2b;
      a += kszcal (a, afn, d_f) + k2sz;
      pn = t2bunpack (a);
      nextpn = indph->ind_nextpn;
      putwul (&pg, 'n');
      roll_level (nextpn);
    }
  nextpn = indph->ind_nextpn;
  putwul (&pg, 'n');
  roll_level (nextpn);
  {
    i4_t i;
    for (i = 0; i < l_emp.count; i++)
      emptypg (seg_n, l_emp.arr[i], 'f');
    l_emp_ini();
  }
  
  downunlock ();
  thread_s_ini();
  return 0;
}
