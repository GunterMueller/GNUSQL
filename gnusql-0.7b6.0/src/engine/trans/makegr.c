/*
 *  makegr.c  - Make a group
 *              Kernel of GNU SQL-server
 *
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

/* $Id: makegr.c,v 1.252 1997/10/22 16:23:38 kml Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "agrflg.h"
#include "strml.h"
#include "fdcltrn.h"

extern struct des_nseg desnseg;

static u2_t
fgr_val (char **agrl, char *tuple, char *cort, struct des_trel *destrel,
         u2_t ng, u2_t * glist, u2_t nf, u2_t * mnf, char *flaglist)
{
  char *gval;
  u2_t k, fn, kg, sz, kagr;
  struct des_field *df;
  char *arrpnt[BD_PAGESIZE];
  u2_t arrsz[BD_PAGESIZE];

  tuple_break (cort, arrpnt, arrsz, &destrel->f_df_tt);
  gval = tuple;
  *gval++ = CORT;
  for (k = 0, kg = 0, *gval = 0; kg < ng; kg++)
    {
      if (arrpnt[glist[kg]] != NULL)	/* a value is present */
	*gval |= BITVL(k);
      k++;
      if (k == 7)
	{
	  k = 0;
	  gval++;
	  *gval = 0;
	}
    }
  for (kagr = 0; kagr < nf; kagr++)
    if (k == 7)
      {
        k = 0;
        gval++;
        *gval = 0;
      }
  if (k == 0)
    gval--;
  *gval++ |= EOSC;
  for (kg = 0; kg < ng; kg++)
    {
      fn = glist[kg];
      if ((sz = arrsz[fn]) != 0)
        {
          bcopy (arrpnt[fn], gval, sz);
          gval += sz;
        }
    }
  sz = gval - tuple;
  df = destrel->f_df_tt.df_pnt;
  for (kagr = 0; kagr < nf; kagr++)
    {
      fn = mnf[kagr];
      agr_frm (agrl[kagr], flaglist[kagr], arrpnt[fn], (df + fn)->field_type);
    }
  return (sz);
}

static void
clean_ttab(u2_t ntab)
{     
  char *asp;
  struct des_trel *destrel;
  u2_t pn, *b;
  struct listtob *l;
  char tmp_buff[BD_PAGESIZE];
    
  destrel = (struct des_trel *) * (desnseg.tobtab + ntab);
  pn = destrel->tobtr.firstpn;	
  destrel->tobtr.prdt.prsort = NSORT;
  destrel->tobtr.lastpn = pn;
  asp = tmp_buff;
  l = (struct listtob *) asp;
  l->prevpn = (u2_t) ~ 0;
  l->nextpn = (u2_t) ~ 0;
  b = (u2_t *) (asp + sizeof (struct listtob));
  *b++ = 0;
  *b = 0;
  destrel->tobtr.free_sz = BD_PAGESIZE - phtrsize;
  write_tmp_page (pn, asp);  
}

static void
get_new_trel (char **agrl, u2_t nf, char *flaglist)
{
  u2_t i;
  char *a;
  float avrg = 0;

  for (i = 0; i < nf; i++)
    {
      a = agrl[i];
      switch (flaglist[i])
	{
	case FN_COUNT:
	  a = agrl[i];
	  *a++ = 1;
	  t4bpack (0, a);
	  break;
	case FN_SUMM:
	  a = agrl[i]; 
	  *a++ = 0;
	  t4bpack (0, a);
	  break;
	case FN_MAX:
	case FN_MIN:
	  a = agrl[i];	  
	  *a++ = 0;
	  t4bpack (0, a);
	  break;
	case FN_AVG:
	  a = agrl[i];	  
	  *a++ = 0;
	  bcopy ((char *)&avrg, a, sizeof (float));
	  t4bpack (0, a + size4b);
	  break;
	case FN_DT_SUMM:
	  clean_ttab ( t2bunpack (a));
	  a += size2b;	  
	  *a++ = 0;
	  t4bpack (0, a);
	  break;
	case FN_DT_COUNT:
	  clean_ttab ( t2bunpack (a));
	  a += size2b;
	  *a++ = 1;
	  t4bpack (0, a);		  
	  break;
	case FN_DT_AVG:
	  clean_ttab ( t2bunpack (a));	  
	  a += size2b;	  	  
	  *a++ = 0;
          bcopy ((char *)&avrg, a, sizeof (float));
	  t4bpack (0, a + size4b);	  
	  break;
	default:
	  continue;
	  break;
	}
    }
}

