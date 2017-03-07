/*
 *  insfltr.c  - Insertion into a filter 
 *               Kernel of GNU SQL-server 
 *
 *  $Id: insfltr.c,v 1.249 1998/09/29 21:25:33 kimelman Exp $
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

#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

extern char **scptab;
extern i2_t maxscan;
extern struct des_nseg desnseg;

i4_t
insfltr (i4_t scnum, i4_t idfl)
{
  struct d_mesc *scpr;
  struct des_tob *destob;
  char sctype;
  struct d_r_t *desrel, *fdesrel;
  struct des_fltr *desfltr;
  struct des_tid tid;
  char tmp_buff[BD_PAGESIZE];

  scpr = (struct d_mesc *) * (scptab + scnum);
  if (scnum >= maxscan || scpr == NULL)
    return (-ER_NDSC);
  if (scpr->prcrt == 0)
    return (-ER_NCR);
  if ((u2_t) idfl > desnseg.mtobnum)
    return (-ER_NIOB);  
  desfltr = (struct des_fltr *) * (desnseg.tobtab + idfl);
  if (desfltr == NULL)
    return (-ER_NIOB);
  destob = &desfltr->tobfl;
  if (destob->prdt.prob != FLTR)
    return (-ER_NIOB);
  fdesrel = desfltr->pdrtf;
  if ((sctype = scpr->obsc) == SCR)
    {				/* relation scan */
      struct d_sc_r *screl;
      screl = (struct d_sc_r *) scpr;
      desrel = (struct d_r_t *) scpr->pobsc;
      tid = screl->curtid;
    }
  else if (sctype == SCI)
    {				/* index scan */
      struct d_sc_i *scind;
      scind = (struct d_sc_i *) scpr;
      desrel = (&scind->dessc)->pdi->dri;
      tid = scind->dessc.ctidi;
    }
  else
    return (-ER_NIOB);
  if (fdesrel != desrel)
    return (-ER_NCF);
  read_tmp_page (destob->lastpn, tmp_buff);
  minsfltr (tmp_buff, destob, &tid);
  write_tmp_page (destob->lastpn, tmp_buff);
  destob->prdt.prsort = NSORT;
  return (OK);
}
