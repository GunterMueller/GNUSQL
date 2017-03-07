/*
 *  fltrel.c  - Relation  Filteration
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

/* $Id: fltrel.c,v 1.251 1998/09/29 21:25:31 kimelman Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

extern struct des_nseg desnseg;
extern char **scptab;

static struct A inpage;

static void
put_crt (char *outasp, char *tuple, struct des_tob *dt,
         struct fun_desc_fields *desf, u2_t fln, u2_t * fl, char *sc, u2_t slsz)
{
  char *a, *b;
  u2_t size, scsz, sz;
  u2_t k, kk, fnk;
  char *val, *aval;
  unsigned char t;
  struct A page2;
  char *arrpnt[BD_PAGESIZE];
  u2_t arrsz[BD_PAGESIZE];

  t = *tuple & MSKCORT;
  if (t == CREM || t == IDTR)
    return;
  if (t == IND)
    {
      u2_t pn, ind;
      IND_REF(page2,inpage.p_sn,tuple);
    }
  if (tstcsel (desf, slsz, sc, tuple, arrpnt, arrsz) != 0)
    {
      aval = val = tuple + scscal (tuple);
      for (size = 1, k = 0, kk = 0; kk < fln; kk++)
        {
          fnk = fl[kk];
          size += arrsz[fnk];
          k++;
          if (k == 7)
            k = 0;
        }
      if (size == 1)
        return;
      scsz = k / 7;
      if (k % 7 != 0)
        scsz++;
      size += scsz;
      a = getloc (outasp, size, dt);
      b = a + scsz + 1;
      for (k = 0, kk = 0, *a++ = CORT; kk < fln; kk++)
        {
          fnk = fl[kk];
          if ((sz = arrsz[fnk]) != 0)
            {
              bcopy (arrpnt[fnk], b, sz);
              b += sz;
              *a |= BITVL(k);	/* a value is present */
            }
          k++;
          if (k == 7)
            {
              k = 0;
              *(++a) = 0;
            }
        }
      if (k == 0)
        a--;
      *a |= EOSC;
    }
  if (t == IND)
    putpg (&page2, 'n');
  return;
}

