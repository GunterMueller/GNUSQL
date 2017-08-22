/*
 *  tmpob.c  - Temporary objects manipulations 
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at Institute of System Programming of Russian Academy of Science
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

/* $Id: tmpob.c,v 1.253 1998/05/20 05:52:44 kml Exp $ */

#include "xmem.h"
#include "destrn.h"
#include "strml.h"
#include "dessrt.h"
#include "fdcltrn.h"
#include "fdclsrt.h"

extern i4_t    freext;
extern i4_t    sizeof_tmp_seg;  /* temporary file size in bytes */ 
extern u2_t    extent_size;     /* extent size in pages */ 
extern struct  des_nseg desnseg;

struct ans_ctob
crtrel (i4_t fn, i4_t fdf, struct des_field *df)
{
  struct ans_ctob ans;
  struct des_trel *destrel;
  struct des_field *dftr;
  i2_t n;
  char tmp_buff[BD_PAGESIZE];

  if (fdf > fn)
    {
      ans.cpncob = -ER_NCF;
      return (ans);
    }
  destrel = (struct des_trel *) gettob (tmp_buff, dtrsize + fn * rfsize, &n, TREL);
  destrel->f_df_tt.f_fn = fn;
  destrel->f_df_tt.f_fdf = fdf;
  destrel->keysntr = 0;
  dftr = (struct des_field *) (destrel + 1);
  destrel->f_df_tt.df_pnt = dftr;
  destrel->row_number = 0;
  for (; fn != 0; fn--)
    *dftr++ = *df++;
  ans.cpncob = OK;
  ans.idob.segnum = NRSNUM;
  ans.idob.obnum = n;
  return (ans);
}
struct ans_ctob
crfltr (struct id_rel *pidrel)
{
  struct ans_ctob ans;
  struct des_fltr *desfltr;
  i2_t n;
  struct d_r_t *desrel;
  char tmp_buff[BD_PAGESIZE];

  if (pidrel->urn.segnum == NRSNUM)
    {
      ans.cpncob = -ER_NIOB;
      return (ans);
    }
  if ((ans.cpncob = contir (pidrel, &desrel)) != OK)
    return (ans);
  desfltr = (struct des_fltr *) gettob (tmp_buff, dflsize, &n, FLTR);
  desfltr->pdrtf = desrel;
  ans.cpncob = OK;
  ans.idob.segnum = NRSNUM;
  ans.idob.obnum = n;
  return (ans);
}

struct des_exns *
getext ()
{
  register i4_t i, j, maxext;
  char found = 0;
  struct des_exns *desext;

  desext = desnseg.dextab;
  maxext = desnseg.mexnum;
  if (freext != 0)
    for (i = 0; i < maxext; i++)
      if (desext[i].freecntr == extent_size)
	{
          freext--;
          desext[i].freecntr--;
          PRINTF(("TRN.getext: freext = %d, fpn = %d\n", freext, desext[i].efpn));
	  return (&desext[i]);
	}
  for (i = 0; i < maxext; i++)	/* new extent is needing */
    if (desext[i].efpn == (u2_t) ~ 0)
      {
	found++;
	break;
      }
  if (!found)
    {
      i = maxext;
      desnseg.lexnum = maxext;
      maxext *=2;
      desnseg.mexnum = maxext;
      desnseg.dextab = (struct des_exns *) xrealloc ((char *)desnseg.dextab,
                                                     maxext * dexsize);
      desext = desnseg.dextab;
      for (j = i; j < maxext; j++)
        desext[j].efpn = (u2_t) ~ 0;
    }
  desext = &desext[i];
  desext->efpn = sizeof_tmp_seg / BD_PAGESIZE;
  desext->funpn = desext->efpn + 1;
  desext->ldfpn = (u2_t) ~ 0;
  desext->freecntr = extent_size - 1;
  sizeof_tmp_seg += extent_size * BD_PAGESIZE;
  PRINTF(("TRN.getext: freext = %d, fpn = %d, sizeof_tmp_seg = %d\n",
          freext, desext->efpn, sizeof_tmp_seg));
  return (desext);
}

void 
putext (u2_t *mfpn, i4_t exnum)
{
  i4_t i, j, maxext;
  u2_t fpn;
  struct des_exns *desext;

  desext = desnseg.dextab;
  maxext = desnseg.mexnum;
  for (i = 0, fpn = *mfpn; i < exnum; i++, fpn = mfpn[i])
    for (j = 0; j < maxext; j++)
      if (fpn == desext[j].efpn)
	{
          PRINTF(("TRN.putext: exnum=%d, fpn = %d\n",exnum, fpn));
          desext[j].freecntr = extent_size;
	  freext++;
	  break;
	}
}

