/*
 *  recmj.c  - Records in Microjournal
 *             Kernel of GNU SQL-server 
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

/* $Id: recmj.c,v 1.247 1997/07/20 17:00:57 vera Exp $ */

#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"
#include "xmem.h"

#define SZMJBF SZMSGBUF-2*size2b
struct rec_mj
{
  struct ADBL addmj;
  char typemj;
  i4_t idmmj;
  u2_t snmj;
  u2_t pnmj;
  u2_t offmj;
  u2_t fsmj;
};
extern struct ADBL admj;
extern struct ADREC blmj;
extern char *bufmj;
extern char *pbufmj;

void
recmjform (i4_t type, struct A *pg, u2_t off, u2_t fs, char *af, i2_t shsize)
{
  u2_t size;
  char *a;
  i4_t idm;

  idm = ((struct p_head *) pg->p_shm)->idmod;
  size = adjsize + 1 + size4b + 4 * size2b;
  if (type != OLD)
    size += size2b;
  else
    size += fs;
  if (type == COMBR || type == COMBL)
    size += shsize;
  size += pbufmj - bufmj;
  if (size > SZMJBF)
    MJ_PUTBL ();

  a = pbufmj;
  bcopy ((char *) &admj, a, adjsize);
  a += adjsize;
  *a++ = type;
  t4bpack (idm, a);
  a += size4b;
  t2bpack (pg->p_sn, a);
  a += size2b;
  t2bpack (pg->p_pn, a);
  a += size2b;
  t2bpack (off, a);
  a += size2b;
  t2bpack (fs, a);
  a += size2b;
  if (type != OLD)
    {
      bcopy ((char *) &shsize, a, size2b);
      a += size2b;
    }
  else
    {
      bcopy (af, a, fs);
      a += fs;
    }
  if (type == COMBR || type == COMBL)
    {
      bcopy (af, a, shsize);
      a += shsize;
    }
  pbufmj = a;
}

void
begmop (char *asp)
{
  beg_mop ();
  ++((struct p_head *) asp)->idmod;
  return;
}

void
beg_mop ()
{
  admj.npage = 1;
  admj.cm = 0;
  pbufmj = bufmj;
}