static int
gr_crt_frm (char *tuple, u2_t tsize, char **agrl, struct des_field *df,
            u2_t ng, u2_t nf, u2_t * mnf, char *flaglist)
{
  char *gval;
  char *loc, *sc, flag;
  u2_t k, kk;

  distagr_frm (agrl, nf, flaglist);
  loc = tuple + tsize;
  for (df += ng, k = 0; k < nf; k++)
    {
      if ((flag = flaglist[k]) == FN_DT_COUNT || flag == FN_DT_SUMM ||
          flag == FN_DT_AVG)
	gval = agrl[k] + size2b;
      else
	gval = agrl[k];
      if (*gval++ != 0)
	{
	  kk = (ng + k) / 7;
	  sc = tuple + 1 + kk;
	  kk = (ng + k) % 7;	  
	  *sc |= BITVL(kk);
          
          if (flag == FN_AVG || flag == FN_DT_AVG)
            loc = write_average ((df + (mnf[k]-ng))->field_type, gval, loc);
          else
            gval = remval (gval, &loc, (df + k)->field_type);
	}
    }
  get_new_trel (agrl, nf, flaglist);
  return (loc - tuple);
}

static int
cmp_grval (char *tuple, char *cort, struct fun_desc_fields *desf,
           u2_t ng, u2_t * glist)
{
  char *gsc, *gaval, *gval, *aval;
  u2_t k, fn, kg;
  struct des_field *df;
  i4_t v;
  char *arrpnt[BD_PAGESIZE];
  u2_t arrsz[BD_PAGESIZE];

  tuple_break (cort, arrpnt, arrsz, desf);
  gsc = tuple + 1;
  gval = gaval = tuple + scscal (tuple);
  df = desf->df_pnt;
  for (k = 0, kg = 0; kg < ng && gsc < gaval; kg++)
    {
      fn = glist[kg];
      if ((aval = arrpnt[fn]) != NULL)
	{
	  if ((*gsc & BITVL(k)) != 0)
	    {
	      v = cmpval (gval, aval, (df + fn)->field_type);
	      if (v != 0)
		return (v);
	    }
	  else
	    return (-1);
	}
      else
	{
	  if ((*gsc & BITVL(k)) != 0)
	    return (1);
	  else
	    return (-1);
	}
      k++;
      if (k == 7)
	{
	  k = 0;
	  gsc++;
	}
    }
  return (0);
}

static void
df_frm (struct des_field *df_out, struct des_field *df, u2_t ng,
        u2_t * glist, u2_t nf, u2_t * mfn, char *flaglist)
{
  u2_t i;
  struct des_field *df_out2;

  df_out2 = df_out;
  for (i = 0; i < ng; i++)
    *df_out++ = df[glist[i]];
  for (i = 0; i < nf; i++, df_out++)
    {
      switch (flaglist[i])
	{
	case FN_COUNT:
	case FN_DT_COUNT:
	  df_out->field_type = T4B;
	  df_out->field_size = size4b;
	  break;
	case FN_MAX:
	case FN_MIN:
	case FN_SUMM:
	case FN_DT_SUMM:
	  *df_out = df[mfn[i]];
	  break;
	case FN_AVG:
	case FN_DT_AVG:
	  df_out->field_type = TFLOAT;
	  df_out->field_size = size4b;
	  break;
	default:
	  break;
	}
    }
}

