/*
 *  sort.c - Sort a temporary table, a filter by keys values and a filter by tids
 *           Kernel of GNU SQL-server. Sorter    
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

/* $Id: sort.c,v 1.249 1997/11/03 14:17:08 vera Exp $ */

#include "setup_os.h"

#include "dessrt.h"
#include "pupsi.h"
#include "sctp.h"
#include "fdclsrt.h"
#include "fdcltrn.h"
 
/*------------Global sort variables -----------------------*/
i4_t N;
i4_t NFP;
i4_t NB;
u2_t pnex,lastpnex;
u2_t outpn, inpn;
u2_t freesz;
u2_t offset;
char *adrseg;
char *regakr;
char *akr;
char **regpkr;
char *outasp, *inasp;
u2_t *arrpn;
u2_t *arrfpn;
u2_t *cutfpn;
char *masp[PINIT];
char *nonsense;
i4_t need_free_pages;
i4_t segsize;
char *reg_tmp_buff;
u2_t load_arrpn[PINIT];

/*---------------------------------------------------------*/

extern u2_t extent_size;

static i4_t extent_count;

#define PRINT(x) PRINTF(x) 

static void
bgnng (void)
{
  u2_t n;
  
  if ((adrseg = (char *) malloc (segsize = BD_PAGESIZE * PINIT)) == NULL)
    {
      perror ("SRT.malloc: No segment");
      exit (1);
    }  
  arrfpn = (u2_t *) malloc (PINIT * size2b);
  cutfpn = (u2_t *) malloc (PINIT * size2b);
  if (PINIT < extent_size)
    NFP = PINIT;
  else
    NFP = extent_size;  
  arrpn = (u2_t *) malloc (NFP * size2b);
  
  N = 0;
  NB = 0;
  extent_count = 0;
  addext ();
  regakr = adrseg;
  regpkr = (char **) (adrseg + segsize);
  freesz = segsize;
  akr = regakr;
  reg_tmp_buff = malloc (PINIT * BD_PAGESIZE);
  masp[0] = reg_tmp_buff;
  for (n = 1; n < PINIT; n++)
    masp[n] = masp[n-1] + BD_PAGESIZE;
}

static void
fin_of_sort (void)
{
  xfree (reg_tmp_buff);
  xfree (arrpn);
  xfree (arrfpn);
  xfree (cutfpn);
}

static u2_t 
getfpn (void)
{
  struct des_exns *desext;
  
  desext = getext ();
  pnex = desext->efpn;
  lastpnex = pnex + extent_size;
  ((struct listtob *) outasp)->prevpn = (u2_t) ~ 0;
  return (pnex);
}

u2_t
srt_trsort(u2_t *fpn, struct fun_desc_fields *desf,
           struct des_sort *sdes)
{
  char *asp;
  u2_t pn, ind, *ai, *ali;
  char tmp_buff_in[BD_PAGESIZE], tmp_buff_out[BD_PAGESIZE];

  PRINT (("SRT: trsort: fpn = %d,  kn = %d\n", *fpn, sdes->s_kn));
  
  bgnng ();
  asp = tmp_buff_in;
  for (pn = *fpn; pn != (u2_t) ~ 0;)
    {
      read_tmp_page (pn, asp);
      ai = (u2_t *) (asp + phtrsize);
      ali = ai + ((struct p_h_tr *) asp)->linptr;
      if (ai == ali && pn == *fpn)
	{
	  outpn = pn;
	  goto end;
	}
      for (ind = 0; ai <= ali; ai++, ind++)
        if (*ai != 0)
          rkfrm (asp + *ai, pn, ind, sdes, MINIT, desf);
      pn = ((struct listtob *) asp)->nextpn;
    }
  PRINT (("SRT: trsort:  2 N = %d, kn = %d\n", N, sdes->s_kn));  
  outasp = tmp_buff_out;
  inasp = tmp_buff_in;
  if (NB == 0)
    {
      char *pkr;
      i4_t i;
      extent_count = 0;
      quicksort (MINIT, sdes);      
      *fpn = outpn = pnex;
      ((struct listtob *) outasp)->prevpn = (u2_t) ~ 0;
      freesz = BD_PAGESIZE - phtrsize;
      for ( i = 0; i < N; i++)
	if (regpkr[i] != nonsense)
	  break;
      inpn = t2bunpack (regpkr[i] + size2b);
      read_tmp_page (inpn, inasp);
      for ( ; i < N; i++)
	{
	  pkr = regpkr[i];
	  if (pkr != nonsense)
	    putcrt (pkr);
	}
    }
  else
    {
      char *a;
      struct el_tree *q = NULL, tree[PINIT];
      
      q = extsort (MINIT, sdes, tree);
      a = q->pket + size2b;
      inpn = t2bunpack (a);
      read_tmp_page (inpn, inasp);
      outpn = getfpn ();
      *fpn = outpn;
      freesz = BD_PAGESIZE - phtrsize;
      need_free_pages = NO;
      push (putcrt, tree, q, sdes, NB);
    }
  ((struct listtob *) outasp)->nextpn = (u2_t) ~ 0;
  write_tmp_page (outpn, outasp);  
  
end:
  putext (arrfpn, extent_count);
  fin_of_sort ();
  return (outpn);
}

