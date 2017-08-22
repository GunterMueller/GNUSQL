/*  dlock.c  - Unlock narrow locks
 *             Kernel of GNU SQL-server. Synchronizer    
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

/* $Id: dlock.c,v 1.249 1998/08/18 05:36:42 kimelman Exp $ */

#include "xmem.h"
#include "dessnch.h"
#include "sctpsyn.h"
#include "sctp.h"
#include "fdclsyn.h"
#include "fdcltrn.h"

/*
 * move blocking tree
 */
static void
bltrrem (struct des_lock *a, struct des_lock *anl)	
{
  struct des_wlock *aw, *aw1;
  
  for (aw = aw1 = a->Ddown; aw != NULL; aw1 = aw, aw = aw->Dqueue)
    aw->Dup = anl;
  aw1->Dqueue = anl->Ddown;
  anl->Ddown = a->Ddown;
}

static char *
twrem (struct des_tran *tr, struct des_lock *a, char *t)
{
  i4_t n;
  n = a->dls;
  if (n == cpsize)
    {
      struct des_cp *cp, *cp1;
      cp = (struct des_cp *) a;
      *(struct des_cp *) t = *cp;
      for (cp1 = tr->plcp; cp1 != NULL; cp1 = cp1->pdcp)
	if (cp1->pdcp == cp)
	  {
	    cp1->pdcp = (struct des_cp *) t;
	    break;
	  }
    }
  else
    {
      bcopy ((char *) a, t, n);
      refrem ((struct des_lock *) t);
    }
  return (t + n);
}

/*
 * fullcv -- chech whether a cover is full or not(partial)
 * a - a new lock, b - an old lock
 */
#define FULL_COVER 1
#define PARTIAL_COVER 0

static int
fullcv (struct des_lock *a, struct des_lock *b)
{
  u2_t at, bt, astsc, bstsc, na1, na2, nb1, nb2, type;
  i4_t (*f) (char *, char *, u2_t, u2_t);
  i4_t v, v1, v2;  
  char *ad, *bd, *a1, *b1, *a2, *b2;
  char *ascale, *bscale;
  struct des_field *field;
  

  ascale = (char *) a + a->dls;
  ascale--;
  bscale = (char *) b + b->dls;
  bscale--;
  astsc = 1;
  bstsc = 1;

  field = (struct des_field *) (a->rel + 1);
  ad = (char *) a + locksize;
  bd = (char *) b + locksize;
  for (;; field++)
    {
      at = ss1 (&ascale, astsc++);
      bt = ss1 (&bscale, bstsc++);
      type = field->field_type;
      if (bt == ENDSC)
	return FULL_COVER;          /* == FULL COVER exit */
      if (at == ENDSC)
	break;
      if (bt == NOTLOCK)
	{
	  if (at == X_X || at == S_S)
	    continue;
	  at = ss1 (&ascale, astsc++);
	  if (at == SS || at == SES || at == SSE || at == SESE)
	    ad = proval (ad, type);
	  ad = proval (ad, type);
	  continue;
	}
      if (at == NOTLOCK)
	break;
      if (at == X_X)
	{
	  if (bt == X_X || bt == S_S)
	    continue;
	  bt = ss1 (&bscale, bstsc++);
	  if (bt == SS || bt == SES || bt == SSE || bt == SESE)
	    bd = proval (bd, type);
	  bd = proval (bd, type);
	  continue;
	}
      if (bt == X_X)
	break;
      if (at == S_S)
	{
	  if (bt == X_D)
	    break;
	  if (bt == S_S)
	    continue;
	  bt = ss1 (&bscale, bstsc++);
	  if (bt == SS || bt == SES || bt == SSE || bt == SESE)
	    bd = proval (bd, type);
	  bd = proval (bd, type);
	  continue;
	}
      if (bt == S_S || (bt == X_D && at == S_D))
	break;
      at = ss1 (&ascale, astsc++);
      bt = ss1 (&bscale, bstsc++);
      if ( (at == SS  || at == SES   || at == SSE || at == SESE ) /*BETWEEN_CMP (at)*/
	&& (bt == SML || bt == SMLEQ || bt == GRT || bt == GRTEQ ))
        return(0);
      if (bt == NEQ && at != NEQ)
	break;
      if (at == EQ)
        {
          if (bt != EQ)
            break;
          v = cmpval (ad, bd, type);
          if (v != 0)
            return (0);
          ad = proval (ad, type);
          bd = proval (bd, type);
          break;
        }
      switch (type)
	{
	case T1B:
	  f = f1b;
	  ad = ftint (at, ad, &a1, &a2, size1b);
	  bd = ftint (bt, bd, &b1, &b2, size1b);
	  break;
	case T2B:
	  f = f2b;
	  ad = ftint (at, ad, &a1, &a2, size2b);
	  bd = ftint (bt, bd, &b1, &b2, size2b);
	  break;
	case T4B:
	  f = f4b;
	  ad = ftint (at, ad, &a1, &a2, size4b);
	  bd = ftint (bt, bd, &b1, &b2, size4b);
	  break;
	case TFLOAT:
	  f = flcmp;
	  ad = ftint (at, ad, &a1, &a2, size4b);
	  bd = ftint (bt, bd, &b1, &b2, size4b);
	  break;
	case TFL:
	  f = ffloat;
	  ad = ftch (at, ad, &a1, &a2, &na1, &na2);
	  bd = ftch (bt, bd, &b1, &b2, &nb1, &nb2);
	  break;
	case TCH:
 	  f = chcmp;
	  ad = ftch (at, ad, &a1, &a2, &na1, &na2);
	  bd = ftch (bt, bd, &b1, &b2, &nb1, &nb2);
	  break;
	default:
	  f = NULL;
	  error ("SYN.fullcv: This type is false");
	  break;
	}

      if (at == SML || at == SMLEQ)
	na1 = 0;
      if (bt == SML || bt == SMLEQ)
	nb1 = 0;
      if (at == GRT || at == GRTEQ)
	na2 = (u2_t) ~0;
      if (bt == GRT || bt == GRTEQ)
	nb2 = (u2_t) ~0; 
      
      v = f (a2, b1, na2, nb1);
      if (at == NEQ)
	{
	  if (bt == NEQ &&  v != 0)
	    break;
	}
      else
	{
	  if (v < 0)
	    break;
	  if ((v1 = f (b1, a1, nb1, na1)) < 0)
	    break;
	  if ((v2 = f (a2, b2, na2, nb2)) < 0)
	    break;	  
	  if (v == 0)     /* a2 was compared with b1 */
	    {
	      if ( bt == EQ && (at == SML || at == SS || at == SES))
		break;
	      if ( (bt == SES || bt == SESE) && (at == SSE || at == SESE))
		break;
	    }
	  if (v1 == 0)    /* b1 was compared with a1 */
	    {
	      if (bt == EQ && (at == GRT || at == SS || at == SSE))
		break;
	      if ((bt == SES || bt == SESE) && (at == GRT || at == SS || at == SSE))
		break;
	    }
	  if (v2 == 0)     /* a2 was compared with b2 */
	    {
	      if (bt == EQ && (at == SES || at == SESE))
		break;
	      if (bt == SMLEQ && at == SML)
		break;	      
	      if ((bt == SSE || bt == SESE) && (at == SS || at == SES))
		break;
	    }
	}
    }
  return PARTIAL_COVER;
}

