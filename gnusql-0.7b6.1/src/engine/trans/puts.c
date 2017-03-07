/*
 *  puts.c - Put a next record into temporary object page
 *           Kernel of GNU SQL-server. Sorter    
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

/* $Id: puts.c,v 1.248 1998/09/29 21:25:43 kimelman Exp $ */

#include "dessrt.h"
#include "pupsi.h"
#include "fdclsrt.h"
#include "fdcltrn.h"
#include "xmem.h"

extern i4_t NFP;
extern u2_t *arrpn;
extern u2_t outpn, offset, pnex, lastpnex, freesz, inpn;
extern u2_t extent_size;
extern char *outasp, *inasp;

static void
middleput ( char *pkr, u2_t size)
{
  if (offset + size > BD_PAGESIZE)
    {   /* get new page for output middle record file */
      u2_t curpn;
      curpn = outpn;
      getptmp ();
      t2bpack (outpn, outasp);
      t2bpack (offset, outasp + size2b);
      write_tmp_page (curpn, outasp);
      offset = size4b;
    }
  bcopy (pkr, outasp + offset, size);
  offset += size;
}

void
putkr (char *pkr)
{
  u2_t size;
  
  size = t2bunpack(pkr);
  middleput (pkr, size);
}

static void
get_ptob (void)  /* get a new page for the resulting sort temporary object */
{
  u2_t curpn;

  curpn = pnex++;
  if (pnex == lastpnex)
    {				/* get new extent */
      struct des_exns *desext;
      desext = getext ();
      pnex = desext->efpn;
      lastpnex = pnex + extent_size;
    }
  ((struct listtob *) outasp)->nextpn = pnex;
  write_tmp_page (curpn, outasp);
  outpn = pnex;
  ((struct listtob *) outasp)->prevpn = curpn;
}

void
putcrt (char *pkr)
{
  char *a, *cort;
  struct p_h_tr *phtr;
  u2_t pn, ind, *ai, *afi, size, corsize;

  pn = t2bunpack(pkr + size2b);
  ind = t2bunpack(pkr + 2*size2b);
  assert( ind < BD_PAGESIZE/sizeof(*ai) );
  if (pn != inpn)
    {
      read_tmp_page (pn, inasp);
      inpn = pn;
    }
  afi = (u2_t *) (inasp + phtrsize);
  ai = afi + ind;
  corsize = calsc (afi, ai);
  size = corsize + size2b;
  assert( *ai <= BD_PAGESIZE );
  cort = inasp + *ai;
  if (freesz < size)
    {			/* get a new page for the resulting sort temporary table */
      get_ptob ();
      freesz = BD_PAGESIZE - phtrsize;
      phtr = (struct p_h_tr *) outasp;
      phtr->linptr = 0;
      a = BD_PAGESIZE + outasp;
      ai = (u2_t *) (outasp + phtrsize);
      *ai = BD_PAGESIZE;
    }
  else
    {
      phtr = (struct p_h_tr *) outasp;
      if (freesz == BD_PAGESIZE - phtrsize)
	{
	  phtr->linptr = 0;
	  a = BD_PAGESIZE + outasp;
	  ai = (u2_t *) (outasp + phtrsize);
	  *ai = BD_PAGESIZE;
	}
      else
	{
	  ai = (u2_t *) (outasp + phtrsize) + phtr->linptr;
	  a = outasp + *ai;
	  phtr->linptr += 1;
	  ai++;
	}
    }
  a -= corsize;
  *ai = a - outasp;
  bcopy (cort, a, corsize);
  freesz -= size;
}

void
puttid (char *pkr)
{
  pkr += size2b;
  put_tid (pkr);
}

void
put_tid (char *pkr)
{
  if (offset + tidsize > BD_PAGESIZE)
    {				/* get a new page for the resulting sort filter */
      get_ptob ();
      offset = phfsize;
    }
  bcopy (pkr, outasp + offset, tidsize);
  offset += tidsize;
  ((struct p_h_f *) outasp)->freeoff = offset;
}

void
middle_put_tid (char *pkr)
{
  middleput (pkr, tidsize);
}

void
getptmp (void)
{
  i4_t i;

  for (i = 0; i < NFP; i++)
    if (arrpn[i] != (u2_t) ~ 0)
      {
	outpn = arrpn[i];
	arrpn[i] = (u2_t) ~ 0;
	break;
      }
  if (i == NFP)
    {
      u2_t pn;
      addext ();
      for (pn = pnex, i = 0; i < extent_size; i++)
	arrpn[i] = pn++;
      outpn = pnex++;
      arrpn[0] = (u2_t) ~ 0;      
    }
}
