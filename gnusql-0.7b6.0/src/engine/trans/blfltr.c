/*
 * blfltr.c  - building of a filter by a DB table,
 *              by a DB table index, by a filter
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

/* $Id: blfltr.c,v 1.251 1997/10/22 16:23:38 kml Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

extern char **scptab;
extern struct des_nseg desnseg;

struct ans_ctob
blflrl (struct id_rel *pidrel, u2_t slsz, char *sc, u2_t kn, u2_t *fsrt)
{
  u2_t *ali, sn, ind, pn, *ai, sz;
  struct des_fltr *desfltr;
  struct fun_desc_fields *desf;
  char *asp = NULL, *outasp;
  i2_t n, nsc;
  struct ans_ctob ans;
  struct d_r_t *desrel;
  struct des_tid tid;
  struct prtob *pr;
  struct A inpage;
  i4_t rep;
  struct d_sc_i *scind;
  struct ldesscan *disc;
  CPNM cpn;
  char tmp_buff_out[BD_PAGESIZE];

  sn = pidrel->urn.segnum;
  if (sn == NRSNUM)
    {
      ans.cpncob = -ER_NIOB;
      return (ans);
    }
  if ((ans.cpncob = contir (pidrel, &desrel)) != OK)
    return (ans);
  desf = &desrel->f_df_bt;
  if (testcond (desf, 0, NULL, &slsz, sc, 0, NULL) != OK)
    {
      ans.cpncob = -ER_NCF;
      return (ans);
    }
  if ((cpn = synlsc (RSC, pidrel, sc, slsz, desf->f_fn, NULL)) != 0)
    {
      ans.cpncob = cpn;
      return (ans);
    }
  outasp = tmp_buff_out;
  desfltr = (struct des_fltr *) gettob (outasp, dflsize + slsz, &n, FLTR);
  desfltr->pdrtf = desrel;
  desfltr->selszfl = slsz;
  bcopy (sc, (char *) (desfltr + 1), slsz);
  scind = rel_scan (&pidrel->urn, (char *) desrel, &nsc,
		    0, NULL, NULL, 0, 0, NULL);
  disc = &scind->dessc;
  rep = fgetnext (disc, &pn, &sz, FASTSCAN);
  while (rep != EOI)
    {
      while ((asp = getpg (&inpage, sn, pn, 's')) == NULL);
      ai = (u2_t *) (asp + phsize);
      ali = ai + ((struct page_head *) asp)->lastin;
      for (ind = 0; ai <= ali; ai++, ind++)
	if (*ai != 0 && CHECK_PG_ENTRY(ai)
            && fndslc (desrel, asp + *ai, sc, slsz, NULL) != 0)
	  {
	    tid.tpn = pn;
	    tid.tindex = ind;
	    minsfltr (outasp, (struct des_tob *) desfltr, &tid);
	  }
      putpg (&inpage, 'n');
      rep = getnext (disc, &pn, &sz, FASTSCAN);
    }
  write_tmp_page (((struct des_tob *) desfltr)->lastpn, outasp);
  delscan (nsc);
  srtr_tid ((struct des_tob *)desfltr);
  desfltr->selszfl = slsz;
  pr = &desfltr->tobfl.prdt;
  pr->prsort = SORT;
  pr->prdbl = NODBL;
  pr->prdrctn = GROW;
  ans.idob.segnum = NRSNUM;
  ans.idob.obnum = n;
  return (ans);
}

struct ans_ctob
blflin (struct id_ind *pidind, u2_t slsz, char *sc,
        u2_t diasz, char *diasc, u2_t kn, u2_t *fsrt)
{
  u2_t sn, pn, oldpn, *afi, *ai, dscsz, kind;
  struct des_fltr *desfltr;
  i4_t rep, index;
  char *asp = NULL, *outasp;
  struct fun_desc_fields *desf;
  struct des_tid tid;
  struct id_rel *pidrel;
  struct ldesind *di;
  struct d_sc_i *scind;
  struct ldesscan *disc;
  i2_t n, nf;
  struct ans_ctob ans;
  struct d_r_t *desrel;
  struct prtob *pr;
  struct A inpage;
  CPNM cpn;
  char tmp_buff_out[BD_PAGESIZE];

  pidrel = &pidind->irii;
  index = pidind->inii;
  sn = pidrel->urn.segnum;
  if (sn == NRSNUM)
    {
      ans.cpncob = -ER_NIOB;
      return (ans);
    }
  if ((ans.cpncob = cont_id (pidind, &desrel, &di)) != OK)
    return (ans);
  desf = &desrel->f_df_bt;
  if (testcond (desf, 0, NULL, &slsz, sc, 0, NULL) != OK)
    {
      ans.cpncob = -ER_NCF;
      return (ans);
    }
  ai = (u2_t *) (di + 1);
  if ((ans.cpncob = testdsc (desrel, &diasz, diasc, ai, &dscsz)) != OK)
    return (ans);
  if ((cpn = synlsc (RSC, pidrel, sc, slsz, desf->f_fn, NULL)) != 0)
    {
      ans.cpncob = cpn;
      return (ans);
    }
  kind = di->ldi.kifn & ~UNIQ & MSK21B;
  if ((ans.cpncob = synlsc (RSC, pidrel, diasc, diasz, kind, ai)) != OK)
    return (ans);

  scind = (struct d_sc_i *) lusc (&n, scisize, (char *) di, SCI, RSC, 0,
                                  NULL, sc, slsz, 0, NULL, diasz + size2b);
  disc = &scind->dessc;
  disc->curlpn = (u2_t) ~ 0;
  asp = (char *) scind + scisize + slsz + size2b;
  if (diasz == 0)
    disc->dpnsc = NULL;
  else
    disc->dpnsc = asp;
  t2bpack (diasz, asp);
  disc->dpnsval = asp + size2b + dscsz;
  bcopy (diasc, asp + size2b, diasz);
  outasp = tmp_buff_out;
  desfltr = (struct des_fltr *) gettob (outasp, dflsize + slsz, &nf, FLTR);
  desfltr->pdrtf = desrel;
  bcopy (sc, (char *) (desfltr + 1), slsz);
  if ((rep = ind_ftid (disc, &tid, FASTSCAN)) != EOI)
    {
      oldpn = tid.tpn;
      while ((asp = getpg (&inpage, sn, oldpn, 's')) == NULL);
      afi = (u2_t *) (asp + phsize);      
    }
  else
    {
      ans.cpncob = -ER_EMFL;
      putpg (&inpage, 'n');
      goto m1;
    }
  for (; rep != EOI; rep = ind_tid (disc, &tid, FASTSCAN))
    {
      pn = tid.tpn;
      if (oldpn != pn)
	{
	  putpg (&inpage, 'n');
	  while ((asp = getpg (&inpage, sn, pn, 's')) == NULL);
          afi = (u2_t *) (asp + phsize);
	  oldpn = pn;
	}
      ai = afi + tid.tindex;
      if (*ai != 0 &&
          fndslc (desrel, asp + *ai, sc, slsz, NULL) != 0)
	minsfltr (outasp, (struct des_tob *) desfltr, &tid);
    }
  putpg (&inpage, 'n');
m1:
  write_tmp_page (((struct des_tob *) desfltr)->lastpn, outasp);
  delscan (n);
  srtr_tid ((struct des_tob *)desfltr);
  desfltr->selszfl = slsz;
  pr = &desfltr->tobfl.prdt;
  pr->prsort = SORT;
  pr->prdbl = NODBL;
  pr->prdrctn = GROW;
  ans.idob.segnum = NRSNUM;
  ans.idob.obnum = nf;
  return (ans);
}

struct ans_ctob
blflfl (i4_t idfl, u2_t slsz, char *sc, u2_t kn, u2_t *fsrt)
{
  u2_t sn, pn, oldpn, flpn, off, *afi, *ai;
  struct des_fltr *desfltr, *desfl;
  char *aspfl, *asp = NULL, *outasp;
  struct fun_desc_fields *desf;
  struct des_tid *tid, *last_tid;
  i2_t n;
  struct ans_ctob ans;
  struct d_r_t *desrel;
  struct prtob *pr;
  struct A inpage;
  char tmp_buff_in[BD_PAGESIZE], tmp_buff_out[BD_PAGESIZE];

  if ((u2_t) idfl > desnseg.mtobnum)
    {
      ans.cpncob = -ER_NIOB;
      return (ans);
    }
  desfl = (struct des_fltr *) * (desnseg.tobtab + idfl);
  if (desfl == NULL)
    {
      ans.cpncob = -ER_NIOB;
      return (ans);
    }    
  if (((struct prtob *) desfl)->prob != FLTR)
    {
      ans.cpncob = -ER_NIOB;
      return (ans);
    }
  desrel = desfl->pdrtf;
  sn = desrel->segnr;
  if (sn == NRSNUM)
    {
      ans.cpncob = -ER_NIOB;
      return (ans);
    }
  desf = &desrel->f_df_bt;
  if (testcond (desf, 0, NULL, &slsz, sc, 0, NULL) != OK)
    {
      ans.cpncob = -ER_NCF;
      return (ans);
    }
  aspfl = tmp_buff_in;
  outasp = tmp_buff_out;
  desfltr = (struct des_fltr *) gettob (outasp, dflsize + slsz, &n, FLTR);
  desfltr->pdrtf = desrel;
  
  bcopy (sc, (char *) (desfltr + 1), slsz);
  for (flpn = desfl->tobfl.firstpn; flpn != (u2_t) ~ 0;)
    {
      read_tmp_page (flpn, aspfl);
      off = ((struct p_h_f *) aspfl)->freeoff;
      tid = (struct des_tid *) (aspfl + phfsize);
      oldpn = tid->tpn;
      while ((asp = getpg (&inpage, sn, oldpn, 's')) == NULL);
      afi = (u2_t *) (asp + phsize);
      last_tid = (struct des_tid *) (aspfl + off);
      for (; tid < last_tid; tid++)
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
              fndslc (desrel, asp + *ai, sc, slsz, NULL) != 0)
	    minsfltr (outasp, (struct des_tob *) desfltr, tid);
	}
      flpn = ((struct p_h_f *) aspfl)->listfl.nextpn;
    }
  putpg (&inpage, 'n');
  write_tmp_page (((struct des_tob *) desfltr)->lastpn, outasp);
  srtr_tid ((struct des_tob *)desfltr);
  desfltr->selszfl = slsz;
  pr = &desfltr->tobfl.prdt;
  pr->prsort = SORT;
  pr->prdbl = NODBL;
  pr->prdrctn = GROW;
  ans.idob.segnum = NRSNUM;
  ans.idob.obnum = n;
  return (ans);
}