struct ans_ctob
makegroup (struct id_rel *pidrel, u2_t ng, u2_t * glist, u2_t nf,
           u2_t * mnf, char *flaglist, char *order)
{
  u2_t pn, *ai, *ali, fn_out, tsize = 0, size, i;
  char *cort, flag;
  struct fun_desc_fields *desf;
  struct des_field *df, *df_out;
  struct des_trel *destrel;
  struct ans_ctob ans;
  i2_t ntob;
  struct des_tob *dt;
  struct id_ob destob;  
  char *outasp, **agrl, *asp, tuple[BD_PAGESIZE];
  char tmp_buff_in[BD_PAGESIZE], tmp_buff_out[BD_PAGESIZE];

  if (pidrel->urn.segnum != NRSNUM)
    {
      ans.cpncob = -ER_NIOB;
      return (ans);
    }
  destrel = (struct des_trel *) * (desnseg.tobtab + pidrel->urn.obnum);
  if (destrel->row_number > 1)
    {
      ans = trsort (pidrel, ng, glist, order, DBL);
      if (ans.cpncob != OK)
        return (ans);
      destrel = (struct des_trel *) * (desnseg.tobtab + ans.idob.obnum);
    }
  else
    ans.cpncob = OK;
  desf = &destrel->f_df_tt;
  df = desf->df_pnt;
  if (desf->f_fdf < glist[0])
    {
      ans.cpncob = -ER_NCF;
      return (ans);
    }
  fn_out = ng + nf;
  size = dtrsize + fn_out * rfsize + ng * size2b;
  outasp = tmp_buff_out;
  dt = gettob (outasp, size, &ntob, TREL);
  pn = destrel->tobtr.firstpn;
  agrl = (char **) xmalloc (nf * sizeof (void *));
  agrl_frm (agrl, df, (u2_t) nf, mnf, flaglist);
  destrel = (struct des_trel *) dt;  
  df_out = (struct des_field *) (destrel + 1);
  df_frm (df_out, df, ng, glist, nf, mnf, flaglist);
  asp = tmp_buff_in;
  read_tmp_page (pn, asp);
  ai = (u2_t *) (asp + phtrsize);
  ali = ai + ((struct p_h_tr *) asp)->linptr;
  for (; ai <= ali; ai++)
    if (*ai != 0)
      {
	tsize = fgr_val (agrl, tuple, asp + *ai, destrel, ng,
                         glist, nf, mnf, flaglist);
	break;
      }
  for (ai++; ai <= ali; ai++)
    if (*ai != 0)
      {
	cort = asp + *ai;
	if (cmp_grval (tuple, cort, desf, ng, glist) != 0)
	  {
	    size = gr_crt_frm (tuple, tsize, agrl, df_out, ng, nf, mnf, flaglist);
	    minstr (outasp, tuple, size, dt);
	    tsize = fgr_val (agrl, tuple, cort, destrel, ng, glist, nf, mnf, flaglist);
	  }
	else
          agrcount (agrl, cort, desf, nf, mnf, flaglist);
      }
  pn = ((struct listtob *) asp)->nextpn;
  while (pn != (u2_t) ~ 0)
    {
      read_tmp_page (pn, asp);
      ai = (u2_t *) (asp + phtrsize);
      ali = ai + ((struct p_h_tr *) asp)->linptr;
      for (; ai <= ali; ai++)
	if (*ai != 0)
	  {
	    cort = asp + *ai;
	    if (cmp_grval (tuple, cort, desf, ng, glist) != 0)
	      {
		size = gr_crt_frm (tuple, tsize, agrl, df_out, ng, nf, mnf, flaglist);
		minstr (outasp, tuple, size, dt);
		tsize = fgr_val (agrl, tuple, cort, destrel, ng, glist, nf, mnf, flaglist);
	      }
	    else
	      agrcount (agrl, cort, desf, nf, mnf, flaglist);
	  }
      pn = ((struct listtob *) asp)->nextpn;
    }
  size = gr_crt_frm (tuple, tsize, agrl, df_out, ng, nf, mnf, flaglist);
  minstr (outasp, tuple, size, dt);
  write_tmp_page (dt->lastpn, outasp);
  destob.segnum = NRSNUM;  
  for (i = 0; i < nf; i++)
    {
      flag = flaglist[i];
      if (flag == FN_DT_COUNT || flag == FN_DT_SUMM || flag == FN_DT_AVG)
	{
	  destob.obnum = t2bunpack(agrl[i]);     
	  deltob (&destob);
	}
      xfree ((void *) agrl[i]);
    }
  xfree ((void *) agrl);
  destrel->f_df_tt.f_fn = fn_out;
  destrel->f_df_tt.f_fdf = 0;
  destrel->f_df_tt.df_pnt = df_out;
  dt->prdt.prsort = SORT;
  dt->prdt.prdbl = DBL;
  dt->prdt.prdrctn =  *order;
  destrel->keysntr = ng;
  size = dtrsize + fn_out * rfsize;
  for (i = 0, ai = (u2_t *) ((char *) destrel + size); i < ng; i++)
    *ai++ = *glist++;
  ans.idob.segnum = NRSNUM;
  ans.idob.obnum = ntob;
  return (ans);
}

