/*
 *  reclj.c  - Records in Logical Journal
 *             Kernel of GNU SQL-server 
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

/* $Id: reclj.c,v 1.248 1998/09/29 21:25:44 kimelman Exp $ */

#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"
#include "xmem.h"

#define SZLJBF BD_PAGESIZE
extern struct ADREC bllj;
extern i4_t idtr;
extern char *pbuflj;

void
wmlj (i4_t type, u2_t size, struct ADBL *cadlj, struct id_rel *idr,
      struct des_tid *tid, CPNM cpn)
{
  char *a;

  bllj.razm = size;
  a = pbuflj;
  if (size > SZLJBF)
    fprintf (stderr,"TR.reclj: LJ's buffer is too small");
  *a++ = type;
  t4bpack (idtr, a);
  a += size4b;
  t2bpack (cadlj->npage, a);
  a += size2b;
  t2bpack (cadlj->cm, a);
  a += size2b;
  if (type == CPRLJ)
    {
      bcopy ((char *) &cpn, a, cpnsize);
      a += cpnsize;
      LJ_put (PUTHREC);
    }
  else if (type == RLBLJ)
    LJ_put (PUTHREC);
  else if (type == RLBLJ_AS_OP)
    LJ_put (PUTREC);
  else
    {
      t2bpack (idr->urn.segnum, a);
      a += size2b;
      t4bpack (idr->urn.obnum, a);
      a += size4b;
      t2bpack (idr->pagenum, a);
      a += size2b;
      t2bpack (idr->index, a);
      a += size2b;
      if (idr->urn.obnum != RDRNUM)
        {
          t2bpack (tid->tpn, a);
          a += size2b;
          t2bpack (tid->tindex, a);
          a += size2b;
        }
      LJ_put (PUTREC);
    }
}
