/*
 *  logj.c  -   Logical Journal
 *              Kernel of GNU SQL-server. Logical Journal 
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

/* $Id: logj.c,v 1.248 1998/09/30 02:39:47 kimelman Exp $ */

#include "setup_os.h"
#include <assert.h>

#include "rnmtp.h"
#include "pupsi.h"
#include "strml.h"
#include "fdecllj.h"
#include "xmem.h"

#define size2b sizeof(i2_t)
#define size4b sizeof(i4_t)

static char **push_ptr;		/* push-pointers*/
u2_t pushpn[2];		        /* contains the page number*/
i4_t PROB[2];			/* state of LJ-pushs (i=0,1) :
	                         PROB[i]=-1 - initial state;
	                         PROB[i]= 2 - filling of a push is continuing(PUTREC,PUTHREC,PUTOUT)
	                         PROB[i]= 1 - write operation of a push on disk has began,
	                                      a push has loaded; 
	                         PROB[i]= 0 - write operation of a push on disk has finished,
	                                      a push has loaded again  */

i4_t ilast;			/* the number of page, contained the last page of journal*/
i4_t RFILE;			/*                          */
i4_t REDLINE;			/*                          */
i4_t MPAGE;
i4_t fdlj;			/* log file descriptor */
struct ADBL ABLOCK;		/* blocks address */
i4_t NB;			/* log version number */
struct ADREC RREC;		/* razmer-record */
i4_t NOP;			/* operation counter */

void
INI ()
{
  struct ADBL adlj;
  push_ptr = helpfu_init();
  getrfile (fdlj);
  get_last_page (fdlj);
  if (NB == 0)
    {
      NB = 1;
      t4bpack (NB, push_ptr[ilast]);
    }
  adlj.npage = pushpn[ilast];
  adlj.cm = t2bunpack (push_ptr[ilast] + size4b);
  NOP = 0;
  /*  rep = BUF_INIFIXB (adlj, NOP);*/
}

i4_t
renew ()
{
  i4_t rep;
  struct ADBL adlj;

  READPG (1, 0, fdlj);
  NB = t4bunpack (push_ptr[0]);
  NB += 1;
  PRINTF (("LOGJ.renew: NB = %d, REDLINE = %d\n", (i4_t)NB, REDLINE));  
  t4bpack (NB, push_ptr[0]);
  t2bpack (RTPAGE, push_ptr[0] + size4b);
  *(push_ptr[0] + size4b + size2b) = SIGN_NOCONT;	/*PCP=free*/
  WRITEPG (0, 1, fdlj);
  adlj.npage = 1;
  adlj.cm = RTPAGE;
  rep = BUF_INIFIXB (adlj, NOP);
  NOP = 0;
  ilast = 0;
  return (rep);
}

void
PUTRC (u2_t razm, char * block)
{
  register u2_t off;
  register char *a, *lastb;

  assert (razm < RPAGE);
  if (PROB[ilast] == 1)
    WAIT (ilast);
  PROB[ilast] = 2;
  a = push_ptr[ilast] + size4b;
  off = t2bunpack (a);
  a = push_ptr[ilast] + off;
  a = write_topblock (razm, off, a);
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
putrec (u2_t razm, char * block) /*This function puts block with record into LJ*/
{
  PUTRC (razm, block);
  NOP++;
}

void
putout (u2_t razm, char * block) /* This function puts block with record and */
                 		  /* writes all push-buffers into disk */
{
  PUTRC (razm, block);
  PRINTF (("LJ.putout: NOP = %d, REDLINE = %d, npage = %d, off = %d\n",
           NOP, REDLINE, ABLOCK.npage, ABLOCK.cm));
  push_microj ();
  out_push (ilast, 1);		/*with wait*/
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
      PRINTF (("LOGJ.write_disk: reguest copy NB = %d, pagenum = %d\n",
               (i4_t)NB, N));
      ADML_COPY ();
      REDLINE = 0;
    }
  WRITEPG (i, c, fdlj);
}

void
overflow_mj ()
{
  if (NOP != 0)
    {
      struct ADBL adlj;
      i4_t rep;
      out_push (ilast, 1);
      adlj.npage = pushpn[ilast];
      adlj.cm = t2bunpack (push_ptr[ilast] + size4b);
      rep = BUF_INIFIXB (adlj, NOP);
      NOP = 0;
    }
}
void
begfix (void)			/*This function writes all push-buffers into disk and sends SPB*/
{
  struct ADBL adlj;
  i4_t rep;

  out_push (ilast, 1);
  adlj.npage = pushpn[ilast];
  adlj.cm = t2bunpack (push_ptr[ilast] + size4b);
  rep = BUF_INIFIXB (adlj, NOP);
  NOP = 0;
  ans_adm (rep);
}

