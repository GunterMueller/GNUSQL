/*
 *  libtran.c  -
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

/* $Id: libtran.c,v 1.252 1998/08/18 22:29:59 kimelman Exp $ */


#include "xmem.h"
#include "destrn.h"
#include "sctp.h"
#include "sql.h"
#include "errors.h"

#include "f1f2decl.h"

extern struct d_r_t *firstrel;
extern CPNM curcpn;
extern char **scptab;
extern struct ldesind **TAB_IFAM;
extern i4_t TIFAM_SZ;
extern i2_t maxscan;
extern u2_t S_SC_S;

u2_t 
scscal (char *key)
{
  int   i;
  
  for (i = 0; (key[i] & EOSC) == 0 && i <= BD_PAGESIZE; i++) ;
  
  assert (i+1 < BD_PAGESIZE);
  
  return i+1;
}

i4_t 
CCS (char *asp)
{
  i4_t n;
  i2_t *a;
  i4_t i, k;

  for (n = 0, a = (i2_t *) asp, k = BD_PAGESIZE / 2, i = 0; i < k; i++)
    n += *a++;
  return (n);
}

#define DIASZ_REL (1+size4b)

struct d_sc_i *
rel_scan (struct id_ob *fullrn, char *ob, i2_t *n, u2_t fnum,
          u2_t *fl, char *sc, u2_t slsz, u2_t fmnum, u2_t *fml)
{
  char *a;
  struct d_sc_i *scind;
  struct ldesscan *disc;
  u2_t val, sn;
  i4_t sst;

  scind = (struct d_sc_i *) lusc (n, scisize, ob, SCR, WSC,
			fnum, fl, sc, slsz, fmnum, fml, DIASZ_REL + size2b);
  disc = &scind->dessc;
  disc->curlpn = (u2_t) ~ 0;
  a = (char *) scind + scisize + (fnum + fmnum) * size2b + size2b + slsz;
  disc->dpnsc = a;
  val = DIASZ_REL;
  t2bpack (val, a);
  a +=size2b;
  disc->dpnsval = a + 1;
  disc->cur_key = NULL;
  sst = 1;
  sct (&a, sst++, EQ);
  sct (&a, sst, ENDSC);
  t4bpack (fullrn->obnum, a);
  sn = fullrn->segnum;
  tab_difam (sn);
  disc->pdi = (struct ldesind *) TAB_IFAM[sn];
  return (scind);
}

i4_t
tab_difam (u2_t sn)
{
  struct ldesind *difam;
  struct des_field *df;
  u2_t *a;

  if (sn < (u2_t) TIFAM_SZ && TAB_IFAM[sn] != NULL)
    return (OK);
  difam = (struct ldesind *) xmalloc (ldisize + 2 * size2b + 2 * rfsize);
  difam->ldi.unindex = 0;
  difam->ldi.rootpn = S_SC_S;
  difam->ldi.kifn = 1;
  difam->i_segn = sn;
  a = (u2_t *) ((char *) difam + ldisize);
  *a++ = 0;
  a++;
  df = (struct des_field *) a;
  df->field_type = T2B;
  df->field_size = size2b;
  df++;
  difam->pdf = df;
  df->field_type = T4B;
  df->field_size = size4b;
  if (sn >= (u2_t) TIFAM_SZ)
    {
      TAB_IFAM = (struct ldesind **) xrealloc (TAB_IFAM,
				     (sn + 1) * sizeof (struct ldesind **));
      TIFAM_SZ = sn + 1;
    }
  TAB_IFAM[sn] = difam;
  return (OK);
}

void
sct (char **a, i4_t st, unsigned char t)
{
  if (st % 2)
    **a = t;
  else
    *(*a)++ |= t << 4; 
}

unsigned char
selsc1 (char **a, i4_t st)
{
  if (st % 2)
    return (**a) & MSKB4B;
  else
    return (*(*a)++ & MSKS4B) >> 4;
}

