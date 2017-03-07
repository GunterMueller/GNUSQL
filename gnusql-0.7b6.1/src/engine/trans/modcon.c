/*
 *  modcon.c  - mass modification of rows satisfyed specific condition
 *              on basis scanning of specific table
 *              by itself, by specific index, by specific filter
 *              Kernel of GNU SQL-server  
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

/* $Id: modcon.c,v 1.254 1998/09/29 21:25:37 kimelman Exp $ */

#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"
#include "xmem.h"

extern struct des_nseg desnseg;
extern char *pbuflj;
extern struct ADBL adlj;
extern i4_t ljmsize;
extern i4_t ljrsize;

static void
mmod (struct d_r_t *dr, char *cort, u2_t corsize, struct des_tid *tid,
      struct des_tid *ref_tid, u2_t mod_count, u2_t *mfn, Colval colval, u2_t *lenval)
{
  char *nc;
  u2_t newsize;
  i4_t n, ni;
  struct ADBL last_adlj;
  struct id_rel idr;
  struct full_des_tuple dtuple;

  nc = cort + corsize;
  newsize = cortform (&dr->f_df_bt, colval, lenval, cort, nc, mod_count, mfn);
  if (newsize <size4b)
    newsize = size4b;
  modmes ();
  last_adlj = adlj;
  idr.urn.segnum = dtuple.sn_fdt = dr->segnr;
  idr.urn.obnum = dtuple.rn_fdt = dr->desrbd.relnum;
  idr.pagenum = dr->pn_r;
  idr.index = dr->ind_r;
  wmlj (MODLJ, ljmsize + corsize + newsize, &adlj, &idr, tid, 0);
  dtuple.tid_fdt = *tid;
  ordmod (&dtuple, ref_tid, corsize, nc, newsize);
  n = dr->desrbd.indnum;
  ni = mproind (dr, n, cort, nc, tid);
  if (ni < n)
    {
      wmlj (RLBLJ, ljrsize, &last_adlj, &idr, tid, 0);
      mproind (dr, ni + 1, nc, cort, tid);
      ordmod (&dtuple, ref_tid, newsize, cort, corsize);
    }
  BUF_endop ();
}

static int
fnd_slc (struct d_r_t *dr, char *tuple, char *selcon,
         u2_t slsz, char *cort, struct des_tid *ref_tid)
{
  unsigned char t;
  struct A inpage;
  char *arrpnt[BD_PAGESIZE];
  u2_t arrsz[BD_PAGESIZE];
  int tuple_size;

  t = *tuple & MSKCORT;
  if (t == CREM || t == IDTR)
    return (-ER_NCR);
  if (t == IND)
    {
      char *asp;
      u2_t pn2, ind2, *ai;
      ref_tid->tindex = ind2 = t2bunpack (tuple + 1);
      ref_tid->tpn = pn2 = t2bunpack (tuple + 1 + size2b);
      while ((asp = getpg (&inpage, dr->segnr, pn2, 's')) == NULL);
      ai = (u2_t *) (asp + phsize) + ind2;
      tuple = asp + *ai;
    }
  else
    ref_tid->tpn = (u2_t) ~ 0;
  tuple_size = tstcsel (&dr->f_df_bt, slsz, selcon, tuple, arrpnt, arrsz);
  if (tuple_size != 0 && cort != NULL)
    bcopy (tuple, cort, tuple_size);
  if (t == IND)
    putpg (&inpage, 'n');
  return (tuple_size);
}

