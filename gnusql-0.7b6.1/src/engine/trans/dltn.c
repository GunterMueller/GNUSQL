/*
 *  dltn.c  - Deletion of a row
 *            Kernel of GNU SQL-server  
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

/* $Id: dltn.c,v 1.252 1998/09/29 21:25:29 kimelman Exp $ */

#include <assert.h>

#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

extern char **scptab;
extern i2_t maxscan;
extern char *pbuflj;
extern i4_t ljmsize;
extern struct ADBL adlj;
extern CPNM curcpn;

static int
delcort (struct d_mesc *s, struct d_r_t *desrel, struct des_tid *tid)
{
  char *cort, *selc;
  u2_t pn, corsize, ndc, fn, sn;
  CPNM cpn;
  struct id_rel idr;
  struct des_tid ref_tid;
  struct full_des_tuple dtuple;

  sn = desrel->segnr;
  pn = tid->tpn;
  if (pn == (u2_t) ~ 0)
    return (-ER_NCR);
  cort = pbuflj + ljmsize;
  if ((corsize = readcort (sn, tid, cort, &ref_tid)) == 0)
    {
      s->prcrt = 0;
      return (-ER_NCR);
    }
  idr.urn.segnum = dtuple.sn_fdt = sn;
  idr.urn.obnum = dtuple.rn_fdt = desrel->desrbd.relnum;
  idr.pagenum = desrel->pn_r;
  idr.index = desrel->ind_r;
  ndc = s->ndc;
  if (ndc < MAXCL)
    {
      if ((cpn = synlock (desrel, cort)) != 0)
        return (cpn);
      s->ndc++;
    }
  else if (ndc == MAXCL)
    {
      selc = s->pslc;
      fn = desrel->desrbd.fieldnum;
      if ((cpn = synlsc (s->modesc, &idr, selc + size2b,
                         *(u2_t *) selc, fn, NULL)) != 0)
        return (cpn);
      s->ndc++;
    }
  modmes ();
  wmlj (DELLJ, ljmsize + corsize, &adlj, &idr, tid, curcpn);
  dtuple.tid_fdt = *tid;
  orddel (&dtuple);
  proind (ordindd, desrel, desrel->desrbd.indnum, cort, tid);
  s->prcrt = 0;
  BUF_endop ();
  return (OK);
}

i4_t
dltn (i4_t scnum)
{
  struct d_mesc *scpr;
  struct ldesscan *desscan;
  i4_t sctype;
  char tmp_buff[BD_PAGESIZE];

  scpr = (struct d_mesc *) * (scptab + scnum);
  if (scnum >= maxscan || scpr == NULL)
    return (-ER_NDSC);
  if (scpr->modesc != WSC && scpr->modesc != DSC)
    return (-ER_NMS);
  if (scpr->prcrt == 0)
    return (-ER_NCR);
  if ((sctype = scpr->obsc) == SCTR)
    {
      struct d_sc_r *screl;
      struct des_trel *destrel;
      u2_t pn, *ai;
      char *asp;
      
      screl = (struct d_sc_r *) scpr;
      destrel = (struct des_trel *) scpr->pobsc;
      pn = screl->curtid.tpn;
      asp = tmp_buff;
      read_tmp_page (pn, asp);
      ai = (u2_t *) (asp + phtrsize) + screl->curtid.tindex;
      if (*ai == 0)
        {
          scpr->prcrt = 0;
          return (-ER_NCR);
        }
      assert (*ai <= BD_PAGESIZE);
      deltr (asp, ai, &(destrel->tobtr), pn);
      scpr->prcrt = 0;
      write_tmp_page (pn, asp);
      return (OK);
    }
  else if (sctype == SCR)
    {				/* relation scan */
      desscan = &((struct d_sc_i *) scpr)->dessc;
      return (delcort (scpr, (struct d_r_t *) scpr->pobsc, &desscan->ctidi));
    }
  else if (sctype == SCI)
    {				/* index scan */
      desscan = &((struct d_sc_i *) scpr)->dessc;
      return (delcort (scpr, desscan->pdi->dri, &desscan->ctidi));
    }
  else
    {				/* filter scan */
      struct d_sc_f *scfltr;
      struct des_tid tid;
      
      scfltr = (struct d_sc_f *) scpr;
      read_tmp_page (scfltr->pnf, tmp_buff);
      tid = *(struct des_tid *) (tmp_buff + scfltr->offf);
      return (delcort (scpr, ((struct des_fltr *) scpr->pobsc)->pdrtf, &tid));
    }
}

