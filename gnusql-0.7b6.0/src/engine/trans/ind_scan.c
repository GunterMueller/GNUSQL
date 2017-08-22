/*
 *  ind_scan.c  - Index Control Programm
 *               functions dealing with scanning of an index
 *               Kernel of GNU SQL-server
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

/* $Id: ind_scan.c,v 1.247 1998/05/20 05:52:43 kml Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "sctp.h"
#include "strml.h"
#include "fdcltrn.h"

extern struct des_field *d_f, *d_f2;
extern u2_t seg_n;
extern u2_t *afn;
extern u2_t k_n;
extern i4_t k2sz;

static char *
find_first_key (struct A *pg, u2_t pn)
{
  char *asp = NULL, *a;
  u2_t cpn, ppn;
  struct ind_page *indph;

  while ((asp = getpg (pg, seg_n, pn, 's')) == NULL);
  indph = (struct ind_page *) asp;
  for (; indph->ind_wpage != LEAF;)
    {
      a = asp + indphsize + size2b;
      a += kszcal (a, afn, d_f) + k2sz;
      cpn = t2bunpack (a);
      putwul (pg, 'n');
      ppn = pg->p_pn;
      while ((asp = getpg (pg, seg_n, cpn, 's')) == NULL);
      indph = (struct ind_page *) asp;
      BUF_unlock (seg_n, 1, &ppn);
    }
  return (asp);
}

#define	BETWEEN_CMP  (t == SS || t == SES || t == SSE || t == SESE)

static int
cmp_with_dia (char *key, char *diasc, char *diaval)
{
  char *kval, *keyval;
  i4_t sst, k1, ftype, v, v1;
  u2_t k;
  unsigned char t;

  sst = 1;
  kval = keyval = key + scscal (key);
  t = selsc1 (&diasc, sst++);
  v = v1 = 0;
  for (k = 0, k1 = 0; t != ENDSC && k < k_n && key < keyval; k++, k1++)
    {
      if (k1 == 7)
	{
	  k1 = 0;
	  key++;
	}
      if ((*key & BITVL(k1)) != 0)
	{
	  if (t == EQUN)
	    return (1);
	  ftype = (d_f + afn[k])->field_type;
	  if (t == NEQUN || t == ANY)
	    kval = proval (kval, ftype);
	  else
	    {
              v = cmpval (kval, diaval, ftype);
	      diaval = proval (diaval, ftype);	
	      if (v == 0) /* current == first */
		{
		  if (t == SML)
		    return (EOI);
		  if (t == NEQ || t == GRT || t == SS || t == SSE)
		    return (1);
		}
	      else if (v > 0) /* current > first */
                {
                  if (t == EQ || t == SML || t == SMLEQ)
                    return (EOI);
                  if (BETWEEN_CMP)
                    {
                      v1 = cmpval (kval, diaval, ftype);
                      diaval = proval (diaval, ftype);
                      if (v1 == 0) /* current == last */
                        if (t == SS || t == SES)
                          return (EOI);
                        else {}
                      else if (v1 > 0) /* current > last */
                        return (EOI);
                    }
                }
              else /* current < first */
                {
                  if (!(t == NEQ || t == SML || t == SMLEQ))
                    return (1);
                }
              kval = proval (kval, ftype);
	    }
	}
      else
	{			/* values are absent into the key */
	  if (!(t == EQUN || t == ANY))
	    return (EOI);
	}
      t = selsc1 (&diasc, sst++);
    }
  if (key == keyval && t != ENDSC)
    return (EOI);
  return (OK);
}

#undef	BETWEEN_CMP