struct des_tob *
gettob (char *asp, u2_t size, i2_t * n, i4_t type)
{
  u2_t *b, pn;
  struct des_exns *desext;
  struct des_tob *dt;
  struct listtob *l;
  
  *n = lunt (&desnseg.tobtab, &desnseg.mtobnum, TOBPTD);
  dt = (struct des_tob *) xmalloc (size);
  *(desnseg.tobtab + *n) = (char *) dt;
  dt->prdt.prob = type;
  dt->prdt.prsort = NSORT;
  desext = getext ();
  pn = desext->efpn;
  dt->firstpn = pn;
  dt->lastpn = pn;
  dt->osctob = 0;
  l = (struct listtob *) asp;
  l->prevpn = (u2_t) ~ 0;
  l->nextpn = (u2_t) ~ 0;
  b = (u2_t *) (asp + sizeof (struct listtob));
  if (type == FLTR)
    {
      *b = phfsize;
      dt->free_sz = BD_PAGESIZE - phfsize;
    }
  else
    {
      dt->free_sz = BD_PAGESIZE - phtrsize;
      *b++ = 0;
      *b = 0;
    }
  write_tmp_page (pn, asp);
  return (dt);
}

static struct des_exns *
ludext (u2_t pn)
{
  struct des_exns *desext;
  u2_t mext, n;

  pn = pn / extent_size * extent_size;
  mext = desnseg.mexnum;
  desext = desnseg.dextab;
  for (n = 0; n < mext; desext++, n++)
    if ( desext->efpn == pn )
      return (desext); 
  return (NULL);
}

static void
freeext (struct des_exns *desext)
{
  desext->freecntr++;
  if (desext->freecntr == extent_size)
    putext (&desext->efpn, 1);
}

i4_t
deltob (struct id_ob *pidtob)
{
  struct des_tob *dt;
  struct des_exns *desext;
  char **a;
  u2_t pn;
  char tmp_buff[BD_PAGESIZE];

  if (pidtob->segnum != NRSNUM)
    return (-ER_NIOB);
  a = desnseg.tobtab + pidtob->obnum;
  dt = (struct des_tob *) * a;
  for (pn = dt->firstpn; pn != (u2_t) ~ 0;)
    {
      if (pn % extent_size == 0)
        {
          desext = ludext (pn);
          putext (&desext->efpn, 1);
        }
      read_tmp_page (pn, tmp_buff);
      pn = ((struct listtob *) tmp_buff)->nextpn;
    }
  delscd (dt->osctob, (char *) dt);
  xfree ((void *) dt);
  *a = NULL;
  return (OK);
}

i4_t
instr (struct des_tob *dt, char *cort, u2_t corsize)
{
  char tmp_buff[BD_PAGESIZE];

  read_tmp_page (dt->lastpn, tmp_buff);
  minstr (tmp_buff, cort, corsize, dt);
  write_tmp_page (dt->lastpn, tmp_buff);
  dt->prdt.prsort = NSORT;
  return (OK);
}

void
minstr (char *asp, char *cort, u2_t corsize, struct des_tob *dt)
{
  char *a;

  a = getloc (asp, corsize, dt);
  bcopy (cort, a, corsize);
  ((struct des_trel *) dt)->row_number += 1;
}

char *
getloc (char *asp, u2_t corsize, struct des_tob *dt)
{
  u2_t *ai, off;
  struct p_h_tr *phtr;
  
  if (dt->free_sz < corsize + size2b)
    {
      getptob (asp, dt);
      phtr = (struct p_h_tr *) asp;
      phtr->linptr = 0;
      off = BD_PAGESIZE - corsize;
      dt->free_sz = BD_PAGESIZE - phtrsize;
    }
  else if (dt->free_sz == BD_PAGESIZE - phtrsize)
    {
      phtr = (struct p_h_tr *) asp;
      phtr->linptr = 0;
      off = BD_PAGESIZE - corsize;
    }
  else
    {
      phtr = (struct p_h_tr *) asp;
      off = *((u2_t *) (asp + phtrsize) + phtr->linptr) - corsize;
      phtr->linptr += 1;
    }
  if (corsize != 0)
    {
      ai = (u2_t *) (phtr + 1) + phtr->linptr;
      *ai = off;
      dt->free_sz -= corsize + size2b;
    }
  return (off + asp);
}

