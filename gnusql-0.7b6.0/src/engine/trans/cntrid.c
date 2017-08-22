/*
 *  cntrid.c  - Relation Identificator Test
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

/* $Id: cntrid.c,v 1.251 1998/08/21 00:28:15 kimelman Exp $ */

#include "destrn.h"
#include "sctp.h"
#include "strml.h"
#include "../synch/sctpsyn.h"
#include "fdcltrn.h"

extern struct d_r_t *firstrel;
extern struct ldesind **TAB_IFAM;
extern struct ADBL adlj;

#define CHECK_IT(cmd) { int rc; rc = cmd ; if(rc) return rc; }

static int
check_seg_id(u2_t sn,u2_t *pn)
{
  int rep;
  while ((rep = BUF_lockpage (sn, *pn, 's')) == -1);
  if (rep == -ER_NO_SUCH_SEG)
    return (-ER_NO_SUCH_SEG);
  else
    BUF_unlock (sn, 1, pn);
  return 0;
}

static int
table_id_synlock(u2_t sn,u2_t size,char *lc/*lc[32]*/)
{
  CPNM cpn;
  struct id_rel idr;
  idr.urn.segnum = sn;
  idr.urn.obnum = RDRNUM;
  if (size > SZSNBF)
    error ("TR.synind: SYN's buffer is too small");
  cpn = sn_lock (&idr, 't', lc, size);
  if (cpn != 0)
    {
      rllbck (cpn, adlj);
      return (cpn);
    }
  return 0;
}

#define CHECK_SEG_ID(sn,pn) CHECK_IT(check_seg_id(sn,&pn))
#define TABLE_ID_SYNLOCK(sn,size,lc) CHECK_IT(table_id_synlock(sn,size,lc))

#define CHECK_TABLE_ID(sn,pn,pg,pg_table,a,pidrel) CHECK_IT(check_table_id(sn,&pn,&pg,&pg_table,&a,pidrel))

static int
check_table_id(u2_t sn,u2_t *pn,struct A *pg,struct A *pg_table,
               char **aptr, struct id_rel *pidrel)
{
  i4_t rnr;
  char *asp, *begagr, *loc, *a;
  u2_t *ai;
  unsigned char t;
  char key[size4b + 1];
  
  a = key;
  *a++ = BITVL(0) | EOSC;
  rnr = RDRNUM;
  t4bpack (rnr, a);
  tab_difam (sn);
m1:
  a = icp_lookup (pg, TAB_IFAM[sn], key, (char *) pn, size2b, &begagr, &loc);
  putwul (pg, 'n');
  if (a == NULL)
    {
      BUF_unlock (sn, 1, &(pg->p_pn));
      return (-ER_NDR);
    }
  if ((asp = getpg (pg_table, sn, *pn, 's')) == NULL)
    {
      BUF_unlock (sn, 1, &pg->p_pn);
      goto m1;
    }
  ai = (u2_t *) (asp + phsize) + pidrel->index;
  if (*ai == 0)
    {
      BUF_unlock (sn, 1, &pg->p_pn);
      putpg (pg_table, 'n');
      return (-ER_NDR);
    }
  a = asp + *ai;
  t = *a & MSKCORT;
  if (t == CREM || t == IDTR)
    {
      BUF_unlock (sn, 1, &pg->p_pn);
      putpg (pg_table, 'n');
      return (-ER_NDR);
    }
  if (t == IND)
    {
      u2_t pn2, ind2;
      ind2 = t2bunpack (a + 1);
      pn2 = t2bunpack (a + 1 + size2b); 
      putpg (pg_table, 'n');
      while ((asp = getpg (pg_table, sn, pn2, 's')) == NULL);
      ai = (u2_t *) (asp + phsize) + ind2;
      assert (*ai != 0);
      a = asp + *ai;
    }
  if(aptr)
    *aptr=a;
  return 0;
}

CPNM
contir (struct id_rel *pidrel, struct d_r_t **desrel)
{
  char *a, *asca;
  char mch[8], lc[32];
  i4_t rn, ast;
  u2_t sn, pn, size;
  struct A pg, pg_table;
  
  rn = pidrel->urn.obnum;
  for (*desrel = firstrel; *desrel != NULL; *desrel = (*desrel)->drlist)
    if ((*desrel)->desrbd.relnum == rn)
      return (OK);
  sn = pidrel->urn.segnum;
  pn = pidrel->pagenum;
  CHECK_SEG_ID(sn,pn);
  
  a = lc + size2b;
  t4bpack (rn, a);
  a += size4b;
  ast = 1;
  asca = mch;
  sct (&asca, ast++, S_D);
  sct (&asca, ast++, EQ);
  sct (&asca, ast, ENDSC);
  a += size2b;
  for (; asca >= mch;)
    *a++ = *asca--;
  size = a - lc;
  t2bpack (size, lc);
  TABLE_ID_SYNLOCK(sn,size,lc);
  CHECK_TABLE_ID(sn,pn,pg,pg_table,a,pidrel);
  if ((*desrel = crtrd (pidrel, a)) == NULL)
    {
      BUF_unlock (sn, 1, &pg.p_pn);
      putpg (&pg_table, 'n');
      return (-ER_NDR);
    }
  putpg (&pg_table, 'n');
  BUF_unlock (sn, 1, &pg.p_pn);
  return (OK);
}