int
tuple_break (char *tuple, char **arrpnt, u2_t * arrsz,
             struct fun_desc_fields * desf)
{
  char *sc, *val, *aval, *newval;
  u2_t k, fn, fdf, field_num, tuple_size;
  struct des_field * df;

  sc = tuple + 1;
  tuple_size = scscal (tuple);
  aval = val = tuple + tuple_size;
  field_num = desf->f_fn;
  fdf = desf->f_fdf;
  df = desf->df_pnt;
  for (k = 0, fn = 0; sc < val; fn++)
    {
      if (fn < fdf || (*sc & BITVL(k)) != 0)
	{			/* a value is present */
	  newval = proval (aval, (df + fn)->field_type);
	  arrpnt[fn] = aval;
	  arrsz[fn] = newval - aval;
          tuple_size += arrsz[fn];
	  aval = newval;
	}
      else
	{
	  arrpnt[fn] = NULL;
	  arrsz[fn] = 0;
	}
      if (fn >= fdf)
	{
	  k++;
	  if (k == 7)
	    {
	      k = 0;
	      sc++;
	    }
	}
    }
  for (; fn < field_num; fn++)
    {
      arrpnt[fn] = NULL;
      arrsz[fn] = 0;	
    }
  return (tuple_size);
}

void
agrl_frm (char **agrl, struct des_field *df, u2_t nf, u2_t * mnf,
          char *flaglist)
{
  u2_t i, size = 0;
  struct des_trel *destrel;
  struct des_field *dftr;
  i2_t n;
  char *a;
  float avrg = 0;
  char tmp_buff[BD_PAGESIZE];

  for (i = 0; i < nf; i++)
    {
      switch (flaglist[i])
	{
	case FN_COUNT:
	  size = size4b + 1;
	  agrl[i] = (char *) xmalloc (size);
	  a = agrl[i];
	  *a++ = 1;
	  t4bpack (0, a);
	  break;
	case FN_SUMM:
	  size = gmax((df + mnf[i])->field_size,size4b) + 1;
	  agrl[i] = (char *) xmalloc (size);
	  a = agrl[i];	  
	  *a++ = 0;
	  t4bpack (0, a);
	  break;
	case FN_MAX:
	case FN_MIN:
	  size = gmax((df + mnf[i])->field_size,size4b) + 1;
	  agrl[i] = (char *) xmalloc (size);
	  a = agrl[i];	  
	  *a++ = 0;
	  t4bpack (0, a);
	  break;
	case FN_AVG:
	  size = sizeof (float) + size4b + 1;
	  agrl[i] = (char *) xmalloc (size);
	  a = agrl[i];	  
	  *a++ = 0;
	  bcopy ((char *)&avrg, a, sizeof (float));
	  t4bpack (0, a + size4b);
	  break;
	case FN_DT_SUMM:
	  destrel = (struct des_trel *) gettob (tmp_buff, dtrsize + rfsize, &n, TREL);
          destrel->f_df_tt.f_fn = 1;
	  destrel->f_df_tt.f_fdf = 0;
	  dftr = (df + mnf[i]);
          destrel->f_df_tt.df_pnt = dftr;
	  size = gmax(dftr->field_size,size4b) + size2b + 1;
	  agrl[i] = (char *) xmalloc (size);
	  t2bpack (n, agrl[i]);	  
	  a = agrl[i] + size2b;	  
	  *a++ = 0;
	  t4bpack (0, a);
	  break;
	case FN_DT_COUNT:
	  destrel = (struct des_trel *) gettob (tmp_buff, dtrsize + rfsize, &n, TREL);
          destrel->f_df_tt.f_fn = 1;
	  destrel->f_df_tt.f_fdf = 0;
	  dftr = (struct des_field *) (destrel + 1);
          destrel->f_df_tt.df_pnt = dftr;
	  dftr->field_type = T4B;
	  dftr->field_size = size4b;
	  size = size4b + size2b + 1;
	  agrl[i] = (char *) xmalloc (size);
	  t2bpack (n, agrl[i]);	  
	  a = agrl[i] + size2b;	  
	  *a++ = 1;
	  t4bpack (0, a);
	  break;
	case FN_DT_AVG:
	  destrel = (struct des_trel *) gettob (tmp_buff, dtrsize + rfsize, &n, TREL);
          destrel->f_df_tt.f_fn = 1;
	  destrel->f_df_tt.f_fdf = 0;
	  dftr = (struct des_field *) (destrel + 1);
          destrel->f_df_tt.df_pnt = dftr;
	  dftr->field_type = TFLOAT;
	  dftr->field_size = size4b;
	  size = sizeof (float) + size4b + 1 + size2b;
	  agrl[i] = (char *) xmalloc (size);
	  t2bpack (n, agrl[i]);	  
	  a = agrl[i] + size2b;	  
	  *a++ = 0;
          bcopy ((char *)&avrg, a, sizeof (float));
	  t4bpack (0, a + size4b);	  
	  break;
	default:
	  break;
	}
    }
}

