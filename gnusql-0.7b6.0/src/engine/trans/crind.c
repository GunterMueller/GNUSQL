/*
 *  crind.c  - create some DB table index
 *              Kernel of GNU SQL-server  
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

/* $Id: crind.c,v 1.250 1997/10/22 16:23:38 kml Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

extern i4_t ljmsize;
extern struct ADBL adlj;
extern char *pbuflj;

struct ans_cind
crind (struct id_rel *pidrel, i4_t prun, i4_t type, i4_t afsize, u2_t *arfn)
{
  u2_t *a, *b, rcorsize, newsize, disize, fn, dfn;
  char *c, *cort, *nc;
  struct d_r_t *desrel;
  struct d_r_bd *drbd;
  struct des_index *di;
  struct ldesind *desind;
  struct id_rel idr;
  u2_t fnk, corsize, num, sn;
  struct ans_cind ans;
  i4_t n, size;
  struct des_tid tid, ref_tid;
  struct full_des_tuple dtuple;

  sn = pidrel->urn.segnum;
  if (sn == NRSNUM)
    {
      ans.cpnci = -ER_NDI;
      return (ans);
    }
  if ((ans.cpnci = cont_fir (pidrel, &desrel)) != OK)
    return (ans);
  drbd = &desrel->desrbd;
  fn = drbd->fieldnum;
  if (afsize > fn)
    {
      ans.cpnci = -ER_NCF;
      return (ans);
    } 
  for (num = 0, a = arfn; num < afsize; num++)
    {
      fnk = *a++;    
      if (fnk > fn)
	{
	  ans.cpnci = -ER_NCF;
	  return (ans);
	}
      for (dfn = num + 1; dfn < afsize; dfn++)
	if (fnk == arfn [dfn])
	  {
	    ans.cpnci = -ER_NCF;
	    return (ans);
	  }
    }  
  if ((ans.cpnci = synind (&pidrel->urn)) != OK)
    return (ans);
  tid.tpn = pidrel->pagenum;
  tid.tindex = pidrel->index;
  cort = pbuflj + ljmsize;
  if ((corsize = readcort (sn, &tid, cort, &ref_tid)) == 0)
    {
      ans.cpnci = -ER_NDR;
      return (ans);
    }
  n = uniqnm ();
  dfn = afsize;
  if ((dfn % 2) != 0)
    dfn += 1;
  desind = (struct ldesind *) xmalloc (ldisize + dfn * size2b + rfsize);
  di = (struct des_index *) & desind->ldi;
  di->unindex = n;
  di->kifn = afsize;
  if (prun == PRUN)
    di->kifn |= UNIQ;
  a = (u2_t *) (desind + 1);
  for (b = a + afsize; a < b;)
    *a++ = *arfn++;
  disize = (char *) a - (char *) di;
  rcorsize = getrc (drbd, cort);
  nc = cort + corsize;
  drbd->indnum += 1;
  c = nc;
  size = scscal (cort);
  bcopy (cort, c, size);
  c += size;
  bcopy ((char *) drbd, c, drbdsize);
  c += drbdsize;
  cort += size + drbdsize;
  size = rcorsize - size - drbdsize;
  bcopy (cort, c, size);
  c += size;
  idr = *pidrel;
  if ((ans.cpnci = synlsc (RSC, &idr, NULL, 0, fn, NULL)) != OK)
    return (ans);  
  modmes ();
  crtid (desind, desrel);
  crindci (desind);
  dipack (di, disize, c);
  c += disize;
  if ((newsize = c - nc) < corsize)
    newsize = corsize;
  ans.idinci.irii = *pidrel;
  idr.urn.obnum = RDRNUM;
  wmlj (CRILJ, ljmsize + corsize + newsize, &adlj, &idr, &tid, 0);
  dtuple.sn_fdt = idr.urn.segnum;
  dtuple.rn_fdt = RDRNUM;
  dtuple.tid_fdt = tid;
  ordmod (&dtuple, &ref_tid, corsize, nc, newsize);

  fill_ind (desrel, desind);
  ans.idinci.inii = n;
  BUF_endop ();
  return (ans);
}

void
dipack (struct des_index *di, i4_t disize, char *pnt)
{
  bcopy ((char *) di, pnt, disize);
}
