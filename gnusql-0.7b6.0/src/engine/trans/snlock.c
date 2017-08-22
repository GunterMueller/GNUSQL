/*
 *  snlock.c  - Synchrolocks (transaction)
 *              Kernel of GNU SQL-server  
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

/* $Id: snlock.c,v 1.247 1998/01/13 12:22:19 vera Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "sctp.h"
#include "../synch/sctpsyn.h"
#include "strml.h"
#include "fdcltrn.h"

extern struct ADBL adlj;

CPNM
synrd (u2_t sn, char *aval, u2_t size)
{
  char *a;
  i4_t ast, n;
  struct id_rel idr;
  i4_t k, scsz;
  char lc[SZSNBF];
  char mch[BD_PAGESIZE], *asca;
  CPNM cpn;

  a = lc + size2b;
  bcopy (aval, a, size);
  a += size;
  ast = 1;
  asca = mch;
  for (n = 0; n < 7; n++)
    {
      sct (&asca, ast++, X_D);
      sct (&asca, ast++, EQ);
    }
  sct (&asca, ast, ENDSC);
  if (ast % 2 == 0)
    asca--;
  scsz = asca + 1 - mch;
  k = (size + scsz) % sizeof (i4_t);
  if (k != 0)
    k = sizeof (i4_t) - k;
  a += k;
  for (; asca >= mch;)
    *a++ = *asca--;
  size = a - lc;
  t2bpack (size, lc);
  idr.urn.segnum = sn;
  idr.urn.obnum = RDRNUM;
  if (size > SZSNBF)
    error ("TR.synrd: SYN's buffer is too small");
  cpn = sn_lock (&idr, 't', lc, size);
  if (cpn != 0)
    rllbck (cpn, adlj);
  return (cpn);
}

CPNM
synlock (struct d_r_t *desrel, char *cort)
{
  i4_t scsz, ast;
  char lc[SZSNBF];
  char mch[BD_PAGESIZE];
  char *a, *b, *d, *asca;
  u2_t size, k, fdf, scsize;
  struct des_field *df;
  CPNM cpn;
  struct id_rel idr;

  scsize = scscal (cort);
  a = lc + size2b;
  ast = 1;
  asca = mch;
  b = cort + scsize;
  d = b;
  df = desrel->f_df_bt.df_pnt;
  fdf = desrel->f_df_bt.f_fdf;
  for (k = 0; k < fdf; k++, df++)
    {				/* for always defined fields */
      b = remval (b, &a, df->field_type);
      sct (&asca, ast++, X_D);
      sct (&asca, ast++, EQ);
    }
  for (k = 0, cort++; cort < d; cort++, k = 0)
    for (; k < 7; k++, df++)
      if ((*cort & BITVL(k)) != 0)
	{
	  b = remval (b, &a, df->field_type);
	  sct (&asca, ast++, X_D);
	  sct (&asca, ast++, EQ);
	}
      else
	sct (&asca, ast++, NOTLOCK);
  sct (&asca, ast, ENDSC);
  if (ast % 2 == 0)
    asca--;
  scsz = asca + 1 - mch;
  size = a - lc - size2b;
  k = (size + scsz) % sizeof (i4_t);
  if (k != 0)
    k = sizeof (i4_t) - k;
  a += k;
  while( asca >= mch)
    *a++ = *asca--;
  size = a - lc;
  t2bpack (size, lc);
  if (size > SZSNBF)
    error ("TR.synlock: SYN's buffer is too small");
  idr.urn.segnum = desrel->segnr;
  idr.urn.obnum = desrel->desrbd.relnum;
  idr.pagenum = desrel->pn_r;
  idr.index = desrel->ind_r;
  cpn = sn_lock (&idr, 't', lc, size);
  if (cpn != 0)
    rllbck (cpn, adlj);
  return (cpn);
}

