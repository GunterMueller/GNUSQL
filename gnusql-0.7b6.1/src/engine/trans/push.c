/*
 *  push.c - Push  a sorted segment of records
 *           Kernel of GNU SQL-server. Sorter    
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

/* $Id: push.c,v 1.248 1998/09/29 21:25:43 kimelman Exp $ */

#include "setup_os.h"

#include "dessrt.h"
#include "fdclsrt.h"
#include "fdcltrn.h"

extern i4_t NFP;
extern char *masp[];
extern u2_t *arrpn;
extern char *nonsense;
extern i4_t need_free_pages;
extern u2_t load_arrpn[];

void
push (void (*putr1) (), struct el_tree *tree, struct el_tree *q,
      struct des_sort *sdes, i4_t nbnum)
{
  char *pkr, *asp, *last_pnt;
  i4_t v;
  u2_t pn;

  if (nbnum != 1)
    {
      struct el_tree *ptree, *qq;
      char prdbl;

      prdbl = sdes->s_prdbl;
      for (;;)
	{
	  (*putr1) (q->pket);
        m1:
	  nbnum = geteltr (nbnum, q, q - tree);
	  for (ptree = q->fe ;; ptree = ptree->fi)
	    {
	      if ((v = cmp_key (sdes, ptree->lsr->pket, q->pket)) < 0)
		{
		  qq = q;
		  q = ptree->lsr;
		  ptree->lsr = qq;
		}
	      if (prdbl == NODBL && v == 0)
		goto m1;
	      if (ptree == tree + 1)
		break;
	    }
          if (nbnum == 1)
            break;
	}
    }
  /* write a tail of the last cut */
  v = q - tree;
  assert( v < PINIT );
  assert( v >= 0    );
  asp = masp[v];
  pkr = q->pket;
  assert( pkr >= asp );
  pn = t2bunpack (asp);
  last_pnt = asp + t2bunpack (asp + size2b);
  for (; pkr < last_pnt; pkr += t2bunpack (pkr))
    (*putr1) (pkr);
  for (; pn != (u2_t) ~ 0;)
    {
      read_tmp_page (pn, asp);
      load_arrpn[v] = pn;
      pkr = asp + size4b;
      last_pnt = asp + t2bunpack (asp + size2b);
      for (; pkr < last_pnt; pkr += t2bunpack (pkr)) 
        (*putr1) (pkr);
      pn = t2bunpack (asp);
    }
}

int
geteltr (i4_t nbnum, struct el_tree *q, i4_t i)
{
  char *pkr;
  u2_t size;
  
  assert( i < PINIT );
  assert( i >= 0    );
  pkr = q->pket;
  size = t2bunpack (pkr);
  pkr += size;
  return (next_el_tree (nbnum, q, i, pkr));
}

int
next_el_tree (i4_t nbnum, struct el_tree *q, i4_t i, char *pkr)
{
  char *a;
  u2_t off;
  
  assert( i < PINIT );
  assert( i >= 0    );
  a = masp[i];
  off = t2bunpack (a + size2b);
  if (pkr == (a + off))
    {
      u2_t pn;
      pn = t2bunpack (a);
      if (pn == (u2_t) ~ 0)
	{
	  q->pket = nonsense;
	  nbnum--;
	}
      else
	{
	  if (need_free_pages == YES)
	    {
              i4_t n;
	      for (n = 0; n < NFP; n++)
		if (arrpn[n] == (u2_t) ~ 0)
		  {
		    arrpn[n] = load_arrpn[i];
		    break;
		  }
	      if (n == NFP)
		{
		  NFP += PINIT;
		  arrpn = (u2_t *) realloc ((void *) arrpn, (size_t) NFP * size2b);
                  n = NFP - PINIT;
                  arrpn[n] = load_arrpn[i];
		  for (n++; n < NFP; n++)
		    arrpn[n] = (u2_t) ~ 0;
		}
	    }
	  read_tmp_page (pn,  masp[i]);
          load_arrpn[i] = pn;
	  q->pket = masp[i] + size4b;
	}
    }
  else
    q->pket = pkr;
  return (nbnum);
}
