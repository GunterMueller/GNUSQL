/*
 *  bdunion.c  - building of an union, an intersection, a difference
 *               Kernel of GNU SQL-server   
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

/* $Id: bdunion.c,v 1.254 1998/09/29 21:25:21 kimelman Exp $ */

#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

#define BD_UNION   1
#define INTERSCTN  2
#define DIFFERENCE 3

extern struct des_nseg desnseg;

static char *outasp;
static struct des_tob *dt;

void
minsfltr (char *aspfl, struct des_tob *dt, struct des_tid *tid)
{
  u2_t offset;

  offset = ((struct p_h_f *) aspfl)->freeoff;
  if (offset + tidsize > BD_PAGESIZE)
    {
      getptob (aspfl, dt);
      offset = phfsize;
    }
  *(struct des_tid *) (aspfl + offset) = *tid;
  ((struct p_h_f *) aspfl)->freeoff = offset + tidsize;
}

static char *
trunn (char *asptr, u2_t **aim)
{
  return getcort (asptr, aim);
}

static char *
trunnp (char *asptr, u2_t **aim)
{
  u2_t *afi, *ai;
  
  afi = (u2_t *) (asptr + phtrsize);
  ai = *aim - 1;
  minstr (outasp, asptr + *ai, calsc (afi, ai), dt);
  return (trunn (asptr, aim));
}

static char *
flunn (char *aspfl, struct A *inpage, struct des_tid **tid)
{    /* get a tuple and next tid */
  char *asprel, *cort;
  u2_t offset, pn;
    
  offset = (char *)tid - aspfl;
  if (offset >= ((struct p_h_f *) aspfl)->freeoff)
    {
      pn = ((struct listtob *) aspfl)->nextpn;
      if (pn == (u2_t) ~0)
        return (NULL);
      read_tmp_page (pn, aspfl);
      *tid = (struct des_tid *)(aspfl + phfsize);
    }
  pn = (*tid)->tpn;
  if (pn != inpage->p_pn)
    {
      putpg (inpage, 'n');
      while ((asprel = getpg (inpage, inpage->p_sn, pn, 's')) == NULL);
    }
  else
    asprel = inpage->p_shm;
  cort = asprel + *((u2_t *) (asprel + phsize) + (*tid)->tindex);
  (*tid)++;
  return (cort);
}

static char *
flunnp (char *aspfl,  struct A *inpage, struct des_tid **tid)
{
  minsfltr (aspfl, dt, *tid - 1);
  return (flunn (aspfl, inpage, tid));
}

static int
tstcmpun (u2_t * afn1, u2_t * afn2, u2_t * afn, u2_t kn)
{
  for (; kn != 0; kn--)
    {
      if (*afn1 != *afn2++)
	return (-ER_NCR);
      *afn++ = *afn1++;
    }
  return (OK);
}