CPNM
synlsc (i4_t type, struct id_rel *idr, char *selcon, u2_t selsize,
        u2_t fn, u2_t * mfn)
{
  i4_t k, scsz, ast;
  char lc[SZSNBF];
  char mch[BD_PAGESIZE];
  char *a, *asca;
  CPNM cpn;
  u2_t size;
  
  ast = 1;
  asca = mch;
  if (selsize == 0)
    {
      u2_t i;
      if (mfn == NULL)
	{
	  if (type == RSC)
	    {
	      for (i = 0; i < fn; i++)
		sct (&asca, ast++, S_S);
	    }
	  else
	    {
	      for (i = 0; i < fn; i++)
		sct (&asca, ast++, X_X);
	    }
	}
      else
	{
          u2_t fnk;
	  for (i = 0, fnk = mfn[0]; i < fn; i++)
	    if (i == fnk)
	      {
		if (type == RSC)
		  sct (&asca, ast++, S_S);
		else
		  sct (&asca, ast++, X_X);
		break;
	      }
	    else
	      sct (&asca, ast++, S_S);
	}
    }
  else
    {
      unsigned char t;
      i4_t sst;
      a = selcon;
      sst = 1;
      for (; (t = selsc1 (&a, sst++)) != ENDSC;)
	if (t == ANY)
	  {
	    if (type == RSC)
	      sct (&asca, ast++, S_S);
	    else
	      sct (&asca, ast++, X_X);
	  }
	else
	  {
	    if (type == RSC)
	      sct (&asca, ast++, S_D);
	    else
	      sct (&asca, ast++, X_D);
	    sct (&asca, ast++, t);
	  }
    }
  a = lc + size2b;
  scsz = asca + 1 - mch;
  if (selsize != 0)
    {
      u2_t scalesize;
      dsccal (fn, selcon, &scalesize);
      bcopy (selcon + scalesize, a, selsize - scalesize);
    }
  sct (&asca, ast, ENDSC);
  if (ast % 2 == 0)
    asca--;
  size = a - lc - size2b;
  k = (size + scsz) % sizeof (i4_t);
  if (k != 0)
    k = sizeof (i4_t) - k;
  for (a += k; asca >= mch;)
    *a++ = *asca--;
  size = a - lc;
  t2bpack (size, lc);
  if (size > SZSNBF)
    error ("TR.synlsc: SYN's buffer is too small");
  cpn = sn_lock (idr, 'w', lc, size);
  if (cpn != 0)
    rllbck (cpn, adlj);
  return (cpn);
}

CPNM
synind (struct id_ob *fullrn)
{
  char *a, *asca;
  i4_t ast, n, scsz;
  struct id_rel idr;
  char lc[SZSNBF];
  char mch[BD_PAGESIZE];
  u2_t size;
  CPNM cpn;

  a = lc + size2b;
  ast = 1;
  asca = mch;
  sct (&asca, ast++, S_D);
  sct (&asca, ast++, EQ);
  for (n = 0; n < 3; n++)
    sct (&asca, ast++, NOTLOCK);
  sct (&asca, ast++, X_D);
  sct (&asca, ast++, EQ);
  sct (&asca, ast++, NOTLOCK);
  sct (&asca, ast++, X_D);
  sct (&asca, ast++, EQ);
  sct (&asca, ast, ENDSC);
  t4bpack (fullrn->obnum, a);
  a +=size4b;
  if (ast % 2 == 0)
    asca--;
  scsz = asca + 1 - mch;
  n = (size4b + scsz) % sizeof (i4_t);
  if (n != 0)
    n = sizeof (i4_t) - n;
  a += n;
  for (; asca >= mch;)
    *a++ = *asca--;
  size = a - lc;
  t2bpack (size, lc);
  idr.urn.segnum = fullrn->segnum;
  idr.urn.obnum = RDRNUM;
  if (size > SZSNBF)
    error ("TR.synind: SYN's buffer is too small");
  cpn = sn_lock (&idr, 't', lc, size);
  if (cpn != 0)
    rllbck (cpn, adlj);
  return (cpn);
}

CPNM
syndmod (struct d_r_t *desrel, u2_t flsz,
         u2_t *fl, Colval colval, u2_t *lenval)
{
  i4_t scsz;
  char lc[SZSNBF];
  char mch[BD_PAGESIZE], *asca;
  char *a;
  i4_t ast;
  u2_t size, k, fn, type;
  CPNM cpn;
  struct des_field *df;
  struct id_rel idr;

  a = lc + size2b;
  ast = 1;
  asca = mch;
  df = desrel->f_df_bt.df_pnt;
  for (k = 0; k < flsz; k++)
    if (colval[k] != NULL)
      {
        fn = fl[k];
        size = lenval[k];
        if ((type = (df + fn)->field_type) == TCH || type == TFL)
          {
            t2bpack (size, a);
            a += size2b;
          }
        bcopy (colval[k], a, size);
        a += size;
        sct (&asca, ast++, X_D);
        sct (&asca, ast++, EQ);
      }
    else
      sct (&asca, ast++, NOTLOCK);
  sct (&asca, ast, ENDSC);
  if (ast % 2 == 0)
    asca--;
  scsz = asca + 1 - mch;
  size = a - lc - size2b;
  k = (size + scsz) % sizeof (i4_t);
  if (k != 0)
    k = sizeof (i4_t) - k;
  a += k;
  for (; asca >= mch;)
    *a++ = *asca--;
  size = a - lc;
  t2bpack (size, lc);
  if (size > SZSNBF)
    error ("TR.syndmod: SYN's buffer is too small");
  idr.urn.segnum = desrel->segnr;
  idr.urn.obnum = desrel->desrbd.relnum;
  idr.pagenum = desrel->pn_r;
  idr.index = desrel->ind_r;
  cpn = sn_lock (&idr, 'w', lc, size);
  if (cpn != 0)
    rllbck (cpn, adlj);
  return (cpn);
}
