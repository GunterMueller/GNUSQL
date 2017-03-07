/*  rkfrm.c - Key record forming
 *            Kernel of GNU SQL-server. Sorter    
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

/* $Id: rkfrm.c,v 1.248 1998/09/29 21:25:45 kimelman Exp $ */

#include <assert.h>
#include "setup_os.h"

#include "dessrt.h"
#include "pupsi.h"
#include "fdclsrt.h"
#include "fdcltrn.h"
#include "xmem.h"

extern u2_t pnex, lastpnex, freesz;
extern u2_t *cutfpn;
extern i4_t N;
extern i4_t NB;
extern char *akr;
extern char *regakr;
extern char **regpkr;
extern char *nonsense;
extern i4_t segsize;

void
rkfrm(char *cort, u2_t pn, u2_t ind, struct des_sort *sdes,
      i4_t M, struct fun_desc_fields *desf)
{
  char *ak, *keyval;
  u2_t kscsz, recsz, keysz = 0, k, k1, fn, sz;
  char *arrpnt[BD_PAGESIZE];
  u2_t arrsz[BD_PAGESIZE];
  u2_t kn, *mfn;

  tuple_break (cort, arrpnt, arrsz, desf);
  kn = sdes->s_kn;
  mfn = sdes->s_mfn;
  for (k1 = 0, k = 0; k < kn; k++)
    {
      fn = mfn[k];
      if (arrpnt[fn] != NULL)
	{
	  k1++;
	  keysz += arrsz[fn];
	}
    }
  kscsz = k1 / 7;
  if ((k1 % 7) != 0)
    kscsz++;
  keysz += kscsz;
  recsz = keysz + size2b + 2 * size2b;
  
  if (freesz < (recsz + pntsize))
    {				/* initial cut form */
      quicksort (M, sdes);
      putkf ();
      N = 0;
      if ((NB % PINIT) == 0)
	cutfpn = (u2_t *) realloc ((void *) cutfpn, (size_t) (PINIT + NB) * size2b);
    }
  
  ak = akr;
  t2bpack (recsz, ak);
  ak += size2b;
  t2bpack (pn, ak);
  ak += size2b;
  assert (ind < BD_PAGESIZE / 2);
  t2bpack (ind, ak);
  ak += size2b;  
  keyval = ak + kscsz;
  for (k1 = 0, k = 0, *ak = 0; k1 < kn; k1++)
    {
      fn = mfn[k1];
      if ((sz = arrsz[fn]) != 0)
	{
          bcopy (arrpnt[fn], keyval, sz);
          keyval += sz;
	  *ak |= BITVL(k);	/* a value is present */
	}
      k++;
      if (k == 7)
	{
	  k = 0;
	  *(++ak) = 0;
	}
    }
  if (k == 0)
    ak--;
  *ak |= EOSC;
  N++;
  *(--regpkr) = akr;
  akr += recsz;
  freesz -= recsz + pntsize;
}

void
putkf (void)
{
  char *asp, *pnt, *pkr;
  i4_t i;
  u2_t size, curpn;
  char tmp_buff[BD_PAGESIZE];

  asp = tmp_buff;
  pnt = asp + size4b;
  cutfpn[NB] = pnex;
  for (i = 0; i < N; i++)
    {
      pkr = regpkr[i];
      if (pkr != nonsense)
	{
	  size = t2bunpack (pkr);
	  if ((pnt + size) > (asp + BD_PAGESIZE))
	    {
              curpn = pnex;
	      ++pnex;
	      if (pnex == lastpnex)
		addext ();
	      t2bpack (pnex, asp);
	      t2bpack (pnt - asp, asp + size2b);
	      write_tmp_page (curpn, asp);
	      pnt = asp + size4b;
	    }
          bcopy (pkr, pnt, size);
          pnt += size;
	}
    }
  t2bpack ((u2_t) ~ 0, asp);
  t2bpack (pnt - asp, asp + size2b);
  write_tmp_page (pnex, asp);
  NB++;
  pnex++;
  if (pnex == lastpnex)
    addext ();  
  akr = regakr;
  regpkr = (char **) (akr + segsize);
  freesz = segsize;
}

