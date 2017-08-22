/*
 * aggrifn.c  - calculation some aggregate functions
 *               by specific table index
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

/* $Id: aggrifn.c,v 1.254 1998/05/20 05:52:42 kml Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "agrflg.h"
#include "strml.h"
#include "fdcltrn.h"
#include "cmpdecl.h"

extern struct des_nseg desnseg;

static void
agrmin (char *val1, char *val2, u2_t type, u2_t * n)
{
  char *a;
  u2_t n2;

  a = val2;
  val2 += scscal (val2);
  if ((*a & BITVL(0)) != 0)
    {
      if (*val1 == 0)
	{
	  *val1++ = 1;
	  n2 = get_length (val2, type);
          bcopy (val2, val1, n2);
	  *n = n2;
	}
      else
	{
	  val1++;
	  if (cmpval (val1, val2, type) > 0)
	    {				/* val2<val1 */
              n2 = get_length (val2, type);
              bcopy (val2, val1, n2);
	      *n = n2;
	    }
	}
    }
}

static u2_t
fkv_frm (char *buf, char *val, u2_t type)
{
  u2_t n;
  
  n = get_length (val, type);
  bcopy (val, buf, n);
  return (n);
}

static u2_t
fkvfrm (char *buf, char *val, u2_t type)
{
  if ((*val & BITVL(0)) == 0)
    {
      *buf = 0;
      return (1);
    }
  *buf++ = 1;
  val += scscal (val);
  return (fkv_frm (buf, val, type) + 1);
}

void
minitab (struct ans_next *ans, struct id_ind *pidind, u2_t slsz,
         char *sc, u2_t diasz, char *diasc)
{
  u2_t sn, pn, oldpn, *ai, *afi = NULL, type, *afn, kn, dscsz;
  struct fun_desc_fields *desf;
  struct ldesscan *disc;
  struct d_sc_i *scind;
  struct ldesind *di;
  struct d_r_t *desrel;
  char *value, *asp = NULL;
  struct id_rel *pidrel;
  struct des_tid tid;
  i4_t rep;
  i2_t n;
  struct A pg;

  pidrel = &pidind->irii;
  sn = pidrel->urn.segnum;
  if ((ans->cotnxt = cont_id (pidind, &desrel, &di)) != OK)
    return;
  desf = &desrel->f_df_bt;
  if ((ans->cotnxt = testcond (desf, 0, NULL, &slsz, sc, 0, NULL)) != OK)
    return;

  afn = (u2_t *) (di + 1);
  if ((ans->cotnxt = testdsc (desrel, &diasz, diasc, afn, &dscsz)) != OK)
    return;

  if ((ans->cotnxt = synlsc (RSC, pidrel, sc, slsz, desf->f_fn, NULL)) != OK)
    return;
  kn = di->ldi.kifn & ~UNIQ & MSK21B;
  if ((ans->cotnxt = synlsc (RSC, pidrel, diasc, diasz, kn, afn)) != OK)
    return;
  scind = (struct d_sc_i *) lusc (&n, scisize, (char *) di, SCI, RSC,
                                  0, NULL, sc, slsz,
				  0, NULL, diasz + size2b);
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
  bcopy (diasc, asp + size2b, diasz);
  type = (desf->df_pnt + *afn)->field_type;
  rep = ind_ftid (disc, &tid, FASTSCAN);
  value = ans->cadnxt;
  ans->csznxt = fkvfrm (value, disc->cur_key, type);
  oldpn = (u2_t) ~ 0;
  for (; rep != EOI; rep = ind_tid (disc, &tid, FASTSCAN))
    {
      pn = tid.tpn;
      if (pn != oldpn)
	{
	  if (oldpn != (u2_t) ~ 0)
	    putpg (&pg, 'n');
	  while ((asp = getpg (&pg, sn, pn, 's')) == NULL);
	  oldpn = pn;
	  afi = (u2_t *) (asp + phsize);
	}
      ai = afi + tid.tindex;
      if (*ai != 0 && fndslc (desrel, asp + *ai, sc, slsz, NULL) != 0)
	agrmin (value, disc->cur_key, type, &ans->csznxt);
    }
  putpg (&pg, 'n');
  xfree (disc->cur_key);
  delscan (n);
}

