/*
 *  obrind.c  -  Addresses to Index Control Programm
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

/* $Id: obrind.c,v 1.250 1998/06/01 15:03:42 kimelman Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"
#include <assert.h>

extern struct ldesind **TAB_IFAM;

u2_t
getrec (struct id_ob *fullrn, u2_t pn, struct A *pg, u2_t * offloc)
{
  char *a;
  u2_t size, sn;
  char key[size4b + 1], key2[size2b];
  char *begagr, *loc;

  a = key;
  *a++ = BITVL(0) | EOSC;
  t4bpack (fullrn->obnum, a);
  t2bpack (pn, key2);
  sn = fullrn->segnum;
  tab_difam (sn);
  a = icp_lookup (pg, TAB_IFAM[sn], key, key2, size2b, &begagr, &loc);
  assert (a != NULL);
  size = t2bunpack (a + size2b);
  *offloc = a + size2b - pg->p_shm;
  putwul (pg, 'n');
  return (size);
}

i4_t
fgetnext (struct ldesscan *desscn, u2_t * pn, u2_t * size, i4_t modescan)
	/* delrel,opscrl */
{
  return (fscan_ind (desscn, (char *) pn, (char *) size, size2b, modescan));
}

i4_t
getnext (struct ldesscan *desscn, u2_t * pn, u2_t * size, i4_t modescan)
	/* delrel,opscrl */
{
  return (scan_ind (desscn, (char *) pn, (char *) size, size2b, modescan));
}

void
modcur (struct ldesscan *desscn, u2_t size)
	/* only for IFAM: ordins,delcon */
{
  char *asp, *a;
  struct A pg;
  u2_t sn, pn, off;

  sn = desscn->pdi->i_segn;
  pn = desscn->curlpn;
  while (BUF_enforce (sn, pn) < 0);
  asp = getwl (&pg, sn, pn);
  off = desscn->offp + size2b;
  a = asp + off;
  begmop (asp);
  recmjform (OLD, &pg, off, size2b, a, 0);
  MJ_PUTBL ();
  t2bpack (size, a);
  putpg (&pg, 'm');
}

void
modrec (struct id_ob *fullrn, u2_t pn, i2_t delta)
	/* only for IFAM: orddel,ordmod */
{
  char *a, *asp;
  char key[size4b + 1];
  char key2[size2b];
  struct A pg;
  char *begagr, *loc;
  u2_t size, pn1, sn;

  a = key;
  *a++ = BITVL(0) | EOSC;
  t4bpack (fullrn->obnum, a);
  t2bpack (pn, key2);
  sn = fullrn->segnum;
  tab_difam (sn);
  a = icp_lookup (&pg, TAB_IFAM[sn], key, key2, size2b, &begagr, &loc);
  assert (a != NULL);
  pn1 = pg.p_pn;
  BUF_enforce (sn, pn1);
  beg_mop ();
  asp = pg.p_shm;
  ++((struct p_head *) asp)->idmod;
  a += size2b;
  recmjform (OLD, &pg, a - asp, size2b, a, 0);
  MJ_PUTBL ();
  size = t2bunpack (a);
  size += delta;
  t2bpack (size, a);
  putpg (&pg, 'm');
}

i4_t
insrec (struct ldesind *desind, i4_t rn, u2_t pn, u2_t size)
	/* only for IFAM: ordins */
{
  char *a;
  char key[size4b + 1], key2[size2b], inf[size2b];

  a = key;
  *a++ = BITVL(0) | EOSC;
  t4bpack (rn, a);
  t2bpack (pn, key2);
  t2bpack (size, inf);
  return (icp_insrtn (desind, key, key2, inf, size2b));
}

int
delrec (struct ldesind *desind, i4_t rn, u2_t pn)
		/* only for IFAM: */
{
  char *a;
  char key[size4b + 1], key2[size2b];

  a = key;
  *a++ = BITVL(0) | EOSC;
  t4bpack (rn, a);
  t2bpack (pn, key2);
  return (icp_rem (desind, key, key2, size2b));
}

void
crindci (struct ldesind *desind)		/* only for ordind: crind */
{
  char *asp;
  u2_t sn, pn;
  struct ind_page *indph;
  struct A pg;

  sn = desind->i_segn;
  pn = getempt (sn);
  asp = getnew (&pg, sn, pn);
  indph = (struct ind_page *) asp;
  indph->ind_ph.idmod = 0L;
  indph->ind_nextpn = (u2_t) ~ 0;
  indph->ind_off = indphsize;
  indph->ind_wpage = LEAF;
  putwul (&pg, 'm');
  desind->ldi.rootpn = pn;
}
/*
static
void
tidpack (struct des_tid *tid, char *pnt)
{
    t2bpack(tid->tindex,pnt); pnt+=size2b;
    t2bpack(tid->tpn,pnt);
}
*/

static
void
tidunpack (struct des_tid *tid, char *pnt)
{
/*
    tid->tindex=t2bunpack(pnt); pnt+=size2b;
    tid->tpn=t2bunpack(pnt);
    */
  bcopy (pnt, (char *) tid, tidsize);
}

i4_t
ordindi (struct ldesind *desind,char * key, struct des_tid *tid)
     /* only for ordind: proind */
{

  /*    tidpack(tid,key2);*/
  return (icp_insrtn (desind, key, (char *) tid, (char *) NULL, 0));
}

i4_t
ordindd (struct ldesind *desind, char *key, struct des_tid *tid)
     /* only for ordind: proind,mproind,rollback */
{
  /*    tidpack(tid,key2);*/
  return (icp_rem (desind, key, (char *) tid, 0));
}

i4_t
ind_tid (struct ldesscan *desscn, struct des_tid *tid, i4_t modescan)
     /* only for ordind: next */
{
  i4_t ans;
  char mas[2 * size2b];
  
  ans = scan_ind (desscn, mas, NULL, 0, modescan);
  if (ans == OK)
    tidunpack (tid, mas);
  return (ans);
}

i4_t
ind_ftid (struct ldesscan *desscn, struct des_tid *tid, i4_t modescan)
/* only for ordind: opscin */
{
  i4_t ans;
  char mas[2 * size2b];
  
  ans = fscan_ind (desscn, mas, NULL, 0, modescan);
  if (ans == OK)
    tidunpack (tid, mas);
  return (ans);
}


