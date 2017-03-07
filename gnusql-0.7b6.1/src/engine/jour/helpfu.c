/*
 *  helpfu.c  - Common finctions of Logical Journal and Microjournal
 *              Kernel of GNU SQL-server. Journals 
 *
 * This file is a part of GNU SQL Server
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

/* $Id: helpfu.c,v 1.248 1998/09/30 02:39:46 kimelman Exp $ */

#include "setup_os.h"
#include <sys/types.h>
#if HAVE_SYS_FILE_H 
#include <sys/file.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <assert.h>

#include "rnmtp.h"
#include "pupsi.h"
#include "strml.h"
#include "fdeclmj.h"
#include "xmem.h"

static char pl1[RPAGE], pl2[RPAGE];
static char *push_ptr[2];

extern u2_t pushpn[2];		/* contains the page number*/
extern i4_t PROB[2];		/* state of JOURN-pushs (i=0,1) */
extern i4_t RFILE;
extern i4_t NB;			/* J-version number*/
extern i4_t ilast;		/* the number of page, contained the last page of jounal*/

#define size2b sizeof(u2_t)
#define size4b sizeof(i4_t)

char **
helpfu_init(void)
{
  push_ptr[0] = pl1;
  push_ptr[1] = pl2;
  return push_ptr;
}

void
getrfile (i4_t fd)
{
  struct stat buf;

  if (fstat (fd, &buf) < 0)
    ADM_ERRFU (MERR);
  RFILE = buf.st_size / (RPAGE);
}

void
get_last_page (i4_t fd)		/* Search the last page of a journal;*/
{        			/* ilast(global) is equal the number of the push */
				/* contained the last page */
  i4_t PCP, i;
  u2_t N;

  PROB[0] = -1;
  PROB[1] = -1;
  pushpn[0] = (u2_t) ~ 0;
  pushpn[1] = 0;
  READPG (1, 0, fd);
  NB = t4bunpack (push_ptr[0]);
  for (N = 1, i = 0;; N++)
    {
      ilast = i;
      if (N == RFILE)
	break;
      PCP = *(push_ptr[i] + size4b + size2b);
      if (PCP == SIGN_NOCONT)
	break;
      i = (i == 0) ? 1 : 0;
      READPG (N, i, fd);
      if (NB != t4bunpack (push_ptr[i]))
	{
	  i = (i == 0) ? 1 : 0;
	  break;
	}
    }
  ilast = i;
}

i4_t
get_page (i4_t j, u2_t N, i4_t fd) /* Prepare page number N into one push*/
                                  /* j - is the number of the current push */
{
  register i4_t k;

  k = (j == 0) ? 1 : 0;
  if (pushpn[k] == N)
    {
      if ((PROB[0] == 1) || (PROB[1] == 1))
	WAIT ((PROB[0] == 1) ? 0 : 1);
    }
  else
    {
      k = (j == ilast) ? k : j;
      READPG (N, k, fd);
    }
  return (k);
}

void
get_rec (char *pnt, struct ADBL adj, i4_t fd)
{
  register char *end_of_record;
  register i4_t k;
  register u2_t n, N, n1, razm;

  N = adj.npage;
  n = adj.cm;
  k = (N == pushpn[0]) ? 0 : ((N == pushpn[1]) ? 1 : (-1));
  if (k == (-1))
    {
      k = (ilast == 0) ? 1 : 0;
      READPG (N, k, fd);
    }
  n -= RTPAGE;
  if (n < RTBLK)
    {			/* the top-block places in two pages*/
      char buff[size2b];
      n1 = RTBLK - n;
      bcopy (push_ptr[k] + RTPAGE, buff + n1, n);
      k = get_page (k, pushpn[k] - 1, fd);
      end_of_record = push_ptr[k] + RPAGE - n1;
      bcopy (end_of_record, buff, n1);
      razm = t2bunpack (buff);
      n = RPAGE - RTPAGE - n1;
    }
  else
    {			/* the top-block places in (N)-page */
      end_of_record = push_ptr[k] + n + RTPAGE - RTBLK;
      razm = t2bunpack (end_of_record);
      n -= RTBLK;
      if (n == 0)
        {
          k = get_page (k, pushpn[k] - 1, fd);
          end_of_record = push_ptr[k] + RPAGE;
          n = RPAGE - RTPAGE;
        }
    }
  t2bpack (razm, pnt);
  pnt += size2b;
  assert (razm <= RPAGE);
  if (n < razm)
    {			/* block-record places in two pages */
      n1 = razm - n;
      bcopy (push_ptr[k] + RTPAGE, pnt + n1, n);
      k = get_page (k, pushpn[k] - 1, fd);
      bcopy (push_ptr[k] + RPAGE - n1, pnt, n1);
    }
  else			/*block-record places in (N)-page all*/
    bcopy (end_of_record - razm, pnt, razm);
}

