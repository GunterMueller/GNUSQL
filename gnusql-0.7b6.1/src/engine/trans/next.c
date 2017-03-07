/*
 *  next.c  - Find a row
 *            Kernel of GNU SQL-server  
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

/* $Id: next.c,v 1.252 1998/09/29 21:25:37 kimelman Exp $ */

#include <assert.h>

#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"
#include "xmem.h"
#include "global.h"

extern char **scptab;
extern i2_t maxscan;

static struct fun_desc_fields *desf;
static char *arrpnt[BD_PAGESIZE];
static u2_t arrsz[BD_PAGESIZE];

#define PUT_VALUE(fn, adf)                                       \
{                                                                \
    if ((pnt = arrpnt[fn]) != NULL)	/* a value is present */ \
      {                                                          \
        *sc++ = 1;                                               \
        if ((type = adf->field_type) != TCH && type != TFL)      \
          {                                                      \
            size += size2b;                                      \
            t2bpack (adf->field_size, values);                   \
            values += size2b;                                    \
          }                                                      \
        sz = arrsz[fn];                                          \
        size += sz;                                              \
        bcopy (pnt, values, sz);                                 \
        values += sz;                                            \
      }                                                          \
    else                                                         \
      *sc++ = 0;                                                 \
}

static int
mem_tuple_values_DU (data_unit_t *colval, u2_t fn, u2_t type)
{
  char *pnt, nl_fl;
  
  if ((pnt = arrpnt[fn]) != NULL)	/* a value is present */ 
    {                                                        
      nl_fl = REGULAR_VALUE;                                 
      if (type == TCH || type == TFL)    
        pnt += size2b;                                       
    }                                                        
  else                                                       
    nl_fl = NULL_VALUE;                                      
  return (mem_to_DU (nl_fl, colval->type.code, arrsz[fn], pnt, colval));
}

static void
selfld (data_unit_t **colval, u2_t fln, u2_t *fl)
{
  u2_t i, fn;
  struct des_field * adf, *df;
  
  df = desf->df_pnt;
  for (i = 0; i < fln; i++)
    {
      fn = fl[i];
      adf = df + fn;   
      mem_tuple_values_DU (colval[i], fn, adf->field_type);
    }
}

static int
fndcort (struct A *pg, char *tuple, u2_t slsz, char *selc)
{
  u2_t tuple_size;
  unsigned char t;
  struct A inpage;

  t = *tuple & MSKCORT;
  if (t == CREM || t == IDTR)
    return (-ER_NCR);
  if (t == IND)
    {
      u2_t pn, ind;
      IND_REF(inpage,pg->p_sn,tuple);
    }
  tuple_size = tstcsel (desf, slsz, selc, tuple, arrpnt, arrsz);
  if (tuple_size != 0)
    {
      if (t == IND)
	{
	  putpg (pg, 'n');
	  *pg = inpage;
	}
      return (OK);
    }
  else
    {
      if (t == IND)
	putpg (&inpage, 'n');
      return (-ER_NCR);
    }
}