static void
dist_count (char *agrl, char flag)
{
  u2_t *ai, *ali, pn;
  struct fun_desc_fields *desf;
  struct des_field *df;
  struct des_trel *destrel;
  char *arrpnt[BD_PAGESIZE];
  u2_t arrsz[BD_PAGESIZE];  
  char *pnt, *asp;
  char tmp_buff[BD_PAGESIZE];
  
  destrel = (struct des_trel *) * (desnseg.tobtab + t2bunpack( agrl));
  desf = &destrel->f_df_tt;
  df = desf->df_pnt;
  pnt = agrl + size2b;
  asp = tmp_buff;
  for (pn = destrel->tobtr.firstpn; pn != (u2_t) ~ 0;)
    {
      read_tmp_page (pn, asp);
      ai = (u2_t *) (asp + phtrsize);
      ali = ai + ((struct p_h_tr *) asp)->linptr;
      for (; ai <= ali; ai++)
	if (*ai != 0)
	  {
	    tuple_break (asp + *ai, arrpnt, arrsz, desf);
	    agr_frm (pnt, flag, arrpnt[0], df->field_type);
	  }
      pn = ((struct listtob *) asp)->nextpn;
    }
}

void
distagr_frm (char **agrl, u2_t nf, char *flaglist)
{
  u2_t i;
  i2_t n;
  struct des_trel *destrel;
  char *pnt, flag;
  struct id_rel idrel;

  idrel.urn.segnum = NRSNUM;
  for (i = 0; i < nf; i++)
    {
      pnt = agrl[i];
      switch (flaglist[i])
	{
	case FN_DT_SUMM:
	  flag = FN_SUMM;
	  break;
	case FN_DT_COUNT:
	  flag = FN_COUNT;
	  break;
	case FN_DT_AVG:
	  flag = FN_AVG;
	  break;
	default:
	  continue;
	  break;
	}
      n = t2bunpack (pnt);
      destrel = (struct des_trel *) * (desnseg.tobtab + n);
      if (destrel->row_number > 1)
        {
          char order;
          u2_t nfs = 0;
          i2_t nnew;
          struct ans_ctob ans;
          idrel.urn.obnum = n;
          order = GROW;
          ans = trsort (&idrel, 1, &nfs, &order, NODBL);
          nnew = ans.idob.obnum;
          if (n != nnew)
            {
              t2bpack(nnew, pnt);
              deltob (&idrel.urn);
            }
        }
      dist_count (pnt, flag);
    }
}