static struct ans_ctob
bd_union (struct id_ob *pit1, struct id_ob *pit2,
          char * (*chng1) (), char * (*chng2) (),
          char * (*flchng1) (), char * (*flchng2) (), int set_type)
{
  struct des_field *df;
  struct des_tob *dt1, *dt2;
  u2_t fn, fdf, kn, pn, fnk, kk, type;
  u2_t *afn1, *afn2, *afn;
  i2_t n;
  int  drctn, d, v;
  char *cort1, *cort2, *asp1, *asp2, tmp_buff_out[BD_PAGESIZE];
  char tmp_buff_in1[BD_PAGESIZE], tmp_buff_in2[BD_PAGESIZE];
  struct ans_ctob ans;
  char *arrpnt1[BD_PAGESIZE];
  u2_t arrsz1[BD_PAGESIZE];  
  char *arrpnt2[BD_PAGESIZE];
  u2_t arrsz2[BD_PAGESIZE];
  
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
  if (dt1->prdt.prdrctn != dt2->prdt.prdrctn)
    {
      ans.cpncob = -ER_D_DRCTN;
      return (ans);
    }
  asp1 = tmp_buff_in1;
  asp2 = tmp_buff_in2;
  outasp = tmp_buff_out;
  if ((drctn = dt1->prdt.prob) == TREL && dt2->prdt.prob == TREL)
    {
      struct des_trel *dtr1, *dtr2, *destrel;
      struct fun_desc_fields *desf1, *desf2;
      u2_t  *ai1, *ai2, corsize1;
      struct des_field *adf, *adf1, *adf2, *df1, *df2;
      
      dtr1 = (struct des_trel *) dt1;
      dtr2 = (struct des_trel *) dt2;
      kn = dtr1->keysntr;
      desf1 = &dtr1->f_df_tt;
      adf1 = df1 = desf1->df_pnt;
      desf2 = &dtr2->f_df_tt;
      adf2 = df2 = desf2->df_pnt;
      fn = desf1->f_fn;
      fdf = desf1->f_fdf;
      if (fn != desf2->f_fn || fdf != desf2->f_fdf || kn != dtr2->keysntr)
	{
	  ans.cpncob = -ER_N_EQV;
	  return (ans);
	}
      dt = gettob (outasp, dtrsize + fn * rfsize + kn * size2b, &n, TREL);
      destrel = (struct des_trel *) dt;      
      adf = df = (struct des_field *) (destrel + 1);
      for (v = 0; v < fn; df1++, df2++, df++, v++)
	{
	  if ((type = df1->field_type) != df2->field_type)
	    {
	      ans.cpncob = -ER_N_EQV;
	      return (ans);
	    }
	  if (type == TCH || type == TFL)
	    {
	      if (df1->field_size >= df2->field_size)
		*df = *df1;
	      else
		*df = *df2;
	    }
	  else
	    *df = *df1;
	}
      afn1 = (u2_t *) df1;
      afn2 = (u2_t *) df2;
      afn = (u2_t *) df;
      if (tstcmpun (afn1, afn2, afn, kn) != OK)
	{
	  ans.cpncob = -ER_N_EQV;
	  return (ans);
	}
      read_tmp_page (dt1->firstpn, asp1);
      ai1 = (u2_t *) (asp1 + phtrsize);
      read_tmp_page (dt2->firstpn, asp2);
      ai2 = (u2_t *) (asp2 + phtrsize);
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
          corsize1 = tuple_break (cort1, arrpnt1, arrsz1, desf1);
	  tuple_break (cort2, arrpnt2, arrsz2, desf2);
      m1:
	  for (kk = 0; kk < kn; kk++)
	    {
	      fnk = afn[kk];
	      type = (adf + fnk)->field_type;
	      if ((v = cmpfv (type, d, arrpnt1[fnk], arrpnt2[fnk])) < 0)
		{
		  if ((cort1 = (*chng1) (asp1, &ai1)) == NULL)
                    goto m2;
		  corsize1 = tuple_break (cort1, arrpnt1, arrsz1, desf1);
		  goto m1;
		}
	      else if (v > 0)
		{
	     	  if ((cort2 = (*chng2) (asp2, &ai2)) == NULL)
		    goto m2;
		  tuple_break (cort2, arrpnt2, arrsz2, desf2);
		  goto m1;
		}
	      }
	  if (set_type != DIFFERENCE)
            minstr (outasp, cort1, corsize1, dt);
	}
    m2:
      if (set_type != INTERSCTN)
        {
          u2_t *afi, *ali;
          if (cort1 == NULL)
            {
              if (set_type == DIFFERENCE)
                goto m5;
              asp1 = asp2;
              ai1 = ai2;
            }
          afi = (u2_t *) (asp1 + phtrsize);
          ali = afi + ((struct p_h_tr *) asp1)->linptr;
          for (;;)
            {
              for (; ai1 <= ali; ai1++)
                if (*ai1 != (u2_t) 0)
                  minstr (outasp, asp1 + *ai1, calsc (afi, ai1), dt);
              pn = ((struct listtob *) asp1)->nextpn;
              if (pn == (u2_t) ~ 0)
                break;
              read_tmp_page (pn, asp1);
              ai1 = afi = (u2_t *) (asp1 + phtrsize);
              ali = ai1 + ((struct p_h_tr *) asp1)->linptr;
            }
        }
    m5:
      destrel->f_df_tt.f_fn = fn;
      destrel->f_df_tt.f_fdf = fdf;
      destrel->keysntr = kn;
      dt->prdt = dt1->prdt;
    }
  else if (dt1->prdt.prob == FLTR && dt2->prdt.prob == FLTR)
    {
      struct des_fltr *dfltr1, *dfltr2, *desfltr;
      struct fun_desc_fields *desf;
      struct des_tid *tid1, *tid2;
      struct d_r_t *dr;
      struct A inpage;
      
      dfltr1 = (struct des_fltr *) dt1;
      dfltr2 = (struct des_fltr *) dt2;
      dr = dfltr1->pdrtf;
      if (dr != dfltr2->pdrtf)
	{
	  ans.cpncob = -ER_N_EQV;
	  return (ans);
	}
      kn = dfltr1->keysnfl;
      if (kn != dfltr2->keysnfl)
	{
	  ans.cpncob = -ER_N_EQV;
	  return (ans);
	}
      dt = gettob (outasp, dflsize + kn * size2b, &n, FLTR);
      desfltr = (struct des_fltr *) dt;
      afn1 = (u2_t *) ((char *) (dfltr1 + 1) + dfltr1->selszfl);
      afn2 = (u2_t *) ((char *) (dfltr2 + 1) + dfltr2->selszfl);
      afn = (u2_t *) (desfltr + 1);
      if (tstcmpun (afn1, afn2, afn, kn) != OK)
	{
	  ans.cpncob = -ER_N_EQV;
	  return (ans);
	}
      if ((drctn = dt1->prdt.prdrctn) == GROW)
	d = 1;
      else
	d = -1;
      if (drctn != dt2->prdt.prdrctn)
	{
	  ans.cpncob = -ER_D_DRCTN;
	  return (ans);
	}
      desf = &dr->f_df_bt;
      df = desf->df_pnt;
      fn = desf->f_fn;
      read_tmp_page (dt1->firstpn, asp1);
      tid1 = (struct des_tid *) (asp1 + phfsize);
      read_tmp_page (dt2->firstpn, asp2);
      tid2 = (struct des_tid *) (asp2 + phfsize);
      
      while (getpg (&inpage, dr->segnr, tid1->tpn, 's') == NULL);
      for (;;)
	{
	  cort1 = flunn (asp1, &inpage, &tid1);
          if (cort1 == NULL) 
            break;
	  cort2 = flunn (asp2, &inpage, &tid2);
          if (cort2 == NULL)
            break;
          tuple_break (cort1, arrpnt1, arrsz1, desf);
	  tuple_break (cort2, arrpnt2, arrsz2, desf);
      m3:
	  for (kk = 0; kk < kn; kk++)
	    {
	      fnk = afn[kk];
	      type = (df + fnk)->field_type;	      
	      if ((v = cmpfv (type, d, arrpnt1[fnk], arrpnt2[fnk])) < 0)
		{
		  if ((cort1 = (*flchng1) (asp1, &inpage, &tid1)) == NULL)
		    goto m4;
		  tuple_break (cort1, arrpnt1, arrsz1, desf);
		  goto m3;
		}
	      else if (v > 0)
		{
		  if ((cort2 = (*flchng2) (asp2, &inpage, &tid2)) == NULL)
		    goto m4;
		  tuple_break (cort2, arrpnt2, arrsz2, desf);
		  goto m3;
		}
	    }
          if (set_type != DIFFERENCE)
            minsfltr (outasp, dt1, tid1);
	}
    m4:
      if (set_type != INTERSCTN)
        {
          char *max_byte;
          if (cort1 == NULL)
            {
              if (set_type == DIFFERENCE)
                goto m6;
              asp1 = asp2;
              tid1 = tid2;
              dt1 = dt2;
            }
          for (;;)
            {
              max_byte = asp1 + ((struct p_h_f *) asp1)->freeoff;
              for (; (char *)tid1 < max_byte; tid1++)
                minsfltr (outasp, dt1, tid1);
              pn = ((struct listtob *) asp1)->nextpn;
              if (pn == (u2_t) ~ 0)
                break;
              read_tmp_page (pn, asp1);
            }
        }
    m6:
      putpg (&inpage, 'n');
      desfltr->pdrtf = dfltr1->pdrtf;
      desfltr->selszfl = 0;
      dt->prdt = dt1->prdt;
      dt->prdt.prsort = NSORT;
    }
  else
    {
      ans.cpncob = -ER_NIOB;
      return (ans);
    }
  write_tmp_page (dt->lastpn, outasp);
  dt->osctob = 0;
  ans.cpncob = OK;
  ans.idob.segnum = NRSNUM;
  ans.idob.obnum = n;
  return (ans);
}

