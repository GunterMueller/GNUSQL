/*
 *  mdfctn.c  - Modification 
 *              Kernel of GNU SQL-server 
 *
 *  $Id: mdfctn.c,v 1.252 1998/09/29 21:25:36 kimelman Exp $
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

#include <assert.h>

#include "destrn.h"
#include "sctp.h"
#include "strml.h"
#include "fdcltrn.h"
#include "xmem.h"

extern char **scptab;
extern i2_t maxscan;
extern char *pbuflj;
extern i4_t ljrsize;
extern i4_t ljmsize;
extern struct ADBL adlj;

int
check_cmod (struct fun_desc_fields *desf, u2_t fmnum,
          u2_t *mfn, Colval colval, u2_t *lenval)
{
  u2_t i, fn, fdf;
  struct des_field *df;

  fdf = desf->f_fdf;
  df = desf->df_pnt;
  for (i = 0; i < fmnum; i++)
    {
      fn = mfn[i];
      if (fn < fdf && colval[i] == NULL)
	return (-ER_NCF);
      if(lenval[i] > (df + fn)->field_size)
        return (-ER_NCF);
    }
  return (OK);
}

static int
mod_tt_tuple(struct d_mesc *scpr, u2_t fmnum, u2_t *mfn, Colval colval, u2_t *lenval)
{
  struct des_trel *destrel;
  struct fun_desc_fields *desf;
  struct des_tid *tid;
  u2_t pn, newsize, *afi, *ai;
  i2_t delta;
  char *asp, *a;
  char tmp_buff[BD_PAGESIZE];
  
  destrel = (struct des_trel *) scpr->pobsc;
  desf = &destrel->f_df_tt;
  if (check_cmod (desf, fmnum, mfn, colval, lenval) != OK)
    return (-ER_NCF);
  tid = &((struct d_sc_r *) scpr)->curtid;
  pn = tid->tpn;
  asp = tmp_buff;
  read_tmp_page (pn, asp);
  afi = (u2_t *) (asp + phtrsize);
  ai = afi + tid->tindex;
  if (*ai == 0)
    {
      scpr->prcrt = 0;
      return (-ER_NCR);
    }
  assert (*ai <= BD_PAGESIZE);
  a = asp + *ai;
  newsize = cortform (desf, colval, lenval, a, pbuflj, fmnum, mfn);
  delta = calsc (afi, ai) - newsize;
  if (delta > 0)
    comptr (asp, ai, delta);
  if (delta >= 0)
    {
      bcopy (pbuflj, a, newsize);
      write_tmp_page (pn, asp);
    }
  else
    {
      deltr (asp, ai, &(destrel->tobtr), pn);
      scpr->prcrt = 0;
      write_tmp_page (pn, asp);
      instr (&(destrel->tobtr), pbuflj, newsize);
    }
  return (OK);
}

static int
mod_cort (struct d_mesc *s, u2_t fmnum, u2_t *mfn, Colval colval, u2_t *lenval,
          struct d_r_t *desrel, struct des_tid *tid)
{  
  char *cort, *nc, *selc;
  u2_t newsize, n, ni, ndc, corsize, fn, sn;
  struct fun_desc_fields *desf;
  CPNM cpn;
  struct id_rel idr;
  struct ADBL last_adlj;
  struct des_tid ref_tid;
  struct full_des_tuple dtuple;

  desf = &desrel->f_df_bt;
  if (check_cmod (desf, fmnum, mfn, colval, lenval) != OK)
    return (-ER_NCF);
  sn = desrel->segnr;
  cort = pbuflj + ljmsize;
  if ((corsize = readcort (sn, tid, cort, &ref_tid)) == 0)
    {
      s->prcrt = 0;
      return (-ER_NCR);
    }
  fn = desrel->desrbd.fieldnum;
  ndc = s->ndc;
  idr.urn.segnum = dtuple.sn_fdt = sn;
  idr.urn.obnum = dtuple.rn_fdt = desrel->desrbd.relnum;
  idr.pagenum = desrel->pn_r;
  idr.index = desrel->ind_r;
  if (ndc < MAXCL)
    {
      if ((cpn = synlock (desrel, cort)) != 0)
	return (cpn);
      s->ndc++;
    }
  else if (ndc == MAXCL)
    {
      selc = s->pslc;
      if ((cpn = synlsc (s->modesc, &idr, selc + size2b,
                         *(u2_t *) selc, fn, NULL)) != 0)
	return (cpn);
      s->ndc++;
    }
  nc = cort + corsize;
  newsize = cortform (desf, colval, lenval, cort, nc, fmnum, mfn);
  if ((cpn = synlock (desrel, nc)) != 0)
    return (cpn);
  modmes ();
  last_adlj = adlj;
  wmlj (MODLJ, ljmsize + corsize + newsize, &adlj, &idr, tid, 0);
  if (newsize <size4b)
    newsize = size4b;
  dtuple.tid_fdt = *tid;
  ordmod (&dtuple, &ref_tid, corsize, nc, newsize);
  n = desrel->desrbd.indnum;
  ni = mproind (desrel, n, cort, nc, tid);
  if (ni < n)
    {
      wmlj (RLBLJ, ljrsize, &last_adlj, &idr, tid, 0);
      mproind (desrel, ni + 1, nc, cort, tid);
      ordmod (&dtuple, &ref_tid, newsize, cort, corsize);
      BUF_endop ();
      return (-ER_NU);
    }
  BUF_endop ();
  return (OK);
}

static int
modcort (struct d_mesc *s, u2_t scsz, struct d_r_t *desrel,
         struct des_tid *tid, Colval colval, u2_t *lenval)
{
  u2_t *mfn;
  
  mfn = (u2_t *) ((char *) s + scsz + s->fnsc * size2b);
  return (mod_cort( s, s->fnsc, mfn, colval, lenval, desrel, tid));
}

int
mdfctn (i4_t scnum, Colval colval, u2_t *lenval)
{
  struct ldesscan *desscan;
  struct d_mesc *scpr;
  i4_t sctype;
  
  scpr = (struct d_mesc *) * (scptab + scnum);
  if (scnum >= maxscan || scpr == NULL)
    return (-ER_NDSC);
  if (scpr->modesc != WSC && scpr->modesc != MSC)
    return (-ER_NMS);
  if (scpr->prcrt == 0)
    return (-ER_NCR);
  if ((sctype = scpr->obsc) == SCR)
    {				/* relation scan */
      desscan = &((struct d_sc_i *) scpr)->dessc;
      return (modcort (scpr, scisize, (struct d_r_t *) scpr->pobsc,
                       &desscan->ctidi, colval, lenval));
    }
  else if (sctype == SCTR)
    {
      u2_t *mfn;
      mfn = (u2_t *) ((char *) scpr + scrsize + scpr->fnsc * size2b);
      return (mod_tt_tuple (scpr, scpr->fmnsc, mfn, colval, lenval));
    }
  else if (sctype == SCI)
    {				/* index scan */
      desscan = &((struct d_sc_i *) scpr)->dessc;
      return (modcort (scpr, scisize, desscan->pdi->dri,
                       &desscan->ctidi, colval, lenval));
    }
  else
    {				/* filter scan */
      struct d_sc_f *scfltr;
      struct des_tid *tid;
      char tmp_buff[BD_PAGESIZE];
      
      scfltr = (struct d_sc_f *) scpr;
      read_tmp_page (scfltr->pnf, tmp_buff);
      tid = (struct des_tid *) (tmp_buff + scfltr->offf);
      return (modcort (scpr, scfsize, ((struct des_fltr *) scpr->pobsc)->pdrtf,
                       tid, colval, lenval));
    }
}

