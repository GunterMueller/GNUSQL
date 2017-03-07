/*
 *  join.c  -  make up a join of two specific tables
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

/* $Id: join.c,v 1.250 1998/09/29 21:25:34 kimelman Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

extern struct des_nseg desnseg;

static u2_t k1;
static char *sc;

static int
scale_frm (u2_t mfnsz, u2_t *mfn, char **arrpnt, u2_t *arrsz)
{
  u2_t i, fnk, size = 0, sz;

  for (i = 0; i < mfnsz; i++)
    {
      fnk = mfn[i];
      if ( arrpnt[fnk] != NULL )
	{
	  k1++;	  
	  if ( (sz = arrsz[fnk]) != 0 )
	    size += sz;
	}
    }
  return (size);
}

static char *
jncrtfrm (u2_t mfnsz, u2_t *mfn, char **arrpnt, u2_t *arrsz, char *val)
{
  u2_t fnk, i, k, sz;
  char *b;
  
  k = 0;
  for (i = 0; i < mfnsz; i++)
    {
      fnk = mfn[i];
      if ( (b = arrpnt[fnk]) != NULL )
	{
	    *sc |= BITVL(k++);		/* read 1 in key scale */
	    if (k == 7)
	      {
		k = 0;
		*(++sc) = 0;
	      }
	    if ( (sz = arrsz[fnk]) != 0 )
              {
                bcopy (b, val, sz);
                val += sz;
              }
	}
    }  
  return (val);
}

/*
 * join relations
 *
 */

struct ans_ctob
join (struct id_rel *pir1, i4_t mfn1sz, u2_t *mfn1,
      struct id_rel *pir2, i4_t mfn2sz, u2_t *mfn2)
{
  struct id_ob *pit1, *pit2;
  struct des_tob *dt1, *dt2, *dt;
  u2_t fn, kn1, kn2, *afn1, *afn2, fnk1, fnk2;
  u2_t *ai1, *ali1, *ai2, *ali2, kk, type, kscsz, size;
  struct des_trel *dtr1, *dtr2, *destrel;
  struct des_field *df1, *df2, *df;
  struct fun_desc_fields *desf1, *desf2;
  char *cort1, *cort2, *keyval, *asp1, *asp2, *outasp;
  i4_t drctn, d, v;
  i2_t n;
  struct ans_ctob ans;
  char *arrpnt1[BD_PAGESIZE];
  u2_t arrsz1[BD_PAGESIZE];
  char *arrpnt2[BD_PAGESIZE];
  u2_t arrsz2[BD_PAGESIZE];
  char tmp_buff_in1[BD_PAGESIZE], tmp_buff_in2[BD_PAGESIZE], tmp_buff_out[BD_PAGESIZE];

  pit1 = &pir1->urn;
  pit2 = &pir2->urn;
  if (pit1->segnum != NRSNUM || pit2->segnum != NRSNUM)
    {
      ans.cpncob = -ER_NIOB;
      return (ans);
    }
  if ((u2_t) pit1->obnum > desnseg.mtobnum || (u2_t) pit2->obnum > desnseg.mtobnum)
    {
      ans.cpncob = -ER_NIOB;
      return (ans);
    }
  dt1 = (struct des_tob *) * (desnseg.tobtab + pit1->obnum);
  dt2 = (struct des_tob *) * (desnseg.tobtab + pit2->obnum);
  if (dt1 == NULL || dt2 == NULL)
    {
      ans.cpncob = -ER_NIOB;
      return (ans);
    }    
  if (dt1->prdt.prob != TREL || dt2->prdt.prob != TREL)
    {
      ans.cpncob = -ER_NDR;
      return (ans);
    }
  if (dt1->prdt.prsort != SORT || dt2->prdt.prsort != SORT)
    {
      ans.cpncob = -ER_N_SORT;
      return (ans);
    }
  if (dt1->prdt.prdbl != NODBL || dt2->prdt.prdbl != NODBL)
    {
      ans.cpncob = -ER_H_DBL;
      return (ans);
    }
  if ((drctn = dt1->prdt.prdrctn) != dt2->prdt.prdrctn)
    {
      ans.cpncob = -ER_D_DRCTN;
      return (ans);
    }