i4_t
modcrl (struct id_rel *pidrl, u2_t slsz, char *sc, u2_t flsz,
        u2_t *fl, Colval colval, u2_t *lenval)
{
  u2_t *ali, *ai, sn, pn, corsize;
  char *asp = NULL, *cort;
  struct fun_desc_fields *desf;
  struct A pg;

  sn = pidrl->urn.segnum;
  if (sn != NRSNUM)
    {
      struct d_r_t *dr;
      struct d_sc_i *scind;
      struct ldesscan *disc;
      struct des_tid tid, ref_tid;
      CPNM cpn;
      i2_t num;
      i4_t rep;
      u2_t size, ind;
      
      if ((cpn = cont_fir (pidrl, &dr)) != OK)
	return (cpn);
      desf = &dr->f_df_bt;
      if (testcond (desf, 0, NULL, &slsz, sc, 0, NULL) != OK)
	return (-ER_NCF);
      if (check_cmod (desf, flsz, fl, colval, lenval) != OK)
	return (-ER_NCF);
      if ((cpn = synlsc (WSC, pidrl, sc, slsz, desf->f_fn, NULL)) != OK)
	return (cpn);
      if ((cpn = syndmod (dr, flsz, fl, colval, lenval)) != OK)
	return (cpn);
      cort = pbuflj + ljmsize;
      scind = rel_scan (&pidrl->urn, (char *) dr,
                        &num, 0, NULL, NULL, 0, 0, NULL);
      disc = &scind->dessc;
      rep = fgetnext (disc, &pn, &size, SLOWSCAN);
      for (; rep != EOI;)
	{
	  ind = 0;
	m1:
          while ((asp = getpg (&pg, sn, pn, 's')) == NULL);
	  ai = (u2_t *) (asp + phsize);
	  ali = ai + ((struct page_head *) asp)->lastin;
	  for (ai += ind; ai <= ali; ai++)
	    if (*ai != 0 && CHECK_PG_ENTRY(ai) &&
		(corsize = fnd_slc (dr, asp + *ai, sc, slsz,
                                    cort, &ref_tid)) != 0)
	      {
		tid.tpn = pn;
		tid.tindex = ind;
                putpg (&pg, 'n');
		mmod (dr, cort, corsize, &tid,
                      &ref_tid, flsz, fl, colval, lenval);
		ind++;
		goto m1;
	      }
	  putpg (&pg, 'n');
	  rep = getnext (disc, &pn, &size, SLOWSCAN);
	}
      delscan (num);
    }
  else
    {
      struct des_trel *dtr;
      struct des_tob *dt;
      i2_t delta;
      u2_t newsize;
      char *outasp;
      struct A pg_out;
      char *arrpnt[BD_PAGESIZE];
      u2_t arrsz[BD_PAGESIZE];
      char tmp_buff[BD_PAGESIZE], tmp_buff_out[BD_PAGESIZE];
  
      dtr = (struct des_trel *) * (desnseg.tobtab + pidrl->urn.obnum);
      dt = (struct des_tob *)dtr;
      if (dtr->tobtr.prdt.prob != TREL)
	return (-ER_NDR);
      desf = &dtr->f_df_tt;
      if (testcond (desf, 0, NULL, &slsz, sc, 0, NULL) != OK)
	return (-ER_NCF);
      if (check_cmod (desf, flsz, fl, colval, lenval) != OK)
	return (-ER_NCF);
      outasp = pg_out.p_shm = tmp_buff_out;
      asp = tmp_buff;
      read_tmp_page (dtr->tobtr.lastpn, outasp);
      for (pn = dtr->tobtr.firstpn; pn != (u2_t) ~ 0;)
	{
	  read_tmp_page (pn, asp);
	  ai = (u2_t *) (asp + phtrsize);
	  ali = ai + ((struct p_h_tr *) asp)->linptr;
	  pn = ((struct listtob *) asp)->nextpn;
	  for (; ai <= ali; ai++)
	    if (*ai != 0 &&
                (corsize = tstcsel (desf, slsz, sc, asp + *ai,
                                    arrpnt, arrsz)) != 0)
              {
                cort = asp + *ai;
                newsize = cortform (desf, colval, lenval, cort, pbuflj, flsz, fl);
                delta = corsize - newsize;
                if (delta > 0)
                  comptr (asp, ai, (u2_t) delta);
                if (delta >= 0)
                  bcopy (pbuflj, cort, newsize);
                else
                  {
                    comptr (asp, ai, corsize);
                    *ai = 0;
                    minstr (outasp, pbuflj, newsize, dt);
                  }
              }
	  if (frptr (asp) == 1)
	    frptob (dt, asp, pn);
	  else
	    write_tmp_page (pn, asp);
	}
      write_tmp_page (dt->lastpn, outasp);
      dtr->tobtr.prdt.prsort = NSORT;
    }
  return (OK);
}

