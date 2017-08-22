/*
 *  opinpg.c  -  Low level operations inside DB pages
 *               Kernel of GNU SQL-server 
 *
 *  This file is a part of GNU SQL Server
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

/* $Id: opinpg.c,v 1.248 1998/05/20 05:52:43 kml Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

extern i4_t minidnt;

static int
lookidtr (char *a) /* 1 - this record may be removed, 0 - not */
{
  unsigned char t;

  t = *a & MSKCORT;
  if (t == IDTR)
    {
      i4_t idtran;
      idtran = t4bunpack (a + 1);
      if (idtran < minidnt)
        return (1);
    }
  return (0);
}

i4_t
testfree (char *asp, u2_t fs, u2_t corsize)
	/*  1 - this page is empty    */
	/* -1 - no allocation         */
        /*  0 - allocation is present */
{
  u2_t *ali, *ai;

  ai = (u2_t *) (asp + phsize);
  for (ali = ai + ((struct page_head *) asp)->lastin; ai <= ali; ai++)
    if (*ai != 0 && CHECK_PG_ENTRY(ai) && (lookidtr (*ai + asp) == 1))
      {
	fs += MIN_TUPLE_LENGTH + size2b;
	if (fs == BD_PAGESIZE - phsize)
	  return (1);		/* this page is empty */
      }
  if (fs >= corsize + size2b)
    return (0);
  return (-1);
}

static void
shift_cut (u2_t *ai, struct A *pg, i4_t idt_count, u2_t *afi)
{
  i2_t shsize = 0;
  u2_t fs, off, *aci;
  char *af, *src, *dst, *asp;
  i4_t n;

  asp = pg->p_shm;
  n = ai - afi;
  if (n != 0)
    {
      shsize = idt_count * MIN_TUPLE_LENGTH;
      off = *afi;
      af = asp + off;
      fs = *afi - *ai;
      recmjform (COMBR, pg, off, fs, af, shsize);
      for (src = af - 1, dst = src + shsize; fs != 0; fs--)
        *dst-- = *src--;
    }
  af = (char *)(afi - idt_count);
  recmjform (OLD, pg, af - asp, idt_count * size2b, af, 0);
  for (aci = (u2_t *)af; idt_count >= 0; idt_count--)
    *aci++ = 0;
  for (; n != 0; n--)
    *aci++ += shsize;
}

void
rempbd (struct A *pg)
{/* remove records about committed transactions deletions */
  i4_t what_do = 0, idt_count = 0;
  u2_t *ali, *ai, *afi;
  struct page_head *ph;
  char *asp;

  asp = pg->p_shm;
  afi = ai = (u2_t *) (asp + phsize);
  ph = (struct page_head *) asp;
  for (ali = ai + ph->lastin; ai <= ali; ai++)
    {
      if (*ai != 0 && CHECK_PG_ENTRY(ai))
        {
          if (lookidtr (*ai + asp) == 1)
            {
              if (what_do == 2)
                {
                  shift_cut (ai - 1, pg, idt_count, afi);
                  what_do = 0;
                  idt_count = 0;
                }
              idt_count++;
              afi = ai;
              what_do = 1;
            }
          else if (what_do == 1)
            what_do = 2;
        }
      else if (what_do != 0)
        {
          shift_cut (ai - 1, pg, idt_count, afi);
          what_do = 0;
          idt_count = 0;
        }
    }
  if (what_do != 0)
    shift_cut (ai - 1, pg, idt_count, afi);
  if (*ali == 0)
    {
      char *b;
      b = (char *) &ph->lastin;
      afi = (u2_t *) (asp + phsize);
      recmjform (OLD, pg, b - asp, size2b, b, 0);
      for (ai = ali - 1; ai >= afi && *ai == 0; )
	ai--;
      ph->lastin = ai - afi;
    }
}

void
inscort (struct A *pg, u2_t ind, char *cort, u2_t corsize)
{
  u2_t *ali, *ai;
  struct page_head *ph;
  char *asp;

  asp = pg->p_shm;
  ai = (u2_t *) (asp + phsize);
  ph = (struct page_head *) asp;
  ali = ai + ph->lastin;
  ai += ind;
  if (ai > ali)
    {
      char *b;
      b = (char *) &ph->lastin;
      recmjform (OLD, pg, b - asp, size2b, b, 0);
      ph->lastin += 1;
      *ai = *ali - corsize;
      bcopy (cort, asp + *ai, corsize);
    }
  else
    exspind (pg, ind, 0, corsize, cort);
}

void
exspind (struct A *pg, u2_t ind, u2_t oldsize, u2_t newsize, char *nc)
{
  char *a, *asp;
  u2_t *afi, *ali, *ai, *aci;
  i2_t delta;

  asp = pg->p_shm;
  afi = (u2_t *) (asp + phsize);
  ai = afi + ind;
  ali = afi + ((struct page_head *) asp)->lastin;
  delta = newsize - oldsize;
  a = (char *) ai;
  recmjform (OLD, pg, a - asp, (ali - ai + 1) * size2b, a, 0);
  for (aci = ai; aci > afi && *aci == 0;)
    aci--;
  if (ai != ali)
    {
      u2_t fs;
      if (aci == afi)
        fs = BD_PAGESIZE - *ali;
      else
        fs = *aci - *ali;
      recmjform (SHF, pg, *ali, fs, NULL, delta);
      a = asp + *ali;
      bcopy (a, a - delta, fs);
    }
  if (*ai != 0)
    {
      recmjform (OLD, pg, *ai, oldsize, *ai + asp, 0);
      *ai -= delta;
    }
  else
    {
      if (aci == afi)
	*ai = BD_PAGESIZE - newsize;
      else
	*ai = *aci - newsize;
    }
  for (afi = ai + 1; afi <= ali; afi++)
    if (*afi != 0)
      *afi -= delta;
  bcopy (nc, asp + *ai, newsize);
}

void
compress (struct A *pg, u2_t ind, u2_t newsize)
{
  char *src, *dst, *af, *asp;
  u2_t *ali, *aci, *ai, *afi, oldsize;
  i2_t delta;

  asp = pg->p_shm;
  afi = (u2_t *)(asp + phsize);
  ai = afi + ind;
  src = asp + *ai;
  oldsize = calsc (afi, ai);
  delta = oldsize - newsize;
  ali = (u2_t *) (asp + phsize) + ((struct page_head *) asp)->lastin;
  af = (char *) ai;
  recmjform (OLD, pg, af - asp, (ali - ai + 1) * size2b, af, 0);
  recmjform (OLD, pg, *ai, oldsize, src, 0);
  if (ai != ali)
    {
      u2_t fs;
      fs = *ai - *ali;
      recmjform (SHF, pg, *ali, fs, NULL, -delta);
      for (src--, dst = src + delta; fs != 0; fs--)
        *dst-- = *src--;
    }
  for (aci = ai; aci <= ali; aci++)
    if (*aci != 0)
      *aci += delta;
}
