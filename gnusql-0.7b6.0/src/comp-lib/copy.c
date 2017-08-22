/*
 *  copy.c  -  engine interaction support
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Olga Dmitrieva, 1994
 *  Fixed by Kostya Dyshlevoy, 1995
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
 *  Contacts: gss@ispras.ru
 *
 */

/* $Id: copy.c,v 1.248 1998/07/30 03:23:34 kimelman Exp $ */

#define   MAIN
#include "global.h"
#include "engine/pupsi.h"
#include "engine/tptrn.h"
#include "engine/expop.h"
#include "engine/fdcltrn.h"
#include "exti.h"
#include "funall.h"
#include "xmem.h"
#include "const.h"
#include "errno.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <assert.h>

i4_t killtran __P((void));
/*--------------------------------------------*/
char buffer[SOC_BUF_SIZE];
char res_buf[SOC_BUF_SIZE];
char *pointbuf = buffer;
i4_t initializing_base = 0;
i4_t commit_done = 0;

/*------------------------------------------------*/
i4_t 
Copy (void *v_to, void *v_from, i4_t len)
{
  i4_t curbufsize = pointbuf - buffer;
  
  if (v_to == pointbuf)
    {
      if (curbufsize + len > SOC_BUF_SIZE)
	return (-ER_BUF);
      pointbuf += len;
    }
  bcopy (v_from, v_to, len);
  return (len);
}

/* 
 * Committing the current transaction and storing
 * all chenges made by it
 */

i4_t
rollback (i4_t cpn)
{
  if (cpn==0)
    drop_statistic();
  return roll_back(cpn);
}

void 
commit (void)
{
  i4_t answer;
  
  if (commit_done)
    return;
  if (cl_debug)
    fprintf (STDOUT, "BASE:commit\n");
  
  answer = killtran ();
  commit_done = 1;
}

static void 
compiler_disconnect (void)
{
  if (!commit_done)
    {
      rollback(0);
      commit();
    }
}


/*----------------------------------------------------------*/
#define   FWR(tabid)    fwrite(&tabid,sizeof(Tabid),1,fb)
#define   FWRID(indid)  fwrite(&indid,sizeof(Indid),1,fb)
#define   FRD(tabid)    fread (&tabid,sizeof(Tabid),1,fb)
#define   FRDID(indid)  fread (&indid,sizeof(Indid),1,fb)
/*----------------------------------------------------------*/
i4_t 
initbas (void)
{
  static i4_t done = 0;
  static i4_t done1 = 0;
  FILE *fb;

  if (done || initializing_base)
    return 0;
  if (cl_debug)
    fprintf (STDOUT, "BASE: begin of initbas \n");
  if(!done1)
    {
      ATEXIT(compiler_disconnect);
      done1=1;
    }
  db_func_init ();
  
  fb = fopen (BASE_DAT, "r");
  if (fb == NULL)
    return (initializing_base) ? 0 : -1;
  fseek (fb, 0, SEEK_SET);

  FRD (systabtabid);
  FRD (syscoltabid);
  FRD (sysindtabid);
  FRD (viewstabid);
  FRD (tabauthtabid);
  FRD (colauthtabid);
  FRD (refconstrtabid);
  FRD (chconstrtabid);
  FRD (chcontwotabid);

  FRDID (indid1);
  FRDID (indid2);
  FRDID (indidcol);
  FRDID (indidcol2);
  FRDID (sysindexind);
  FRDID (sysauthindid);
  FRDID (syscolauthindid);
  FRDID (sysrefindid);
  FRDID (sysrefindid1);
  FRDID (chconstrindid);
  FRDID (chconstrtwoind);
  FRDID (viewsind);

  fclose (fb);
  
  if (cl_debug)
    fprintf (STDOUT, "BASE: end of initbas\n");
  done = 1;
  return 0;
}