static void
agrmax (char *val1, char *val2, u2_t type, u2_t * n)
{
  char *a;
  u2_t n2;

  a = val2;
  val2 += scscal (val2);
  if ((*a & BITVL(0)) != 0)
    {
      if (*val1 == 0)
	{
	  *val1++ = 1;
	  n2 = get_length (val2, type);
          bcopy (val2, val1, n2);
	  *n = n2;
	}
      else
	{
	  val1++; 
	  if (cmpval (val1, val2, type) < 0)
	    {				/* val2>val1 */
              n2 = get_length (val2, type);
              bcopy (val2, val1, n2);
	      *n = n2;
	    }
	}
    }
}

void
maxitab(struct ans_next *ans, struct id_ind *pidind,
        u2_t slsz, char *sc, u2_t diasz, char *diasc)
{
  u2_t sn, pn, oldpn, *ai, *afi = NULL, type, *afn, kn, dscsz;
  struct fun_desc_fields *desf;
  struct ldesscan *disc;
  struct d_sc_i *scind;
  struct ldesind *di;
  struct d_r_t *desrel;
  char *a, *asp = NULL;
  struct id_rel *pidrel;
  struct des_tid tid;
  i4_t rep;
  i2_t n;
  struct A pg;

  pidrel = &pidind->irii;
  sn = pidrel->urn.segnum;
  if ((ans->cotnxt = cont_id (pidind, &desrel, &di)) != OK)
    return;
  desf = &desrel->f_df_bt; 
  if ((ans->cotnxt = testcond (desf, 0, NULL, &slsz, sc, 0, NULL)) != OK)
    return;
  afn = (u2_t *) (di + 1);
  if ((ans->cotnxt = testdsc (desrel, &diasz, diasc, afn, &dscsz)) != OK)
    return;

  if ((ans->cotnxt = synlsc (RSC, pidrel, sc, slsz, desf->f_fn, NULL)) != OK)
    return;
  kn = di->ldi.kifn & ~UNIQ & MSK21B;
  if ((ans->cotnxt = synlsc (RSC, pidrel, diasc, diasz, kn, afn)) != OK)
    return;
  scind = (struct d_sc_i *) lusc (&n, scisize, (char *) di, SCI, RSC,
                                  0, NULL, sc, slsz,
				  0, NULL, diasz + size2b);
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
  bcopy (diasc, asp + size2b, diasz);
  type = (desf->df_pnt + *afn)->field_type;
  rep = ind_ftid (disc, &tid, FASTSCAN);
  a = ans->cadnxt;
  ans->csznxt = fkvfrm (a, disc->cur_key, type);
  oldpn = (u2_t) ~ 0;
  for (; rep != EOI; rep = ind_tid (disc, &tid, FASTSCAN))
    {
      pn = tid.tpn;
      if (pn != oldpn)
	{
	  if (oldpn != (u2_t) ~ 0)
	    putpg (&pg, 'n');
	  while ((asp = getpg (&pg, sn, pn, 's')) == NULL);
	  oldpn = pn;
          afi = (u2_t *) (asp + phsize);
	}
      ai = afi + tid.tindex;
      if (*ai != 0 && fndslc (desrel, asp + *ai, sc, slsz, NULL) != 0)
	agrmax (a, disc->cur_key, type, &ans->csznxt);
    }
  putpg (&pg, 'n');
  xfree (disc->cur_key);
  delscan (n);
  return;
}