struct ans_ctob
bdunion (struct id_ob *pit1, struct id_ob *pit2)
{
  return (bd_union (pit1, pit2, trunnp, trunnp, flunnp, flunnp, BD_UNION));
}

struct ans_ctob
intersctn (struct id_ob *pit1, struct id_ob *pit2)
{
  return (bd_union (pit1, pit2, trunn, trunn, flunn, flunn, INTERSCTN));
}

struct ans_ctob
differnc (struct id_ob *pit1, struct id_ob *pit2)
{
  return (bd_union (pit1, pit2, trunnp, trunn, flunnp, flunn, DIFFERENCE));
}

char *
getcort (char *asptr, u2_t **ai)
{     /* get a tuple and next index address */
  u2_t off, *ali;

  ali = (u2_t *) (asptr + phtrsize) + ((struct p_h_tr *) asptr)->linptr;
  if (*ai > ali)
    {
      u2_t pn;
      pn = ((struct listtob *) asptr)->nextpn;
      if (pn == (u2_t) ~ 0)
	return (NULL);
      read_tmp_page (pn, asptr);
      *ai = (u2_t *) (asptr + phtrsize);
      ali = *ai + ((struct p_h_tr *) asptr)->linptr;
    }
  for (; *ai <= ali; (*ai)++)
    if ((off = **ai) != (u2_t) 0)
      {
	*ai += 1;
	return (asptr + off);
      }
  error ("TR.getcort: Serious error in the temporary relation page");
  return (NULL);
}

i4_t
cmpfv (u2_t type, i4_t d, char *val1, char *val2)
{
  if (val1 == NULL)
    {
      if (val2 == NULL)
	return (0);
      else
	return(d);
    }
  else
    if (val2 == NULL)
      return (-d);
  return (cmpval (val1, val2, type) * d);
}

