/*
 * aggravg.c  - calculation of average aggregate functions
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

/* $Id: aggravg.c,v 1.254 1998/09/29 22:23:37 kimelman Exp $ */

#include <assert.h>

#include "xmem.h"
#include "destrn.h"
#include "sctp.h"
#include "strml.h"
#include "fdcltrn.h"

extern struct des_nseg desnseg;

static int
agravg (double *avg, i4_t n_avg, u2_t type, char *key, u2_t fn)
{
  char *sc;
  u2_t k, k1;
  double val;

  sc = key;
  key += scscal (key);
  for (k = 0, k1 = 0; k < fn; k1 = 0, sc++)
    for (; k1 < 7 && k < fn; k++, k1++)
      if ((*sc & BITVL(k1)) != 0)
	key = proval (key, type);
  val = retval (key, type);
  
  *avg = ((*avg) * n_avg + val) / (n_avg + 1);
  n_avg += 1;
  return (n_avg);
}

struct ans_avg
avgitab (struct id_ind * pidind, u2_t slsz,
         char *sc, u2_t diasz, char *diasc)
{
  u2_t sn, *ai, kn, type, dscsz;
  struct fun_desc_fields *desf;
  struct ldesscan *disc;
  struct d_sc_i *scind;
  struct ldesind *di;
  struct d_r_t *desrel;
  char *asp = NULL;
  struct id_rel *pidrel;
  struct des_tid tid;
  i4_t rep, n_avg = 0;
  i2_t n;
  double avg = 0;
  struct A pg;
  struct ans_avg ans;

  pidrel = &pidind->irii;
  sn = pidrel->urn.segnum;
  if ((ans.cotavg = cont_id (pidind, &desrel, &di)) != OK)
    return (ans);
  desf = &desrel->f_df_bt;
  if ((ans.cotavg = testcond (desf, 0, NULL, &slsz, sc, 0, NULL)) != OK)
    return (ans);

  ai = (u2_t *) (di + 1);
  if ((ans.cotavg = testdsc (desrel, &diasz, diasc, ai, &dscsz)) != OK)
    return (ans);

  if ((ans.cotavg = synlsc (RSC, pidrel, sc, slsz, desf->f_fn, NULL)) != OK)
    return (ans);
  kn = di->ldi.kifn & ~UNIQ & MSK21B;
  if ((ans.cotavg = synlsc (RSC, pidrel, diasc, diasz, kn, ai)) != OK)
    return (ans);
  scind = (struct d_sc_i *) lusc (&n, scisize, (char *) di, SCI, RSC, 0,
                                  NULL, sc, slsz, 0, NULL, diasz + size2b);
  disc = &scind->dessc;
  disc->pdi = di;
  disc->curlpn = (u2_t) ~ 0;
  asp = (char *) (scind + 1) + slsz + size2b;
  if (diasz == 0)
    disc->dpnsc = NULL;
  else
    disc->dpnsc = asp;
  t2bpack (diasz, asp);
  disc->dpnsval = asp + size2b + dscsz;
  asp += size2b;
  bcopy (diasc, asp, diasz);
  ai = (u2_t *) (disc->pdi + 1);
  type = (desf->df_pnt + *ai)->field_type;
  if (type == TCH || type == TFL)
    {
      ans.cotavg = -ER_NCF;
      return (ans);
    }
  rep = ind_ftid (disc, &tid, SLOWSCAN);
  n_avg = agravg (&avg, n_avg, type, disc->cur_key, 0);
  for (; rep != EOI; rep = ind_tid (disc, &tid, SLOWSCAN))
    {
      while ((asp = getpg (&pg, sn, tid.tpn, 's')) == NULL);
      ai = (u2_t *) (asp + phsize) + tid.tindex;
      if (*ai != 0 &&
              fndslc (desrel, asp + * ai, sc, slsz, NULL) != 0)
	{
	  n_avg = agravg (&avg, n_avg, type, disc->cur_key, 0);
	}
      putpg (&pg, 'n');
    }
  xfree (disc->cur_key);
  delscan (n);
  ans.agr_avg = avg;
  return (ans);
}

