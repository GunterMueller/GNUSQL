/*
 *  delind.c  - deletion of specific DB table index
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

/* $Id: delind.c,v 1.250 1998/09/29 21:25:26 kimelman Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

extern i4_t ljmsize;
extern struct ADBL adlj;
extern char *pbuflj;

CPNM
delind (struct id_ind *pidind)
{
  u2_t size, rcorsize, scsize, newsize, n;
  char *cort, *c, *d, *nc;
  struct d_r_t *desrel;
  struct d_r_bd *drbd;
  struct ldesind *desind, *prevdi;
  struct des_index *di;
  struct des_tid tid, ref_tid;
  struct id_rel *pidrel;
  struct full_des_tuple dtuple;
  i4_t unind;
  u2_t corsize, sn;
  CPNM cpn;

  pidrel = &pidind->irii;
  sn = pidrel->urn.segnum;
  if (sn == NRSNUM)
    return (-ER_NDR);
  if ((cpn = cont_fir (pidrel, &desrel)) != OK)
    return (cpn);
  if ((cpn = synind (&pidrel->urn)) != OK)
    return (cpn);
  cort = pbuflj + ljmsize;
  tid.tpn = pidrel->pagenum;
  tid.tindex = pidrel->index;
  if ((corsize = readcort (sn, &tid, cort, &ref_tid)) == 0)
    return (-ER_NDR);
  drbd = &desrel->desrbd;
  unind = pidind->inii;
  n = drbd->fieldnum * rfsize;
  prevdi = NULL;
  for (desind = desrel->pid; desind != NULL; prevdi = desind, desind = desind->listind)
    {
      di = &desind->ldi;
      size = dinsize + (di->kifn & ~UNIQ & MSK21B) * size2b;
      if (di->unindex == unind)
	{
	  rcorsize = getrc (drbd, cort);
	  nc = cort + corsize;
	  --drbd->indnum;
          scsize = scscal (cort);
          bcopy (cort, nc, scsize);
          c = nc + scsize;
          bcopy ((char *) drbd, c, drbdsize);
          c += drbdsize;
          d = cort + scsize + drbdsize;
          bcopy (d, c, n);
          c += n;
          d += n + size;
          size = cort + rcorsize - d;
          bcopy (d, c, size);
          c += size;
	  if (rcorsize < corsize)
	    newsize = corsize;
	  else
	    newsize = c - nc;
	  modmes ();
          pidrel->urn.obnum = RDRNUM;
	  wmlj (DLILJ, ljmsize + corsize + newsize, &adlj, pidrel, &tid, cpn);
          dtuple.sn_fdt = sn;
          dtuple.rn_fdt = RDRNUM;
          dtuple.tid_fdt = tid;
	  ordmod (&dtuple, &ref_tid, corsize, nc, newsize);
	  killind (desind);
	  if (prevdi == NULL)
	    desrel->pid = NULL;
	  else
	    prevdi->listind = desind->listind;
	  delscd (desind->oscni, (char *) desind);
	  xfree ((void *) desind);
	  BUF_endop ();
	  return (OK);
	}
      else
	n += size;
    }
  return (-ER_NDI);
}
