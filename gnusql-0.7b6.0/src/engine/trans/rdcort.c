/*
 *  rdcort.c  -  Read  a tuple 
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

/* $Id: rdcort.c,v 1.249 1997/05/08 12:26:58 vera Exp $ */

#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"
#include "xmem.h"

u2_t
readcort (u2_t sn, struct des_tid *tid, char *cort, struct des_tid *ref_tid)
{
  char *asp = NULL, *tuple;
  u2_t corsize;
  struct A pg;

  while ((asp = getpg (&pg, sn, tid->tpn, 's')) == NULL);
  GET_IND_REF();  /* indirect reference */
  bcopy (tuple, cort, corsize);
  putpg (&pg, 'n');
  return (corsize);
}

u2_t
calsc (u2_t *afi, u2_t *ai)
{
  u2_t *aci;
  
  for (aci = ai - 1; aci >= afi && *aci == 0; aci--);
  if (aci < afi)
    return (BD_PAGESIZE - *ai);
  else
    return (*aci - *ai);
}

