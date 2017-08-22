/*  extsrt.c - External sorting
 *             Kernel of GNU SQL-server. Sorter   
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

/* $Id: extsrt.c,v 1.247 1997/08/19 19:18:40 kml Exp $ */

#include "setup_os.h"

#include "dessrt.h"
#include "fdclsrt.h"
#include "fdcltrn.h"

extern i4_t NB, NFP;
extern u2_t pnex, lastpnex, outpn;
extern u2_t *arrpn;
extern u2_t *cutfpn;
extern char *outasp;
extern u2_t offset;
extern char *masp[];
extern u2_t extent_size;
extern i4_t need_free_pages;
extern u2_t load_arrpn[];

struct el_tree *
extsort (i4_t M, struct des_sort *sdes, struct el_tree *tree)
{
  i4_t i, l, j, i1, v, nbnum, cutcnt;
  u2_t P;
  struct el_tree *ptr1, *ptr2, *ptr3, *q;
  char prdbl;

  quicksort (M, sdes);
  putkf ();
  
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
  prdbl = sdes->s_prdbl;
  for (cutcnt = 0; ; cutcnt = 0)
    {		/* The external sort by replacement selecting */
      for (P = 0; P < NB;)
	{
	  if (NB > PINIT)
	      if (NB - P + cutcnt < PINIT)
		{
		  for (; P < NB; P++)
		    cutfpn[cutcnt++] = cutfpn[P];
		  NB= cutcnt;
		  P = 0;
		}
	  for (nbnum = 0; nbnum < PINIT && P < NB; nbnum++, P++)
            {
              read_tmp_page (cutfpn[P], masp[nbnum]);
              load_arrpn[nbnum] = cutfpn[P];
            }
	  if (nbnum == 0)
	    perror ("SRT.extsort: No segments for key file pages");
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
	      ptr1->pket = masp[i] + size4b;
	      ptr1->fe = tree + (i + nbnum) / 2;
	      ptr1->fi = tree + i / 2;
	    }
      m1:
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
	      if ((v = cmp_key (sdes, ptr1->pket, ptr2->pket)) <= 0)
		{
		  ptr3->lsr = ptr2;
		  ptr3->first = ptr1;
		}
	      else
		{
		  ptr3->lsr = ptr1;
		  ptr3->first = ptr2;
		}
	      if (prdbl == NODBL && v == 0)
		{
		  if ((nbnum = geteltr (nbnum, ptr2, i1)) == 1)
		    goto m2;
		  else
		    goto m1;
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
	      if ((v = cmp_key (sdes, ptr1->pket, ptr2->pket)) <= 0)
		{
		  ptr3->lsr = ptr2;
		  ptr3->first = ptr1;
		}
	      else
		{
		  ptr3->lsr = ptr1;
		  ptr3->first = ptr2;
		}
	      if (prdbl == NODBL && v == 0)
		{
		  if ((nbnum = geteltr (nbnum, ptr2, i1)) == 1)
		    break;
		  else
		    goto m1;
		}
	    }
      m2:
	  q = tree->lsr = (tree + 1)->first;	/* The new champion */
	  if (NB <= PINIT)
	    {
	      NB = nbnum;
	      return (q);
	    }
	  getptmp ();
	  offset = size4b;	
	  cutfpn[cutcnt++] = outpn;
                              /*Into key record file*/
	  push (putkr, tree, q, sdes, nbnum);
	  t2bpack ((u2_t) ~0, outasp);
	  t2bpack (offset, outasp + size2b);
	  write_tmp_page (outpn, outasp);	  
	}
      NB = cutcnt;
    }
}

