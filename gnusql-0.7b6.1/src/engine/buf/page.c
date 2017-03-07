/*
 *  page.c  -  This file contains functions supporting the possibility
 *               of entering into the page table.
 *             Kernel of GNU SQL-server. Buffer  
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

/* $Id: page.c,v 1.247 1998/09/29 21:25:03 kimelman Exp $ */

#include "setup_os.h"
#include "inpop.h"
#include "bufdefs.h"
#include "fdeclbuf.h"

/*****************************************************************************

                                HASH TABLE
*/

struct PAGE *hash_table[HASHSIZE];

void
init_hash (void)			/* initilization of hash table */
{
  u2_t item;

  for (item = 0; item < HASHSIZE; item++)
    hash_table[item] = NULL;
}

u2_t
hash (u2_t page)			/* hash address */
{
  return (page & (HASHSIZE - 1));
}

/*****************************************************************************

                               PAGE TABLE

************************* page rings service ********************************/

struct PAGE *
new_page (u2_t seg_num, u2_t page_num)	/* full page address */
{
  struct PAGE *page;
  u2_t item;

  page = (struct PAGE *) get_empty (sizeof (struct PAGE));
  page->p_seg = seg_num;
  page->p_page = page_num;
  page->p_status = 0;
  page->p_ltype = NO_LOCK;
  page->p_queue = NULL;
  page->p_buf = NULL;

  item = hash (page_num);
  if (hash_table[item] == NULL)
    {
      page->p_next = page;
      page->p_prev = page;
      hash_table[item] = page;
    }
  else
    {
      page->p_prev = hash_table[item]->p_prev;
      hash_table[item]->p_prev->p_next = page;
      page->p_next = hash_table[item];
      hash_table[item]->p_prev = page;
    }
  return (page);
}

void
del_page (struct PAGE *page)	     /* exclude item from page ring */
{
  u2_t item;

  item = hash (page->p_page);
  if (page->p_next == page)
    hash_table[item] = NULL;
  else
    {
      page->p_next->p_prev = page->p_prev;
      page->p_prev->p_next = page->p_next;
      if (hash_table[item] == page)
	hash_table[item] = page->p_next;
    }
  xfree ((char *) page);
}

/************************* finding page descriptor **************************/

struct PAGE *
find_page (u2_t n_seg, u2_t n_page)	/* full page number */
{
  struct PAGE *page, *page0;

  page = page0 = hash_table[hash (n_page)];
  if (page != NULL)		/* if there is a nontrivial ring */
    do
      if (page->p_seg == n_seg && page->p_page == n_page)
	return (page);
      else
	page = page->p_next;
    while (page != page0);	/* search in page ring */
  return (NULL);
}

/********************************** the end *********************************/
