/*
 *  inscon.c  - mass insertion of rows satisfyed specific condition
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

/* $Id: inscon.c,v 1.253 1998/09/29 22:23:39 kimelman Exp $ */

#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"
#include "xmem.h"

extern struct des_nseg desnseg;
extern char *pbuflj;
extern struct ADBL adlj;
extern i4_t ljmsize;
extern i4_t ljrsize;
extern struct ldesind **TAB_IFAM;

#define CMP_REL \
{\
  struct fun_desc_fields *desf2;\
  if (testcond (desf, 0, NULL, &slsz, sc, 0, NULL) != OK)\
    return (-ER_NCF);\
  if (sn_out != NRSNUM)\
    {\
      if ((cpn = cont_fir (pidrl_out, &dr_out)) != OK)\
	return (cpn);\
      desf2 = &dr_out->f_df_bt;\
      if ((cpn = synlsc (WSC, pidrl_out, sc, slsz, desf2->f_fn, NULL)) != OK)\
        return (cpn);\
    }\
  else\
    {				/* The insertion to the temporary relation */\
      dtr_out = (struct des_trel *) * (desnseg.tobtab + pidrl_out->urn.obnum);\
      if (dtr_out->tobtr.prdt.prob != TREL)\
	return (-ER_NDR);\
      desf2 = &dtr_out->f_df_tt;\
      dt = (struct des_tob *)dtr_out;\
    }\
  if ((cpn = cmprel (desf, desf2)) != OK)\
    return (cpn);\
}

/*-------------------------------------------------------------*/

#define INSERTION_BY_TID(pn,ind)                                \
  if (oldpn != pn)                                              \
    {                                                           \
      putpg (&inpage, 'n');                                     \
      while ((asp = getpg (&inpage, sn_in, pn, 's')) == NULL);  \
      afi = (u2_t *) (asp + phsize);                            \
      oldpn = pn;                                               \
     }                                                          \
  ai = afi + ind;
/*-------------------------------------------------------------*/

#define MASS_INSERTION                                          \
  if (*ai != 0 && CHECK_PG_ENTRY(ai))                           \
    {                                                           \
      if ((corsize = fndslc (dr_in, asp + *ai, sc,              \
                             slsz, cort)) != 0)                 \
        {                                                       \
          if (sn_out != NRSNUM)                                 \
            mins (cort, corsize, &outpage, &freesz, dr_out);    \
          else                                                  \
            minstr (outasp, cort, corsize, dt);                 \
        }                                                       \
    }
/*-------------------------------------------------------------*/

#define END_MASS_INS \
if (sn_out != NRSNUM)\
    eop_mass_ins (pidrl_out->urn.obnum, &outpage, freesz);\
  else\
    {\
      dtr_out->tobtr.prdt.prsort = NSORT;\
      write_tmp_page (dt->lastpn, outasp);\
    }
/*-------------------------------------------------------------*/

static int
cmprel (struct fun_desc_fields *desf1, struct fun_desc_fields *desf2)
{
  struct des_field *ldf, *df1, *df2;
  u2_t type, fn1, fn2, fdf1, fdf2;

  fn1 = desf1->f_fn;
  fn2 = desf2->f_fn;
  fdf1 = desf1->f_fdf;
  fdf2 = desf2->f_fdf;
  if (fn1 != fn2 || fdf1 != fdf2)
    return (-ER_N_EQV);
  df1 = desf1->df_pnt;
  df2 = desf2->df_pnt;
  for (ldf = df1 + fn1; df1 < ldf; df1++, df2++)
    {
      if ((type = df1->field_type) != df2->field_type)
	return (-ER_N_EQV);
      if (type == TCH || type == TFL)
	if (df1->field_size > df2->field_size)
	  return (-ER_N_EQV);
    }
  return (OK);
}

static void
eop_mass_ins (i4_t rn, struct A *outpage, u2_t freesz)
{
  if (outpage->p_shm != NULL)
    {
      u2_t sn;
      sn = outpage->p_sn;
      tab_difam (sn);
      insrec (TAB_IFAM[sn], rn, outpage->p_pn, freesz);
      putwul (outpage, 'm');
    }
}