  dtr1 = (struct des_trel *) dt1;
  dtr2 = (struct des_trel *) dt2;
  kn1 = dtr1->keysntr;
  kn2 = dtr2->keysntr;
  desf1 = &dtr1->f_df_tt;
  desf2 = &dtr2->f_df_tt;
  df1 = desf1->df_pnt;
  df2 = desf2->df_pnt;
/* if(fn1!=fn2 || fdf1!=fdf2 || kn!=kn2) { ans.cpncob= N_EQV; return(ans); }*/
  afn1 = (u2_t *) (df1 + desf1->f_fn);
  afn2 = (u2_t *) (df2 + desf2->f_fn);
  for (k1 = 0; k1 < kn1 && k1 < kn2; k1++)
    {
      fnk1 = *afn1++;
      fnk2 = *afn2++;
      if ((df1 + fnk1)->field_type != (df2 + fnk2)->field_type)
	{
	  ans.cpncob = -ER_N_EQV;
	  return (ans);
	}
    }
  fn = mfn1sz + mfn2sz;
  outasp = tmp_buff_out;
  dt = gettob (outasp, dtrsize + fn * rfsize, &n, TREL);
  destrel = (struct des_trel *) dt;
  df = (struct des_field *) (destrel + 1);
  asp1 = tmp_buff_in1;
  read_tmp_page (dt1->firstpn, asp1);
  ai1 = (u2_t *) (asp1 + phtrsize);
  ali1 = ai1 + ((struct p_h_tr *) asp1)->linptr;
  asp2 = tmp_buff_in2;
  read_tmp_page (dt2->firstpn, asp2);
  ai2 = (u2_t *) (asp2 + phtrsize);
  ali2 = ai2 + ((struct p_h_tr *) asp2)->linptr;
  if (drctn == GROW)
    d = 1;
  else
    d = -1;
  for (;;)
    {
      cort1 = getcort (asp1, &ai1);
      if (cort1 == NULL)
	break;
      cort2 = getcort (asp2, &ai2);
      if (cort2 == NULL)
	break;
      tuple_break (cort1, arrpnt1, arrsz1, desf1);
      tuple_break (cort2, arrpnt2, arrsz2, desf2);
  m1:
      for (kk = 0; kk < kn1 && kk < kn2 ; kk++)
	{
	  fnk1 = afn1[kk];
	  type = (df1 + fnk1)->field_type;
	  fnk2 = afn2[kk];
          v = cmpfv (type, d, arrpnt1[fnk1], arrpnt2[fnk2]);
	  if (v < 0)
	    {
	      cort1 = getcort (asp1, &ai1);
	      if (cort1 == NULL)
		goto m2;
	      tuple_break (cort1, arrpnt1, arrsz1, desf1);
	      goto m1;
	    }
	  else if (v > 0)
	    {
	      cort2 = getcort (asp2, &ai2);
	      if (cort2 == NULL)
		goto m2;
	      tuple_break (cort2, arrpnt2, arrsz2, desf2);     
	      goto m1;
	    }
	}
      k1 = 0;
      size = scale_frm (mfn1sz, mfn1, arrpnt1, arrsz1);
      size += scale_frm (mfn2sz, mfn2, arrpnt2, arrsz2);      
      kscsz = k1 / 7;
      if ((k1 % 7) != 0)
	kscsz++;
      size += kscsz + 1;
      sc = getloc (outasp, size, dt);
      *sc++ = CORT;
      keyval = sc + kscsz;
      keyval = jncrtfrm (mfn1sz, mfn1, arrpnt1, arrsz1, keyval);
      keyval = jncrtfrm (mfn2sz, mfn2, arrpnt2, arrsz2, keyval);
      if ( (k1 % 7) == 0)
	sc--;
      *sc |= EOSC;
      destrel->row_number += 1; 
    }
m2:
  write_tmp_page (dt->lastpn, outasp);
  destrel->f_df_tt.f_fn = fn;
  destrel->f_df_tt.f_fdf = 0;
  destrel->f_df_tt.df_pnt = df;
  for ( v = 0; v < mfn1sz; v++)
    *df++ = df1[mfn1[v]];
  for ( v = 0; v < mfn2sz; v++)
    *df++ = df2[mfn2[v]];  
  ans.cpncob = OK;
  ans.idob.segnum = NRSNUM;
  ans.idob.obnum = n;
  return (ans);
}