int
testcmod (struct fun_desc_fields *desf, u2_t fmn,
          u2_t *mfn, char *fml, char **fval)
{
  u2_t i, fn, type, fdf;
  struct des_field *df;
  unsigned char t;
  i4_t sst;
  char *a, *sc;

  sst = 1;
  a = fml;
  fdf = desf->f_fdf;
  for (i = 0; ((t = selsc1 (&a, sst++)) != ENDSC) && (i < fmn); i++)
    {
      fn = mfn[i];
      if (t != EQ && t != NEQ && t != EQUN)
	return (-ER_NCF);
      if (fn < fdf && t == EQUN)
	return (-ER_NCF);
    }
  if (t != ENDSC)
    return (-ER_NCF);
  if (sst % 2 == 0)
    a++;
  *fval = a;
  sst = 1;
  sc = fml;
  df = desf->df_pnt;
  for (i = 0; (t = selsc1 (&sc, sst++)) != ENDSC; i++)
    {
      fn = mfn[i];
      if(t2bunpack(a)>(df+fn)->field_size) return (-ER_NCF);
      type= (df+fn)->field_type;
      if(type== T1B || type==T2B || type==T4B || type== TFLOAT)
	a+=size2b;
      a=proval(a,type);
    }  
  return (OK);
}

int
mod_spec_flds (i4_t scnum, u2_t fmnum, u2_t *fmn,
               Colval colval, u2_t *lenval)
{
  struct ldesscan *desscan;
  struct d_mesc *scpr;
  i4_t sctype;

  scpr = (struct d_mesc *) * (scptab + scnum);
  if (scnum >= maxscan || scpr == NULL)
    return (-ER_NDSC);
  if (scpr->modesc != WSC && scpr->modesc != MSC)
    return (-ER_NMS);
  if (scpr->prcrt == 0)
    return (-ER_NCR);
  if ((sctype = scpr->obsc) == SCR)
    {				/* relation scan */
      desscan = &((struct d_sc_i *) scpr)->dessc;
      return (mod_cort (scpr, fmnum, fmn, colval, lenval,
                        (struct d_r_t *) scpr->pobsc, &desscan->ctidi));
    }
  else if (sctype == SCTR)
    {
      return (mod_tt_tuple (scpr, fmnum, fmn, colval, lenval));
    }
  else if (sctype == SCI)
    {				/* index scan */
      desscan = &((struct d_sc_i *) scpr)->dessc;
      return (mod_cort (scpr, fmnum, fmn, colval, lenval, desscan->pdi->dri,
                        &desscan->ctidi));
    }
  else
    {				/* filter scan */
      struct d_sc_f *scfltr;
      struct des_tid *tid;
      char tmp_buff[BD_PAGESIZE];
      
      scfltr = (struct d_sc_f *) scpr;
      read_tmp_page (scfltr->pnf, tmp_buff);
      tid = (struct des_tid *) (tmp_buff + scfltr->offf);
      return (mod_cort (scpr, fmnum, fmn, colval, lenval,
                        ((struct des_fltr *) scpr->pobsc)->pdrtf, tid));
    }
}
