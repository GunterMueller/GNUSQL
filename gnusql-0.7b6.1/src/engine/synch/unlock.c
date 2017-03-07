/*
 * unlock.c - Unlock, Kernel of GNU SQL-server. Synchronizer
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

/* $Id: unlock.c,v 1.250 1998/09/29 21:25:18 kimelman Exp $ */

#include "setup_os.h"
#include "dessnch.h"
#include "fdclsyn.h"
#include <assert.h>

extern struct des_rel *firstrel;

static void
delsrd (struct des_rel *r)	/* To delete the relation descriptor */
{
  if (r->brellist == NULL)
    firstrel = r->frellist;
  else
    r->brellist->frellist = r->frellist;
  if (r->frellist != NULL)
    r->frellist->brellist = r->brellist;
  xfree ((void *) r);
}

void
unlock (struct des_tran *tr, struct des_lock *alock)
{
  struct des_lock *a, *bl, *pa, *ff, *wff;
  struct des_wlock *a1, *aw;
  struct des_rel *r;
  i4_t size, k;
  u2_t pn, ind, *a2b;
  char *ab1, *ab2;
  COST newcost;
  
  PRINTF(("run unlock\n"));
  
  ff = (struct des_lock *) tr->firstfree;
  for (a = alock; a < ff;)
    {				/* the first pass along unlock locks */
      if ((k = a->dls) != cpsize)
	{
	  /* remove the lock from the relation locks list */
          PRINTF(("before lilore: rel=%p, lock=%p, flock=%p,block=%p\n",a->rel,a,a->of,a->ob));
          
	  lilore (a->rel, a, a->of, a->ob);
          
          PRINTF(("after lilore\n"));
          
	  if (a->lockin == 'm' && (pa = a->rel->rof) != NULL && a < a->tran->pwlock)
	    {
	      r = a->rel;
	      delsrd (r);
	      a2b = (u2_t *) ((char *) a + locksize);
	      pn = *a2b++;
	      ind = *a2b;
	      r = crtsrd (tr->idtr,&r->idrel, pn, ind);
	      r->rof = pa;
	    }
	}
      a = (struct des_lock *) ((char *) a + k);
    }
  if ((aw = (struct des_wlock *) tr->pwlock) != NULL)
    {
      bl = aw->Dup;		/* the block lock address */
      if (bl->Ddown == aw)
        bl->Ddown = aw->Dqueue;
      else
        {
          for (a1 = bl->Ddown; a1 != NULL; a1 = a1->Dqueue)
            if (a1->Dqueue == aw)
              break;
          assert(a1); /* this lock must exist */
          a1->Dqueue = aw->Dqueue;
        }
    }
  for (; alock < ff;)
    {				/* the second pass along unlock locks */
      if ((k = alock->dls) != cpsize)
	/* the cycle on suns in the block queue */
	for (aw = alock->Ddown; aw != NULL; aw = a1)
	  {
	    a1 = aw->Dqueue;
	    size = aw->l.dls - wlocksize;
	    ab1 = (char *) aw + wlocksize;
	    if (alock->ob == NULL)
	      bl = alock->rel->rof;
	    else
	      bl = alock->ob->of;
	    if (shartest ((struct des_lock *) aw, size, ab1, bl) != 0)
              continue;	/* not shared */
	    tr = aw->l.tran;
            /*	    f = tr->firstfree;*/
	    ab1 = (char *) aw + locksize;
            ab2 = (char *) aw + wlocksize;
	    newcost = aw->newcost;
            size = tr->firstfree - ab2;
            bcopy (ab2, ab1, size);
	    aw->l.dls -= wlsize;
	    tr->freelb += wlsize;
	    tr->firstfree -= wlsize;
	    wff = (struct des_lock *) tr->firstfree;
	    tr->pbltr = NULL;
	    a = (struct des_lock *) ((char *) aw + aw->l.dls);
	    for (; a < wff; a = (struct des_lock *) ((char *) a + a->dls))
	      refrem (a);
	    for (; a < wff;)
	      {
		size = a->dls - locksize;
		if (shartest (a, size, (char *) a + locksize, bl) != 0)
		  {
		    /* a wait lock isn't satisfyed */
		    ((struct des_wlock *) a)->newcost = newcost;
		    return;
		  }
		/* a wait lock is satisfyed */
		a = (struct des_lock *) ((char *) a + a->dls);
	      }
	    tr->pwlock = NULL;
	    tr->trcost = newcost;
	    answer_opusk (tr->idtr, (CPNM) 0);
	  }
      alock = (struct des_lock *) ((char *) alock + k);
    }
}

void
lilore (struct des_rel *r, struct des_lock *a, struct des_lock *f, struct des_lock *b)
{
  PRINTF(("lilore: rel=%p, lock=%p, flock=%p,block=%p\n",r,a,f,b));
  if (a->ob == NULL)
    r->rof = f;			/* if the lock is first in the list */
  else
    a->ob->of = f;
  if (a->of == NULL)
    r->rob = b;			/* if the lock is last */
  else
    a->of->ob = b;
  PRINTF(("lilore: exit \n"));
}
