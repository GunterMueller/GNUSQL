/*
 *  libini.c  - BD objects initialization Library 
 *              Kernel of GNU SQL-server. Recovery utilities     
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

/* $Id: libini.c,v 1.248 1998/09/29 21:25:09 kimelman Exp $ */

#include "setup_os.h"

#include <sys/types.h>
#include <sys/stat.h>
#if HAVE_FCNTL_H 
#include <fcntl.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "xmem.h"
#include "destrn.h"
#include "adfstr.h"
#include "admdef.h"
#include "strml.h"
#include "totdecl.h"
#define adfsize sizeof(struct ADF)

void
ini_adm_file (char *name_adm_file)
{
  register i4_t fdaf;
  struct ADF adf;
  
  /* the initialization of the admfile */
  unlink (name_adm_file);    
  if ( (fdaf = open (name_adm_file, O_RDWR|O_CREAT, DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("ADMFILE: open error");
      exit (1);
    }
  adf.uid = 1;
  adf.uidconst = 1;
  adf.unname = 1;
  adf.sizext = 4 * BD_PAGESIZE;
  adf.maxtrans = 64;
  adf.mjred = 1024;
  adf.ljred = 20000;
  adf.mjmpage = 2;
  adf.ljmpage = 2;
  adf.sizesc = 2;
  adf.optbufnum = 64;
  adf.maxnbuf = 3 * adf.maxtrans;
  adf.maxtact = 2;
  adf.fshmsegn = 100000;
  if (write (fdaf, (char *)&adf, adfsize) != adfsize)
    {
      perror ("ADMFILE: write error");
      exit (1);
    } 
  close (fdaf);
}

void
ini_mj (char *name_mj_file)
{
  i4_t NV;
  i4_t fdmj;
  char page[RPAGE];
  
  /* the initialization of MJ */
  NV = 0;
  unlink (name_mj_file);
  if ( (fdmj = open (name_mj_file, O_RDWR|O_CREAT, DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("LIBINI: MJ open error");
      exit (1);
    }
  t4bpack (NV, page);
  if (write (fdmj, page, RPAGE) != RPAGE)
    {
      perror ("MJ: write error");
      exit (1);
    } 
  close (fdmj);
}

void
ini_lj (char *name_lj_file)
{
  i4_t fdlj;
  char page[RPAGE];  

  bzero(page,sizeof(page));
  /* the initialization of LJ */
  unlink (name_lj_file);
  if ( (fdlj = open (name_lj_file, O_RDWR|O_CREAT, DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("LJ: open error");
      exit (1);
    }
  t2bpack (RTPAGE, page+size4b);
  *(page + size4b + size2b)= SIGN_NOCONT;
  if (write (fdlj, page, RPAGE) != RPAGE)
    {
      perror ("LJ: write error");
      exit (1);
    } 
  close (fdlj);
}

void
ini_BD_seg (char *name_BD_seg, u2_t scsize)
{
  struct ind_page *indph;
  char mch[BD_PAGESIZE];
  char *a;
  struct stat bufstat;
  i4_t fd_seg;
  i4_t i;

  /* the initialization of BD segment */
  unlink (name_BD_seg);
  if ( (fd_seg = open (name_BD_seg, O_RDWR|O_CREAT, DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("SEG1: open error");
      exit (1);
    }
  assert(scsize < 8);
  for (a = mch + 2 * size4b, *a = 0, i = 0; i <= scsize; i++)
    *a|= BITVL(i);
  for (a += 1, i = 2 * size4b + 1; i < BD_PAGESIZE; i++)
    *a++ = 0;
  if (write (fd_seg, mch, BD_PAGESIZE) != BD_PAGESIZE)
    {
      perror ("SEG1: write error");
      exit (1);
    } 
  for (a = mch, i = 0; i < BD_PAGESIZE; i++)
    *a++ = 0;
  for (i = 1; i < scsize; i++)
    if (write (fd_seg, mch, BD_PAGESIZE) != BD_PAGESIZE)
      {
        perror ("SEG1: write error");
        exit (1);
      }
  indph = (struct ind_page *)mch;
  indph->ind_wpage = LEAF;
  indph->ind_off = indphsize;
  indph->ind_nextpn = (u2_t)~0;
  if (write (fd_seg, mch, indphsize) != indphsize)
    {
      perror ("SEG1: write error");
      exit (1);
    }
  fstat (fd_seg, &bufstat);
  fprintf (stderr, "SEG1_size=%ld\n", bufstat.st_size);
  close (fd_seg);
}