i4_t
agrfind (data_unit_t **colval, struct id_ind *pidind, u2_t nf,
         u2_t * mnf, u2_t slsz, char *sc, u2_t diasz,
         char *diasc, char *flaglist)
{
  u2_t i, pn, oldpn, sn;
  struct fun_desc_fields *desf;
  struct des_field *df;
  struct ldesscan *disc;
  struct d_sc_i *scind;
  struct ldesind *di;
  struct d_r_t *desrel;
  char **agrl, *asp = NULL;
  struct id_rel *pidrel;
  struct des_tid tid;
  struct A pg;
  i4_t rep, ans;
  i2_t n;
  u2_t *ai, *afi = NULL, *afn, kn, dscsz;
  char tuple[BD_PAGESIZE];
  

  pidrel = &pidind->irii;
  sn = pidrel->urn.segnum;
  if ((ans = cont_id (pidind, &desrel, &di)) != OK)
    return ans;
  desf = &desrel->f_df_bt;
  if ((ans = testcond (desf, 0, NULL, &slsz, sc, 0, NULL)) != OK)
    return ans;
  afn = (u2_t *) (di + 1);
  if ((ans = testdsc (desrel, &diasz, diasc, afn, &dscsz)) != OK)
    return ans;

  if ((ans = synlsc (RSC, pidrel, sc, slsz, desf->f_fn, NULL)) != OK)
    return ans;
  kn = di->ldi.kifn & ~UNIQ & MSK21B;
  if ((ans = synlsc (RSC, pidrel, diasc, diasz, kn, afn)) != OK)
    return ans;
  scind = (struct d_sc_i *) lusc (&n, scisize, (char *) di, SCI, RSC,
                                  0, NULL, sc, slsz,
				  0, NULL, diasz + size2b);
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
  bcopy (diasc, asp + size2b, diasz);
  rep = ind_ftid (disc, &tid, FASTSCAN);
  if (rep == EOI)
    i = 0;
  else
    i = 1;
  agrl = (char **) xmalloc (nf * sizeof (char *));
  df = desf->df_pnt;
  agrl_frm (agrl, df, (u2_t) nf, mnf, flaglist);
  oldpn = (u2_t) ~ 0;
  for (; rep != EOI; rep = ind_tid (disc, &tid, FASTSCAN))
    {
      pn = tid.tpn;
      if (pn != oldpn)
	{
	  if (oldpn != (u2_t) ~ 0)
	    putpg (&pg, 'n');
	  while((asp = getpg (&pg, sn, pn, 's')) == NULL);
	  oldpn = pn;
	  afi = (u2_t *) (asp + phsize);
	}
      ai = afi + tid.tindex;
      if (*ai != 0 &&
          fndslc (desrel, asp + *ai, sc, slsz, tuple) != 0)
	agrcount (agrl, tuple, desf, nf, mnf, flaglist);
    }
  if (i == 1)
    {
      putpg (&pg, 'n');
      xfree (disc->cur_key);
      distagr_frm (agrl, nf, flaglist);
    }
  write_aggr_val (colval, agrl, df, nf, mnf, flaglist);
  for (i = 0; i < nf; i++)
    xfree (agrl[i]);
  xfree ((char *) agrl);
  delscan (n);
  return OK;
}

#if 0

/*
void
ffrm_agrcount (char **agrl, char *tuple, struct fun_desc_fields *desf,
               u2_t nf, u2_t * mnf, char *flaglist)
{
  char *val;
  u2_t kn, fn;
  char *arrpnt[BD_PAGESIZE];
  u2_t arrsz[BD_PAGESIZE];

  tuple_break (tuple, arrpnt, arrsz, desf);
  df = desf->df_pnt;
  for (kn = 0; kn < nf; kn++)
    {
      fn = mnf[kn];
      if ((val = arrpnt[fn]) != NULL)
	agr_ffrm (agrl[kn], flaglist[kn], val, (df + fn)->field_type);
    }
}
*/

void
agr_ffrm (char *agrl, char flag, char *val, u2_t type)
{
  char *a, *b, *c;
  i4_t count;
  float avg;

  switch (flag)
    {
    case FN_COUNT:
      count = 1;
      t4bpack (count, agrl);
      break;
    case FN_AVG:
      count = 1;
      avg = retval (val, type);
      bcopy ((char *) &avg, agrl, sizeof (float));
      t4bpack (count, agrl + sizeof (float));
      break;
    case FN_MAX:
    case FN_MIN:
    case FN_SUMM:
      fkv_frm (agrl, val, type);
      break;
    case FN_DT_COUNT:
    case FN_DT_AVG:
    case FN_DT_SUMM:
      val = agr_dt_cfrm (val, type, agrl);
      break;
    default:
      break;
    }
}


#endif

float
retval (char *val, u2_t type)
{
  float flval;

  switch (type)
    {
    case T1B:
      flval = *val;
      break;
    case T2B:
      flval = t2bunpack (val);
      break;
    case TFLOAT:
      bcopy (val, (char *) &flval, size4b);
      break;
    case T4B:
      flval = t4bunpack (val);
      break;
    default:
      error ("TRN.retval: This data type is incorrect");
      break;
    }
  return (flval);
}

