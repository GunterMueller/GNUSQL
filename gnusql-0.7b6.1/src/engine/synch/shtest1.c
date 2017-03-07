/*  shtest1.c -  Check sharing on fields
 *               Kernel of GNU SQL-server. Synchronizer    
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

/* $Id: shtest1.c,v 1.247 1998/09/29 21:25:16 kimelman Exp $ */


#include "dessnch.h"
#include "sctp.h"
#include "sctpsyn.h"
#include "fdclsyn.h"

i4_t
shtest1 (struct des_lock *anl, char *con, i4_t size, struct des_lock *bl)
     /* 0- is shared, 1- not */
{
  i4_t at, bt, astsc, bstsc;
  i4_t (*f) (char *, char *, u2_t, u2_t);
  u2_t na1, na2, nb1, nb2;
  i4_t i, j, ans;
  char *bd, *a1, *b1, *a2, *b2;
  char *ascale, *bscale;
  struct des_field *field;
  u2_t type;

  ascale = con + size;
  ascale--;
  bscale = (char *) bl + bl->dls;
  bscale--;
  astsc = 1;
  bstsc = 1;
  at = ss1 (&ascale, astsc++);
  bt = ss1 (&bscale, bstsc++);
  field = (struct des_field *) (anl->rel + 1);
  if (bl != bl->tran->pwlock)
    bd = (char *) bl + locksize;
  else
    bd = (char *) bl + wlocksize;
  ans = 0;
  for (; at != ENDSC && bt != ENDSC; field++)
    {
      type = field->field_type;
      if (at == X_X && bt != NOTLOCK)
	ans = 1;
      if (bt == X_X && at != NOTLOCK)
	ans = 1;
      if (at == X_D && bt != NOTLOCK)
	ans = 1;
      if (bt == X_D && at != NOTLOCK)
	ans = 1;
      if (at == NOTLOCK || at == X_X || at == S_S)
	{
	  if (bt == S_D || bt == X_D)
	    {
	      bt = ss1 (&bscale, bstsc++);
	      if (bt == SS || bt == SES || bt == SSE || bt == SESE)
		bd = proval (bd, type);
	      bd = proval (bd, type);
	    }
	  goto m1;
	}
      if (bt == NOTLOCK || bt == X_X || bt == S_S)
	{
	  if (at == S_D || at == X_D)
	    {
	      at = ss1 (&ascale, astsc++);
	      if (at == SS || at == SES || at == SSE || at == SESE)
		con = proval (con, type);
	      con = proval (con, type);
	    }
	  goto m1;
	}
      at = ss1 (&ascale, astsc++);
      bt = ss1 (&bscale, bstsc++);
      if (((at == SML || at == SMLEQ) && (bt == SML || bt == SMLEQ)) ||
	  ((at == GRT || at == GRTEQ) && (bt == GRT || bt == GRTEQ)))
	{
	  at = ss1 (&ascale, astsc++);
	  if (at == SS || at == SES || at == SSE || at == SESE)
	    con = proval (con, type);
	  con = proval (con, type);
	  bt = ss1 (&bscale, bstsc++);
	  if (bt == SS || bt == SES || bt == SSE || bt == SESE)
	    bd = proval (bd, type);
	  bd = proval (bd, type);
	  goto m1;
	}
      if (at == NEQ && bt == NEQ)
	{
	  con = proval (con, type);
	  bd = proval (bd, type);
	  goto m1;	  
	}
      switch (type)
	{
	case T1B:
	  f = f1b;
	  con = ftint (at, con, &a1, &a2, size1b);
	  bd = ftint (bt, bd, &b1, &b2, size1b);
	  break;
	case T2B:
	  f = f2b;
	  con = ftint (at, con, &a1, &a2, size2b);
	  bd = ftint (bt, bd, &b1, &b2, size2b);
	  break;
	case T4B:
	  f = f4b;
	  con = ftint (at, con, &a1, &a2, size4b);
	  bd = ftint (bt, bd, &b1, &b2, size4b);
	  break;
	case TFLOAT:
	  f = flcmp;
	  con = ftint (at, con, &a1, &a2, size4b);
	  bd = ftint (bt, bd, &b1, &b2, size4b);
	  break;
	case TFL:
	  f = ffloat;
	  con = ftch (at, con, &a1, &a2, &na1, &na2);
	  bd = ftch (bt, bd, &b1, &b2, &nb1, &nb2);
	  break;
	case TCH:
	  f = chcmp;
	  con = ftch (at, con, &a1, &a2, &na1, &na2);
	  bd = ftch (bt, bd, &b1, &b2, &nb1, &nb2);
	  break;
	default:
	  f = NULL;
	  error ("SYN.error: Incorrect data type");
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
      
      i = (*f) (a2, b1, na2, nb1);
      if (at == NEQ)
	{
	  if (bt == EQ && i != 0)
	    goto m1;
	  return (0);
	}
      if (bt == NEQ)
	{
	  if (at == EQ && i != 0)
	    goto m1;
	  return (0);
	}
      if (i < 0)
	return (0);
      if ((j = (*f) (b2, a1, nb2, na1)) < 0)
	return (0);
      if (i > 0 && j > 0)
	goto m1;
      if (i == 0 && (at == EQ || at == SMLEQ || at == SSE || at == SESE) &&
	  (bt == EQ || bt == GRTEQ || bt == SES || bt == SESE))
	goto m1;
      if (j == 0 && (at == GRTEQ || at == SES || at == SESE) &&
	  (bt == SMLEQ || bt == SSE || bt == SESE))
	goto m1;
      return (0);
    m1:
      at = ss1 (&ascale, astsc++);
      bt = ss1 (&bscale, bstsc++);
    }
  return (ans);
}
