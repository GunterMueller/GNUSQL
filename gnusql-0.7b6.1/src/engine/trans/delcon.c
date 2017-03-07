/*
 *  delcon.c  - mass deletion of rows satisfyed specific condition
 *               on basis scanning of specific table
 *               by itself, by specific index, by specific filter
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

/* $Id: delcon.c,v 1.253 1998/09/29 21:25:26 kimelman Exp $ */

#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"
#include "xmem.h"

extern struct des_nseg desnseg;
extern char *pbuflj;
extern struct ADBL adlj;
extern i4_t ljmsize;

static void
mdel (struct d_r_t *dr, char *cort, u2_t corsize,
      struct id_rel *idr, struct full_des_tuple *dtuple)
{
  struct des_tid *tid;

  modmes ();
  tid = &dtuple->tid_fdt;
  wmlj (DELLJ, ljmsize + corsize, &adlj, idr, tid, 0);
  orddel (dtuple);
  proind (ordindd, dr, dr->desrbd.indnum, cort, tid);
}

CPNM
delcrl (struct id_rel *pidrl, u2_t slsz, char *sc)
{
  u2_t sn, pn, *ali, *ai, corsize;
  struct fun_desc_fields *desf;
  char *asp = NULL;
  char *arrpnt[BD_PAGESIZE];
  u2_t arrsz[BD_PAGESIZE];

  sn = pidrl->urn.segnum;
  if (sn != NRSNUM)
    {
      struct d_r_t *dr;
      struct d_sc_i *scind;
      struct ldesscan *disc;
      char *cort;
      struct des_tid tid;
      u2_t ind, sz;
      i4_t ans;
      i2_t n;
      CPNM cpn;
      struct A pg;
      struct full_des_tuple dtuple;
  
      if ((cpn = cont_fir (pidrl, &dr)) != OK)
	return (cpn);
      desf = &dr->f_df_bt;
      if (testcond (desf, 0, NULL, &slsz, sc, 0, NULL) != OK)
	return (-ER_NCF);
      if ((cpn = synlsc (WSC, pidrl, sc, slsz, desf->f_fn, NULL)) != OK)
	return (cpn);
      cort = pbuflj + ljmsize;
      scind = rel_scan (&pidrl->urn, (char *) dr,
                        &n, 0, NULL, NULL, 0, 0, NULL);
      disc = &scind->dessc;
      ans = fgetnext (disc, &pn, &sz, SLOWSCAN);
      dtuple.sn_fdt = dr->segnr;
      dtuple.rn_fdt = dr->desrbd.relnum;
      while (ans != EOI)
	{
	  ind = 0;
	m1:
	  while ((asp = getpg (&pg, sn, pn, 's')) == NULL);
	  ai = (u2_t *) (asp + phsize);
	  ali = ai + ((struct page_head *) asp)->lastin;
	  for (ai += ind; ai <= ali; ai++, ind++)
	    {
	      if (*ai != 0 && CHECK_PG_ENTRY(ai) &&
		  (corsize = fndslc (dr, asp + *ai, sc, slsz, cort)) != 0)
		{
		  tid.tpn = pn;
		  tid.tindex = ind;
                  putpg (&pg, 'n');
                  dtuple.tid_fdt = tid;
                  mdel (dr, cort, corsize, pidrl, &dtuple);
		  BUF_endop ();
		  ind++;
		  goto m1;
		}
	    }
	  putpg (&pg, 'n');
	  ans = getnext (disc, &pn, &sz, SLOWSCAN);
	}
      delscan (n);
    }
  else
    {
      struct des_trel *dtr;
      char tmp_buff[BD_PAGESIZE];
      
      dtr = (struct des_trel *) * (desnseg.tobtab + pidrl->urn.obnum);
      if (dtr->tobtr.prdt.prob != TREL)
	return (-ER_NDR);
      desf = &dtr->f_df_tt;
      if (testcond (desf, 0, NULL, &slsz, sc, 0, NULL) != OK)
	return (-ER_NCF);
      asp = tmp_buff;
      for (pn = dtr->tobtr.firstpn; pn != (u2_t) ~ 0;)
	{
	  read_tmp_page (pn, asp);
	  ai = (u2_t *) (asp + phtrsize);
	  ali = ai + ((struct p_h_tr *) asp)->linptr;
	  pn = ((struct listtob *) asp)->nextpn;
	  for (; ai <= ali; ai++)
	    if (*ai != 0 && (corsize = tstcsel (desf, slsz, sc, asp + *ai,
                                                arrpnt, arrsz)) != 0)
	      {
		comptr (asp, ai, corsize);
		*ai = 0;
	      }
	  if (frptr (asp) == 1)
	    frptob ((struct des_tob *) dtr, asp, pn);
	}
    }
  return (OK);
}