CPNM
modcin (struct id_ind *pidind, u2_t slsz, char *sc, u2_t diasz,
        char *diasc, u2_t flsz, u2_t * fl, Colval colval, u2_t *lenval)
{
  u2_t *ai, sn, pn, oldpn, corsize, dscsz, kn;
  struct fun_desc_fields *desf;
  char *asp = NULL, *cort;
  struct des_field *df;
  struct ldesscan *disc;
  struct d_sc_i *scind;
  struct ldesind *di;
  struct id_rel *pidrl;
  struct d_r_t *dr;
  i4_t pr, rep;
  i2_t n;
  CPNM cpn;
  struct des_tid tid, ref_tid;
  struct A pg;

  oldpn = 0;
  pidrl = &pidind->irii;
  sn = pidrl->urn.segnum;
  if ((cpn = cont_id (pidind, &dr, &di)) != OK)
    return (cpn);
  desf = &dr->f_df_bt;
  df = desf->df_pnt;
  if (testcond (desf, 0, NULL, &slsz, sc, 0, NULL) != OK)
    return (-ER_NCF);
  if (check_cmod (desf, flsz, fl, colval, lenval) != OK)
    return (-ER_NCF);
  ai = (u2_t *) (di + 1);
  if (testdsc (dr, &diasz, diasc, ai, &dscsz) != OK)
    return (-ER_NCF);

  if ((cpn = synlsc (WSC, pidrl, sc, slsz, desf->f_fn, NULL)) != OK)
    return (cpn);
  if ((cpn = syndmod (dr, flsz, fl, colval, lenval)) != OK)
    return (cpn);
  kn = di->ldi.kifn & ~UNIQ & MSK21B;
  if ((cpn = synlsc (RSC, pidrl, diasc, diasz, kn, ai)) != OK)
    return (cpn);

  cort = pbuflj + ljmsize;
  scind = (struct d_sc_i *) lusc (&n, scisize, (char *) di, SCI, 'w',
                                  0, NULL, sc, slsz,
				  0, NULL, diasz + size2b);
  disc = &scind->dessc;
  disc->curlpn = (u2_t) ~ 0;
  asp = (char *) scind + scisize + slsz + size2b;
  disc->dpnsc = asp;
  t2bpack (diasz, asp);
  disc->dpnsval = asp + size2b + dscsz;
  bcopy (diasc, asp + size2b, diasz);
  pr = 1;
  if ((rep = ind_ftid (disc, &tid, SLOWSCAN)) != EOI)
    {
      oldpn = tid.tpn;
      while ((asp = getpg (&pg, sn, oldpn, 's')) == NULL);
      pr = 0;
    }
  for (; rep != EOI; rep = ind_tid (disc, &tid, SLOWSCAN))
    {
      pn = tid.tpn;
      assert( pr == 1 || oldpn != 0 );
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
	  (corsize = fnd_slc (dr, asp + *ai, sc, slsz, cort, &ref_tid)) != 0)
	{
          putpg (&pg, 'n');
	  mmod (dr, cort, corsize, &tid, &ref_tid,flsz, fl, colval, lenval);
	  pr = 1;
	}
    }
  delscan (n);
  if (pr == 0)
    putpg (&pg, 'n');
  return (OK);
}

int
modcfl (i4_t idfl, u2_t slsz, char *sc, u2_t flsz, u2_t *fl,
        Colval colval, u2_t *lenval)
{
  u2_t *afi, *ai, pn, off, oldpn, flpn, sn, corsize;
  char *asp = NULL, *cort, *aspfl;
  struct d_r_t *dr;
  struct fun_desc_fields *desf;
  struct des_tid *tid, *tidb, ref_tid;
  struct des_fltr *desfl;
  struct id_rel idrl;
  struct A pg;
  CPNM cpn;
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
  if (check_cmod (desf, flsz, fl, colval, lenval) != OK)
    return (-ER_NCF);
  sn = dr->segnr;
  idrl.urn.segnum = sn;
  idrl.urn.obnum = dr->desrbd.relnum;
  idrl.pagenum = dr->pn_r;
  idrl.index = dr->ind_r;
  if ((cpn = synlsc (WSC, &idrl, sc, slsz, desf->f_fn, NULL)) != OK)
    return (cpn);
  if ((cpn = syndmod (dr, flsz, fl, colval, lenval)) != OK)
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
	      (corsize = fnd_slc (dr, asp + *ai, sc, slsz, cort, &ref_tid)) != 0)
            {
              putpg (&pg, 'n');
              mmod (dr, cort, corsize, tid, &ref_tid, flsz, fl, colval, lenval);
            }
	}
      flpn = ((struct p_h_f *) aspfl)->listfl.nextpn;
    }
  putpg (&pg, 'n');
  return (OK);
}