struct ans_avg
avgstab (struct id_rel *pidrel)
{
  u2_t pn, *ai, *ali, kn, type;
  struct des_trel *destrel;
  char *asp, tmp_buff[BD_PAGESIZE];
  u2_t ntob;
  i4_t n_avg = 0;
  double avg = 0;
  struct ans_avg ans;

  if (pidrel->urn.segnum != NRSNUM)
    ans.cotavg = -ER_NIOB;
  ntob = pidrel->urn.obnum;
  if (ntob > desnseg.mtobnum)
    ans.cotavg = -ER_NIOB;
  destrel = (struct des_trel *) * (desnseg.tobtab + ntob);
  if (destrel == NULL)
    ans.cotavg = -ER_NIOB;
  if (((struct prtob *) destrel)->prob != TREL)
    ans.cotavg = -ER_NIOB;
  if (((struct prtob *) destrel)->prsort != SORT)
    ans.cotavg = -ER_N_SORT;
  kn = *(u2_t *) ((char *) destrel + destrel->f_df_tt.f_fn * rfsize);
  type = (destrel->f_df_tt.df_pnt + kn)->field_type;
  if (type == TCH || type == TFL)
    {
      ans.cotavg = -ER_NCF;
      return (ans);
    }
  asp = tmp_buff;
  for (pn = destrel->tobtr.firstpn; pn != (u2_t) ~ 0;)
    {
      read_tmp_page (pn, asp);
      ai = (u2_t *) (asp + phtrsize);
      ali = ai + ((struct p_h_tr *) asp)->linptr;
      for (; ai <= ali; ai++)
	if (*ai != 0)
	  n_avg = agravg (&avg, n_avg, type, asp + *ai + 1, kn);
      pn = ((struct listtob *) asp)->nextpn;
    }
  ans.agr_avg = avg;
  return (ans);
}

i4_t
fndslc (struct d_r_t *desrel, char *tuple,
        char *selcon, u2_t slsz, char *cort)
{
  i4_t tuple_size;
  unsigned char t;
  struct A inpage;
  char *arrpnt[BD_PAGESIZE];
  u2_t arrsz[BD_PAGESIZE];

  t = *tuple & MSKCORT;
  if (t == CREM || t == IDTR)
    return (0);
  if (t == IND)
    {
      u2_t pn, ind;
      IND_REF(inpage,desrel->segnr,tuple);
    }
  tuple_size = tstcsel (&desrel->f_df_bt, slsz, selcon, tuple, arrpnt, arrsz);
  if (tuple_size != 0 && cort != NULL)
    bcopy (tuple, cort, tuple_size);
  if (t == IND)
    putpg (&inpage, 'n');
  return (tuple_size);
}

#define	BETWEEN_CMP  (t == SS || t == SES || t == SSE || t == SESE)

static int
fcv (i4_t t, u2_t field_type, char **sel_vals, char *tuple_value)
{
  i4_t v, v1, res;

  if (t == ANY || t == NEQUN)
    return (OK);
  
  v = cmpval (tuple_value, *sel_vals, field_type);
  *sel_vals = proval (*sel_vals, field_type);
  if (v == 0) /* current == first */
    res = (t == EQ || t == SES || t == SESE ||
	   t == SMLEQ || t == GRTEQ) ? OK : -ER_NCR;
  else if (v < 0) /* current < first */
    res = (t == SML || t == SMLEQ || t == NEQ) ? OK : -ER_NCR;
  else /* current > first */
    {
      if (BETWEEN_CMP)
        {
          v1 = cmpval (tuple_value, *sel_vals, field_type);
          *sel_vals = proval (*sel_vals, field_type);
          if (v1 == 0) /* current == last */
            res = (t == SSE || t == SESE) ? OK : -ER_NCR;
          else 
            res = (v1 > 0) ? -ER_NCR : OK;
        }
      else
        res = (t == NEQ || t == GRT || t == GRTEQ) ? OK : -ER_NCR;
    }
  return res;
}

#undef	BETWEEN_CMP

u2_t
tstcsel (struct fun_desc_fields *desf, u2_t slsz, char *selc,
         char *tuple, char **arrpnt, u2_t *arrsz)
{
  unsigned char t;
  i4_t sst;
  u2_t n = 0, f_n;
  struct des_field *df;
  char *sel_vals, *pnt;
  int tuple_size;

  tuple_size = tuple_break (tuple, arrpnt, arrsz, desf);
  if (slsz == 0)
    return (tuple_size);
  sel_vals = selc;
  for (sst = 1; (t = selsc1 (&sel_vals, sst++)) != ENDSC;);
  if (sst % 2 == 0)
    sel_vals++;
  df = desf->df_pnt;
  f_n = desf->f_fn;
  for (sst = 1; (t = selsc1 (&selc, sst++)) != ENDSC && n < f_n; n++)
    {
      if ((pnt = arrpnt[n]) == NULL) /* NULL VALUE */
        {
          if(t != EQUN && t != ANY)
            return (0);
        }
      else if (fcv (t, (df + n)->field_type, &sel_vals, pnt) != OK)
          return (0);
    }
  return (tuple_size);
}