static void
mins (char *cort, u2_t corsize, struct A *outpage,
      u2_t *freesz, struct d_r_t *desrel)
{
  char *outasp;
  struct page_head *ph;
  i4_t n, ni, rn;
  u2_t size, delta, sn;
  struct des_tid tid;
  struct id_rel idr;
  struct ADBL last_adlj;

  modmes ();
  *cort = CORT;
  delta = corsize + size2b;
  last_adlj = adlj;
  sn = desrel ->segnr;
  rn = desrel->desrbd.relnum;
  if (delta > * freesz)
    {
      u2_t newpn;
      eop_mass_ins (rn, outpage, *freesz);
      newpn = getempt (sn);
      outasp = getnew (outpage, sn, newpn);
      tid.tpn = newpn;
      tid.tindex = 0;
      size = BD_PAGESIZE - corsize;
      ph = (struct page_head *) outasp;
      ph->lastin = 0;
      t2bpack (size, outasp + phsize);
      *freesz = BD_PAGESIZE - phsize - delta;
    }
  else
    {
      u2_t *ai;
      outasp = outpage->p_shm;
      ph = (struct page_head *) outasp;
      ai = (u2_t *) (outasp + phsize) + ph->lastin;
      size = *ai - corsize;
      ph->lastin++;
      tid.tpn = outpage->p_pn;
      tid.tindex = ph->lastin;
      *(ai + 1) = size;
      *freesz -= delta;
    }
  idr.urn.segnum = sn;
  idr.urn.obnum = rn;
  idr.pagenum = desrel->pn_r;
  idr.index = desrel->ind_r;
  wmlj (INSLJ, ljmsize + corsize, &adlj, &idr, &tid, 0);
  bcopy (cort, outasp + size, corsize);
  n = desrel->desrbd.indnum;
  ni = proind (ordindi, desrel, n, cort, &tid);
  if (ni < n)
    {
      struct full_des_tuple dtuple;
      wmlj (RLBLJ, ljrsize, &last_adlj, &idr, &tid, 0);
      proind (ordindd, desrel, ni, cort, &tid);
      dtuple.sn_fdt = sn;
      dtuple.rn_fdt = rn;
      dtuple.tid_fdt = tid;
      orddel (&dtuple);
    }
}

