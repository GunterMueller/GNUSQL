/*
 *  rllbfn.c  - Rollback Functions 
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

/* $Id: rllbfn.c,v 1.252 1997/07/20 17:00:57 vera Exp $ */ 

#include "xmem.h"
#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"
#include "totdecl.h"
#include <assert.h>

extern struct d_r_t *firstrel;
extern i2_t maxscan;
extern char **scptab;

#define INDEX_PTR(table_tuple_ptr,table_descriptor_ptr) \
  (table_tuple_ptr + scscal (table_tuple_ptr) + drbdsize + table_descriptor_ptr->desrbd.fieldnum * rfsize)


/*
 * redo relation tuple deletion
 */

void
redo_dltn (struct full_des_tuple *dtuple, struct d_r_t *desrel, char *a)
{
  i4_t ordrn;

  orddel (dtuple);
  if (dtuple->rn_fdt != RDRNUM)
    {
      proind (ordindd, desrel, desrel->desrbd.indnum, a, &dtuple->tid_fdt);
      return;
    }
  /* else  for RDRNUM */
  ordrn = t4bunpack (a + scscal (a));
      
  /* find relation descriptor */
  for (desrel = firstrel; desrel != NULL; desrel = desrel->drlist)
    if (desrel->desrbd.relnum == ordrn)
      break;
      
  if (desrel == NULL)           /* if descrel not found  */
    return;                     /* just return - but it looks strange -- may be we should produce warning ?? */

  /* delete all indexes, defined on given table */
  while ( desrel->pid != NULL )
    {
      struct ldesind *di;
      di = desrel->pid;
      desrel->pid = di->listind;
      delscd (di->oscni, (char *) di);
      killind (di);
      xfree ((void *) di);
    }
      
  delscd (desrel->oscnum, (char *) desrel);
  /* remove desrel from list */
  if (desrel == firstrel)
    firstrel = desrel->drlist;
  else
    {
      struct d_r_t *dr;
      for (dr = firstrel; dr->drlist != desrel; dr = dr->drlist);
      assert( dr->drlist == desrel );
      dr->drlist = desrel->drlist;
    }
  xfree ((void *) desrel);
}

/*
 * redo tuple insertation
 */

void
redo_insrtn (struct full_des_tuple *dtuple, struct d_r_t *desrel,
             i2_t n, char *a)
{
  if (nordins (dtuple, CORT, MIN_TUPLE_LENGTH, n, a) != 0)
    doindir (dtuple, MIN_TUPLE_LENGTH, n, a);
  if (dtuple->rn_fdt != RDRNUM)
    proind (ordindi, desrel, desrel->desrbd.indnum, a, &dtuple->tid_fdt);
  else
    {
      i4_t ordrn;
      struct d_r_t *desrel1;
      struct ldesind *di;
      struct id_rel idr;
      
      ordrn = t4bunpack (a + scscal (a));
      idr.urn.segnum = dtuple->sn_fdt;
      idr.urn.obnum = ordrn;
      idr.pagenum = dtuple->tid_fdt.tpn;
      idr.index = dtuple->tid_fdt.tindex;
      desrel1 = crtfrd (&idr, a);
      for (di = desrel1->pid; di != NULL; di = di->listind)
	crindci (di);
    }
}

/*
 * redo "delete index" operation.
 *
 * DESCREL point to present relation structure b_e_f_o_r_e deletion operation
 * NEW_TABLE_TUPLE points to new descriptor tuple a_f_t_e_r modification
 *
 * this function compares index description prior and after modification and
 * executes all required operations to tranform index structure
 */

void
redo_dind (struct d_r_t *desrel, char *new_table_tuple)
{
  struct ldesind *di, *prdi;
  char *pnt;
  char *indexes_prt;
  u2_t kn, indn, i;

  indexes_prt =  INDEX_PTR(new_table_tuple,desrel);
  
  indn = desrel->desrbd.indnum - 1; /* (++) this function expect deletion of only one index */
  
  for (prdi = NULL, di = desrel->pid ;
       di != NULL;
       prdi = di, di = (di?di->listind:desrel->pid))
    {                                             /* for every existent index   */
      pnt = indexes_prt;                          
      for (i = 0; i < indn; i++)                  /* scan  new table tuple      */
	{ 
          struct des_index desind;
          
	  BUFUPACK(pnt,desind);                   /* unpack it indexes descr    */
	  kn = desind.kifn & ~UNIQ & MSK21B;      /* calculate number of keys   */
	  pnt += kn * size2b;                     /* and skip keys' descriptors */
	  if (desind.unindex == di->ldi.unindex)  /* if existent index is in    */
            break;                                /* new tuple check another one*/
	}
      if (i < indn)                               /* if current index has not   */
        continue;                                 /* been removed goto next one */
      /* current index WAS removed -- redo index deletion                       */
      /* delete index from table list */
      if (prdi == NULL)
	desrel->pid = di->listind;
      else
	prdi->listind = di->listind;
      desrel->desrbd.indnum--;
      delscd (di->oscni, (char *) di);
      killind (di);
      xfree ((void *) di);
      di = prdi;
      /* here we have done redo of one index deletion. */
      break; /* this function expect deletion of only one index - see (++) above */ 
    }
}