void
dlock (struct des_lock *anl)			/* unlock narrow locks */
{
  char *c, *t, *b;
  struct des_rel *r;
  struct des_tran *tr;
  struct des_lock *a, *a1;
  struct des_cp *cp, *cpnext;
  i4_t anldls, delta, n, adls, p = 0;

  r = anl->rel;
  tr = anl->tran;
  anldls = anl->dls;
  a = (struct des_lock *) ((char *) tr->ptlb + cpsize);
  for (t = (char *) a; a != anl; a = (struct des_lock *) ((char *) a + adls))
    if ((adls = a->dls) != cpsize && a->rel == r && a->lockin == 't'
	&& fullcv (anl, a) == 1)
      {				/* if cover is full */
	if (p == 0)
	  {
	    p = 1;
	    lilore (r, anl, anl->of, anl->ob);
	  }
	else
	  lilore (r, a, a->of, a->ob);
	if (a->Ddown != NULL)
	  bltrrem (a, anl);	/* move blocking tree */
      }
    else
      {
	if (t != (char *) a)
	  {
	    delta = anldls - ((char *) a - t);
	    if (delta > 0)
	      {
		for (c = tr->firstfree - 1, b = c + delta; c < (char *) a;)
		  *b-- = *c--;
		if (t <= (char *) tr->plcp)
		  tr->plcp = (struct des_cp *) ((char *) tr->plcp + delta);
		cp = tr->plcp;
		for (cpnext = cp->pdcp; t <= (char *) cpnext; cpnext = cp->pdcp)
		  {
		    cp->pdcp = (struct des_cp *) ((char *) cp->pdcp + delta);
		    cp = cp->pdcp;
		  }
		a = (struct des_lock *) ((char *) a + delta);
		tr->firstfree += delta;
		tr->freelb -= delta;
		c = tr->firstfree;
		a1 = a;
		for (; c > (char *) a1; a1 = (struct des_lock *) ((char *) a1 + n))
		  if ((n = a1->dls) != cpsize)
		    refrem (a1);
		anl = (struct des_lock *) ((char *) anl + delta);
	      }
            bcopy ((char *) anl, t, anldls);
            anl = (struct des_lock *) t;
            t += anldls;
	    tr->firstfree -= anldls;
	    tr->freelb += anldls;
	    t = twrem (tr, a, t);
	    break;
	  }
	else
	  {
	    t += adls;
	  }
      }
  if (p == 1)
    {
      b = tr->firstfree;
      for (; b != (char *) a; a = (struct des_lock *) ((char *) a + adls))
	if ((adls = a->dls) != cpsize && a->rel == r && a->lockin == 't'
	    && fullcv (anl, a) == 1)
	  {
	    lilore (r, a, a->of, a->ob);
	    if (a->Ddown != NULL)
	      bltrrem (a, anl);	/* move blocking tree */
	  }
	else
	  {
	    if (t != (char *) a)
	      t = twrem (tr, a, t);
	    else
	      t += adls;
	  }
      n = b - t;
      tr->firstfree -= n;
      tr->freelb += n;
    }
}
