/*
 *  buflock.c  - This file contains the functions which support main
 *               operations lock/enforce/unlock/tact.
 *               Kernel of GNU SQL-server. Buffer 
 *
 *    The functions deal with three parts of information. Page table
 *  contains pages' descriptors including all information concerning
 *  satisfied locks and presence of pages in the pool of buffers. Hash
 *  table serves a fast search of a page descriptor by the corresponding
 *  page number. Queue table contains all waiting locks.
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

/* $Id: buflock.c,v 1.247 1998/05/20 05:48:21 kml Exp $ */

#include "setup_os.h"
#include <assert.h>
#include "inpop.h"
#include "bufdefs.h"
#include "fdeclbuf.h"

/*
 * TIMER SERVICE
 */
u2_t C_tact = 0;		/* current tact number */
extern i4_t MAXTACT;
extern struct PAGE *hash_table[];

#define size2b sizeof(u2_t)

void
tactup (void)
{
  if (++C_tact == MAXTACT)
    C_tact = 0;
}

/********************************* next tact ********************************/

void
tact (void)
{
  register i4_t i;
  register struct PAGE *page, *page0;

  tactup ();
  for (i = 0; i < HASHSIZE; i++)
    if ((page = page0 = hash_table[i]) != NULL)
      do
	{
	  for (; page->p_queue != NULL; page = page->p_next)
            if (page->p_queue->w_tact == C_tact)
              {
                u2_t trnum;
                trnum = page->p_queue->w_conn;
                if (page->p_queue->w_prget == 1)
                  buf_to_user (trnum, BACKUP);
                else
                  user_p (trnum, BACKUP);
                out_first (page);
              }
            else
              break;
	}
      while (page != page0);
}

/*****************************************************************************

                            INTERFACE FUNCTIONS

****************************** lock a page **********************************/

i4_t
buflock (u2_t conn, u2_t segn, u2_t pn, char type, u2_t prget)
		/* conn - a connection to user */
		/* segn, pn - full page number */
     		/* type - type of lock */
     		/* prdet - get page or not */
{
  struct PAGE *page;

  page = find_page (segn, pn);
  if (page == NULL)
    {
      page = new_page (segn, pn);
      page->p_ltype = type;
    }
  else
    {
      u2_t ptype;
      struct BUFF *buf;
      ptype = page->p_ltype;
      if ((ptype == STRONG) || (type == STRONG && ptype == WEAK))
	{
          struct WAIT *wait;
	  wait = new_wait (conn, type, C_tact, prget);
	  into_wait (page, wait);
          PRINTF (("BUF.buflock.e: ret -1 sn=%d,pn=%d, ptype = %d, "
                   "type = %d\n", segn, pn, ptype, type));
	  return (-1);
	}
      else
	page->p_ltype = type;
      if ((buf = page->p_buf) != NULL)	/* if it can be locked */
	if (buf->b_status < LOCKED)
	  change_prio (buf, LOCKED);
    }
  if (page->p_ltype == WEAK)
    page->p_status++;
  return (0);
}

/**************************** enforce a lock ********************************/

i4_t
enforce (u2_t conn, u2_t segn, u2_t pn)
	/* conn - connection to user   */
	/* segn, pn - full page number */
{
  struct PAGE *page;

  page = find_page (segn, pn);
  if (page == NULL)
    printf ("BUF.enforce: sn=%d,pn=%d, status = %d\n", segn,pn, page->p_status);
  assert (page != NULL);
  if (page->p_status == 1)
    {				/* if lock can be enforced immediately */
      page->p_ltype  = STRONG;
      page->p_status = 0;
      return (0);
    }
  else
    {
      into_wait (page, new_wait (conn, STRONG, C_tact, 0));
      PRINTF (("BUF.enforce.e: ret -1 sn=%d,pn=%d\n", segn, pn));
      return (-1);
    }
}

/***************************** unlock a page ********************************/

void
unlock (u2_t segn, u2_t lnum, char *p)
{
  register i4_t stat;
  u2_t i, trnum, prg, pn;
  struct PAGE *page;
  struct WAIT *wait;
  struct BUFF *buf;

  for (i = 0; i < lnum; i++)
    {
      BUFUPACK(p,pn);
      page = find_page (segn, pn);
      if (page == NULL)
        printf ("BUF.unlock: sn=%d,lnum=%d,pn=%d,i=%d\n",segn,lnum,pn,i);
      assert (page != NULL);
      switch(page->p_ltype)
	{
        case  WEAK:
          stat = (--page->p_status);
          wait = page->p_queue;
	  if (wait == NULL)
	    {
	      if (stat == 0)
		page->p_ltype = NO_LOCK;
	      if (stat == 0 && (buf = page->p_buf) != NULL)
		change_prio (buf, FREE);
	    }
	  else if (stat==0)
	    {/* if somebody wait for a STRONG LOCK and it's possible now */ 
	      page->p_ltype = STRONG;
	      prg = wait->w_prget;
	      trnum = wait->w_conn;
	      out_first (page);
	      if (prg == 1)
		{
		  buf = get (segn, pn, 'r');
		  buf_to_user (trnum, buf->b_seg->keyseg);
		}
	      else
		user_p (trnum, 0);
	    }
          /* else we just released one of the weak locks */ 
          break;
        case STRONG:
	  if (page->p_queue == NULL)
	    {
	      page->p_ltype = NO_LOCK;
	      if ((buf = page->p_buf) != NULL)
	    	change_prio (buf, FREE);
	    }
	  else
	    set_weak_locks (page);
          break;
        default:
          assert("incorrect type of lock??");
	}
    }
}
