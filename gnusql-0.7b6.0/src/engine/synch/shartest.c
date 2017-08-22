/*  shartest.c - Check sharing with a locks qroup
 *               Kernel of GNU SQL-server. Synchronizer     
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

/* $Id: shartest.c,v 1.248 1998/08/18 05:36:43 kimelman Exp $ */

#include "dessnch.h"
#include "fdclsyn.h"

int
shartest (struct des_lock *anl, i4_t size, char *con, struct des_lock *bl)
     /* bl- begin, anl- end */
     /* 0- is shared with the locks qroup */
{
  struct des_lock *a;
  struct des_cp *cp;
  struct des_tran *tr, *bltran, *rolltr;
  char *b, *aold, *anew, *f;
  COST ccost, mincost;
  CPNM rollcpn;

  for (; bl != anl && bl != NULL; bl = bl->of)
    {
      if (anl->tran == bl->tran)
	continue;		/* from the same transaction */
      if (anl->lockin == 'm' || bl->lockin == 'm' || shtest1 (anl, con, size, bl) != 0)
	{
	  /* not shared */
	  tr = anl->tran;
	  for (bltran = bl->tran; bltran != NULL; bltran = bltran->pbltr)
	    if (bltran->pbltr == tr)
	      {			/* deadlock */
		rolltr = tr;
		cp = (struct des_cp *) tr->ptlb;
		rollcpn = cp->cpnum;
		mincost = tr->trcost - cp->cpcost;
		/* the second pass along the deadlock ring */
		for (bltran = bl->tran, a = bl; bltran != NULL; bltran = bltran->pbltr)
		  {
		    a = (struct des_lock *) ((char *) a + a->dls);
		    if (a <= (struct des_lock *) bltran->plcp)
		      {
			/* block lock is before a checkpoint */
			while (a->dls != cpsize)
			  a = (struct des_lock *) ((char *) a + a->dls);
			cp = ((struct des_cp *) a)->pdcp;
		      }
		    else
		      cp = bltran->plcp;
		    ccost = bltran->trcost - cp->cpcost;	/* a rollback cost */
		    if (ccost < mincost)
		      {
			mincost = ccost;
			rolltr = bltran;	/* a rollback victim */
			rollcpn = cp->cpnum;
		      }
		    if (bltran == tr)
		      break;
		    a = ((struct des_wlock *) bltran->pwlock)->Dup;
		  }
/*                  rolltr->pbltr=NULL;     break up the deadlock chain */
		answer_opusk (rolltr->idtr, rollcpn);
		if (rolltr == tr)
		  {
		    return (2);
		  }
		break;
	      }
	  if (anl != (a = tr->pwlock) && a != NULL)
	    {			/*if the lock isn't first wait*/
	      f = tr->firstfree;
	      aold = f - 1;
	      anew = aold + wlsize;
	      b = (char *) anl + locksize - 1;
	      for (; aold >= b;)
		*anew-- = *aold--;
	      anl->dls += wlsize;
	      f += wlsize;
	      tr->freelb -= wlsize;
	      a = (struct des_lock *) ((char *) anl + anl->dls);
	      for (; f > (char *) a; a = (struct des_lock *) ((char *) a + a->dls))
		refrem (a);
	    }
	  tr->pbltr = bl->tran;
	  ((struct des_wlock *) anl)->Dup = bl;
	  ((struct des_wlock *) anl)->Dqueue = bl->Ddown;
	  bl->Ddown = (struct des_wlock *) anl;
	  return (1);
	}
    }
  return (0);
}

void
refrem (struct des_lock *a)
{
  struct des_wlock *aw;
  
  lilore (a->rel, a, a, a);
  for (aw = a->Ddown; aw != NULL; aw = aw->Dqueue)
    aw->Dup = a;
}