i4_t
delcin (struct id_ind *pidind, u2_t slsz, char *sc, u2_t diasz, char *diasc)
{
  u2_t *ai, sn, pn, oldpn;
  char *asp = NULL, *cort;
  struct fun_desc_fields *desf;
  struct ldesscan *disc;
  struct d_sc_i *scind;
  struct ldesind *di;
  i4_t pr, rep;
  struct d_r_t *dr;
  struct id_rel *pidrl;
  u2_t corsize, kn, dscsz;
  i2_t n;
  i4_t ans;
  struct des_tid tid;
  struct A pg;
  struct full_des_tuple dtuple;

  pidrl = &pidind->irii;
  sn = pidrl->urn.segnum;
  if ((ans = cont_id (pidind, &dr, &di)) != OK)
    return (ans);
  desf = &dr->f_df_bt;
  if (testcond (desf, 0, NULL, &slsz, sc, 0, NULL) != OK)
    return (-ER_NCF);
  ai = (u2_t *) (di + 1);
  if ((ans = testdsc (dr, &diasz, diasc, ai, &dscsz)) != OK)
    return (ans);

  if ((ans = synlsc (RSC, pidrl, sc, slsz, desf->f_fn, NULL)) != OK)
    return (ans);
  kn = di->ldi.kifn & ~UNIQ & MSK21B;
  if ((ans = synlsc (RSC, pidrl, diasc, diasz, kn, ai)) != OK)
    return (ans);
  cort = pbuflj + ljmsize;
  scind = (struct d_sc_i *) lusc (&n, scisize, (char *) di,
                                  SCI, DSC, 0, NULL, sc, slsz,
				  0, NULL, diasz + size2b);
  disc = &scind->dessc;
  disc->curlpn = (u2_t) ~ 0;
  asp = (char *) scind + scisize + slsz + size2b;
  if (diasz == 0)
    disc->dpnsc = NULL;
  else
    disc->dpnsc = asp;
  t2bpack (diasz, asp);
  disc->dpnsval = asp + size2b + dscsz;
  asp += size2b;
  bcopy (diasc, asp, diasz);
  oldpn = (u2_t) ~0;
  if ((rep = ind_ftid (disc, &tid, SLOWSCAN)) != EOI)
    {
      oldpn = tid.tpn;
      while ((asp = getpg (&pg, sn, oldpn, 's')) == NULL);
    }
  pr = 0;
  dtuple.sn_fdt = sn;
  dtuple.rn_fdt = dr->desrbd.relnum;
  for (; rep != EOI; rep = ind_tid (disc, &tid, SLOWSCAN))
    {
      pn = tid.tpn;
      if (pr == 1)
	{
	  while ((asp = getpg (&pg, sn, pn, 's')) == NULL);
	  oldpn = pn;
	  pr = 0;
	}
      else if (oldpn != pn)
	{
	  putpg (&pg, 'n');
	  while ((asp = getpg (&pg, sn, pn, 's')) == NULL);
	  oldpn = pn;
	}
      ai = (u2_t *) (asp + phsize) + tid.tindex;
      if (*ai != 0 &&
	  (corsize = fndslc (dr, asp + *ai, sc, slsz, cort)) != 0)
	{
          putpg (&pg, 'n');
          dtuple.tid_fdt = tid;
	  mdel (dr, cort, corsize, pidrl, &dtuple);
	  pr = 1;
	}
    }
  delscan (n);
  return (OK);
}

CPNM
delcfl (i4_t idfl, u2_t slsz, char *sc)
{
  u2_t *afi, *ai, sn, pn, off, oldpn, flpn;
  char *asp = NULL, *cort, *aspfl;
  struct d_r_t *dr;
  struct fun_desc_fields *desf;
  struct des_tid *tid, *tidb;
  struct des_fltr *desfl;
  struct id_rel idrl;
  u2_t corsize;
  CPNM cpn;
  struct A pg;
  struct full_des_tuple dtuple;
  char tmp_buff[BD_PAGESIZE];

  if ((u2_t) idfl > desnseg.mtobnum)
    return (-ER_NIOB);
  desfl = (struct des_fltr *) * (desnseg.tobtab + idfl);
  if (desfl == NULL)
    return (-ER_NIOB);    
  if (((struct prtob *) desfl)->prob != FLTR)
    return (-ER_NIOB);
  dr = desfl->pdrtf;
  desf = &dr->f_df_bt;
  if (testcond (desf, 0, NULL, &slsz, sc, 0, NULL) != OK)
    return (-ER_NCF);
  sn = dr->segnr;
  idrl.urn.segnum = dtuple.sn_fdt = sn;
  idrl.urn.obnum = dtuple.rn_fdt = dr->desrbd.relnum;
  idrl.pagenum = dr->pn_r;
  idrl.index = dr->ind_r;
  if ((cpn = synlsc (WSC, &idrl, sc, slsz, desf->f_fn, NULL)) != OK)
    return (cpn);
  cort = pbuflj + ljmsize;
  aspfl = tmp_buff;
  for (flpn = desfl->tobfl.firstpn; flpn != (u2_t) ~ 0;)
    {
      read_tmp_page (flpn, aspfl);
      off = ((struct p_h_f *) aspfl)->freeoff;
      tid = (struct des_tid *) (aspfl + phfsize);
      oldpn = tid->tpn;
      while ((asp = getpg (&pg, sn, oldpn, 's')) == NULL);
      afi = (u2_t *) (asp + phsize);
      tidb = (struct des_tid *) (aspfl + off);
      for (; tid < tidb; tid++)
	{
	  pn = tid->tpn;
	  if (oldpn != pn)
	    {
	      putpg (&pg, 'n');
	      while ((asp = getpg (&pg, sn, pn, 's')) == NULL);
	      afi = (u2_t *) (asp + phsize);
	      oldpn = pn;
	    }
	  ai = afi + tid->tindex;
	  if (*ai != 0 &&
	      (corsize = fndslc (dr, asp + *ai, sc, slsz, cort)) != 0)
            {
              putpg (&pg, 'n');
              dtuple.tid_fdt = *tid;
              mdel (dr, cort, corsize, &idrl, &dtuple);
            }
	}
      flpn = ((struct p_h_f *) aspfl)->listfl.nextpn;
    }
  putpg (&pg, 'n');
  return (OK);
}