CPNM
cont_fir (struct id_rel *pidrel, struct d_r_t **desrel)
{
  i4_t rn, scsz, ast, n;
  u2_t sn, pn, size;
  char *a, *asca;
  char lc[SZSNBF];
  struct A pg, pg_table;
  char mch[BD_PAGESIZE];
  
  rn = pidrel->urn.obnum;
  for (*desrel = firstrel; *desrel != NULL; *desrel = (*desrel)->drlist)
    if ((*desrel)->desrbd.relnum == rn)
      break;
  if (*desrel != NULL && (*desrel)->pid != NULL)
    return (OK);
  if (*desrel != NULL && (*desrel)->desrbd.indnum == 0)
    return (OK);
  sn = pidrel->urn.segnum;
  pn = pidrel->pagenum;
  CHECK_SEG_ID(sn,pn);
  
  a = lc + size2b;
  t4bpack (rn, a);
  a += size4b;
  ast = 1;
  asca = mch;
  sct (&asca, ast++, S_D);
  sct (&asca, ast++, EQ);
  for (n = 0; n < 3; n++)
    sct (&asca, ast++, NOTLOCK);
  sct (&asca, ast++, S_S);
  sct (&asca, ast++, NOTLOCK);
  sct (&asca, ast++, S_S);
  sct (&asca, ast, ENDSC);
  if (ast % 2 == 0)
    asca--;
  scsz = asca + 1 - mch;
  size = a - lc - size2b;
  n = (size + scsz) % sizeof (i4_t);
  if (n != 0)
    n = sizeof (i4_t) - n;
  a += n;
  for (; asca >= mch;)
    *a++ = *asca--;
  size = a - lc;
  t2bpack (size, lc);
  TABLE_ID_SYNLOCK(sn,size,lc);
  if (*desrel == NULL)
    {
      CHECK_TABLE_ID(sn,pn,pg,pg_table,a,pidrel);
      *desrel = crtfrd (pidrel, a);
      if (*desrel == NULL)
	{
	  BUF_unlock (sn, 1, &pg.p_pn);
	  putpg (&pg_table, 'n');
	  return (-ER_NDR);
	}
    }
  if ((*desrel)->pid == NULL && (*desrel)->desrbd.indnum != 0)
    crt_all_id (*desrel, a);
  putpg (&pg_table, 'n');
  BUF_unlock (sn, 1, &pg.p_pn);
  return (OK);
}

CPNM
tabcl (struct id_rel *pidrel, u2_t fln, u2_t * fmn)
{
  struct des_field *df;
  struct d_r_t *desrel;
  CPNM cpn;

  if ((cpn = contir (pidrel, &desrel)) != OK)
    return (cpn);
  if (fln > desrel->desrbd.fieldnum)
    return (-ER_NCR);
  df = (struct des_field *) (desrel + 1);
  for (; fln != 0; df++, fln--)
    *fmn++ = df->field_type;
  return (OK);
}

CPNM
cont_id (struct id_ind *pidind, struct d_r_t **desrel, struct ldesind **di)
{
  struct id_rel *pidrel;
  i4_t rn, index;
  struct ldesind *cdi;
  CPNM cpn;
                                
  pidrel = &pidind->irii;
  rn = pidrel->urn.obnum;
  index = pidind->inii;
  for (*desrel = firstrel; *desrel != NULL; *desrel = (*desrel)->drlist)
    if ((*desrel)->desrbd.relnum == rn)
      {
	for (*di = (*desrel)->pid; *di != NULL; *di = (*di)->listind)
	  if ((*di)->ldi.unindex == index)
	    return (OK);
      }
  if ((cpn = cont_fir (pidrel, desrel)) != OK)
    return (cpn);

  for (cdi = (*desrel)->pid; cdi != NULL; cdi = cdi->listind)
    {
      if (cdi->ldi.unindex == index)
	break;
    }
  if (cdi == NULL)
    return (-ER_NDI);
  *di = cdi;
  return (OK);
}

