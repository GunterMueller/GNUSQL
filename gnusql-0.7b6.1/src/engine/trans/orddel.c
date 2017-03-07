/*
 *  orddel.c  - Ordinary Deletion
 *              Kernel of GNU SQL-server 
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

/* $Id: orddel.c,v 1.252 1998/09/30 01:39:10 kimelman Exp $ */

#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

extern i4_t idtr;

void
orddel (struct full_des_tuple *dtuple)
{
  char *tuple, *asp = NULL;
  u2_t *ai, *afi, sn, pn, oldsize, ind;
  struct A pg;
  i4_t rn;
  struct des_tid ref_tid;
  unsigned char t;
  struct id_ob fullrn;

  sn = dtuple->sn_fdt;
  pn = dtuple->tid_fdt.tpn;
  while ((asp = getpg (&pg, sn, pn, 'x')) == NULL);
  afi = (u2_t *) (asp + phsize);
  ind = dtuple->tid_fdt.tindex;
  ai = afi + ind;
  tuple = asp + *ai;
  begmop (asp);
  tuple = asp + *ai;
  t = *tuple & MSKCORT;
  if (t == IND)
    {
      oldsize = 0;
      ref_tid.tindex = t2bunpack (tuple + 1);
      ref_tid.tpn = t2bunpack (tuple + 1 + size2b);
      recmjform (OLD, &pg, *ai, MIN_TUPLE_LENGTH, tuple, 0);
    }
  else
    {
      oldsize = calsc (afi, ai);
      compress (&pg, ind, MIN_TUPLE_LENGTH);
      tuple += oldsize - MIN_TUPLE_LENGTH;
    }
  t4bpack (idtr, tuple + 1);
  *tuple = IDTR;
  MJ_PUTBL ();
  putpg (&pg, 'm');
  rn = dtuple->rn_fdt;
  fullrn.segnum = sn;
  fullrn.obnum = rn;
  if (t == IND)
    {
      u2_t pn2;

      pn2 = ref_tid.tpn;
      while ((asp = getpg (&pg, sn, pn2, 'x')) == NULL);
      begmop (asp);
      afi = (u2_t *) (asp + phsize);
      ai = afi + ref_tid.tindex;
      oldsize = calsc (afi, ai);
      compress (&pg, ref_tid.tindex, 0);
      MJ_PUTBL ();
      putpg (&pg, 'm');
      modrec (&fullrn, pn2, oldsize + size2b);
      modrec (&fullrn, pn, MIN_TUPLE_LENGTH + size2b);
    }
  else
    {
      assert(oldsize > 0 );
      modrec (&fullrn, pn, oldsize + size2b);
    }
}
