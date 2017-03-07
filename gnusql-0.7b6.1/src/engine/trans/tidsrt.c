/*  tidsrt.c - Functions dealing with sort a filter by tid's
 *             Kernel of GNU SQL-server. Sorter     
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

/* $Id: tidsrt.c,v 1.249 1998/09/29 21:25:48 kimelman Exp $ */

#include "xmem.h"
#include "dessrt.h"
#include "fdcltrn.h"
#include "fdclsrt.h"

extern i4_t N, NB, NFP;
extern char *nonsense;
extern char *regakr;
extern u2_t pnex, lastpnex, freesz;
extern u2_t offset;
extern i4_t extent_size;
extern u2_t *arrpn;
extern u2_t *cutfpn;
extern u2_t outpn;
extern char *outasp;
extern char *masp[];
extern char *akr;
extern char **regpkr;
extern i4_t segsize;
extern i4_t need_free_pages;
extern u2_t load_arrpn[];

void
quick_sort_tid (i4_t M)
{
  i4_t bstek[STEKSZ];
  i4_t *stek, l, r, i, j, v;
  char **ptp, *curpk;

  ptp = (char **) regakr;
  stek = bstek;
  for (l = 1, r = N;;)
    {
      if ((r - l) < M)
	{			/* The simple insertion sort */
	  for (j = l + 1; j <= r; j++)
	    {
	      curpk = ptp[j];
	      for (i = j - 1; i >= l;)
		if ((v = cmp_tid (curpk, ptp[i])) < 0)
		  {
		    ptp[i + 1] = ptp[i];
		    i--;
		  }
		else
		  break;
	      ptp[i + 1] = curpk;
	    }
	  if (stek == bstek)
	    break;
	  r = *stek--;
	  l = *stek--;
	  continue;
	}
      i = l;			/* quicksort */
      j = r;
      curpk = ptp[l];
  m1:
      for (v = -1; v < 0; j--)
	v = cmp_tid (curpk, ptp[j]);
      if (i >= j)
	{
	  ptp[i] = curpk;
	  goto m3;
	}
      ptp[i++] = ptp[j];
      for (v = -1; v < 0; i++)
	v = cmp_tid (ptp[i], curpk);
      if (i < j)
	{
	  ptp[j] = ptp[i];
	  j--;
	  goto m1;
	}
      ptp[j] = curpk;
      i = j;
    m3:if ((i - l) <= (r - i))
	{
	  *stek++ = i + 1;
	  *stek++ = r;
	  r = i - 1;
	}
      else
	{
	  *stek++ = l;
	  *stek++ = i - 1;
	  l = i + 1;
	}
    }
}

void
puts_tid (void)
{
  char *pnt;
  i4_t i;
  u2_t off, curpn;
  struct des_tid *tid;
  char tmp_buff[BD_PAGESIZE];

  off = size4b;
  pnt = tmp_buff + off;
  cutfpn[NB] = pnex;
  for (tid = (struct des_tid *) regakr, i = 0; i < N; i++, tid++)
    {
      if ((tidsize + off) > BD_PAGESIZE)
	{
          curpn = pnex;
	  ++pnex;
	  if (pnex == lastpnex)
	    addext ();
	  t2bpack (pnex, tmp_buff);
	  t2bpack (off, tmp_buff + size2b);
	  write_tmp_page (curpn, tmp_buff);
	  off = size4b;
	  pnt = tmp_buff + off;
	}
      bcopy ((char *) tid, pnt, tidsize);
      pnt += tidsize;
      off += tidsize;
    }
  t2bpack ((u2_t) ~ 0, tmp_buff);
  t2bpack (off, tmp_buff + size2b);
  write_tmp_page (pnex, tmp_buff);
  NB++;
  pnex++;
  if (pnex == lastpnex)
    addext ();
  akr = regakr;
  freesz = segsize;  
}