void
delscan (i2_t scnum)
{
  char **t;
  
  t = scptab + scnum;
  xfree (*t);
  *t = NULL;
}

void
dfunpack (struct d_r_t *desrel, u2_t size, char *pnt)
{
  bcopy (pnt, (char *) (desrel + 1), size);
}

void
drbdunpack (struct d_r_bd *drbd, char *pnt)
{
  bcopy (pnt, (char *) drbd, drbdsize);
}

struct d_r_t *
crtrd (struct id_rel *pidrel, char *a)
{
  u2_t size;
  struct d_r_t *desrel;
  struct d_r_bd drbd;

  a += scscal (a);
  drbdunpack (&drbd, a);
  if (pidrel->urn.obnum != drbd.relnum)
    return (NULL);
  size = drbd.fieldnum * rfsize;
  desrel = (struct d_r_t *) xmalloc (size + sizeof (struct d_r_t));
  desrel->desrbd = drbd;
  dfunpack (desrel, size, a + drbdsize);
  desrel->pid = NULL;
  desrel->drlist = firstrel;
  firstrel = desrel;
  desrel->segnr = pidrel->urn.segnum;
  desrel->pn_r = pidrel->pagenum;
  desrel->ind_r = pidrel->index;
  desrel->f_df_bt.df_pnt = (struct des_field *) (desrel + 1);
  desrel->f_df_bt.f_fn = drbd.fieldnum;
  desrel->f_df_bt.f_fdf = drbd.fdfnum;
  desrel->oscnum = 0;
  desrel->cpndr = curcpn;
  return (desrel);
}

void
crtid (struct ldesind *desind, struct d_r_t *desrel)
{
  struct des_field *df2;
  u2_t kn;

  desind->pdf = (struct des_field *) (desrel + 1);
  desind->cpndi = curcpn;
  desind->i_segn = desrel->segnr;
  desind->dri = desrel;
  desind->oscni = 0;
  kn = desind->ldi.kifn & ~UNIQ & MSK21B;
  if (kn % 2 != 0)
    kn += 1;
  df2 = (struct des_field *) ((char *) desind + ldisize + kn * size2b);
  df2->field_type = T4B;
  df2->field_size = size4b;
}

char *
lusc (i2_t * num, u2_t size, char *aob, i4_t type, i4_t mode, u2_t fn,
      u2_t * fl, char *selc, u2_t selsize, u2_t fmn, u2_t * fml, u2_t dsize)
{
  u2_t i, *a;
  struct d_mesc *scpr;
  char *s;

  s = (char *) xmalloc (size + size2b + selsize + size2b * (fn + fmn) + dsize);
  *num = lunt (&scptab, &maxscan, DTSCAN);
  a = (u2_t *) (s + size);
  for (i = 0; i < fn; i++)
    *a++ = *fl++;
  for (i = 0; i < fmn; i++)
    *a++ = *fml++;
  *a = selsize;
  bcopy (selc, (char *) a + size2b, selsize);
  *(scptab + *num) = s;
  scpr = (struct d_mesc *) s;
  scpr->obsc = type;
  scpr->modesc = mode;
  scpr->prcrt = 0;
  scpr->pobsc = aob;
  scpr->cpnsc = curcpn;
  scpr->ndc = 0;
  scpr->fnsc = fn;
  scpr->fmnsc = fmn;
  scpr->pslc = (char *) a;
  return (s);
}

i2_t
lunt (char ***tab, i2_t * maxn, i2_t delta)
{
  char **s;
  i2_t num, newmax, n;


  for (num = 0, s = *tab; num < *maxn && s[num] != NULL;)
    num++;
  if (num == *maxn)
    {
      newmax = *maxn + delta;
      *tab = (char **) xrealloc ((void *) (*tab), sizeof(char*) * newmax);
      for (n = *maxn, s = *tab; n < newmax; n++)
	s[n] = NULL;
      *maxn = newmax;
    }
  return (num);
}
