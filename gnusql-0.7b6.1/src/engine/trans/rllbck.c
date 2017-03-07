/*
 *  rllbck.c  - Rollback
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

/* $Id: rllbck.c,v 1.252 1998/09/29 21:25:46 kimelman Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

extern char **scptab;
extern struct d_r_t *firstrel;
extern u2_t maxscan;
extern struct ADREC bllj;
extern struct ADBL adlj;
extern i4_t idtr;
extern CPNM curcpn;
extern i4_t ljrsize;
extern i4_t ljmsize;
extern char *pbuflj;
extern i4_t IAMM;

i4_t
roll_back (i4_t cpn)
{
  if (cpn < 0 || IAMM == 0)
    return (OK);
  if (cpn <= curcpn)
    {
      rllbck ((CPNM) cpn, adlj);
      return (OK);
    }
  else
    return (-ER_NCF);
}

void
rllbck (CPNM cpn, struct ADBL cadlj)
{
  u2_t sctype, n, corsize, sn;
  struct d_r_t *desrel, *prdr;
  struct ldesind *di, *prdi;
  struct d_mesc *scpr;
  struct d_r_bd drbd;
  CPNM cpnlj;
  struct des_tid tid;
  char *b, type, **t, *a;
  i4_t rn, ordrn, cidtr;
  struct id_rel idr;
  struct full_des_tuple dtuple;
  char mch[2*BD_PAGESIZE];

  for (; cadlj.cm != 0;)
    {
      LJ_GETREC (&cadlj);
      a = pbuflj;
      type = *a++;
      cidtr = t4bunpack (a);
      a += size4b;
      if (cidtr != idtr)
	error ("TR.rllbck: The TR's record in LJ is false");
      cadlj.npage = t2bunpack (a);
      a += size2b;
      cadlj.cm = t2bunpack (a);
      a += size2b;
      if (type == RLBLJ || type == RLBLJ_AS_OP)
	continue;
      if (type == CPRLJ)
	{
          bcopy (a, (char *) &cpnlj, cpnsize);
	  if (cpnlj == cpn)
	    break;
	}
      else
	{
          
	  idr.urn.segnum = dtuple.sn_fdt = sn = t2bunpack (a);
	  a += size2b;
	  idr.urn.obnum = dtuple.rn_fdt = rn = t4bunpack (a);
	  a += size4b;
	  idr.pagenum = t2bunpack (a);
	  a += size2b;
	  idr.index = t2bunpack (a);
	  a += size2b;
	  if (rn != RDRNUM)
	    {
              tid.tpn = t2bunpack (a);
              a += size2b;
              tid.tindex = t2bunpack (a);
              a += size2b;
	      for (desrel = firstrel; desrel != NULL; desrel = desrel->drlist)
		if (desrel->desrbd.relnum == rn)
		  break;
	      if (desrel == NULL)
		error ("TR.rllbck: The correspondent desrel is absent\n");
	    }
	  else
            {
              tid.tpn = idr.pagenum;
              tid.tindex = idr.index;
              desrel = NULL;
            }
	  n = bllj.razm - ljmsize;
          bcopy (a, mch, n);
	  a = mch;
	  wmlj (RLBLJ_AS_OP, ljrsize, &cadlj, &idr, &tid, 0);
          dtuple.tid_fdt = tid;
	  if (type == DELLJ)
	    {
	      redo_insrtn (&dtuple, desrel, n, a);
	    }
	  else if (type == INSLJ)
	    {
	      redo_dltn (&dtuple, desrel, a);
	    }
	  else
	    {
              struct des_tid ref_tid;
	      corsize = get_placement (sn, &tid, &ref_tid);
	      n -= corsize;
	      ordmod (&dtuple, &ref_tid, corsize, a, n);
	      if (rn != RDRNUM)
		{
		  b = a + n;
		  mproind (desrel, desrel->desrbd.indnum, b, a, &tid);
		}
	      else
		{
		  ordrn = t4bunpack (a + scscal (a));
		  desrel = firstrel;
		  for (; desrel->desrbd.relnum != ordrn; desrel = desrel->drlist);
		  if (desrel == NULL)
		    error ("TR.rllbck: The correspondent desrel is absent\n");
		  if (type == CRILJ)
		    {		/* nead to delete this index */
		      redo_dind (desrel, a);
		    }
                  else if (type == DLILJ)
                    {			/* nead to create this index */
                      redo_cind (desrel, a);
                    }
		  else if (type == ADFLJ)
		    {		/*nead to delete these fields */
		      drbdunpack (&drbd, a + scscal (a));
		      desrel->desrbd.fieldnum = drbd.fieldnum;
		    }
		}
	    }
          BUF_endop ();
	}
    }
  sn_unltsp (cpn);
  for (n = 0; n < maxscan; n++)
    {
      t = scptab + n;
      scpr = (struct d_mesc *) * t;
      if (scpr == NULL || scpr->cpnsc > cpn)
	continue;
      if ((sctype = scpr->obsc) == SCR)
	{			/* relation scan */
	  desrel = (struct d_r_t *) scpr->pobsc;
	  desrel->oscnum--;
	}
      else if (sctype == SCI)
	{			/* index scan */
	  di = (struct ldesind *) scpr->pobsc;
	  di->oscni--;
	}
      xfree ((void *) *t);
      *t = NULL;
    }
  desrel = firstrel;
  for (prdr = NULL; desrel != NULL; prdr = desrel, desrel = desrel->drlist)
    if (desrel->cpndr <= cpn)
      {
	for (di = desrel->pid; di != NULL; di = di->listind)
	  xfree ((void *) di);
	if (prdr == NULL)
	  firstrel = desrel->drlist;
	else
	  prdr->drlist = desrel->drlist;
	xfree ((void *) desrel);
      }
    else
      {
	for (prdi = NULL, di = desrel->pid; di != NULL; prdi = di, di = di->listind)
	  {
	    if (di->cpndi >= cpn)
	      continue;
	    if (prdi == NULL)
	      desrel->pid = NULL;
	    else
	      prdi->listind = di->listind;
	    xfree ((void *) di);
	  }
      }
}
