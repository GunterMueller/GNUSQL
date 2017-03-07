/*
 *  microj.c  - Microjournal
 *              Kernel of GNU SQL-server. Microjournal
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

/* $Id: microj.c,v 1.249 1998/09/30 02:39:47 kimelman Exp $ */

#include "setup_os.h"
#if HAVE_UNSTD_H
#include <unistd.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "sql.h"
#include <assert.h>

#include "rnmtp.h"
#include "pupsi.h"
#include "strml.h"
#include "fdeclmj.h"
#include "xmem.h"

#define size2b sizeof(i2_t)
#define size4b sizeof(i4_t)

static char **push_ptr;		/* push-pointers*/
u2_t pushpn[2]; 		/*contains the page number*/
i4_t PROB[2];			/*state of MJ-pushs (i=0,1) :
                               PROB[i]=-1 - initial state;
                               PROB[i]= 2 - filling of a push is continuing (PUTBL);
                               PROB[i]= 1 - write operation of a push on disk has began,
	                                      a push has loaded;
                               PROB[i]= 0 - write operation of a push on disk has finished,
	                                      a push has loaded again */
i4_t ilast;			/* the karman number contained  the last page of a microjournal*/
i4_t RFILE;			/* size of MJ-file (in pages) */
i4_t REDLINE, RET_RLINE;
i4_t MPAGE;
i4_t fdmj = 0;			/* log file descriptor */
struct ADBL ABLOCK;		/* blocks address */
i4_t NB;			/* log version number */


int
INI (char *pnt)			/* MJ- initialisation */
{
  push_ptr = helpfu_init();
  if ((fdmj = open (pnt, O_RDWR, 0644)) < 0)
    {
      perror ("MJ: open error");
      exit (1);
    }

  getrfile (fdmj);
  get_last_page (fdmj);
  RET_RLINE = REDLINE;
  if (NB == 0)
    {
      PICTURE (2);
      NB = 1;
      t4bpack (NB, push_ptr[ilast]);
      if (write (fdmj, (char *) &NB, size4b) != size4b)
	{
	  perror ("MJ: write error");
	  exit (1);
	}
    }
  else
    {
      return (MJ_PPS);
    }
  return (OK);
}

void
putbl (u2_t razm, char * block) 
{    /*this function puts block ADBL in the last page MJ */
  register u2_t off;
  register char *a, *lastb;

  assert (razm < RPAGE);
  if (PROB[ilast] == 1)
    WAIT (ilast);
  PROB[ilast] = 2;
  a = push_ptr[ilast] + size4b;
  off = t2bunpack (a);
  a = push_ptr[ilast] + off;
  lastb = push_ptr[ilast] + RPAGE;
  if (a + razm > lastb)
    {				/* block-record places in two page */
      u2_t n, n1;
      n1 = lastb - a;
      bcopy (block, a, n1);
      do_cont ();
      a = push_ptr[ilast] + RTPAGE;
      n = razm - n1;
      bcopy (block + n1, a, n);
      a += n;
    }
  else
    {
      bcopy (block, a, razm);
      a += razm;
    }
  a = write_topblock (razm, a - push_ptr[ilast], a);
  off = a - push_ptr[ilast];
  t2bpack (off, push_ptr[ilast] + size4b);
  ABLOCK.cm = off;
  ABLOCK.npage = pushpn[ilast];
}

void
dofix (char *pnt)	/*this function puts a new first page into MJ*/
     			/*NEWTOP- contains a new top for the microjournal */
{
  register char *a;

   READPG (1, 0, fdmj);
  pushpn[1] = 0;
  a = push_ptr[0];
  NB += 1;
  if (NB == 0)
    NB = 1;
  t4bpack (NB, a);
  a += size4b;
  t2bpack (RTPAGE + RTJOUR, a);
  a += size2b;
  *a++ = 1;
  t2bpack (t2bunpack (pnt), a);
  a += size2b;
  t2bpack (t2bunpack (pnt + size2b), a);
  a += size2b;
  *a = *(pnt + 2 * size2b);
  WRITEPG (0, 1, fdmj);
  REDLINE = RET_RLINE;
  ilast = 0;
  PRINTF (("MJ.dofix: NB = %d\n", (i4_t)NB));
}

void
outdisk (u2_t n)	/* Test, all blocks are */
			/* in the disk (beginning with "ADR"-block);*/
{			/* if they aren't, output needed page*/
  if (n == pushpn[ilast])
    out_push (ilast, 1);	/*with wait*/
}

void
write_disk (i4_t i, i4_t c)	/* Write push number "i" to disk */
                                /* c= 1 - wait, 0 - no wait;*/
{
  i4_t N;

  N = pushpn[i];
  if (N > RFILE)
    MOREFILE (MPAGE);

  if (N == REDLINE)
    {
      LJ_ovflmj ();
      REDLINE = 0;
    }
  WRITEPG (i, c, fdmj);
}
