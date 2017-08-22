/*  incrs.c  - Transaction locks region extension
 *             Kernel of GNU SQL-server. Synchronizer    
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

/* $Id: incrs.c,v 1.247 1998/08/18 05:36:42 kimelman Exp $ */

#include "xmem.h"
#include "dessnch.h"
#include "fdclsyn.h"

#include <assert.h>

extern int reg_tran_size;


 /*********************************/
 /* Transaction locks region copy */
 /*********************************/

static void
lbrem (struct des_tran *tr, char *newadd, i4_t N_new)
{
  struct des_lock *a, *a1, *end_of_locks;
  struct des_rel *r;
  struct des_wlock *aw, *oldadd;
  char *b, *oldb, *ff;
  i4_t N, N_old, adls;

  ff = tr->firstfree;
  oldb = tr->ptlb;		/* an old place */
  tr->ptlb = newadd;
  tr->plcp = (struct des_cp *) newadd;
  N_old = ff - oldb;
  bcopy (oldb, newadd, N_old);
  N = newadd - oldb;
  if (tr->pwlock != NULL)
    tr->pwlock = (struct des_lock *) ((char *) tr->pwlock + N);
  tr->freelb = N_new - N_old;
  tr->firstfree = newadd + N_old;
  a = (struct des_lock *) (newadd + cpsize);	/* The first lock address */
  end_of_locks = (struct des_lock *) tr->firstfree;  
  for (; a < end_of_locks; a = (struct des_lock *) ((char *) a + adls))
    {
      adls = a->dls;
      assert (adls % sizeof (i4_t) == 0);
      if (adls == cpsize)
	{
	  ((struct des_cp *) a)->pdcp = tr->plcp;
	  tr->plcp = (struct des_cp *) a;
	}
      else
	{
	  r = a->rel;
	  b = (char *) a->of;
	  if (b != NULL)
	    {
	      if (b >= oldb && b < ff)
		a->of = (struct des_lock *) ((char *) a->of + N);
	      else
		a->of->ob = a;
	    }
	  else
	    r->rob = a;		/* if the lock is last */
	  b = (char *) a->ob;
	  if (b != NULL)
	    {
	      if (b >= oldb && b < ff)
		a->ob = (struct des_lock *) ((char *) a->ob + N);
	      else
		a->ob->of = a;
	    }
	  else
	    r->rof = a;		/* if the lock is first in the list */
	  for (aw = a->Ddown; aw != NULL; aw = aw->Dqueue)
	    aw->Dup = a;
	  if (a == tr->pwlock)
	    {			/* if the lock is first wait */
	      oldadd = (struct des_wlock *) (oldb + ((char *) a - newadd));
	      aw = (struct des_wlock *) a;
	      a1 = aw->Dup;	/* a block lock address */
	      if (a1->Ddown == oldadd)
		a1->Ddown = aw;
	      else
		{
		  aw = a1->Ddown;
		  while (aw->Dqueue != oldadd)
		    aw = aw->Dqueue;
		  aw->Dqueue = (struct des_wlock *) a;
		}
	    }
	}
    }
}

void
increase (struct des_tran *tr)
     /* increase Transaction locks region */
{
  char *oldptlb, *newadd;
  int n;

  oldptlb = tr->ptlb;
  n = tr->firstfree - tr->ptlb + tr->freelb + reg_tran_size;
  newadd = (char *) xmalloc (n);
  lbrem (tr, newadd, n);
  xfree ((void *) oldptlb);
}