u2_t
srt_flsort(u2_t segn, u2_t *fpn, struct fun_desc_fields *desf,
           struct des_sort *sdes)
{
  char *asp, *aspfl, *lastb;
  u2_t pn, ind, flpn, *ai, *afi, oldpn;
  struct el_tree *q = NULL;
  struct des_tid *tid;
  struct A inpage;
  char tmp_buff_in[BD_PAGESIZE], tmp_buff_out[BD_PAGESIZE];
  
  PRINT (("SRT: flsort: fpn = %d, kn = %d\n", *fpn, sdes->s_kn));
  
  bgnng ();
  aspfl = tmp_buff_in;
  oldpn = (u2_t) ~ 0;
  for (flpn = *fpn; flpn != (u2_t) ~ 0;)
    {
      read_tmp_page (flpn, aspfl);
      lastb = aspfl + ((struct p_h_f *) aspfl)->freeoff;
      tid = (struct des_tid *) (aspfl + phfsize);
      oldpn = tid->tpn;
      asp = getpg (&inpage, segn, oldpn, 's');
      afi = (u2_t *) (asp + phsize);
      for (; tid < (struct des_tid *) lastb; tid++)
	{
	  pn = tid->tpn;
	  if (oldpn != pn)
	    {
	      putpg (&inpage, 'n');
	      asp = getpg (&inpage, segn, pn, 's');
	      oldpn = pn;
	      afi = (u2_t *) (asp + phsize);
	    }
	  ind = tid->tindex;
	  ai = afi + ind;
	  if (*ai != 0)
	    rkfrm (asp + *ai, pn, ind, sdes, MINIT, desf);
	}
      flpn = ((struct listtob *) aspfl)->nextpn;
    }
  if (oldpn != (u2_t) ~ 0)
    putpg (&inpage, 'n');
  outasp = tmp_buff_out;
  if (NB == 0)
    {
      char *pkr;
      i4_t i;
      extent_count = 0;
      quicksort (MINIT, sdes);      
      *fpn = outpn = pnex;
      ((struct listtob *) outasp)->prevpn = (u2_t) ~ 0;
      offset = phfsize;
      for ( i = 0; i < N; i++)
	if (regpkr[i] != nonsense)
	  break;
      for ( ; i < N; i++)
	{
	  pkr = regpkr[i];
	  if (pkr != nonsense)
	    puttid (pkr);
	}
    }
  else
    {
      struct el_tree tree[PINIT];
      q = extsort (MINIT, sdes, tree);
      outpn = getfpn ();
      *fpn = outpn;
      offset = phfsize;
      need_free_pages = NO;      
      push (puttid, tree, q, sdes, NB);
    }
  ((struct listtob *) outasp)->nextpn = (u2_t) ~ 0;
  write_tmp_page (outpn, outasp); 
  putext (arrfpn, extent_count);
  fin_of_sort ();
  return (outpn);
}

u2_t
tidsort (u2_t * fpn)
{
  char *aspfl, *lastb;
  i4_t recsz;
  u2_t flpn, kn;
  struct el_tree *q = NULL;
  struct des_tid *tid;
  char tmp_buff_in[BD_PAGESIZE], tmp_buff_out[BD_PAGESIZE];

  PRINT (("SRT: tidsort:    fpn = %d\n", *fpn));
  
  bgnng ();
  kn = 2;
  recsz = 2 * size2b;
  aspfl = tmp_buff_in;
  for (flpn = *fpn; flpn != (u2_t) ~ 0;)
    {
      read_tmp_page (flpn, aspfl);
      lastb = aspfl + ((struct p_h_f *) aspfl)->freeoff;
      tid = (struct des_tid *) (aspfl + phfsize);
      for (; tid < (struct des_tid *) lastb; tid++)
	{
	  if (freesz < recsz)
	    {			/* initial cut form */
	      quick_sort_tid (MINIT);
	      puts_tid ();
	      N = 0;
	      if ((NB % PINIT) == 0)
		cutfpn = (u2_t *) realloc ((void *) cutfpn,
                                            (size_t) (PINIT + NB) * size2b);
	    }
          bcopy ((char *) tid, akr, recsz);
          akr += recsz;
	  freesz -= recsz;
	}
      flpn = ((struct listtob *) aspfl)->nextpn;
    }
  outasp = tmp_buff_out;
  if (NB == 0)
    {
      i4_t i;
      extent_count = 0;
      quick_sort_tid (MINIT);      
      *fpn = outpn = pnex;
      ((struct listtob *) outasp)->prevpn = (u2_t) ~ 0;
      offset = phfsize;
      for (i = 0; i < N; i++, regakr += tidsize)
	put_tid (regakr);
    }
  else
    {
      struct el_tree tree[PINIT];
      q = ext_sort_tid (MINIT, tree);
      outpn = getfpn ();
      *fpn = outpn;
      offset = phfsize;
      freesz = BD_PAGESIZE - offset;
      need_free_pages = NO;      
      push_tid (put_tid, tree, q, N);
    }
  ((struct listtob *) outasp)->nextpn = (u2_t) ~ 0;
  write_tmp_page (outpn, outasp);
  putext (arrfpn, extent_count);
  fin_of_sort ();
  return (outpn);
}


void
addext (void)
{
  struct des_exns *desext;
  
  desext = getext ();
  pnex = desext->efpn;
  arrfpn[extent_count++] = pnex;
  lastpnex = pnex + extent_size;
  if ((extent_count % PINIT) == 0)
    arrfpn = (u2_t *) realloc ((void *) arrfpn,
                               (size_t) (extent_count + PINIT) * size2b);
}