/*
 * redo "create index" operation.
 *                                unclear logic ??? /mk
 *
 */

void
redo_cind (  struct d_r_t *desrel, char *a)
{
  struct ldesind *di;
  char *tuple;
  struct des_index desind;
  u2_t kn = 0, size = 0, indn, i;
  
  tuple = a;
  a = INDEX_PTR(tuple,desrel);
  indn = desrel->desrbd.indnum + 1; /* we expect creation of only one index */
  assert( indn >0);
  for (i = 0; i < indn; i++)
    {
      BUFUPACK(a,desind);
      kn = desind.kifn & ~UNIQ & MSK21B;
      size = kn * size2b;
      for (di = desrel->pid; di != NULL; di = di->listind)
	if (desind.unindex == di->ldi.unindex)
          break;
        else
          a += size;  /* ???/mk */
    }
  if ((kn % 2) != 0)
    size += size2b;
  di = (struct ldesind *) xmalloc (size + ldisize + rfsize);
  bcopy (a, (char *) (di + 1), kn * size2b);
  di->ldi = desind;
  crtid (di, desrel);
  crindci (di);
  desrel->desrbd.indnum++;
  fill_ind (desrel, di);
}

u2_t
get_placement (u2_t sn, struct des_tid *tid, struct des_tid *ref_tid)
{
  char *tuple, *asp = NULL;
  u2_t corsize;
  struct A pg;

  while ((asp = getpg (&pg, sn, tid->tpn, 's')) == NULL);
  GET_IND_REF();  /* indirect reference */
  putpg (&pg, 'n');
  return (corsize);
}

void
delscd (u2_t n, char *a)
{
  char *s;
  i4_t k;

  for (k = 0; n != 0 && k < maxscan; k++)
    for (; (s = *(scptab + k)) != NULL; k++)
      if (a == ((struct d_mesc *) s)->pobsc)
	{
	  xfree ((void *) s);
	  n--;
	}
}

void
fill_ind (struct d_r_t *desrel, struct ldesind *desind)
{
  char *asp = NULL;
  u2_t *ai, *ali, ind, sn, pn, size;
  struct A inpg;
  struct d_sc_i *scind;
  struct ldesscan *disc;
  i2_t num;
  i4_t rep;
  struct des_tid tid;
  struct ldesind *di, *prevdi;
  char mas[BD_PAGESIZE];
  struct id_ob fullrn;

  sn = desrel->segnr;
  fullrn.segnum = sn;
  fullrn.obnum = desrel->desrbd.relnum;
  scind = rel_scan (&fullrn, (char *) desrel,
                    &num, 0, NULL, NULL, 0, 0, NULL);
  disc = &scind->dessc;
  rep = fgetnext (disc, &pn, &size, FASTSCAN);
  while (rep != EOI)
    {
      while ((asp = getpg (&inpg, sn, pn, 's')) == NULL);
      ai = (u2_t *) (asp + phsize);
      ali = ai + ((struct page_head *) asp)->lastin;
      tid.tpn = pn;
      for (ind = 0; ai <= ali; ai++, ind++)
	if (*ai != 0 && CHECK_PG_ENTRY(ai))
	  {
	    tid.tindex = ind;
	    keyform (desind, mas, asp + *ai);
	    ordindi (desind, mas, &tid);
	  }
      putpg (&inpg, 'n');
      rep = getnext (disc, &pn, &size, FASTSCAN);
    }
  delscan (num);
  if ((di = desrel->pid) == NULL)
    desrel->pid = desind;
  else
    {
      do
	{
	  prevdi = di;
	  di = di->listind;
	}  
      while( di != NULL);
      prevdi->listind = desind;
    }
  desind->listind = NULL;
}

struct d_r_t *
crtfrd (struct id_rel *pidrel, char *tuple)
{
  struct d_r_t *desrel;

  desrel = crtrd (pidrel, tuple);
  if (desrel->desrbd.indnum != 0)
    crt_all_id (desrel, tuple);
  return (desrel);
}

void
crt_all_id (struct d_r_t *desrel, char *a)
{
  u2_t i, indn, size;
  u2_t kn1, size1;
  struct des_index cdi;
  struct ldesind *di;

  a += scscal (a) + drbdsize + desrel->desrbd.fieldnum * rfsize;
  indn = desrel->desrbd.indnum;
  for (i = 0; i < indn; i++)
    {
      dindunpack (&cdi, a);
      a += dinsize;
      kn1 = cdi.kifn & ~UNIQ & MSK21B;
      size = kn1 * size2b;
      size1 = size;
      if (kn1 % 2 != 0)
	size1 += size2b;
      size1 += rfsize;
      di = (struct ldesind *) xmalloc (ldisize + size1);
      di->ldi = cdi;
      bcopy (a, (char *) (di + 1), size);
      a += size;
      di->listind = desrel->pid;
      desrel->pid = di;
      crtid (di, desrel);
    }
}

void
dindunpack (struct des_index *di, char *pnt)
{
/*
    di->unindex=t4bunpack(pnt); pnt+=size4b;
    di->rootpn=t2bunpack(pnt); pnt+=size2b;
    di->kifn=t2bunpack(pnt); pnt+=size2b;
    */
  bcopy (pnt, (char *) di, dinsize);
}
