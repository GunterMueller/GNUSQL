/*
 * aggrsfn.c  - calculation of aggregate functions
 *               on specific temporary table
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

/* $Id: aggrsfn.c,v 1.254 1997/10/22 16:23:38 kml Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

extern struct des_nseg desnseg;

static u2_t
fsvfrm (char *buf, char *val, struct des_trel *destrel)
{
  char *sc;
  u2_t type, k, k1, fn, n;
  struct des_field *df;

  df = destrel->f_df_tt.df_pnt;
  fn = *(u2_t *) ((char *) destrel + destrel->f_df_tt.f_fn * rfsize);
  type = (df + fn)->field_type;
  sc = val + 1;
  val += scscal (val);
  for (k = 0, k1 = 0; k < fn; k1 = 0, sc++)
    for (; k1 < 7 && k < fn; k++, k1++)
      if ((*sc & BITVL(k1)) != 0)
	val = proval (val, type);
  n = get_length (val, type);
  bcopy (val, buf, n);
  return (n);
}

u2_t
minstab (struct id_rel *pidrel, char *val)
{
  u2_t *ai, sz, ntob;
  struct des_trel *destrel;
  char *asp, tmp_buff[BD_PAGESIZE];

  if (pidrel->urn.segnum != NRSNUM)
    return (-ER_NIOB);
  ntob = pidrel->urn.obnum;
  if (ntob > desnseg.mtobnum)
    return (-ER_NIOB);
  destrel = (struct des_trel *) * (desnseg.tobtab + ntob);
  if (destrel == NULL)
    return (-ER_NIOB);
  if (((struct prtob *) destrel)->prob != TREL)
    return (-ER_NIOB);
  if (((struct prtob *) destrel)->prsort != SORT)
    return (-ER_N_SORT);
  asp = tmp_buff;
  if (((struct prtob *) destrel)->prdrctn == GROW)
    {
      read_tmp_page (destrel->tobtr.firstpn, asp);
      for (ai = (u2_t *) (asp + phtrsize); *ai != 0; ai++);
    }
  else
    {
      read_tmp_page (destrel->tobtr.lastpn, asp);
      ai = (u2_t *) (asp + phtrsize) + ((struct p_h_tr *) asp)->linptr;
    }
  sz = fsvfrm (val, asp + *ai, destrel);
  return (sz);
}

u2_t
maxstab (struct id_rel * pidrel, char *val)
{
  u2_t *ai, sz, ntob;
  struct des_trel *destrel;
  char *asp, tmp_buff[BD_PAGESIZE];

  if (pidrel->urn.segnum != NRSNUM)
    return (-ER_NIOB);
  ntob = pidrel->urn.obnum;
  if (ntob > desnseg.mtobnum)
    return (-ER_NIOB);
  destrel = (struct des_trel *) * (desnseg.tobtab + ntob);
  if (((struct prtob *) destrel)->prob != TREL)
    return (-ER_NIOB);
  if (((struct prtob *) destrel)->prsort != SORT)
    return (-ER_N_SORT);
  asp = tmp_buff;
  if (((struct prtob *) destrel)->prdrctn == GROW)
    {
      read_tmp_page (destrel->tobtr.lastpn, asp);
      ai = (u2_t *) (asp + phtrsize) + ((struct p_h_tr *) asp)->linptr;
    }
  else
    {
      read_tmp_page (destrel->tobtr.firstpn, asp);
      for (ai = (u2_t *) (asp + phtrsize); *ai != 0; ai++);
    }
  sz = fsvfrm (val, asp + *ai, destrel);
  return (sz);
}

void
agrfrel (struct ans_next *ans, struct id_rel *pidrel, u2_t nf,
         u2_t * mnf, u2_t slsz, char *sc, char *flaglist)
{
  u2_t sn, *ai, *ali, pn;
  struct fun_desc_fields *desf;
  struct des_field *df;
  i4_t i = 0;
  char **agrl, *asp = NULL, tmp_buff[BD_PAGESIZE];
  struct A pg;
  char *arrpnt[BD_PAGESIZE];
  u2_t arrsz[BD_PAGESIZE];

  sn = pidrel->urn.segnum;
  if (sn == NRSNUM)
    {
      struct des_trel *destrel;
      u2_t tbl_id;
      
      tbl_id = pidrel->urn.obnum;
      if (tbl_id > desnseg.mtobnum)
	{
	  ans->cotnxt = -ER_NIOB;
	  return;
	}
      destrel = (struct des_trel *) * (desnseg.tobtab + tbl_id);
      if (destrel == NULL)
	{
	  ans->cotnxt = -ER_NIOB;
	  return;
	}
      if (((struct prtob *) destrel)->prob != TREL)
	{
	  ans->cotnxt = -ER_NIOB;
	  return;
	}
      desf = &destrel->f_df_tt;
      df = desf->df_pnt;
      if ((ans->cotnxt = testcond (desf, 0, NULL, &slsz, sc, 0, NULL)) != OK)
	return;
      agrl = (char **) xmalloc (nf * sizeof (char *));
      agrl_frm (agrl, df, nf, mnf, flaglist);      
      pn = destrel->tobtr.firstpn;
      if (pn != (u2_t) ~ 0)
        i = 1;
      asp = tmp_buff;
      while (pn != (u2_t) ~ 0)
	{
	  read_tmp_page (pn, asp);
	  ai = (u2_t *) (asp + phtrsize);
	  ali = ai + ((struct p_h_tr *) asp)->linptr;
          for (; ai <= ali; ai++)
            if (*ai != 0 &&
                tstcsel (desf, slsz, sc, asp + *ai, arrpnt, arrsz) != 0)
              agrcount (agrl, asp + *ai, desf, nf, mnf, flaglist);
          pn = ((struct listtob *) asp)->nextpn;
	}
    }
  else
    {
      struct d_sc_i *scind;
      struct d_r_t *desrel;
      struct ldesscan *disc;
      u2_t size;
      i4_t rep;
      u2_t tbl_id;
      char tuple[BD_PAGESIZE];
      
      if ((ans->cotnxt = contir (pidrel, &desrel)) != OK)
	return;
      desf = &desrel->f_df_bt;
      if ((ans->cotnxt = testcond (desf, 0, NULL, &slsz, sc, 0, NULL)) != OK)
	return;
      if ((ans->cotnxt = synlsc (RSC, pidrel, sc, slsz, desf->f_fn, NULL)) != OK)
	return;
      scind = rel_scan (&pidrel->urn, (char *) desrel,
                        &tbl_id, 0, NULL, sc, slsz, 0, NULL);
      agrl = (char **) xmalloc (nf * sizeof (char *));
      df = desf->df_pnt;
      agrl_frm (agrl, df, nf, mnf, flaglist);      
      disc = &scind->dessc;
      rep = fgetnext (disc, &pn, &size, FASTSCAN);
      if (rep != EOI)
        i = 1;      
      while (rep != EOI)
	{
          while ((asp = getpg (&pg, sn, pn, 's')) == NULL);
          ai = (u2_t *) (asp + phsize);
          ali = ai + ((struct page_head *) asp)->lastin;
          for (; ai <= ali; ai++)
            if (*ai != 0 && CHECK_PG_ENTRY(ai) &&
                fndslc (desrel, asp + *ai, sc, slsz, tuple) != 0)
              agrcount (agrl, tuple, desf, nf, mnf, flaglist);
          putpg (&pg, 'n');
          rep = getnext (disc, &pn, &size, FASTSCAN);
        }
      if (i == 1)
        xfree (disc->cur_key);
      delscan (tbl_id);
    }
  if (i == 1)
    distagr_frm (agrl, nf, flaglist);
  ans->csznxt = write_val (ans->cadnxt, agrl, df, nf, mnf, flaglist);
  for (i = 0; i < nf; i++)
    xfree (agrl[i]);
  xfree ((char *) agrl);
  return;
}
