/*
 * cnttab.c  - calculation of a number of DB table rows
 *             on basis  of scanning of this DB table
 *             by itself, by some index, by some filter
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

/* $Id: cnttab.c,v 1.252 1998/09/29 21:25:24 kimelman Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "strml.h"
#include "agrflg.h"
#include "fdcltrn.h"

extern struct des_nseg desnseg;

void
cntttab (struct ans_cnt *ans, struct id_rel *pidrel, u2_t condsz, char *cond)
{
  struct fun_desc_fields *desf;
  u2_t *ali, sn, pn, *ai, size;
  i4_t rep, cntnum = 0;
  struct d_r_t *desrel;
  struct d_sc_i *scind;
  struct ldesscan *disc;
  i2_t num;
  char *asp = NULL;
  struct A pg;

  if ((ans->cpncnt = contir (pidrel, &desrel)) != OK)
    return;
  desf = &desrel->f_df_bt;
  if ((ans->cpncnt = testcond (desf, 0, NULL, &condsz, cond, 0, NULL)) != OK)
    return;
  if ((ans->cpncnt = synlsc (RSC, pidrel, cond, condsz, desf->f_fn, NULL)) != OK)
    return;
  sn = desrel->segnr;
  scind = rel_scan (&pidrel->urn, (char *) desrel,
                    &num, 0, NULL, NULL, 0, 0, NULL);
  disc = &scind->dessc;
  rep = fgetnext (disc, &pn, &size, SLOWSCAN);
  while (rep != EOI)
    {
      while ((asp = getpg (&pg, sn, pn, 's')) == NULL);
      ai = (u2_t *) (asp + phsize);
      ali = ai + ((struct page_head *) asp)->lastin;
      for (; ai <= ali; ai++)
	if (*ai != 0 && CHECK_PG_ENTRY(ai)
            && fndslc (desrel, asp + *ai, cond, condsz, NULL) != 0)
	  cntnum += 1;
      putpg (&pg, 'n');
      rep = getnext (disc, &pn, &size, SLOWSCAN);
    }
  delscan (num);
  ans->cntn = cntnum;
}

void
cntitab (struct ans_cnt *ans, struct id_ind *pidind,
         u2_t condsz, char *cond, u2_t diasz, char *diasc)
{
  u2_t sn, *ai, kn, dscsz;
  struct fun_desc_fields *desf;
  struct ldesind *di;
  struct ldesscan *disc;
  struct d_sc_i *scind;
  i4_t rep, cntnum = 0, index;
  struct d_r_t *desrel;
  struct id_rel *pidrel;
  struct des_tid tid;
  i2_t n;
  char *asp = NULL;
  struct A pg;

  pidrel = &pidind->irii;
  if ((ans->cpncnt = contir (pidrel, &desrel)) != OK)
    return;
  sn = desrel->segnr;
  desf = &desrel->f_df_bt;
  if ((ans->cpncnt = testcond (desf, 0, NULL, &condsz, cond, 0, NULL)) != OK)
    return;
  index = pidind->inii;
  for (di = desrel->pid; di->ldi.unindex != index && di != NULL; di = di->listind);
  if (di == NULL)
    {
      ans->cpncnt = -ER_NDI;
      return;
    }
  ai = (u2_t *) (di + 1);
  if ((ans->cpncnt = testdsc (desrel, &diasz, diasc, ai, &dscsz)) != OK)
    return;

  if ((ans->cpncnt = synlsc (RSC, pidrel, cond, condsz, desf->f_fn, NULL)) != OK)
    return;
  kn = di->ldi.kifn & ~UNIQ & MSK21B;
  if ((ans->cpncnt = synlsc (RSC, pidrel, diasc, diasz, kn, ai)) != OK)
    return;
  scind = (struct d_sc_i *) lusc (&n, scisize, (char *) di, SCI, RSC, 0, NULL,
                                  cond, condsz, 0, NULL, diasz + size2b);
  disc = &scind->dessc;
  disc->curlpn = (u2_t) ~ 0;
  asp = (char *) scind + scisize + condsz + size2b;
  if (diasz == 0)
    disc->dpnsc = NULL;
  else
    disc->dpnsc = asp;
  t2bpack (diasz, asp);
  disc->dpnsval = asp + size2b + dscsz;
  bcopy (diasc, asp + size2b, diasz);
  rep = ind_ftid (disc, &tid, SLOWSCAN);
  while (rep != EOI)
    {
      while ((asp = getpg (&pg, sn, tid.tpn, 's')) == NULL);
      ai = (u2_t *) (asp + phsize) + tid.tindex;
      if (*ai != 0 &&
          fndslc (desrel, asp + *ai, cond, condsz, NULL) != 0)
	cntnum += 1;
      putpg (&pg, 'n');
      rep = ind_tid (disc, &tid, SLOWSCAN);
    }
  delscan (n);
  ans->cntn = cntnum;
}

i4_t
cntftab (i4_t idfl, u2_t condsz, char *cond)
{
  i4_t cntnum = 0;
  u2_t flpn, oldpn, pn, *ai, *afi, off, sn;
  struct des_tid *tid, *tidb;
  struct des_fltr *desfl;
  struct d_r_t *desrel;
  struct fun_desc_fields *desf;
  char *asp = NULL, *aspfl;
  struct A inpage;
  char tmp_buff[BD_PAGESIZE];

  if ((u2_t) idfl > desnseg.mtobnum)
    return (-ER_NIOB);
  desfl = (struct des_fltr *) * (desnseg.tobtab + idfl);
  if (desfl == NULL)
    return (-ER_NIOB);
  if (((struct prtob *) desfl)->prob != FLTR)
    return (-ER_NIOB);
  desrel = desfl->pdrtf;
  sn = desrel->segnr;
  desf = &desrel->f_df_bt;
  if (testcond (desf, 0, NULL, &condsz, cond, 0, NULL) != OK)
    return (-ER_NCF);
  aspfl = tmp_buff;
  for (flpn = desfl->tobfl.firstpn; flpn != (u2_t) ~ 0;)
    {
      read_tmp_page (flpn, aspfl);
      off = ((struct p_h_f *) aspfl)->freeoff;
      tid = (struct des_tid *) (aspfl + phfsize);
      oldpn = tid->tpn;
      while ((asp = getpg (&inpage, sn, oldpn, 's')) == NULL);
      afi = (u2_t *) (asp + phsize);
      tidb = (struct des_tid *) (aspfl + off);
      for (; tid < tidb; tid++)
	{
	  pn = tid->tpn;
	  if (oldpn != pn)
	    {
	      putpg (&inpage, 'n');
	      while ((asp = getpg (&inpage, sn, pn, 's')) == NULL);
	      afi = (u2_t *) (asp + phsize);
	      oldpn = pn;
	    }
	  ai = afi + tid->tindex;
	  if (*ai != 0 &&
              fndslc (desrel, asp + *ai, cond, condsz, NULL) != 0)
	    cntnum += 1;
	}
      flpn = ((struct p_h_f *) aspfl)->listfl.nextpn;
    }
  putpg (&inpage, 'n');
  return (cntnum);
}

void
sumtmpt (struct ans_next *ans, struct id_rel *pidrel)
{
  u2_t pn, *ai, *ali, ntob, mnf;
  struct fun_desc_fields *desf;
  struct des_field *df;
  struct des_trel *destrel;
  char *asp, flaglist, **agrl;
  char tmp_buff[BD_PAGESIZE];

  if (pidrel->urn.segnum != NRSNUM)
    {
      ans->cotnxt = -ER_NIOB;
      return;
    }
  else
    ans->cotnxt = OK;            
  ntob = pidrel->urn.obnum;
  destrel = (struct des_trel *) * (desnseg.tobtab + ntob);
    mnf = 0;
  flaglist = FN_SUMM;
  agrl = (char **) xmalloc (sizeof (void *));
  desf = &destrel->f_df_tt;
  df = desf->df_pnt;
  agrl_frm (agrl, df, 1, &mnf, &flaglist);
  asp = tmp_buff;
  for (pn = destrel->tobtr.firstpn; pn != (u2_t) ~ 0;)
    {
      read_tmp_page (pn, asp);
      ai = (u2_t *) (asp + phtrsize);
      ali = ai + ((struct p_h_tr *) asp)->linptr;
      for (; ai <= ali; ai++)
	if (*ai != 0)
	  agrcount (agrl, asp + *ai, desf, 1, &mnf, &flaglist);
      pn = ((struct listtob *) asp)->nextpn;
    }
  ans->csznxt = write_val (ans->cadnxt, agrl, df, 1, &mnf, &flaglist);
  xfree (agrl[0]);
  xfree ((char *) agrl);  
  return ;
}