CPNM
inscrl (struct id_rel *pidrl_in, struct id_rel *pidrl_out,
        u2_t fln, u2_t * fl, u2_t slsz, char *sc)
{
  u2_t *ali, sn_in, sn_out, *ai, pn, corsize, freesz = 0;
  char *cort, *asp = NULL, *outasp = NULL;
  struct fun_desc_fields *desf;
  struct d_r_t *dr_out;
  struct des_trel *dtr_out = NULL;
  struct des_tob *dt = NULL;
  CPNM cpn;
  struct A outpage;
  char *arrpnt[BD_PAGESIZE];
  u2_t arrsz[BD_PAGESIZE];
  char tmp_buff_out[BD_PAGESIZE];

  sn_in = pidrl_in->urn.segnum;
  sn_out = pidrl_out->urn.segnum;
  if (sn_in == sn_out && pidrl_in->urn.obnum == pidrl_out->urn.obnum)
    return (-ER_NDR);
  cort = pbuflj + ljmsize;
  outpage.p_shm = NULL;
  if (sn_in != NRSNUM)
    {
      struct d_r_t *dr_in;
      struct d_sc_i *scind;
      struct ldesscan *disc;
      u2_t size;
      i4_t rep;
      i2_t num;
      struct A inpage;
      
      if ((cpn = contir (pidrl_in, &dr_in)) != OK)
	return (cpn);
      desf = &dr_in->f_df_bt;
      CMP_REL;
      if ((cpn = synlsc (RSC, pidrl_in, sc, slsz, desf->f_fn, NULL)) != OK)
	return (cpn);      
      
      if (sn_out == NRSNUM)
        {
          outasp = tmp_buff_out;
          read_tmp_page (dtr_out->tobtr.lastpn, outasp);
        }
      scind = rel_scan (&pidrl_in->urn, (char *) dr_in,
                        &num, 0, NULL, NULL, 0, 0, NULL);
      disc = &scind->dessc;
      rep = fgetnext (disc, &pn, &size, FASTSCAN);
      while (rep != EOI)
	{
	  while ((asp = getpg (&inpage, sn_in, pn, 's')) == NULL);
	  ai = (u2_t *) (asp + phsize);
	  ali = ai + ((struct page_head *) asp)->lastin;
	  for (; ai <= ali; ai++)
            MASS_INSERTION;
	  putpg (&inpage, 'n');
	  rep = getnext (disc, &pn, &size, FASTSCAN);
	}
      delscan (num);
    }
  else       /* sn_in == NRSNUM */
    {
      struct des_trel *dtr_in;
      char tmp_buff_in[BD_PAGESIZE];
      
      dtr_in = (struct des_trel *) * (desnseg.tobtab + pidrl_in->urn.obnum);
      if (dtr_in->tobtr.prdt.prob != TREL)
	return (-ER_NDR);
      desf = &dtr_in->f_df_tt;
      CMP_REL;
      if (sn_out == NRSNUM)
        {
          outasp = tmp_buff_out;
          read_tmp_page (dtr_out->tobtr.lastpn, outasp);
        }
      asp = tmp_buff_in;
      for (pn = dtr_in->tobtr.firstpn; pn != (u2_t) ~ 0;)
	{
	  read_tmp_page (pn, asp);
	  ai = (u2_t *) (asp + phtrsize);
	  ali = ai + ((struct p_h_tr *) asp)->linptr;
	  for (; ai <= ali; ai++)
	    if (*ai != 0 &&
                (corsize = tstcsel (desf, slsz, sc, asp + *ai,
                                    arrpnt, arrsz)) != 0 )
              {
		if (sn_out != NRSNUM)
                  mins (asp + *ai, corsize, &outpage, &freesz, dr_out);
		else
                  minstr (outasp, asp + *ai, corsize, dt);
	      }	      
	  pn = ((struct listtob *) asp)->nextpn;
	}
    }
  END_MASS_INS;
  return (OK);
}

i4_t
inscin (struct id_ind *pidind, struct id_rel *pidrl_out, u2_t fln,
        u2_t * fl, u2_t slsz, char *sc, u2_t diasz, char *diasc)
{
  u2_t sn_in, sn_out, oldpn;
  struct fun_desc_fields *desf;
  char *asp = NULL, *cort, *outasp = NULL;
  struct ldesscan *disc;
  struct d_sc_i *scind;
  struct ldesind *di;
  u2_t *afi, *ai, corsize, kn, dscsz, freesz = 0;
  struct d_r_t *dr_in, *dr_out;
  struct id_rel *pidrl_in;
  struct des_tid tid;
  struct des_trel *dtr_out = NULL;
  i2_t n;
  i4_t cpn, rep;
  struct des_tob *dt = NULL;  
  struct A inpage, outpage;
  char tmp_buff_out[BD_PAGESIZE];

  pidrl_in = &pidind->irii;
  sn_in = pidrl_in->urn.segnum;
  sn_out = pidrl_out->urn.segnum;
  if (sn_in == sn_out && pidrl_in->urn.obnum == pidrl_out->urn.obnum)
    return (-ER_NDR);
  if ((cpn = cont_id (pidind, &dr_in, &di)) != OK)
    return (cpn);
  desf = &dr_in->f_df_bt;
  CMP_REL;
  ai = (u2_t *) (di + 1);
  if ((cpn = testdsc (dr_in, &diasz, diasc, ai, &dscsz)) != OK)
    return (cpn);
  if ((cpn = synlsc (RSC, pidrl_in, sc, slsz, desf->f_fn, NULL)) != OK)
    return (cpn);
  kn = di->ldi.kifn & ~UNIQ & MSK21B;
  if ((cpn = synlsc (RSC, pidrl_in, diasc, diasz, kn, ai)) != OK)
    return (cpn);
  
  scind = (struct d_sc_i *) lusc (&n, scisize, (char *) di, SCI, WSC,
                                  0, NULL, sc, slsz,
				  0, NULL, diasz + size2b);
  disc = &scind->dessc;
  disc->curlpn = (u2_t) ~ 0;
  asp = (char *) scind + scisize + slsz + size2b;
  disc->dpnsc = asp;
  t2bpack (diasz, asp);
  disc->dpnsval = asp + size2b + dscsz;
  bcopy (diasc, asp + size2b, diasz);
  cort = pbuflj + ljmsize;
  outpage.p_shm = NULL;
  if ((rep = ind_ftid (disc, &tid, FASTSCAN)) != EOI)
    {
      oldpn = tid.tpn;
      while ((asp = getpg (&inpage, sn_in, oldpn, 's')) == NULL);
      afi = (u2_t *) (asp + phsize);
    }
  else
    goto m1;
  if (sn_out == NRSNUM)
    {
      outpage.p_shm = outasp = tmp_buff_out;
      read_tmp_page (dtr_out->tobtr.lastpn, outasp);
    }
  for (; rep != EOI; rep = ind_tid (disc, &tid, FASTSCAN))
    {
      INSERTION_BY_TID(tid.tpn,tid.tindex);
      MASS_INSERTION;
    }
  END_MASS_INS;
m1:
  delscan (n);
  return (OK);
}

