/*
 *  queue.c  -  This file contains functions maintaining queues of 
 *              waiting locks
 *              Kernel of GNU SQL-server. Buffer 
 *
 *     We assume: the last element of a ring lies in the field "p_queue"
 *  of the current page descriptor; we include queue items at the end of the
 *  ring and exclude items at the beginning of it.
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

/* $Id: queue.c,v 1.247 1998/09/29 21:25:03 kimelman Exp $ */

#include "setup_os.h"
#include "inpop.h"
#include "bufdefs.h"
#include "fdeclbuf.h"
      
struct WAIT *
new_wait (u2_t conn, u2_t type, u2_t tact, u2_t prg)
{
  struct WAIT *wait;

  wait = (struct WAIT *) get_empty (sizeof (struct WAIT));
  wait->w_conn = conn;
  wait->w_type = type;
  wait->w_tact = tact;
  wait->w_prget = prg;
  return (wait);
}

void
into_wait (struct PAGE *page, struct WAIT *wait) /* insert new waiting lock */
{
  struct WAIT *w;

  w = page->p_queue;
  wait->w_next = NULL;
  if (!w)
    page->p_queue = wait;
  else
    {
      while (w->w_next)
        w = w->w_next;
      w->w_next = wait;
    }
}

void
out_first (struct PAGE *page)		/* exclude first waiting lock */
{
  struct WAIT *wait;

  wait = page->p_queue;
  assert(wait);
  page->p_queue = wait->w_next;
  xfree ((char *) wait);
}

void
set_weak_locks (struct PAGE *page)
{
  struct WAIT *wait;
  struct BUFF *buf;
  u2_t prg, trnum;

  while (page->p_queue->w_type == WEAK)
    {
      wait = page->p_queue;
      page->p_status++;
      prg = wait->w_prget;
      trnum = wait->w_conn;
      out_first (page);
      if (prg == 1)
	{
	  buf = get (page->p_seg, page->p_page, 'r');
	  buf_to_user (trnum, buf->b_seg->keyseg);
	}
      else
	user_p (trnum, 0);
    }
}

/********************************* the end **********************************/
