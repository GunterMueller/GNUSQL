/*
 *  delrel.c  - deletion of specific DB table
 *              Kernel of GNU SQL-server 
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

/* $Id: delrel.c,v 1.252 1998/09/29 21:25:27 kimelman Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

extern i4_t ljmsize;
extern struct d_r_t *firstrel;
extern struct ADBL adlj;
extern char *pbuflj;

CPNM
delrel (struct id_rel *pidrel)
{
  char *cort;
  u2_t rcorsize, scsize, sn, corsize;
  struct ldesind *desind;
  struct d_r_t *desrel, *dr, *prevdr;
  struct des_tid ref_tid;
  CPNM cpn;
  char array[BD_PAGESIZE];
  struct des_tid tid;
  struct id_rel idr;
  struct full_des_tuple dtuple;

  sn = pidrel->urn.segnum;
  if (sn == NRSNUM)
    return (-ER_NDR);
  cort = array;
  if ((desrel = getrd (pidrel, &ref_tid, cort, &corsize)) == NULL)
    return (-ER_NDR);
  rcorsize = getrc (&desrel->desrbd, cort);
  scsize = scscal (cort);
  if ((cpn = synrd (sn, cort + scsize, rcorsize - scsize)) != OK)
    return (cpn);
  delcrl (pidrel, 0, NULL);

  for (desind = desrel->pid; desind != NULL; desind = desind->listind)
    {
      delscd (desind->oscni, (char *) desind);
      killind (desind);
      xfree ((void *) desind);
    }
  
  modmes ();
  bcopy (cort, pbuflj + ljmsize, corsize);
  idr = *pidrel;
  idr.urn.obnum = dtuple.rn_fdt = RDRNUM;
  tid.tpn = pidrel->pagenum;
  tid.tindex = pidrel->index;
  wmlj (DELLJ, ljmsize + corsize, &adlj, &idr, &tid, 0);
  dtuple.sn_fdt = sn;
  dtuple.tid_fdt = tid;
  orddel (&dtuple);
  delscd (desrel->oscnum, (char *) desrel);
  if (desrel == firstrel)
    firstrel = desrel->drlist;
  else
    {
      for (prevdr = dr = firstrel; dr != desrel; dr = dr->drlist)
	prevdr = dr;
      prevdr->drlist = desrel->drlist;
    }
  xfree ((void *) desrel);
  BUF_endop ();
  return (OK);
}

struct d_r_t *
getrd (struct id_rel *pidrel, struct des_tid *ref_tid, char *cort, u2_t * corsize)
{
  struct d_r_t *desrel;
  i4_t rn;
  struct des_tid tid;

  rn = pidrel->urn.obnum;
  for (desrel = firstrel; desrel != NULL; desrel = desrel->drlist)
    if (desrel->desrbd.relnum == rn)
      break;
  tid.tpn = pidrel->pagenum;
  tid.tindex = pidrel->index;
  if ((*corsize = readcort (pidrel->urn.segnum, &tid, cort, ref_tid)) == 0)
    return (NULL);
  if (desrel == NULL)
    desrel = crtfrd (pidrel, cort);
  return (desrel);
}

u2_t
getrc (struct d_r_bd *drbd, char *cort)
{
  char *a;
  u2_t size = 0, k, kn = 0, indn, size1, size2;
  struct des_index cdi;

  indn = drbd->indnum;
  size1 = scscal (cort) + drbdsize + drbd->fieldnum * rfsize;
  a = cort + size1;
  for (; kn < indn; kn++)
    {
      bcopy (a, (char *) &cdi, dinsize);
      a += dinsize;
      k = cdi.kifn & ~UNIQ & MSK21B;
      size2 = k * size2b;
      size += dinsize + size2;
      a += size2;
    }
  return (size1 + size);
}
