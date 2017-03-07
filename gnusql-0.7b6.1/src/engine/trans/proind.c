/*
 *  proind.c  -  Processing of all DB table indexes in the time of an insertion, a deletion or a modification
 *               Kernel of GNU SQL-server 
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

/* $Id: proind.c,v 1.248 1998/09/29 21:25:42 kimelman Exp $ */

#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

u2_t
proind (i4_t (*f) (), struct d_r_t *desrel, u2_t cinum, char *cort, struct des_tid *tid)
{
  u2_t n;
  struct ldesind *desind;
  char mas[BD_PAGESIZE];

  desind = desrel->pid;
  for (n = 0; desind != NULL && n != cinum; desind = desind->listind, n++)
    {
      keyform (desind, mas, cort);
      if ((*f) (desind, mas, tid) != OK)
	return (n);
    }
  return (n);
}

u2_t
mproind (struct d_r_t *desrel, u2_t cinum, char *cort,
         char *newc, struct des_tid *tid)
{
  u2_t n;
  struct ldesind *desind;
  char mas[BD_PAGESIZE];

  desind = desrel->pid;
  for (n = 0; desind != NULL && n != cinum; desind = desind->listind, n++)
    {
      keyform (desind, mas, cort);
      ordindd (desind, mas, tid);
      keyform (desind, mas, newc);
      if (ordindi (desind, mas, tid) != OK)
	return (n);
    }
  return (n);
}