static char *
find_next_agr (struct A *pg, char *diasc, char *diaval,
               i4_t elsz, char *key, i4_t *agrloc)
{
  char *lastb, *agr, *asp = NULL;
  struct ind_page *indph;
  u2_t pn, ppn, n, off;
  i4_t keysz, l;

  asp = pg->p_shm;
  pn = pg->p_pn;
  indph = (struct ind_page *) asp;

  for (;;)
    {
      off = indph->ind_off;
      lastb = asp + off;
      for (; key < lastb; key += keysz + n * elsz)
	{
	  agr = key;
	  n = t2bunpack (key);
	  key += size2b;
	  keysz = kszcal (key, afn, d_f);
	  if (diasc == NULL)
	    {
	      *agrloc = agr - asp;
	      return (key + keysz);
	    }
	  if ((l = cmp_with_dia (key, diasc, diaval)) == EOI)
            return (NULL);
	  if (l == OK)
	    {
	      *agrloc = agr - asp;
	      return (key + keysz);
	    }
	}
      pn = indph->ind_nextpn;
      if (pn == (u2_t) ~ 0)
        return (NULL);
      putwul (pg, 'n');
      ppn = pg->p_pn;
      while ((asp = getpg (pg, seg_n, pn, 's')) == NULL);
      indph = (struct ind_page *) asp;
      BUF_unlock (seg_n, 1, &ppn);
      key = asp + indphsize;
    }
}

static char *
first_key_frm (char *diasc, char *diaval, char *mas)
{
  char *a;
  i4_t ftype;
  u2_t size;
  unsigned char t;

  t = selsc1 (&diasc, 1);
  if (t == EQ || t == GRT || t == GRTEQ || t == SS || t == SES || t == SSE || t == SESE)
    {
      a = mas;
      ftype = (d_f + afn[0])->field_type;
      if (ftype == TCH || ftype == TFL)
	{
	  size = t2bunpack (diaval);
	  diaval += size2b;
	  t2bpack (size, a);
	  a += size2b;
	}
      else
	size = (d_f + afn[0])->field_size;
      bcopy (diaval, a, size);
    }
  else
    mas = NULL;
  return (mas);
}

static int
cmpfval (char *key, char *aval)
     /* key - in the index, aval - from diasc */
{
  char *kval;
  i4_t v = 1;

  kval = key + scscal (key);
  if ((*key & BITVL(0)) != 0)
    v = cmpval (kval, aval, (d_f + afn[0])->field_type);
  return (v);
}

static char *
flookup (struct A *pg, u2_t pn, char *key, i4_t infsz, char **rasp)
{
  char *asp = NULL, *a, *lastb;
  u2_t ppn, off;
  i4_t elsz;
  u2_t n;
  struct ind_page *indph;

  while ((asp = getpg (pg, seg_n, pn, 's')) == NULL);
  indph = (struct ind_page *) asp;
  for (; indph->ind_wpage != LEAF;)
    {
      off = indph->ind_off;
      lastb = asp + off;
      elsz = k2sz + size2b;
      for (a = asp + indphsize; a < lastb; a += kszcal (a, afn, d_f) + n * elsz)
	{
	  n = t2bunpack (a);
	  a += size2b;
	  if (cmpfval (a, key) >= 0)
	    {
	      a += kszcal (a, afn, d_f) + k2sz;
	      break;
	    }
	}
      if (a == lastb)
	{
	  *rasp = asp;
	  return (a);
	}
      pn = t2bunpack (a);
      a += size2b;
      putwul (pg, 'n');
      ppn = pg->p_pn;
      while ((asp = getpg (pg, seg_n, pn, 's')) == NULL);
      indph = (struct ind_page *) asp;
      BUF_unlock (seg_n, 1, &ppn);
    }
  elsz = k2sz + infsz;
  off = indph->ind_off;
  lastb = asp + off;
  for (a = asp + indphsize; a < lastb; a += kszcal (a, afn, d_f) + n * elsz)
    {
      n = t2bunpack (a);
      a += size2b;
      if (cmpfval (a, key) >= 0)
	{
	  a -= size2b;
	  break;
	}
    }
  *rasp = asp;
  return (a);
}