void
getptob (char *asp, struct des_tob *destob)
{  /* asp - a tmp_buff pointer to the last page of destob (temporary object) */
  u2_t pn, oldpn, *b;
  struct des_exns *desext;
  struct listtob *lsttob;
  i4_t i = 0;
  
  oldpn = destob->lastpn;
  if (destob->prdt.prsort == SORT)
    {
      desext = getext ();
      pn = desext->efpn;
    }
  else
    {
      u2_t cpn;
      desext = ludext (oldpn);
      if ((pn = desext->funpn) != (u2_t) ~ 0)
	{
	  cpn = pn + 1;
	  if (cpn == desext->efpn + extent_size)
	    desext->funpn = (u2_t) ~ 0;
	  else
	    desext->funpn = cpn;
	}
      else
	{
	  if ((pn = desext->ldfpn) != (u2_t) ~ 0)
	    i = 1;
	  else
	    {
	      desext = desnseg.dextab + desnseg.lexnum;
	      if ((pn = desext->funpn) != (u2_t) ~ 0)
		{
		  cpn = pn + 1;
		  if (cpn == desext->efpn + extent_size)
		    desext->funpn = (u2_t) ~ 0;
		  else
		    desext->funpn = cpn;
		}
	      else
		{
		  if ((pn = desext->ldfpn) != (u2_t) ~ 0)
		    i = 1;
		  else
                    {
                      desext = getext ();
                      pn = desext->efpn;
                    }
		}
	    }
	}
    }
  lsttob = (struct listtob *) asp;
  lsttob->nextpn = pn;
  write_tmp_page (oldpn, asp);
  lsttob = (struct listtob *) asp;
  if (i != 0)
    desext->ldfpn = lsttob->prevpn;
  lsttob->nextpn = (u2_t) ~ 0;
  lsttob->prevpn = oldpn;
  b = (u2_t *) (asp + sizeof (struct listtob));
  if (destob->prdt.prob == FLTR)
    {
      *b = phfsize;
      destob->free_sz = BD_PAGESIZE - phfsize;
    }
  else
    {
      destob->free_sz = BD_PAGESIZE - phtrsize;
      *b++ = 0;
      *b = 0;
    }
  desext->freecntr--;
  destob->lastpn = pn;
  return;
}

void
deltr (char *asp, u2_t * ai, struct des_tob *destob, u2_t pn)
{
  u2_t *afi;
  
  afi = (u2_t *) (asp + phtrsize);
  comptr (asp, ai, calsc (afi, ai));
  *ai = 0;
  if (frptr (asp) == 1)
    frptob (destob, asp, pn);
  destob->prdt.prsort = NSORT;
}

i4_t
frptr (char *asp)
{
  u2_t *ali, *ai;

  ai = (u2_t *) (asp + phtrsize);
  ali = ai + ((struct p_h_tr *) asp)->linptr;
  for (; ai <= ali; ai++)
    if (*ai != 0)
      return (0);
  return (1);
}

void
comptr (char *asp, u2_t * ai, u2_t size)
{
  u2_t *ali;
  char *a, *b, *c;

  ali = (u2_t *) (asp + phtrsize) + ((struct p_h_tr *) asp)->linptr;
  for (; ai <= ali; ai++)
    if (*ai != 0)
      *ai += size;
  for (a = asp + *ai - 1, b = a - size, c = asp + *ali; a <= c;)
    *a-- = *b--;
}

static void
corltob (u2_t pn, u2_t type, u2_t newpn)
{
  char tmp_buff[BD_PAGESIZE];
  
  read_tmp_page (pn, tmp_buff);
  if (type == 1)
    ((struct listtob *) tmp_buff)->prevpn = newpn;
  else
    ((struct listtob *) tmp_buff)->nextpn = newpn;
  write_tmp_page (pn, tmp_buff);
}

void
frptob (struct des_tob *destob, char *asp, u2_t pn)
{
  struct listtob *phtob;
  struct des_exns *desext;
  u2_t npn, ppn;

  phtob = (struct listtob *) asp;
  npn = phtob->nextpn;
  phtob->nextpn = (u2_t) ~ 0;
  ppn = phtob->prevpn;
  phtob->prevpn = (u2_t) ~ 0;
  desext = ludext (pn);
  if (desext != NULL)
    {
      *(u2_t *) asp = desext->ldfpn;
      desext->ldfpn = pn;
      if (pn == destob->firstpn)
	{
	  if (pn != destob->lastpn)
	    {
	      destob->firstpn = npn;
	      corltob (npn, 1, 0);
	    }
	}
      else
	{
	  if (pn == destob->lastpn)
	    {
	      destob->lastpn = ppn;
	      corltob (ppn, 2, 0);
	    }
	  else
	    {
	      corltob (ppn, 2, npn);
	      corltob (npn, 1, ppn);
	    }
	}
      freeext (desext);
    }
}  