i4_t
inscfl (i4_t idfl, struct id_rel *pidrl_out, u2_t fln, u2_t * fl,
        u2_t slsz, char *sc)
{
  u2_t sn_out, flpn, off;
  struct fun_desc_fields *desf;
  struct des_tid *tid, *tidb;
  char *aspfl, *cort, *asp = NULL, *outasp = NULL;
  u2_t oldpn, *afi, *ai, corsize, sn_in, freesz = 0;
  struct d_r_t *dr_in, *dr_out;
  struct des_trel *dtr_out = NULL;
  struct des_fltr *desfl;
  struct des_tob *dt = NULL;  
  CPNM cpn;
  struct A inpage, outpage;
  char tmp_buff_out[BD_PAGESIZE], tmp_buff_fl[BD_PAGESIZE];

  if ((u2_t) idfl > desnseg.mtobnum)
    return (-ER_NIOB);
  desfl = (struct des_fltr *) * (desnseg.tobtab + idfl);
  if (desfl == NULL)
    return (-ER_NIOB);    
  if (((struct prtob *) desfl)->prob != FLTR)
    return (-ER_NIOB);
  dr_in = desfl->pdrtf;
  desf = &dr_in->f_df_bt;
  sn_in = dr_in->segnr;
  sn_out = pidrl_out->urn.segnum;
  cort = pbuflj + ljmsize;
  outpage.p_shm = NULL;
  CMP_REL; 
  if (sn_out == NRSNUM)
    {
      outpage.p_shm = outasp = tmp_buff_out;
      read_tmp_page (dtr_out->tobtr.lastpn, outasp);
    }
  aspfl = tmp_buff_fl;
  for (flpn = desfl->tobfl.firstpn; flpn != (u2_t) ~ 0;)
    {
      read_tmp_page (flpn, aspfl);
      off = ((struct p_h_f *) aspfl)->freeoff;
      tid = (struct des_tid *) (aspfl + phfsize);
      oldpn = tid->tpn;
      while ((asp = getpg (&inpage, sn_in, oldpn, 's')) == NULL);
      afi = (u2_t *) (asp + phsize);
      tidb = (struct des_tid *) (aspfl + off);
      for (; tid < tidb; tid++)
        {
          INSERTION_BY_TID(tid->tpn,tid->tindex);
          MASS_INSERTION;
        }
      flpn = ((struct p_h_f *) aspfl)->listfl.nextpn;
    }
  END_MASS_INS;  
  return (OK);
}