void
agrcount (char **agrl, char *tuple, struct fun_desc_fields *desf,
          u2_t nf, u2_t * mnf, char *flaglist)
{
  u2_t kn = 0, fn;
  struct des_field *df;
  char *arrpnt[BD_PAGESIZE];
  u2_t arrsz[BD_PAGESIZE];

  tuple_break (tuple, arrpnt, arrsz, desf);
  df = desf->df_pnt;
  for (; kn < nf; kn++)
    {
      fn = mnf[kn];
      agr_frm (agrl[kn], flaglist[kn], arrpnt[fn], (df + fn)->field_type);
    }
}

static char *
agr_dt_cfrm (char *val, u2_t type, char *agrl)
{
  char *newt, mch[BD_PAGESIZE], tmp_buff[BD_PAGESIZE];
  struct des_tob *dt;
  i2_t n;
  u2_t corsize;

  newt = mch;
  *newt = 0;
  *newt |= EOSC;
  if ( val != NULL)
    {
      *newt |= BITVL(0);
      newt++;
      val = remval (val, &newt, type);
    }
  else
    newt++;
  corsize = newt - mch;
  n = t2bunpack (agrl);
  dt = (struct des_tob *) * (desnseg.tobtab + n);
  read_tmp_page (dt->lastpn, tmp_buff);
  minstr (tmp_buff, mch, corsize, dt);
  write_tmp_page (dt->lastpn, tmp_buff);
  return (val);
}

static void
sumfld (char *buf, char *val, u2_t type)
{
  u2_t val2, sum2;
  i4_t val4, sum4;
  float fval, sumf;

  switch (type)
    {
    case T1B:
      *buf += *val;
      break;
    case T2B:
      val2 = t2bunpack (val);
      sum2 = t2bunpack (buf) + val2;
      t2bpack (sum2, buf);
      break;
    case T4B:
      val4 = t4bunpack (val);
      sum4 = t4bunpack (buf) + val4;
      t4bpack (sum4, buf);
      break;
    case TFLOAT:
      bcopy (buf, (char *) &sumf, size4b);
      bcopy (val, (char *) &fval, size4b);
      sumf += fval;
      bcopy ((char *) &sumf, buf, size4b);
      break;
    default:
      error ("TRN.sumfld: This data type is incorrect");
      break;
    }
}
static i4_t
avgfld (float *avg, i4_t n_avg, u2_t type, char *fval)
{
  float val;

  val = retval (fval, type);
  *avg = ((*avg) * n_avg + val) / (n_avg + 1);
  n_avg += 1;
  return (n_avg);
}

void
agr_frm (char *agrl, i4_t flag, char *val, u2_t type)
{
  char *b;
  u2_t n;
  float avg;
  i4_t count;

  switch (flag)
    {
    case FN_COUNT:
      if (val != NULL)
	{
	  agrl++;
	  count = t4bunpack (agrl);
	  count += 1;
	  t4bpack (count, agrl);
	}
      break;
    case FN_SUMM:
      if (val != NULL)
	{
	  *agrl++ = 1;      
	  sumfld (agrl, val, type);
	}
      break;      
    case FN_MAX:
      if (val != NULL)
	{
	  if (*agrl == 0)
	    {
	      *agrl++ = 1;
	      n = get_length (val, type);
              bcopy (val, agrl, n);
	    }
	  else
	    {
	      agrl++;
	      if (cmpval (agrl, val, type) < 0)	/* val2>val1 */
                {
                  n = get_length (val, type);
                  bcopy (val, agrl, n);
                }
	    }
	}
      break;
    case FN_MIN:
      if (val != NULL)
	{
	  if (*agrl == 0)
	    {
	      *agrl++ = 1;
	      n = get_length (val, type);
              bcopy (val, agrl, n);
	    }
	  else
	    {
	      agrl++;      
	      if (cmpval (agrl, val, type) > 0)	/* val2<val1 */
                {
                  n = get_length (val, type);
                  bcopy (val, agrl, n);
                }
	    }
	}
      break;
    case FN_AVG:
      if (val != NULL)
	{
	  *agrl++ = 1;
          bcopy (agrl, (char *) &avg, sizeof (float));
          b = agrl + sizeof (float);
	  count = t4bunpack (b);
	  count = avgfld (&avg, count, type, val);
          bcopy ((char *) &avg, agrl, sizeof (float));
	  t4bpack (count, b);
	}
      break;      
    case FN_DT_COUNT:
    case FN_DT_AVG:
    case FN_DT_SUMM:
      val = agr_dt_cfrm (val, type, agrl);
      break;
    default:
      break;
    }
}

