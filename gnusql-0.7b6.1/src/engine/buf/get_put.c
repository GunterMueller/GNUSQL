/*
 *  get_put.c  -  This file contains the functions which support
 *                operations get/put pages.
 *                Kernel of GNU SQL-server. Buffer  
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

/* $Id: get_put.c,v 1.246 1998/09/29 21:25:02 kimelman Exp $ */


#include "setup_os.h"
#include "inpop.h"
#include "bufdefs.h"
#include "fdeclbuf.h"
#include "totdecl.h"

/*****************************************************************************

                               GET/PUT PAGE
*/
#define size2b sizeof(u2_t)

extern i4_t cur_endmj;

struct BUFF *
get (u2_t sn, u2_t pn, char pr) /* get page */
{/* pr - read or not */
  struct BUFF *buf;
  struct PAGE *page;

  page = find_page (sn, pn);
  if (page == NULL)
    page = new_page (sn, pn);
  buf = page->p_buf;
  if (buf == NULL)
    {
      buf = get_buf ();
      page->p_buf = buf;
      buf->b_page = page;
      set_prio (buf, USED);
      if (pr == 'r')
	read_buf (buf);
    }
  else if (buf->b_status < USED)
    change_prio (buf, USED);

  buf->b_status++;
  return (buf);
}

void
put (u2_t sn, u2_t pn, i4_t address, char prmod)
{ /* prmod - to push buffer or not */
  struct PAGE *page;
  struct BUFF *buf;

  page = find_page (sn, pn);
  buf = page->p_buf;
  if (address != NULL_MICRO)
    {
      u2_t off, pn_eomj, off_eomj;
      char *a, *b;
      buf->b_micro = address;
      a = (char *) &address;
      b = (char *) &cur_endmj;
      pn = t2bunpack (a);
      pn_eomj = t2bunpack (b);
      off = t2bunpack (a + size2b);
      off_eomj = t2bunpack (b + size2b);
      if (pn > pn_eomj)
	cur_endmj = address;
      else if (pn == pn_eomj && off > off_eomj)
	cur_endmj = address;
    }
  if (buf->b_prmod != PRMOD)
    buf->b_prmod = prmod;
  buf->b_status--;
  if (buf->b_status == USED)
    {
      if (page->p_queue != NULL)
	change_prio (buf, LOCKED);
      else
	change_prio (buf, FREE);
    }
}

/******************************** the end ***********************************/