struct el_tree *
ext_sort_tid (i4_t M, struct el_tree *tree)
{
  i4_t i, l, j, i1, v, nbnum, cutcnt;
  u2_t P;
  struct el_tree *ptr1, *ptr2, *ptr3, *q;

  quick_sort_tid (M);
  puts_tid ();
  
  need_free_pages = YES;
  for (i = 0; pnex != lastpnex; i++)
    {
      arrpn[i] = pnex++;
      if (i == NFP)
	{
	  NFP += extent_size;
	  arrpn = (u2_t *) realloc ((void *) arrpn, (size_t) NFP * size2b);
	}
    }
  for (; i < NFP; i++)
    arrpn[i] = (u2_t) ~ 0;
  q = NULL;
  for (cutcnt = 0; ; cutcnt = 0)
    {			/* The external sort by replacement selecting */
      for (P = 0; P < NB;)
	{
	  if (NB > PINIT)
	    if (NB - P + cutcnt < PINIT)
	      {
		for (i = P; P < NB; i++)
		  cutfpn[cutcnt++] = cutfpn[i];
		NB= cutcnt;
	      }	  
	  for (nbnum = 0; nbnum < PINIT && P < NB; nbnum++, P++)
            {
              read_tmp_page (cutfpn[P], masp[nbnum]);
              load_arrpn[nbnum] = cutfpn[P];
            }
	  if (nbnum == 0)
	    perror ("SRT.ext_sort_tid: No segments for key file pages");
	  if (P == 1)
	    {
	      tree->pket = masp[0] + size4b;
	      return (tree);
	    }
	  if (nbnum == 1)
	    perror ("SRT.extsort: Serious error nbnum == 1");	    

	  for (i = 0; i < nbnum; i++)
	    {			/* initial tree */
	      ptr1 = tree + i;
	      ptr1->pket = masp[i] + phfsize;
	      ptr1->fe = tree + (i + nbnum) / 2;
	      ptr1->fi = tree + i / 2;
	    }
	  l = nbnum / 2;
	  if (nbnum % 2 != 0)
	    l++;
	  for (j = nbnum - 1; j >= l; j--)
	    {			
	      i = (2 * j) % nbnum;	/* j - node counter, i - tree index */
	      i1 = i + 1;
	      ptr1 = tree + i;
	      ptr2 = tree + i1;
	      ptr3 = tree + j;
	      if ((v = cmp_tid (ptr1->pket, ptr2->pket)) <= 0)
		{
		  ptr3->lsr = ptr2;
		  ptr3->first = ptr1;
		}
	      else
		{
		  ptr3->lsr = ptr1;
		  ptr3->first = ptr2;
		}
	    }
	  for (; j > 0; j--)
	    {			
	      i = 2 * j;
	      if ((i1 = i + 1) == nbnum)
		i1 = 0;
	      tree->first = tree;
	      ptr1 = (tree + i)->first;
	      ptr2 = (tree + i1)->first;
	      ptr3 = tree + j;
	      if ((v = cmp_tid (ptr1->pket, ptr2->pket)) <= 0)
		{
		  ptr3->lsr = ptr2;
		  ptr3->first = ptr1;
		}
	      else
		{
		  ptr3->lsr = ptr1;
		  ptr3->first = ptr2;
		}
	    }
	  q = tree->lsr = (tree + 1)->first;	/* The new champion */
	  if (NB <= PINIT)
	    {
	      NB = nbnum;
	      return (q);
	    }
	  getptmp ();
	  offset = size4b;
	  cutfpn[cutcnt++] = outpn;		  
	  push_tid (middle_put_tid, tree, q, nbnum);
	  t2bpack ((u2_t) ~0, outasp);
	  t2bpack (offset, outasp + size2b);
	  write_tmp_page (outpn, outasp);		  
	}
      NB = cutcnt;
    }
}

void
push_tid (void (*pnt_puttid) (), struct el_tree *tree, struct el_tree *q, i4_t nbnum)
{
  struct el_tree *ptree, *qq;
  char *pkr, *asp, *last_pnt;
  i4_t v;
  u2_t pn;

  if (nbnum != 1)
    {
      for (;; )
	{				/*To find a loser */
	  (*pnt_puttid) (q->pket);	
	  nbnum = get_el_tr_tid (nbnum, q, q - tree);
	  for (ptree = q->fe ;; ptree = ptree->fi)
	    {
	      if ((v = cmp_tid (ptree->lsr->pket, q->pket)) < 0)
		{
		  qq = q;
		  q = ptree->lsr;
		  ptree->lsr = qq;
		}
	      if (ptree == tree + 1)
		break;
	    }
          if (nbnum == 1)
            break;
	}
    }
  /* write a tail of the last cut */
  v = q - tree;
  asp = masp[v];
  pkr = q->pket;
  pn = t2bunpack (asp);
  last_pnt = asp + t2bunpack (asp + size2b);
  for (; pkr < last_pnt; pkr += tidsize)
    (*pnt_puttid) (pkr);
  for (; pn != (u2_t) ~ 0;)
    {
      read_tmp_page (pn, asp);
      load_arrpn[v] = pn;
      pkr = asp + size4b;
      last_pnt = asp + t2bunpack (asp + size2b);
      for (; pkr < last_pnt; pkr += tidsize)
	(*pnt_puttid) (pkr);
      pn = t2bunpack (asp);      
    }
}

int
get_el_tr_tid (i4_t nbnum, struct el_tree *q, i4_t i)
{
  char *pkr;

  pkr = q->pket;
  pkr += tidsize;
  return (next_el_tree (nbnum, q, i, pkr));
}

int
cmp_tid (char *pnt1, char *pnt2)
{
  i4_t i, v;

  for (i = 0; i < 2; i++)
    {
      if ((v = t2bunpack (pnt1) - t2bunpack (pnt2)) != 0)
	return (v);
      pnt1 += size2b;
      pnt2 += size2b;
    }
  return (0);
}