int
next (i4_t scnum, data_unit_t **colval)
{
  struct d_mesc *scpr;
  i4_t sctype;
  struct d_r_t *desrel;
  u2_t pn, ind, *ai, *ali, sn, cpn, slsz;
  struct des_tid tid;
  char *asp = NULL, *selc;
  u2_t *fl, fln, fmn;
  struct A pg;

  if ( scnum > maxscan)
    return -ER_NDSC;
  scpr = (struct d_mesc *) * (scptab + scnum);
  if (scpr == NULL)
    return -ER_NDSC;
  fln = scpr->fnsc;
  fmn = scpr->fmnsc;
  if ((sctype = scpr->obsc) == SCR)
    {				/* relation scan */
      struct d_sc_i *scind;
      struct ldesscan *desscan;
      u2_t size;
      
      scind = (struct d_sc_i *) scpr;
      desscan = &scind->dessc;
      desrel = (struct d_r_t *) scpr->pobsc;
      sn = desrel->segnr;
      desf = &desrel->f_df_bt;      
      fl = (u2_t *) (scind + 1);
      slsz = t2bunpack ((char *) (fl + fln + fmn));
      selc = (char *) (fl + fln + fmn + 1);
      pn = desscan->ctidi.tpn;
      if (pn == (u2_t) ~ 0)
        return -ER_EOSCAN;
      ind = desscan->ctidi.tindex;
      if (scpr->prcrt != 0)
	ind += 1;
      do
	{
	  while ((asp = getpg (&pg, sn, pn, 's')) == NULL);
	  ai = (u2_t *) (asp + phsize);
	  ali = ai + ((struct page_head *) asp)->lastin;
	  for (ai += ind; ai <= ali; ai++, ind++)
            if (*ai != 0 && CHECK_PG_ENTRY(ai) &&
                fndcort (&pg, asp + *ai, slsz, selc) == OK)
              {
                selfld (colval, fln, fl);
                putpg (&pg, 'n');
                desscan->ctidi.tpn = pn;
                desscan->ctidi.tindex = ind;
                scpr->prcrt = 1;
                return OK;
              }
	  putpg (&pg, 'n');
	  ind = 0;
	}
      while (getnext (desscan, &pn, &size, SLOWSCAN) != EOI);
      desscan->ctidi.tpn = (u2_t) ~ 0;
    }
  else if (sctype == SCTR)
    {
      struct d_sc_r *screl;
      struct des_trel *destrel;
      char tmp_buff[BD_PAGESIZE];
      
      screl = (struct d_sc_r *) scpr;
      destrel = (struct des_trel *) scpr->pobsc;
      desf = &destrel->f_df_tt;     
      fl = (u2_t *) (screl + 1);
      slsz = t2bunpack ((char *) (fl + fln + fmn));
      selc = (char *) (fl + fln + fmn + 1);
      pn = screl->curtid.tpn;
      if (pn == (u2_t) ~ 0)
        return -ER_EOSCAN;
      ind = screl->curtid.tindex;
      if (scpr->prcrt != 0)
	ind += 1;
      asp = tmp_buff;
      for (; pn != (u2_t) ~ 0; ind = 0)
	{
	  read_tmp_page (pn, asp);
	  ai = (u2_t *) (asp + phtrsize);
	  ali = ai + ((struct p_h_tr *) asp)->linptr;
	  for (ai += ind; ai <= ali; ai++, ind++)
            if (*ai != 0 &&
                tstcsel (desf, slsz, selc, asp + *ai, arrpnt, arrsz) != 0)
              {
                selfld (colval, fln, fl);
                screl->curtid.tpn = pn;
                screl->curtid.tindex = ind;
                scpr->prcrt = 1;
                return OK;
              }
	  pn = ((struct listtob *) asp)->nextpn;
	}
      screl->curtid.tpn = (u2_t) ~ 0;
    }
  else if (sctype == SCI)
    {				/* index scan */
      struct d_sc_i *scind;
      struct ldesscan *desscan;
      
      scind = (struct d_sc_i *) scpr;
      desscan = &scind->dessc;
      desrel = desscan->pdi->dri;
      sn = desrel->segnr;
      fl = (u2_t *) (scind + 1);
      desf = &desrel->f_df_bt;
      slsz = t2bunpack ((char *) (fl + fln + fmn));
      selc = (char *) (fl + fln + fmn + 1);
      tid = desscan->ctidi;
      if (tid.tpn == (u2_t) ~ 0)
        return -ER_EOSCAN;
      if ((scpr->prcrt == 0) || ind_tid (desscan, &tid, SLOWSCAN) == OK)
	{
	  cpn = tid.tpn;
	  while ((asp = getpg (&pg, sn, cpn, 's')) == NULL);
	  ai = (u2_t *) (asp + phsize) + tid.tindex;
	  if (*ai != 0 && fndcort (&pg, asp + *ai, slsz, selc) == OK)
	    {
              selfld (colval, fln, fl);
	      putpg (&pg, 'n');
	      desscan->ctidi = tid;
	      scpr->prcrt = 1;
	      return OK;
	    }
	  for (; ind_tid (desscan, &tid, SLOWSCAN) != EOI;)
	    {
	      if ((pn = tid.tpn) != cpn)
		{
		  putpg (&pg, 'n');
		  while ((asp = getpg (&pg, sn, pn, 's')) == NULL);
		  cpn = pn;
		}
	      ai = (u2_t *) (asp + phsize) + tid.tindex;
	      if (*ai != 0 && fndcort (&pg, asp + *ai, slsz, selc) == OK)
		{
                  selfld (colval, fln, fl);
		  putpg (&pg, 'n');
		  desscan->ctidi = tid;
		  scpr->prcrt = 1;
		  return OK;
		}
	    }
          putpg (&pg, 'n');
	} /* if */
      desscan->ctidi.tpn = (u2_t) ~ 0;
    }
  else
    {				/* filter scan */
      struct d_sc_f *scfltr;
      char *aspf;
      u2_t nxtpn, off, fpn;
      char tmp_buff[BD_PAGESIZE];
      
      scfltr = (struct d_sc_f *) scpr;
      desrel = ((struct des_fltr *) scpr->pobsc)->pdrtf;
      sn = desrel->segnr;
      fl = (u2_t *) (scfltr + 1);
      desf = &desrel->f_df_bt;
      slsz = t2bunpack ((char *) (fl + fln + fmn));
      selc = (char *) (fl + fln + fmn + 1);
      pn = (u2_t) ~ 0;
      aspf = tmp_buff;
      for (fpn = scfltr->pnf, nxtpn = fpn; fpn != (u2_t) ~ 0; fpn = nxtpn)
	{
	  read_tmp_page (fpn, aspf);
	  for (off = scfltr->offf; off < BD_PAGESIZE; off += size2b)
	    {
	      tid = *(struct des_tid *) (aspf + scfltr->offf);
	      nxtpn = ((struct listtob *) aspf)->nextpn;
	      if ((cpn = tid.tpn) != pn)
		{
                  if (pn != (u2_t) ~ 0)
                    putpg (&pg, 'n');
		  while ((asp = getpg (&pg, sn, cpn, 's')) == NULL);
		}
	      ai = (u2_t *) (asp + phsize) + tid.tindex;
	      if (*ai != 0 && fndcort (&pg, asp + *ai, slsz, selc) == OK)
		{
                  selfld (colval, fln, fl);
		  putpg (&pg, 'n');
		  scfltr->pnf = pn;
		  scfltr->offf = off;
		  scpr->prcrt = 1;
		  return OK;
		}
	    }
	}
      scfltr->pnf = (u2_t) ~ 0;
    }
  scpr->prcrt = 0;
  return -ER_EOSCAN;
}