i4_t
fscan_ind (struct ldesscan *desscn, char *key2, char *inf, i4_t infsz, char modescan)
{
  char *a, *diasc, *diaval, *ckey;
  u2_t rootpn, d_fn;
  struct ldesind *desind;
  struct A pg;
  i4_t agrloc, keysz;
  char *asp, *key;
  char mas[BD_PAGESIZE];

  desind = desscn->pdi;
  rootpn = desind->ldi.rootpn;
  seg_n = desind->i_segn;
  diasc = desscn->dpnsc;
  diaval = desscn->dpnsval;
  afn = (u2_t *) (desind + 1);
  k_n = desind->ldi.kifn & ~UNIQ & MSK21B;
  d_f = desind->pdf;
  d_fn = k_n;
  if ((k_n % 2) != 0)
    d_fn += 1;
  d_f2 = (struct des_field *) (afn + d_fn);
  k2sz = d_f2->field_size;
  if (diasc != NULL)
    {
      diasc += size2b;
      key = first_key_frm (diasc, diaval, mas);
      if (key != NULL)
	{
	  a = flookup (&pg, rootpn, key, infsz, &asp);
	}
      else
	{
	  asp = find_first_key (&pg, rootpn);
	  a = asp + indphsize;
	}
      a = find_next_agr (&pg, diasc, diaval, k2sz + infsz, a, &agrloc);
      asp = pg.p_shm;
      if (a == NULL)
	{
	  desscn->curlpn = (u2_t) ~ 0;
	  putpg (&pg, 'n');
	  return (EOI);
	}
    }
  else
    {
      asp = find_first_key (&pg, rootpn);
      if (((struct ind_page *) asp)->ind_off == indphsize)
	{
	  desscn->curlpn = (u2_t) ~ 0;
	  putpg (&pg, 'n');
	  return (EOI);
	}
      a = asp + indphsize;
      agrloc = a - asp;
      a += size2b; 
      a += kszcal (a, afn, d_f);
    }
  bcopy (a, key2, k2sz);
  a += k2sz;
  if (inf != NULL)
    {
      bcopy (a, inf, infsz);
      a += infsz;
    }
  /* write cursors into desscn */
  desscn->curlpn = pg.p_pn;
  desscn->offa = agrloc;
  desscn->offp = a - asp - k2sz - infsz;
  desscn->sidmod = ((struct p_head *) asp)->idmod;
  a = asp + agrloc + size2b;
  keysz = kszcal (a, afn, d_f);
  ckey = (char *) xmalloc (keysz);
  desscn->cur_key = ckey;
  bcopy (a, ckey, keysz);
  if (modescan == FASTSCAN)
    putwul (&pg, 'n');
  else
    putpg (&pg, 'n');
  return (OK);
}

static
int
find_next_key(struct ldesscan *desscn, struct A *pg, char *diasc, char *diaval, i4_t elsz, char *agr, char *pnt, i4_t *agrloc, i4_t *lockey)
{
  i4_t agsz;
  u2_t n;
  char *asp;

  asp = pg->p_shm;
  *agrloc = agr - asp;
  n = t2bunpack (agr);
  agsz = size2b + kszcal (agr + size2b, afn, d_f) + n * elsz;
  pnt += elsz;
  if (pnt == agr + agsz)
    {
      pnt = find_next_agr (pg, diasc, diaval, elsz, pnt, agrloc);
      if (pnt == NULL)
	{
	  putpg (pg, 'n');
	  desscn->curlpn = (u2_t) ~ 0;
	  return (EOI);
	}
      asp = pg->p_shm;
    }
  *lockey = pnt - asp;
  return (OK);
}

