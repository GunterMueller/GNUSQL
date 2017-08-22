/*
 *  opscrl.c  -  Open scanning of a DB table by itself
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

/* $Id: opscrl.c,v 1.249 1997/10/22 16:23:38 kml Exp $ */

#include "destrn.h"
#include "sctp.h"
#include "strml.h"
#include "fdcltrn.h"

extern struct des_nseg desnseg;

struct ans_opsc
opscrel (struct id_rel *pidrel, i4_t mode, u2_t fnum, u2_t * fl,
         u2_t slsz, char *sc, u2_t fmnum, u2_t * fml)
{
  struct fun_desc_fields *desf;
  i2_t num;
  struct ans_opsc ans;
  u2_t sn, pn;

  sn = pidrel->urn.segnum;
  if (sn == NRSNUM)
    {
      struct d_sc_r *screl;
      struct des_trel *destrel;
      u2_t ntob;
      ntob = pidrel->urn.obnum;
      if (ntob > desnseg.mtobnum)
	{
	  ans.cpnops = -ER_NIOB;
	  return (ans);
	}
      destrel = (struct des_trel *) * (desnseg.tobtab + ntob);
      if (destrel == NULL)
	{
	  ans.cpnops = -ER_NIOB;
	  return (ans);
	}	
      if (((struct prtob *) destrel)->prob != TREL)
	{
	  ans.cpnops = -ER_NIOB;
	  return (ans);
	}
      desf = &destrel->f_df_tt;
      if ((ans.cpnops = testcond (desf, fnum, fl, &slsz, sc, fmnum, fml)) != OK)
	return (ans);
      screl = (struct d_sc_r *) lusc (&num, scrsize, (char *) destrel, SCTR, mode,
				      fnum, fl, sc, slsz, fmnum, fml, 0);
      pn = destrel->tobtr.firstpn;
      if (pn == (u2_t) ~ 0)
	{
	  ans.cpnops = -ER_EOSCAN;
	}
      else
	{
	  destrel->tobtr.osctob++;
	  ans.cpnops = OK;
	}
      screl->curtid.tpn = pn;
      screl->curtid.tindex = 0;
      screl->mescr.prcrt = 0;
      screl->memtid.tpn = (u2_t) ~ 0;
    }
  else
    {
      struct d_sc_i *scind;
      struct ldesscan *disc;
      i4_t rep;
      struct d_r_t *desrel;
      u2_t size;
      if ((ans.cpnops = cont_fir (pidrel, &desrel)) != OK)
	return (ans);
      desf = &desrel->f_df_bt;
      if ((ans.cpnops = testcond (desf, fnum, fl, &slsz, sc, fmnum, fml)) != OK)
	return (ans);
      if ((ans.cpnops = synlsc (RSC, pidrel, sc, slsz, desf->f_fn, NULL)) != OK)
	return (ans);
      scind = rel_scan (&pidrel->urn, (char *) desrel, &num,
			fnum, fl, sc, slsz, fmnum, fml);
      disc = &scind->dessc;
      rep = fgetnext (disc, &pn, &size, SLOWSCAN);
      if (rep == EOI)
	{
	  pn = (u2_t) ~ 0;
	  ans.cpnops = -ER_EOSCAN;
	}
      else
	{
	  desrel->oscnum++;
	  ans.cpnops = OK;
	}
      disc->ctidi.tpn = pn;
      disc->ctidi.tindex = 0;
      scind->mesci.prcrt = 0;
      disc->mtidi.tpn = (u2_t) ~ 0;
    }
  ans.scnum = num;
  return (ans);
}

static int
sel_cmpr (char *diasc, u2_t * diasz, u2_t dscsz, struct des_field *df)
{
  u2_t fnk;
  char *lastb, *diaval;
  i4_t sst;
  unsigned char t;
  char *scpnt;

  sst = 1;
  lastb = diasc + *diasz;
  diaval = diasc + dscsz;
  scpnt = diasc;
  for (fnk = 0; (t = selsc1 (&scpnt, sst++)) != ENDSC; fnk++)
    {
      diaval = pred_compress (diaval, lastb, df + fnk, t);
      if (diaval == NULL)
        return (-ER_NCF);
    }
  *diasz = diaval - diasc;
  return (OK);
}

int
testcond (struct fun_desc_fields *desf, u2_t fnum, u2_t * fl,
          u2_t * slsz, char *selcon, u2_t fmnum, u2_t * fml)
{
  u2_t fn;

  fn = desf->f_fn;
  for (; fnum != 0; fnum--, fl++)
    if (*fl > fn)
      return (-ER_NCF);
  for (; fmnum != 0; fmnum--, fml++)
    if (*fml > fn)
      return (-ER_NCF);
  if (*slsz != 0)
    {
      u2_t i, scsz;
      i4_t sst, ans = OK, fdf;
      struct des_field *df;
      unsigned char t;
      char *a;
      fdf = desf->f_fdf;
      df = desf->df_pnt;
      a = selcon;
      sst = 1;
      for (i = 0; (t = selsc1 (&a, sst++)) != ENDSC && i < fn ; i++)
	{
	  if (i< fdf && t == EQUN)
	    return (-ER_NCF);
	  if (t < EQ || (t > ANY && t < ENDSC))
	    return (-ER_NCF);
	}
      if (t != ENDSC)
	return (-ER_NCF);
      if (sst % 2 == 0)
	a++;
      scsz = a - selcon;
      if (*slsz != 0)
	ans = sel_cmpr (selcon, slsz, scsz, df);
      if (ans < 0)
	return (-ER_NCF);
    }
  return (OK);
}
