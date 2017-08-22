/*
 *  empty_page.c  -  Operations with pages inside a segment 
 *                   Kernel of GNU SQL-server  
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

/* $Id: empty_page.c,v 1.248 1997/07/20 17:00:57 vera Exp $ */

#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

extern u2_t S_SC_S;

void
emptypg (u2_t sn, u2_t pn, char type)
{
  i4_t n, k;
  char *asp = NULL, *a;
  u2_t rpn;
  struct A pg;

  beg_mop ();
  if (type == 'f')
    {
      asp = getwl (&pg, sn, pn);
      ++((struct p_head *) asp)->idmod;
      recmjform (OLD, &pg, 0, BD_PAGESIZE, asp, 0);
      putwul (&pg, 'n');
    }
  n = pn / 8 + 2 * size4b;
  if (pn < 7)
    k = pn;
  else
    k = pn % 8;
  rpn = n / BD_PAGESIZE;
  if (rpn >= S_SC_S)
    fprintf (stderr, "TRN.emptypg: A page number isn't correct");
  while ((asp = getpg (&pg, sn, rpn, 'x')) == NULL);
  a = asp + n;
  if ((*a & BITVL(k)) != 0)
    {
      ++((struct p_head *) asp)->idmod;
      recmjform (OLD, &pg, (u2_t) (a - asp), size1b, a, 0);
      MJ_PUTBL ();
      *a &= ~BITVL(k);
      putpg (&pg, 'm');
    }
  else
    putpg (&pg, 'n');
/*    BUF_unlock(sn,1,&pn);*/
}

u2_t
getempt (u2_t sn)
{
  char *asp = NULL, *a, *lastb;
  i4_t k;
  u2_t pn;
  struct A pg;

  for (pn = (u2_t) 0; pn < S_SC_S; pn++)
    {
      while ((asp = getpg (&pg, sn, pn, 's')) == NULL);
      lastb = asp + BD_PAGESIZE;
      for (a = asp + 2 * size4b, k = 0; a < lastb; a++, k = 0)
        {
          if ((*a & 0377) != 0377)
            {
              for (; k < 8; k++)
                if ((*a & BITVL(k)) == 0)
                  {
                    begmop (asp);
                    recmjform (OLD, &pg, (u2_t) (a - asp), size1b, a, 0);
                    MJ_PUTBL ();
                    *a |= BITVL(k);
                    pn = (a - asp - 2 * size4b) * 8 + k;
                    putpg (&pg, 'm');
                    return (pn);
                  }
            }
        }
      putpg (&pg, 'n');
    }
  fprintf (stderr, "TRN.getempt: No free pages in the scale");
  return 0;
}
