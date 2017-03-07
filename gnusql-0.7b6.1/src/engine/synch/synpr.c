/* synpr.c -  Synchronizator
 *            Kernel of GNU SQL-server. Synchronizer    
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

/* $Id: synpr.c,v 1.249 1998/09/29 21:25:17 kimelman Exp $ */

#include "xmem.h"
#include <sys/types.h>
#include "dessnch.h"
#include "fdclsyn.h"

#define SEG_SYN_SIZE  11	/* segment degree size */

struct des_tran artran[TRNUM];
int reg_tran_size;

int
synstart (u2_t trnum)
{
  struct des_tran *tr;
  struct des_cp *cp;
  register i4_t i;

  /* find a free row in transaction descriptors table */
  for (i = 0; i < TRNUM && artran[i].ptlb != NULL; i++);
  if (i == (TRNUM + 1) && artran[i].ptlb != NULL)
    error ("SYN: excess max transations number in DBMS\n");
  tr = artran + i;
  reg_tran_size = 1 << LBSIZE;
  tr->ptlb = (char *) xmalloc (reg_tran_size);
  tr->idtr = trnum;
  tr->plcp = (struct des_cp *) tr->ptlb;
  tr->pwlock = NULL;
  tr->freelb = reg_tran_size - cpsize;
  tr->firstfree = tr->ptlb + cpsize;
  tr->trcost = 0;
  cp = (struct des_cp *) tr->ptlb;
  cp->dls = cpsize;
  cp->pdcp = NULL;
  cp->cpnum = 1;
  cp->cpcost = 0;
  return (0);
}

CPNM 
svpnt_syn (u2_t trnum)
{
  struct des_tran *tr;
  struct des_cp *cp;
  i4_t i;

  for (i = 0; i < TRNUM && artran[i].idtr != trnum; i++);
  tr = artran + i;
  if (cpsize > tr->freelb)
    increase (tr);
  cp = (struct des_cp *) tr->firstfree;
  tr->firstfree += cpsize;
  tr->freelb -= cpsize;
  cp->dls = cpsize;
  cp->pdcp = tr->plcp;
  cp->cpnum = tr->plcp->cpnum + 1;
  cp->cpcost = tr->trcost;
  tr->plcp = cp;
  return (cp->cpnum);
}

void
unltsp (u2_t trnum, CPNM cpnum)
{
  struct des_tran *tr;
  struct des_lock *a;
  struct des_cp *cpd;
  int i;

  for (i = 0; i < TRNUM && artran[i].idtr != trnum; i++);
  tr = artran + i;
  cpd = tr->plcp;
  if (cpnum == 0)
    cpnum = 1;
  if (cpd->cpnum < cpnum)
    cpnum = 1;
  while (cpd->cpnum != cpnum)
    cpd = cpd->pdcp;
  unlock (tr, a = (struct des_lock *) ((char *) cpd + cpsize));
  i = tr->firstfree - (char *) a;
  tr->plcp = cpd;
  tr->pwlock = NULL;
  tr->pbltr = NULL;
  tr->freelb += i;
  tr->firstfree -= i;
  tr->trcost = cpd->cpcost;
}

void
commit (u2_t trnum)
{
  struct des_tran *tr;
  int i;

  for (i = 0; i < TRNUM && artran[i].idtr != trnum; i++);
  tr = artran + i;
  unlock (tr, (struct des_lock *) (tr->ptlb + cpsize));
  xfree ((void *) tr->ptlb);
  tr->ptlb = NULL;
  tr->idtr = (u2_t) ~ 0;
}

void
error (char *s)
{
  printf ("error: %s\n", s);
  exit (0);
}