i4_t
write_val (char *mas, char **agrl, struct des_field *df,
           u2_t nf, u2_t * mnf, char *flaglist)
{
  char *b, *sc, *pnt;
  u2_t k, type;
  u2_t size;

  sc = mas;
  pnt = sc + nf;
  for (k = 0; k < nf; k++)
    {
      b = agrl[k];      
      switch (flaglist[k])
	{
	case FN_DT_COUNT:
          b += size2b; /* skip tmptable id */
	case FN_COUNT:
	  *sc++ = *b++;
	  t2bpack (size4b, pnt);
	  pnt += size2b;
          bcopy (b, pnt, size4b);
          pnt += size4b;
	  break;
	case FN_DT_AVG:
          b += size2b; /* skip tmptable id */
	case FN_AVG:
	  *sc++ = *b++;
	  t2bpack (size4b, pnt);
	  pnt += size2b;	  
          pnt = write_average (TFLOAT, b, pnt);
	  break;
	case FN_DT_SUMM:
          b += size2b; /* skip tmptable id */
	case FN_MAX:
	case FN_MIN:
	case FN_SUMM:
	  *sc++ = *b++;
	  type = (df + mnf[k])->field_type;
	  if (type == TCH || type == TFL)
	    {
	      size = t2bunpack (b);
	      b += size2b;
	    }
	  else
	    size = (df + mnf[k])->field_size;
	  t2bpack (size, pnt);
	  pnt += size2b;
          bcopy (b, pnt, size);
          pnt += size;
	  break;
	default:
	  break;
	}
    }
  return (pnt - mas);
}

void
write_aggr_val (data_unit_t **colval, char **agrl, struct des_field *df,
                u2_t nf, u2_t * mnf, char *flaglist)
{
  char *b, nl_fl;
  u2_t k, type, size;
  data_unit_t *data_unit;
  float avrg;

  for (k = 0; k < nf; k++)
    {
      b = agrl[k];
      data_unit = colval[k];
      switch (flaglist[k])
	{
	case FN_DT_COUNT:
          b += size2b; /* skip tmptable id */
	case FN_COUNT:
	  nl_fl = *b++;
          mem_to_DU (nl_fl, data_unit->type.code, size4b, b, data_unit);
	  break;
	case FN_DT_AVG:
          b += size2b; /* skip tmptable id */
	case FN_AVG:
          nl_fl = *b++;
          bcopy (b, (char *) &avrg, sizeof (float));
          mem_to_DU (nl_fl, data_unit->type.code, size4b, (char *) &avrg, data_unit);
	  break;
	case FN_DT_SUMM:
          b += size2b; /* skip tmptable id */
	case FN_MAX:
	case FN_MIN:
	case FN_SUMM:
          nl_fl = *b++;
	  type = (df + mnf[k])->field_type;
	  if (type == TCH || type == TFL)
	    {
	      size = t2bunpack (b);
	      b += size2b;
	    }
	  else
	    size = (df + mnf[k])->field_size;
          mem_to_DU (nl_fl, data_unit->type.code, size, b, data_unit);
	  break;
	default:
	  break;
	}
    }
}

char *
write_average (u2_t type, char *pnt_from, char *pnt_to)
{
  u2_t avrg2;
  i4_t avrg4;
  float avrg;

  bcopy (pnt_from, (char *) &avrg, sizeof (float));
  if (type == T1B)
    *pnt_to++ = (i1_t) avrg;
  else if (type == T2B)
    {
      avrg2 = (u2_t) avrg;
      t2bpack (avrg2, pnt_to);
      pnt_to += size2b;
    }
  else if (type == T4B)
    {
      avrg4 = (i4_t) avrg;
      t4bpack (avrg4, pnt_to);
      pnt_to += size4b;
    }
  else if (type == TFLOAT)
    {
      bcopy ((char *) &avrg, pnt_to, sizeof (float));
      pnt_to += sizeof (float);
    }
  return (pnt_to);
}