int
readrow (i4_t scnum, i4_t fln, u2_t * fl, data_unit_t **colval)
{
  struct d_mesc *scpr;
  struct d_r_t *desrel;
  u2_t *ai;
  i4_t sctype;
  struct des_tid tid;
  char *asp = NULL;
  struct A pg;

  scpr = (struct d_mesc *) * (scptab + scnum);
  if (scnum >= maxscan || scpr == NULL)
    return -ER_NCF;
  if (scpr->prcrt == 0)
    return -ER_NCR;
  if ((sctype = scpr->obsc) == SCTR)
    {
      struct d_sc_r *screl;
      struct des_trel *destrel;
      char tmp_buff[BD_PAGESIZE];
      
      screl = (struct d_sc_r *) scpr;
      destrel = (struct des_trel *) scpr->pobsc;
      desf = &destrel->f_df_tt;      
      tid = screl->curtid;
      asp = tmp_buff;
      read_tmp_page (tid.tpn, asp);
      ai = (u2_t *) (asp + phtrsize) + tid.tindex;
      tuple_break (asp + *ai, arrpnt, arrsz, desf);
      selfld (colval, fln, fl);
      return OK;
    }
  else if (sctype == SCR)
    {				/* relation scan */
      struct d_sc_i *scind;
      struct ldesscan *desscan;
      
      scind = (struct d_sc_i *) scpr;
      desscan = &scind->dessc;
      desrel = (struct d_r_t *) scpr->pobsc;
      tid = desscan->ctidi;
    }
  else if (sctype == SCI)
    {				/* index scan */
      struct d_sc_i *scind;
      struct ldesscan *desscan;
      
      scind = (struct d_sc_i *) scpr;
      desscan = &scind->dessc;
      desrel = desscan->pdi->dri;
      tid = desscan->ctidi;
    }
  else
    {				/* filter scan */
      struct d_sc_f *scfltr;
      char tmp_buff[BD_PAGESIZE];
      
      scfltr = (struct d_sc_f *) scpr;
      desrel = ((struct des_fltr *) scpr->pobsc)->pdrtf;
      read_tmp_page (scfltr->pnf, tmp_buff);
      tid = *(struct des_tid *) (tmp_buff + scfltr->offf);
    }
  desf = &desrel->f_df_bt;
  while ((asp = getpg (&pg, desrel->segnr, tid.tpn, 's')) == NULL);
  ai = (u2_t *) (asp + phsize) + tid.tindex;
  if (fndcort (&pg, asp + *ai, 0, NULL) != OK)
    return -ER_NCR;
  selfld (colval, fln, fl);
  putpg (&pg, 'n');
  return OK;
}