static
char *
look_up(struct A *pg, u2_t pn, char *key, char *key2, i4_t infsz, char **agr, char **loc)
{
  char *a, *lastb, *asp = NULL;
  i4_t elsz, l, l2 = 0, ksz;
  u2_t n, ppn;
  struct ind_page *indph;

  while ((asp = getpg (pg, seg_n, pn, 's')) == NULL);
  indph = (struct ind_page *) asp;
  elsz = k2sz + size2b;
  for (; indph->ind_wpage != LEAF;)
    {
      lastb = asp + indph->ind_off;
      for (a = asp + indphsize; a < lastb; )
	{
	  n = t2bunpack (a);
	  a += size2b;
          ksz = kszcal (a, afn, d_f);
	  if ((l = cmpkeys (k_n, afn, d_f, a, key)) == 0)
	    {
	      a += ksz;
              for (; n != 0; n--, a += elsz)
                if ((l2 = cmp2keys (d_f2->field_type, a, key2)) < 0)
                  continue;
                else
                  break;
              if (n == 0 && l2 < 0)
                continue;
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
	  *loc = NULL;
	  return (NULL);
	}
      a += k2sz;
      ppn = pn;
      pn = t2bunpack (a);
      putwul (pg, 'n');
      while ((asp = getpg (pg, seg_n, pn, 's')) == NULL);
      indph = (struct ind_page *) asp;
      BUF_unlock (seg_n, 1, &ppn);
    }
  lastb = asp + indph->ind_off;
  a = asp + indphsize;
  if (a == lastb)
    {
      *loc = NULL;
      return (NULL);
    }
  elsz = k2sz + infsz;
  for (; a < lastb; a += kszcal (a, afn, d_f) + n * elsz)
    {
      *agr = a;
      n = t2bunpack (a);
      a += size2b;
      if ((l = cmpkeys (k_n, afn, d_f, a, key)) == 0)
	{
	  a += kszcal (a, afn, d_f);
	  for (; n != 0; n--, a += elsz)
	    {
	      if ((l2 = cmp2keys (d_f2->field_type, a, key2)) < 0)
		continue;
              if (l2 == 0)
                {
                  *loc = a;
                  return (a);
                }
              *loc = a;
              return (NULL);
	    }
	  *loc = a;
          return (NULL);
	}
      else if (l > 0)
	{
	  a += kszcal (a, afn, d_f);
          *loc = a;
	  return (NULL);
	}
    }
  *loc = NULL;
  return (NULL);
}

i4_t
scan_ind (struct ldesscan *desscn, char *key2, char *inf, i4_t infsz, char modescan)
{
  char *asp = NULL, *a, *lastb, *diasc, *diaval, *ckey;
  u2_t pn, n, d_fn, off;
  i4_t elsz, agrloc, loc;
  struct ind_page *indph;
  struct ldesind *desind;
  struct A pg;
  char *agr, *lockey;
  i4_t keysz;
  i4_t l, l2=0;

  pn = desscn->curlpn;
  if (pn == (u2_t) ~ 0)
    return (EOI);
  diasc = desscn->dpnsc + size2b;
  diaval = desscn->dpnsval;
  desind = desscn->pdi;
  seg_n = desind->i_segn;
  afn = (u2_t *) (desind + 1);
  k_n = desind->ldi.kifn & ~UNIQ & MSK21B;
  d_f = desind->pdf;
  d_fn = k_n;
  if ((k_n % 2) != 0)
    d_fn += 1;
  d_f2 = (struct des_field *) (afn + d_fn);
  k2sz = d_f2->field_size;
  elsz = k2sz + infsz;
  if (modescan == FASTSCAN)
    asp = getwl (&pg, seg_n, pn);
  else
    while ((asp = getpg (&pg, seg_n, pn, 's')) == NULL);

  indph = (struct ind_page *) asp;
  off = indph->ind_off;
  lastb = asp + off;
  if (desscn->sidmod != indph->ind_ph.idmod)
    {
      ckey = desscn->cur_key;
      a = asp + indphsize;
      if (a < lastb && cmpkeys (k_n, afn, d_f, a + size2b, ckey) <= 0)
        {         /* look up in this page */
          for (; a < lastb; a += kszcal (a, afn, d_f) + n * elsz)
            {			
              agr = a;
              n = t2bunpack (a);
              a += size2b;
              if ((l = cmpkeys (k_n, afn, d_f, a, ckey)) >= 0)
                {
                  a += kszcal (a, afn, d_f);
                  if (l == 0)         /* current key is present */
                    {
                      for (; n != 0; n--, a += elsz)
                        if ((l2 = cmp2keys (d_f2->field_type, a, key2)) < 0)
                          continue;
                        else
                          break;
                      if (l2 == 0)     /* current key2 is present */
                        {
                          if (find_next_key (desscn, &pg, diasc, diaval, elsz, agr, a, &agrloc, &loc) == EOI)
                            return (EOI);
                          goto m1;
                        }
                      else             /* current key2 is absent */
                        {
                          agrloc = agr - asp;
                          loc = a - asp;
                          goto m1;
                        }
                    }
                  else                 /* current key is absent */
                    {
                      if (cmp_with_dia (agr + size2b, diasc, diaval) == EOI)
                        {
                          putpg (&pg, 'n');
                          return (EOI);
                        }
                      agrloc = agr - asp;      /* next key */
                      loc = a - asp;
                      goto m1;
                    }
                }
            }
        }
      putpg (&pg, 'n');
      look_up(&pg, desind->ldi.rootpn, desscn->cur_key, key2, infsz, &agr, &lockey);
      /*      a = icp_lookup (&pg, desind, desscn->cur_key, key2, infsz, &agr, &lockey);*/
      asp = pg.p_shm;
      if (lockey == NULL)
        {
          desscn->curlpn = (u2_t) ~ 0;
          putpg (&pg, 'n');
          return (EOI);
        }
      if (a == NULL)    /* current key is absent */
        {
          if (cmp_with_dia (agr + size2b, diasc, diaval) == EOI)
            {
              putpg (&pg, 'n');
              return (EOI);
            }
          loc = lockey - asp;
          agrloc = agr - asp;
        }
      else              /* current key is present */
        {
          if (find_next_key (desscn, &pg, diasc, diaval, elsz, agr, lockey, &agrloc, &loc) == EOI)
            return (EOI);
        }
    }
  else
    {
      agr = asp + desscn->offa;
      a = asp + desscn->offp;
      if (find_next_key (desscn, &pg, diasc, diaval, elsz, agr, a, &agrloc, &loc) == EOI)
        return (EOI);
    }
m1:
  asp = pg.p_shm;
  a = asp + loc;
  bcopy (a, key2, k2sz);
  a += k2sz;
  if (inf != NULL)
    bcopy (a, inf, infsz);
  /* write cursors into desscn */
  desscn->curlpn = pg.p_pn;
  desscn->offa = agrloc;
  desscn->offp = loc;
  desscn->sidmod = ((struct p_head *) asp)->idmod;
  if (desscn->cur_key != NULL)
    xfree (desscn->cur_key);
  a = asp + agrloc + size2b;
  keysz = kszcal (a, afn, d_f);
  ckey = (char *) xmalloc (keysz);
  desscn->cur_key = ckey;
  bcopy (a, ckey, keysz);
  if (modescan == SLOWSCAN)
    putpg (&pg, 'n');
  else
    putwul (&pg, 'n');
  return (OK);
}

char *
icp_lookup (struct A *pg, struct ldesind *desind, char *key, char *key2, i4_t infsz, char **agr, char **loc)
{
  u2_t d_fn;

  seg_n = desind->i_segn;
  afn = (u2_t *) (desind + 1);
  k_n = desind->ldi.kifn & ~UNIQ & MSK21B;
  d_f = desind->pdf;
  d_fn = k_n;
  if ((k_n % 2) != 0)
    d_fn += 1;
  d_f2 = (struct des_field *) (afn + d_fn);
  k2sz = d_f2->field_size;
  return (look_up(pg, desind->ldi.rootpn, key, key2, infsz, agr, loc));
}
