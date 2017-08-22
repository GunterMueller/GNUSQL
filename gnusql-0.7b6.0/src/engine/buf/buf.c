/*
 *  buf.c  - This file contains functions maintaining the pool of buffers
 *           Kernel of GNU SQL-server. Buffer
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

/* $Id: buf.c,v 1.245 1997/03/31 03:46:38 kml Exp $ */

#include "fdeclbuf.h"
#include "bufdefs.h"
#include "rnmtp.h"
#include "inpop.h"

/*****************************************************************************

     This file contains functions maintaining the pool of buffers

********************************* base data *********************************/

struct BUFF *prios[PRIORITIES];	/* priority rings */
extern i4_t N_buf;		/* number of buffers */
extern i4_t N_opt;		/* optimal number of buffers */
extern i4_t max_buffers_number;	/* max number of buffers */


/*************************** change priority ********************************/

void
set_prio (struct BUFF *buf, u2_t new_prio) /* set priority */
{
  if (new_prio < USED)
    {
      if (prios[new_prio] == NULL)
	{
	  buf->b_next = buf;
	  buf->b_prev = buf;
	  prios[new_prio] = buf;
	}
      else
	{
	  buf->b_prev = prios[new_prio]->b_prev;
	  prios[new_prio]->b_prev->b_next = buf;
	  buf->b_next = prios[new_prio];
	  prios[new_prio]->b_prev = buf;
	}
    }
  else
    {
      buf->b_next = NULL;
      buf->b_prev = NULL;
    }
  buf->b_status = new_prio;
}

void
unset_prio (struct BUFF *buf)		/* unset priority */
{
  u2_t status;

  status = buf->b_status;
  if (status < USED)
    if (buf->b_next == buf)
      prios[status] = NULL;
    else
      {
	buf->b_next->b_prev = buf->b_prev;
	buf->b_prev->b_next = buf->b_next;
	if (prios[status] == buf)
	  prios[status] = buf->b_next;
      }
}

void
change_prio (struct BUFF *buf, u2_t new_prio)	/* change priority */
{
  unset_prio (buf);
  set_prio (buf, new_prio);
}

/********************** find the least useful buffer ************************/

struct BUFF *
find_buf ()
{
  struct BUFF *buf;
  u2_t prio;

  for (prio = 0; prio < PRIORITIES; prio++)
    if (prios[prio] != NULL)
      {
	buf = prios[prio];
	change_prio (buf, USED);
	return (buf);
      }
  return (NULL);
}

/*************************** take new buffer ********************************/

struct BUFF *
get_buf ()
{
  struct BUFF *buf;
  
  if (N_buf > N_opt)
    {
      if ((buf = find_buf ())!= NULL)
	{
          struct PAGE *page;
	  push_buf (buf);
	  unset_prio (buf);
	  page = buf->b_page;
	  if (page->p_status == 0 && page->p_ltype != STRONG)
	    del_page (page);
	  else
	    page->p_buf = NULL;
	  return (buf);
	}
    }
  if ((N_buf + 1) == max_buffers_number)
    {
      waitfor_seg (max_buffers_number);
      buf = find_buf ();
    }
  else
    {
      buf = (struct BUFF *) get_empty (sizeof (struct BUFF));
      buf->b_seg = new_seg ();
      N_buf++;
    }
  buf->b_next = NULL;
  buf->b_prev = NULL;
  buf->b_status = EMP;
  buf->b_page = NULL;
  buf->b_prmod = PRNMOD;
  buf->b_micro = NULL_MICRO;

  return (buf);
}

/****************************** delete buffer *******************************/

void
del_buf (struct BUFF *buf)
{
  push_buf (buf);
  buf->b_page->p_buf = NULL;
  del_seg (buf->b_seg);
  unset_prio (buf);
  xfree ((char *) buf);
  N_buf--;
}

void
push_buf (struct BUFF *buf)
{
  if (buf->b_micro != NULL_MICRO)
    push_micro (buf->b_micro);
  if (buf->b_prmod == PRMOD)
    write_buf (buf);
}

/*****************************************************************************

                        OPTIMAL NUMBER OF BUFFERS
*/

extern i4_t N_opt;		/* optimal number of buffers */

void
optimal (u2_t num)
{
  struct BUFF *buf;

  N_opt = num;
  for (; N_buf <= N_opt;)
    {
      if ((buf = find_buf ())== NULL)
	break;
      del_buf (buf);
    }
}

char *
get_empty (unsigned size)
{
  char *item;

  while ((item = (char *) xmalloc (size)) == NULL)
    ;
  return (item);
}

/********************************** the end *********************************/