void
MOREFILE (i4_t n)	/* Add n page to base-file and pictures them */
{
  RFILE = RFILE + n;
  PICTURE (RFILE - n + 1);
}

void
PICTURE (i4_t n)	/* Put standard top-journal and  top-pages into disk, */
{        	/* beginning with the page number "n" */
  register i4_t i, j;
  if (n > 1)
    j = (ilast == 0) ? 1 : 0;
  else
    j = 0;
  for (i = n; i <= RFILE; i++)
    {
      t4bpack (0, push_ptr[j]);
      PROB[j] = 2;
      pushpn[j] = i;
      out_push (j, 1);		/*with wait*/
    }
}

void
out_push (i4_t i, i4_t c)		/* Write push number "i" into disk;*/
{               		/*(if is it needed). c=1 -wait; c=0 -no wait; */
  

  if (PROB[i] == 1)
    {
      WAIT (i);
    }
  else if (PROB[i] == 2)
    {
      i4_t j;
      j = ((i == 0) ? 1 : 0);
      if (PROB[j] == 1)
	WAIT (j);
      write_disk (i, c);
    }
}

void
WAIT (i4_t i)			/* i- push number */
{
  PROB[i] = 0;
}

void
READPG (u2_t N, i4_t i, i4_t fd) /* Read "N"-page from JRN-basefile into i-push */
{
  if ((PROB[0] == 1) || (PROB[1] == 1))
    WAIT ((PROB[0] == 1) ? 0 : 1);
  if (lseek (fd, (i4_t) (RPAGE * (N - 1)), SEEK_SET) < 0)
    {
      perror ("JRN.lseek: READPG");
      exit (1);
    }
  if (read (fd, (i == 0) ? pl1 : pl2, RPAGE) != RPAGE)
    {
      perror ("JRN.read: READPG");
      exit (1);
    }
  pushpn[i] = N;
}

void
WRITEPG (i4_t i,i4_t c,i4_t fd)		/* Write push number "i" into page with number N */
{                        		/* of the JRN-basefile; c=1 -wait; c=0 -no wait; */
  i4_t N;

  N = pushpn[i];
  switch (c)
    {
    case 1:
      if (lseek (fd, (i4_t) (RPAGE * (N - 1)), SEEK_SET) < 0)
	ADM_ERRFU (MERR);
      if (write (fd, (i == 0) ? pl1 : pl2, RPAGE) != RPAGE)
	ADM_ERRFU (MERR);
      PROB[i] = 0;
      break;
    case 0:
      if (lseek (fd, (i4_t) (RPAGE * (N - 1)), SEEK_SET) < 0)
	ADM_ERRFU (MERR);
#if 0
      printf ("helpfu.WRITEPG: no wait fd = %d, N = %d\n", fd, N);
      for (j = 0, a = (i == 0) ? pl1 : pl2 ; j < 16; j++)
        printf (" %X", a[j]);
      printf ("\n");
#endif
      if (write (fd, (i == 0) ? pl1 : pl2, RPAGE) != RPAGE)
	ADM_ERRFU (MERR);
      PROB[i] = 1;
      break;
    default:
      printf ("err in wait-code\n");
      break;
    }
}

char *
write_topblock (u2_t size, u2_t off, char *a)
{
  if (off + RTBLK > RPAGE)
    {				/* the top-block places in two pages*/
      u2_t n, n1;
      char buff[size2b];
      n = RPAGE - off;
      t2bpack (size, buff);
      bcopy (buff, a, n);
      do_cont ();
      a = push_ptr[ilast] + RTPAGE;
      n1 = RTBLK - n;
      bcopy (buff + n, a, n1);
      a += n1;
    }
  else
    {				/* the top-block places in (N)-page */
      t2bpack (size, a);
      a += size2b;
    }
  return (a);
}

void
do_cont (void)
{
  register char *a;

  *(push_ptr[ilast] + size4b + size2b) = SIGN_CONT;
  out_push (ilast, 0);
  pushpn[(ilast == 0) ? 1 : 0] = pushpn[ilast] + 1;
  ilast = (ilast == 0) ? 1 : 0;
  a = push_ptr[ilast];
  t4bpack (NB, a);
  a += size4b;
  t2bpack (RTPAGE, a);
  a += size2b;
  *a = SIGN_NOCONT;
}
