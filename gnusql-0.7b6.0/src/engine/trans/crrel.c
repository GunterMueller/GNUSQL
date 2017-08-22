/*
 *  crrel.c  - create a DB table
 *              Kernel of GNU SQL-server  
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

/* $Id: crrel.c,v 1.247 1997/10/22 16:23:38 kml Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

extern i4_t ljmsize;
extern char *pbuflj;

struct ans_cr
crrel (i4_t sn, i4_t fn, i4_t fdf, struct des_field *df)
{
  char *a, *c, *val;
  struct des_field *ldf, *cdf;
  u2_t n;
  struct d_r_bd drbd;
  i4_t rn;
  struct ans_cr ans;
  struct des_tid tid;
  struct id_rel idr;
    
  if (fdf > fn)
    {
      ans.cpnacr = -ER_NCF;
      return (ans);
    }
  for (ldf = df + fn, cdf = df; cdf < ldf; cdf++)
    {
      if ((n = cdf->field_type) == T1B || n == T2B || n == T4B || n == TCH ||
	  n == TFL || n == TFLOAT)
	continue;
      else
	{
	  ans.cpnacr = -ER_NCF;
	  return (ans);
	}
    }
  if (sn == NRSNUM)
    {
      ans.cpnacr = -ER_NDR;
      return (ans);
    }
  rn = uniqnm ();
  drbd.relnum = rn;
  drbd.fieldnum = fn;
  drbd.fdfnum = fdf;
  drbd.indnum = 0;
  c = a = pbuflj + ljmsize;
  *a++ = CORT;
  *a++ |= EOSC;
  val = a;
  drbdpack (&drbd, a);
  a += drbdsize;
  dfpack (df, fn, a);
  n = a + fn * rfsize - c;
  if ((ans.cpnacr = synrd (sn, val, n - scscal (c))) != OK)
    return (ans);
  modmes ();
  idr.urn.segnum = sn;
  idr.urn.obnum = RDRNUM;
  tid = ordins (&idr, c, n + DELRD, 'w');
  ans.idracr.urn.segnum = sn;
  ans.idracr.urn.obnum = rn;
  ans.idracr.pagenum = tid.tpn;
  ans.idracr.index = tid.tindex;
  BUF_endop ();
  return (ans);
}

void
drbdpack (struct d_r_bd *drbd, char *pnt)
{
/*
    t4bpack(drbd->relnum,pnt); pnt+=size4b;
    t2bpack(drbd->fieldnum,pnt); pnt+=size2b;
    t2bpack(drbd->fdfnum,pnt); pnt+=size2b;
    t2bpack(drbd->indnum,pnt); pnt+=size2b;
    */
  bcopy ((char *) drbd, pnt, drbdsize);
}

void
dfpack (struct des_field * df, u2_t fn, char * pnt)
{
  bcopy ((char *) df, pnt, fn * rfsize);
}

struct ans_ctob
crview (
	 u2_t sn,
	 u2_t fn,
	 struct des_field *df
)
{
  i4_t rn;
  struct ans_ctob ansctob;

  rn = uniqnm ();
  ansctob = crtrel (fn, fn, df);
  return (ansctob);
}