struct ans_ctob
trsort (struct id_rel *pidrel, u2_t kn, u2_t * mfn, char *drctn, char prdbl)
{
  struct des_tob *dt, *dtnew;
  struct des_trel *destrel;
  u2_t *a, *b, size, fpn, lpn;
  i2_t n;
  struct ans_ctob ans;
  struct des_sort sdes;

  if (pidrel->urn.segnum != NRSNUM)
    {
      ans.cpncob = -ER_NIOB;
      return (ans);
    }
  dt = (struct des_tob *) * (desnseg.tobtab + pidrel->urn.obnum);
  if (dt->prdt.prob != TREL)
    {
      ans.cpncob = -ER_NDR;
      return (ans);
    }
  fpn = dt->firstpn;
  destrel = (struct des_trel *) dt;
  sdes.s_df = destrel->f_df_tt.df_pnt;
  sdes.s_kn = kn;
  sdes.s_mfn = mfn;
  sdes.s_drctn = drctn;
  sdes.s_prdbl = prdbl;
  lpn = srt_trsort (&fpn, &destrel->f_df_tt, &sdes);
  n = lunt (&desnseg.tobtab, &desnseg.mtobnum, TOBPTD);
  size = dtrsize + destrel->f_df_tt.f_fn * rfsize;
  dtnew = (struct des_tob *) xmalloc (size + kn * size2b);
  *(desnseg.tobtab + n) = (char *) dtnew;
  bcopy ((char *) dt, (char *) dtnew, size);
  ((struct des_trel *) dtnew)->keysntr = kn;
  for (a = mfn, b = (u2_t *) ((char *)dtnew + size); kn != 0; kn--)
    *b++ = *a++;
  dtnew->prdt.prob = TREL;
  dtnew->prdt.prsort = SORT;
  dtnew->prdt.prdbl = prdbl;
  dtnew->prdt.prdrctn = *drctn;
  dtnew->osctob = 0;
  dtnew->firstpn = fpn;
  dtnew->lastpn = lpn;
  ((struct des_trel *)dtnew)->f_df_tt.df_pnt =
    (struct des_field *) ((char *)dtnew + dtrsize);
  
  ans.cpncob = OK;
  ans.idob.segnum = NRSNUM;
  ans.idob.obnum = n;
  return (ans);
}
struct ans_ctob
flsort (struct id_ob *pidtob, u2_t kn, u2_t *mfn, char *drctn, char prdbl)
{
  struct des_tob *dt, *dtnew;
  struct des_fltr *desfltr;
  struct d_r_bd *drbd;
  u2_t *a, *b, sn, size, fpn, lpn;
  i2_t n;
  struct ans_ctob ans;
  struct des_sort sdes;

  if (pidtob->segnum != NRSNUM)
    {
      ans.cpncob = -ER_NIOB;
      return (ans);
    }
  dt = (struct des_tob *) * (desnseg.tobtab + pidtob->obnum);
  if (dt->prdt.prob != FLTR)
    {
      ans.cpncob = -ER_NIOB;
      return (ans);
    }
  fpn = dt->firstpn;
  desfltr = (struct des_fltr *) dt;
  sn = desfltr->pdrtf->segnr;
  drbd = &desfltr->pdrtf->desrbd;
  sdes.s_df = desfltr->pdrtf->f_df_bt.df_pnt;
  sdes.s_kn = kn;
  sdes.s_mfn = mfn;
  sdes.s_drctn = drctn;
  sdes.s_prdbl = prdbl;
  lpn = srt_flsort (sn, &fpn, &desfltr->pdrtf->f_df_bt, &sdes);
  n = lunt (&desnseg.tobtab, &desnseg.mtobnum, TOBPTD);
  size = dflsize + desfltr->selszfl;
  dtnew = (struct des_tob *) xmalloc (size + kn * size2b);
  *(desnseg.tobtab + n) = (char *) dtnew;
  bcopy ((char *) dt, (char *) dtnew, size);
  ((struct des_fltr *) dtnew)->keysnfl = kn;
  for (a = mfn, b = (u2_t *) ((char *)dtnew + size); kn != 0; kn--)
    *b++ = *a++;
  dtnew->prdt.prsort = SORT;
  if (prdbl == 'd')
    dtnew->prdt.prdbl = NODBL;
  else
    dtnew->prdt.prdbl = DBL;
  if (*drctn == 'g')
    dtnew->prdt.prdrctn = GROW;
  else
    dtnew->prdt.prdrctn = DECR;
  dtnew->osctob = 0;
  dtnew->firstpn = fpn;
  dtnew->lastpn = lpn;
  ans.cpncob = OK;
  ans.idob.segnum = NRSNUM;
  ans.idob.obnum = n;
  return (ans);
}

