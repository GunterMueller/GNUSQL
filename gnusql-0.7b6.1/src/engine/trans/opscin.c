/*
 *  opscin.c  -  Open scanning by  a DB table index
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

/* $Id: opscin.c,v 1.250 1998/09/29 21:25:39 kimelman Exp $ */

#include "destrn.h"
#include "sctp.h"
#include "strml.h"
#include "fdcltrn.h"
#include "xmem.h"

struct ans_opsc
opscin (struct id_ind *pidind, i4_t mode, u2_t fln, u2_t * fl, u2_t slsz,
        char *sc, u2_t diasz, char *diasc, u2_t fmn, u2_t * fml)
{
  struct fun_desc_fields *desf;
  struct ldesscan *disc;
  struct d_sc_i *scind;
  struct ldesind *di;
  struct d_r_t *desrel;
  char *asp;
  struct id_rel *pidrel;
  struct des_tid tid;
  u2_t dscsz;
  i2_t n;
  u2_t *ai, kn;
  struct ans_opsc ans;

  pidrel = &pidind->irii;
  if (pidrel->urn.segnum == NRSNUM)
    {
      ans.cpnops = -ER_NDI;
      return (ans);
    }
  if ((ans.cpnops = cont_id (pidind, &desrel, &di)) != OK)
    return (ans);
  desf = &desrel->f_df_bt;
  if ((ans.cpnops = testcond (desf, fln, fl, &slsz, sc, fmn, fml)) != OK)
    return (ans);

  ai = (u2_t *) (di + 1);
  if ((ans.cpnops = testdsc (desrel, &diasz, diasc, ai, &dscsz)) != OK)
    return (ans);

  if ((ans.cpnops = synlsc (RSC, pidrel, sc, slsz, desf->f_fn, NULL)) != OK)
    return (ans);
  kn = di->ldi.kifn & ~UNIQ & MSK21B;
  if ((ans.cpnops = synlsc (RSC, pidrel, diasc, diasz, kn, ai)) != OK)
    return (ans);
  scind = (struct d_sc_i *) lusc (&n, scisize, (char *) di, SCI, mode, fln, fl, sc, slsz,
				  fmn, fml, diasz + size2b);
  disc = &scind->dessc;
  disc->pdi = di;
  disc->curlpn = (u2_t) ~ 0;
  asp = (char *) scind + scisize + size2b * (fln + fmn) + slsz + size2b;
  if (diasz == 0)
    disc->dpnsc = NULL;
  else
    disc->dpnsc = asp;
  t2bpack (diasz, asp);
  disc->dpnsval = asp + size2b + dscsz;
  bcopy (diasc, asp + size2b, diasz);
  disc->cur_key = NULL;
  if (ind_ftid (disc, &tid, SLOWSCAN) == EOI)
    {
      disc->ctidi.tpn = (u2_t) ~ 0;
      ans.cpnops = -ER_EOSCAN;
    }
  else
    {
      disc->ctidi.tpn = tid.tpn;
      disc->ctidi.tindex = tid.tindex;
      ans.cpnops = OK;
    }
  scind->mesci.prcrt = 0;
  di->oscni++;
  ans.scnum = n;
  return (ans);
}

#define DIA_COMPRESS(diaval, lastb) \
{                                   \
  char *a, *b;                      \
                                    \
  a = diaval;                       \
  b = diaval + size2b;              \
  if (type == T1B)                  \
    *a = *b;                        \
  else if (type == T2B)             \
    {                               \
      val2b = t2bunpack (b);        \
      t2bpack (val2b, a);           \
    }                               \
  else                              \
    {                               \
      val4b = t4bunpack (b);        \
      t4bpack (val4b, a);           \
    }                               \
  diaval += n;                      \
  b = diaval + size2b;              \
  sz = lastb - b;                   \
  bcopy (b, diaval, sz);            \
  lastb -= size2b;                  \
}

char *
pred_compress(char *diaval, char *lastb, struct des_field *df, unsigned char t)
{
  u2_t n, type, val2b;
  i4_t val4b;
  i4_t sz;

  if (t == ANY || t == EQUN || t == NEQUN)
    return (diaval);
  n = t2bunpack (diaval);
  /*      t2bpack (n, diaval);*/
  type = df->field_type;
  if (df->field_size < n)
    return (NULL);
  if (type == T1B || type == T2B || type == T4B || type == TFLOAT)
    {
      DIA_COMPRESS (diaval, lastb);
      if (t == SS || t == SES || t == SSE || t == SESE)
        {
          n = t2bunpack (diaval);
          if (df->field_size < n)
            return (NULL);
          /*	      t2bpack (n, diaval);*/
          DIA_COMPRESS (diaval, lastb);
        }
    }
  else
    {
      diaval += size2b + n;
      if (t == SS || t == SES || t == SSE || t == SESE)
        {
          n = t2bunpack (diaval);
          if (df->field_size < n)
            return (NULL);
          diaval += size2b + n;
        }
    }
  return (diaval);
}

static
int
dia_cmpr (char *diasc, u2_t * diasz, u2_t dscsz, struct des_field *df, u2_t * fnm)
{
  u2_t fnk;
  char *lastb, *diaval;
  i4_t sst;
  char *scpnt;
  unsigned char t;

  sst = 1;
  lastb = diasc + *diasz;
  diaval = diasc + dscsz;
  scpnt = diasc;
  for (fnk = *fnm++; (t = selsc1 (&scpnt, sst++)) != ENDSC; fnk = *fnm++)
    {
      diaval = pred_compress (diaval, lastb, df + fnk, t);
      if (diaval == NULL )
        return (-ER_NCF);
    }
  *diasz = diaval - diasc;
  return (OK);
}

i4_t
testdsc (struct d_r_t *desrel, u2_t * diasz, char *diasc,
         u2_t * mfn, u2_t * dscsz)
{
  if (*diasz != 0)
    {
      u2_t i, fn, fdf;
      i4_t sst;
      char *a;
      struct des_field *df;
      unsigned char t;

      fn = desrel->desrbd.fieldnum;
      fdf = desrel->desrbd.fdfnum;
      df = (struct des_field *) (desrel + 1);
      a = diasc;
      sst = 1;
      for (i = 0; (t = selsc1 (&a, sst++)) != ENDSC && i<fn; i++)
	{
	  if (i < fdf && t == EQUN)
	    return (-ER_NCF);
	  if (t < EQ || (t > ANY && t < ENDSC))
	    return (-ER_NCF);
	}
      if (t != ENDSC)
	return (-ER_NCF);
      if (dsccal (fn, diasc, dscsz) != OK)
	return (-ER_NCF);
      if (dia_cmpr (diasc, diasz, *dscsz, df, mfn) != OK)
	return (-ER_NCF);
    }
  return (OK);
}

i4_t
dsccal (u2_t fn, char *diasc, u2_t * dscsz)
{
  u2_t i;
  i4_t sst;
  char *a;
  unsigned char t;

  sst = 1;
  a = diasc;
  fn++;
  for (i = 0; (t = selsc1 (&a, sst++)) != ENDSC && i < fn ; i++);
  if (t != ENDSC)
    return (-ER_NCF);
  if (sst % 2 == 0)
    a++;
  *dscsz = a - diasc;
  return (OK);
}