struct ans_ctob
rflrel (struct id_rel *pidrel, u2_t fln, u2_t *fl, u2_t slsz, char *sc)
{
  u2_t *ali, sn, pn, *ai, i;
  struct des_field *dftr, *df;
  struct fun_desc_fields *desf;
  i2_t n;
  struct des_tob *dt;
  struct des_trel *destrel;
  char *asp = NULL, *outasp;
  struct ans_ctob ans;
  char tmp_buff[BD_PAGESIZE];

  sn = pidrel->urn.segnum;
  outasp = tmp_buff;
  if (sn == NRSNUM)
    {
      struct des_trel *dtrin;
      struct des_field *df;
      char tmp_buff_in[BD_PAGESIZE];
      
      n = pidrel->urn.obnum;
      if ((u2_t) n > desnseg.mtobnum)
	{
	  ans.cpncob = -ER_NIOB;
	  return (ans);
	}
      dtrin = (struct des_trel *) * (desnseg.tobtab + n);
      if (dtrin == NULL)
	{
	  ans.cpncob = -ER_NIOB;
	  return (ans);
	}	
      if (((struct prtob *) dtrin)->prob != TREL)
	{
	  ans.cpncob = -ER_NIOB;
	  return (ans);
	}
      desf = &dtrin->f_df_tt;
      if ((ans.cpncob = testcond (desf, fln, fl, &slsz, sc, 0, NULL)) != OK)
	return (ans);
      dt = gettob (outasp, dtrsize + fln * rfsize, &n, TREL);
      destrel = (struct des_trel *) dt;
      dftr = (struct des_field *) (destrel + 1);
      asp = tmp_buff_in;
      for (pn = dtrin->tobtr.firstpn; pn != (u2_t) ~ 0;)
	{
	  read_tmp_page (pn, asp);
	  ai = (u2_t *) (asp + phtrsize);
	  ali = ai + ((struct p_h_tr *) asp)->linptr;
	  for (; ai <= ali; ai++)
	    if (*ai != 0)
              put_crt (outasp, asp + *ai, dt, desf, fln, fl, sc, slsz);
	  pn = ((struct listtob *) asp)->nextpn;
	}
      if (dtrin->tobtr.prdt.prob == SORT)
	{
          u2_t kn, k1, k2, j, *ks, *ksort;

          df = desf->df_pnt;
	  ksort = (u2_t *) (df + desf->f_fn);
	  ks = (u2_t *) (dftr + fln);
          kn = dtrin->keysntr;
	  for (k1 = 0, k2 = 0, i = 0, j = 0; j < fln && k1 < kn; i++)
	    if (i == fl[j])
	      {
		if (i == ksort[k1])
		  ks[k2++] = ksort[k1++];
		j++;
	      }
	    else if (i == k1)
	      k1++;
	  if (k2 != 0)
	    {
	      dt->prdt.prsort = SORT;
	      destrel->keysntr = k2;
	    }
	}
    }
  else
    {
      struct d_r_t *desrel;
      struct d_sc_i *scind;
      struct ldesscan *disc;
      i4_t rep;
      i2_t num;
      u2_t size;
      
      if ((ans.cpncob = contir (pidrel, &desrel)) != OK)
	return (ans);
      desf = &desrel->f_df_bt;
      if ((ans.cpncob = testcond (desf, fln, fl, &slsz, sc, 0, NULL)) != OK)
	return (ans);
      if ((ans.cpncob = synlsc (RSC, pidrel, sc, slsz, desf->f_fn, NULL)) != 0)
	return (ans);
      dt = gettob (outasp, dtrsize + fln * rfsize, &n, TREL);
      destrel = (struct des_trel *) dt;
      dftr = (struct des_field *) (destrel + 1);
      scind = rel_scan (&pidrel->urn, (char *) desrel, &num,
			0, NULL, NULL, 0, 0, NULL);
      disc = &scind->dessc;
      rep = fgetnext (disc, &pn, &size, FASTSCAN);
      for (; rep != EOI;)
	{
	  while ((asp = getpg (&inpage, sn, pn, 's')) == NULL);
	  ai = (u2_t *) (asp + phsize);
	  ali = ai + ((struct page_head *) asp)->lastin;
	  for (; ai <= ali; ai++)
	    if (*ai != 0 && CHECK_PG_ENTRY(ai))
              put_crt (outasp, asp + *ai, dt, desf, fln, fl, sc, slsz);
	  putpg (&inpage, 'n');
	  rep = getnext (disc, &pn, &size, FASTSCAN);
	}
      destrel->keysntr = 0;
      delscan (num);
    }
  write_tmp_page (dt->lastpn, outasp);
  destrel->f_df_tt.f_fn = fln;
  destrel->f_df_tt.f_fdf = 0;
  destrel->f_df_tt.df_pnt = dftr;
  df = desf->df_pnt;
  for (i = 0; i < fln; i++)
    *dftr++ = df[fl[i]];
  ans.idob.segnum = NRSNUM;
  ans.idob.obnum = n;
  return (ans);
}
struct ans_ctob
rflind (struct id_ind *pidind, u2_t fln, u2_t *fl, u2_t slsz,
        char *sc, u2_t diasz, char *diasc)
{
  struct d_r_t *desrel;
  u2_t ind, size, i, j, k1, k2;
  struct des_field *dftr, *df;
  struct fun_desc_fields *desf;
  struct des_tid tid;
  u2_t sn, pn, *ai, *kind, *ks, kn, dscsz, inpn;
  struct des_tob *dt;
  struct des_trel *destrel;
  struct id_rel *pidrel;
  struct ldesind *di;
  struct d_sc_i *scind;
  struct ldesscan *disc;
  char *asp = NULL, **t, *outasp;
  i2_t scnum, n;
  struct ans_ctob ans;
  i4_t rep;
  char tmp_buff[BD_PAGESIZE];

  pidrel = &pidind->irii;
  sn = pidrel->urn.segnum;
  if (sn == NRSNUM)
    {
      ans.cpncob = -ER_NIOB;
      return (ans);
    }
  if ((ans.cpncob = cont_id (pidind, &desrel, &di)) != OK)
    return (ans);
  desf = &desrel->f_df_bt;
  if (testcond (desf, fln, fl, &slsz, sc, 0, NULL) != OK)
    {
      ans.cpncob = -ER_NCF;
      return (ans);
    }
  kind = (u2_t *) (di + 1);
  if ((ans.cpncob = testdsc (desrel, &diasz, diasc, kind, &dscsz)) != OK)
    return (ans);

  if ((ans.cpncob = synlsc (RSC, pidrel, sc, slsz, desf->f_fn, NULL)) != 0)
    return (ans);
  kn = di->ldi.kifn & ~UNIQ & MSK21B;
  if ((ans.cpncob = synlsc (RSC, pidrel, diasc, diasz, kn, kind)) != OK)
    return (ans);
  scind = (struct d_sc_i *) lusc (&scnum, scisize, (char *) di, SCI, RSC, fln, fl,
				  sc, slsz, 0, NULL, diasz + size2b);
  disc = &scind->dessc;
  disc->curlpn = (u2_t) ~ 0;
  asp = (char *) scind + scisize + slsz + size2b;
  disc->dpnsc = asp;
  t2bpack (diasz, asp);
  disc->dpnsval = asp + size2b + dscsz;
  bcopy (diasc, asp + size2b, diasz);
  di->oscni++;
  size = dtrsize + fln * rfsize;
  outasp = tmp_buff;
  dt = gettob (outasp, size, &n, TREL);
  destrel = (struct des_trel *) dt;
  destrel->f_df_tt.f_fn = fln;
  destrel->f_df_tt.f_fdf = 0;
  dftr = (struct des_field *) (destrel + 1);
  df = desf->df_pnt;
  for (i = 0; i < fln; i++)
    *dftr++ = df[fl[i]];
  if ((rep = ind_ftid (disc, &tid, FASTSCAN)) != EOI)
    {
      inpn = tid.tpn;
      while ((asp = getpg (&inpage, sn, inpn, 's')) == NULL);
    }
  else
    inpn = (u2_t) ~ 0;
  for( ; rep != EOI; rep = ind_tid (disc, &tid, FASTSCAN) )
    {
      pn = tid.tpn;
      ind = tid.tindex;
      if (pn != inpn)
	{
	  putpg (&inpage, 'n');
          while ((asp = getpg (&inpage, sn, pn, 's')) == NULL);
        }
      ai = (u2_t *) (asp + phsize) + ind;;
      if (*ai != 0)
        put_crt (outasp, asp + *ai, dt, desf, fln, fl, sc, slsz);
    }
  t = scptab + scnum;
  xfree ((void *) *t);
  *t = NULL;
  ks = (u2_t *) (dftr + fln);
  for (k1 = 0, k2 = 0, i = 0, j = 0; j < fln && k1 < kn; i++)
    if (i == fl[j])
      {
	if (i == kind[k1])
	  ks[k2++] = kind[k1++];
	j++;
      }
    else if (i == k1)
      k1++;
  if (k2)
    {
      dt->prdt.prsort = SORT;
      destrel->keysntr = k2;
    }
  write_tmp_page (dt->lastpn, outasp);
  delscan (scnum);
  ans.idob.segnum = NRSNUM;
  ans.idob.obnum = n;
  return (ans);
}
struct ans_ctob
rflflt (u2_t idfl, u2_t fln, u2_t * fl, u2_t slsz, char *sc)
{
  struct d_r_t *desrel;
  struct des_tid *tid, *tidb;
  struct des_field *df, *dftr;
  struct fun_desc_fields *desf;
  struct des_fltr *desfl;
  struct des_trel *destrel;
  u2_t sn, fpn, pn, *afi, *ai, off, oldpn, flpn, k;
  i2_t n;
  struct des_tob *dt;
  char *asp = NULL, *aspfl, *outasp;
  struct ans_ctob ans;
  struct A outpg;
  char tmp_buff[BD_PAGESIZE], tmp_buff_in[BD_PAGESIZE];

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
  desf = &desrel->f_df_bt;
  if (testcond (desf, fln, fl, &slsz, sc, 0, NULL) != OK)
    {
      ans.cpncob = -ER_NCF;
      return (ans);
    }
  outasp = tmp_buff;
  dt = gettob (outasp, dtrsize + fln * rfsize, &n, TREL);
  destrel = (struct des_trel *) dt;
  destrel->f_df_tt.f_fn = fln;
  destrel->f_df_tt.f_fdf = 0;
  dftr = (struct des_field *) (destrel + 1);
  destrel->f_df_tt.df_pnt = dftr;
  df = desf->df_pnt;
  for (k = 0; k < fln; k++)
    *dftr++ = df[fl[k]];
  fpn = desfl->tobfl.firstpn;
  aspfl = tmp_buff_in;
  for (flpn = fpn; flpn != (u2_t) ~ 0;)
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
	      oldpn = pn;
	      afi = (u2_t *) (asp + phsize);
	    }
	  ai = afi + tid->tindex;
	  if (*ai != 0)
            put_crt (outasp, asp + *ai, dt, desf, fln, fl, sc, slsz);
	}
      flpn = ((struct listtob *) aspfl)->nextpn;
    }
  write_tmp_page (outpg.p_pn, outasp);
  if (desfl->tobfl.prdt.prsort == SORT && destrel->keysntr == 0)
    dt->prdt.prsort = SORT;
  ans.cpncob = OK;
  ans.idob.segnum = NRSNUM;
  ans.idob.obnum = n;
  return (ans);
}

