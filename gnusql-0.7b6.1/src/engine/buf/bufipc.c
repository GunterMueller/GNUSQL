/*
 *  bufipc.c  - Buffer communication functions 
 *              Kernel of GNU SQL-server. Buffer
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

/* $Id: bufipc.c,v 1.255 1998/09/29 22:23:34 kimelman Exp $ */

#include "setup_os.h"

#ifdef HAVE_SYS_FILES_H
#include <sys/file.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/times.h>

#include "fdeclbuf.h"

#include <sys/msg.h>
#include <sys/shm.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef STDC_HEADERS
#include <string.h>
#else
#include <strings.h>
#endif

#include <sql.h>
#include "rnmtp.h"
#include "inpop.h"
#include "pupsi.h"
#include "bufdefs.h"
#include "strml.h"
#include "totdecl.h"

#ifndef CLK_TCK
#define CLK_TCK 60
#endif

#if defined(HEAVY_IPC_DEBUG)
#define PRINT(x, y) PRINTF((x, y))
#else
#define PRINT(x, y) /*PRINTF((x, y))*/
#endif

#define size4b sizeof(i4_t)
#define size2b sizeof(i2_t)

static struct tms buffer;
static i4_t tt, buftime;

extern int errno;
i4_t msqidm, msqidbf;
i4_t MAXNBUF, MAXTACT, MAXNOP, MAXSEGNUM;
key_t FSEGNUM;
i4_t N_buf, N_opt;
char PRXR = 0;

i4_t *fdseg;
char **tabdseg;
char **seg_file_name;
i4_t N_seg;
pid_t parent;
i4_t max_buffers_number;

u2_t I_fix = NO;		/* indicator of fixation */

i4_t N_log = 0;			/* number of operations by log */
i4_t N_op = 0;			/* number of ended operations */

#define ARG(num, what, type)   sscanf (argv[num], "%d", &argum); \
                               what = (type)(argum)

int
main (int argc, char **argv)
{
  i4_t i, k;
  i4_t n;
  key_t keymj, keybf;
  i4_t argum;
  
  MEET_DEBUGGER;

  setbuf (stdout, NULL);

  ARG(1, keybf, key_t);
  ARG(2, keymj, key_t);
  ARG(3, N_opt, i4_t);
  ARG(4, MAXNBUF, i4_t);
  ARG(5, MAXTACT, i4_t);
  ARG(6, MAXNOP, i4_t);
  ARG(7, FSEGNUM, key_t);
  ARG(argc - 1, parent, pid_t);
  /* 
   * reading DB segments information 
   */
  for (i = 8, n = 0; i < argc - 1; i += 2)
    {
      if (!sscanf (argv[i], "%d", &k))
	break;
      if (n < k)
	n = k;
    }
  /* 'n' contains the maximum number of segment */
  MAXSEGNUM = n + 1;
  /* allocating space for segment information */
  fdseg         = (i4_t *)  xmalloc (MAXSEGNUM * sizeof (i4_t));
  seg_file_name = (char **) xmalloc (MAXSEGNUM * sizeof (char **));
  for (i = 0; i < MAXSEGNUM ; i++)
    {
      fdseg[i] = -1;
      seg_file_name[i] = NULL;
    }

  for (i = 8; i < argc - 1; i+=2)
    {
      sscanf (argv[i], "%d", &k);
      seg_file_name[k] = xmalloc (strlen (argv[i+1]));
      strcpy (seg_file_name[k], argv[i+1]);
    }
  /*---------------------------------------------------------*/
  if ((msqidbf = msgget (keybf, IPC_CREAT | DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("BUF.msgget: Queue for BUF");
      exit (1);
    }
  MSG_INIT (msqidm, keymj, "MJ");
  init_hash ();
  tabdseg = (char **) xmalloc (MAXNBUF * sizeof (char *));
  for (i = 0; i < MAXNBUF; i++)
    tabdseg[i] = NULL;
  max_buffers_number = MAXNBUF;
  N_buf = 0;
  N_seg = 0;
  
  for (;;)
    {
      msg_rcv ();
      tact ();
    }
}

#define UNPACKMSG(p) { BUFUPACK(p,trnum);BUFUPACK(p,segn);BUFUPACK(p,pn);}

void
msg_rcv (void)
{
  struct msg_buf rbuf;
  u2_t trnum, pn, segn;
  char *p, *p1;
  register i4_t op;
  i4_t admj;
  struct BUFF *buf;

  rbuf.mtype = 0;
      
  PRINT ("BUF.msg_rcv: msqidbf = %d\n", (i4_t)msqidbf);
  
  __MSGRCV(msqidbf, &rbuf, SZMSGBUF, -(ANSBUF - 1), 0,"BUF.msgrcv");
      
  PRINT ("BUF.msg_rcv: rbuf.mtype = %d\n", rbuf.mtype);
      
  times (&buffer);
  tt = buffer.tms_utime;
  op = rbuf.mtype;
  p = rbuf.mtext;
  switch (op)
    {
    case INIFIXB:
      inifixb (t4bunpack (p), t4bunpack (p + size4b));
      break;
    case GETPAGE:
      UNPACKMSG(p);
      buf = get (segn, pn, 'r');
      buf_to_user (trnum, (key_t) buf->b_seg->keyseg);
      break;
    case PUTPAGE:
      UNPACKMSG(p);
      BUFUPACK(p,admj);
      put (segn, pn, admj, *p);
      user_p (trnum, 0);
      break;
    case LOCKPAGE:
      UNPACKMSG(p);
      if (segn >= (u2_t) MAXSEGNUM || seg_file_name[segn] == NULL)
	user_p (trnum, -ER_NO_SUCH_SEG);
      else
	{
          PRINTF (("BUF.msg_rcv: LOCKPAGE segn=%d,pn = %d,f_name=%s\n",
                  segn, pn, seg_file_name[segn]));
	  if (buflock (trnum, segn, pn, *p, 0) == 0)
	    user_p (trnum, 0);
	}
      break;
    case ENFORCE:
      UNPACKMSG(p);
      if (enforce (trnum, segn, pn) == 0)
	user_p (trnum, 0);
      break;
    case UNLKPG:
      UNPACKMSG(p);
      unlock (segn, pn, p);
      user_p (trnum, 0);
      break;
    case NEWGET:
      UNPACKMSG(p);
      buf = get (segn, pn, 'n');
      buf_to_user (trnum, (key_t) buf->b_seg->keyseg);
      break;
    case LOCKGET:
      UNPACKMSG(p);
      if (segn >= (u2_t) MAXSEGNUM || seg_file_name[segn] == NULL)
	buf_to_user (trnum, (key_t)-ER_NO_SUCH_SEG);
      else if (buflock (trnum, segn, pn, *p, 1) == 0)
        {
          buf = get (segn, pn, 'r');
          buf_to_user (trnum, (key_t) buf->b_seg->keyseg);
	}
      break;
    case PUTUNL:
      p1 = p + 2 * size2b;
      UNPACKMSG(p);
      BUFUPACK(p,admj);
      put (segn, pn, admj, *p);
      unlock (segn, 1, p1);
      user_p (trnum, 0);
      break;
    case ENDOP:
      end_op ();
      break;
    case BEGTACT:
      tact ();
      break;
    case OPTNUM:
      optimal (t4bunpack (p));
      MAXTACT = t4bunpack (p + size4b);
      break;
    case FINIT:
      finit ();
      break;
    default:
      printf ("BUF.msg_rcv: No such operation in interface BUF\n");
      break;
    }
  times (&buffer);
  tt = buffer.tms_utime - tt;
  buftime += tt;
}

void
buf_to_user (u2_t trnum, key_t keys)
{
  struct msg_buf sbuf;

  sbuf.mtype = trnum;
  TPACK(keys, sbuf.mtext);
  __MSGSND(msqidbf, &sbuf, sizeof (key_t), 0,"BUF.msgsnd: Answer TRN");
}

void
user_p (u2_t trnum, i4_t type)
{
  struct msg_buf sbuf;
  sbuf.mtype = trnum;
  TPACK(type, sbuf.mtext);
  __MSGSND(msqidbf, &sbuf, size4b, 0,"BUF.msgsnd: Answer TRN");
}

/*  Functions realizing calls of Microlog */

i4_t cur_endmj = NULL_MICRO;	/* current end of microlog */
i4_t pushaddmj = NULL_MICRO;	/* last push MJ address */

void
push_micro (i4_t addr)		/* push microlog buffer */
{
  u2_t pn, pn_push, off, off_push;
  char *a, *b;

  a = (char *) &addr;
  b = (char *) &pushaddmj;
  pn = t2bunpack (a);
  pn_push = t2bunpack (b);
  off = t2bunpack (a + size2b);
  off_push = t2bunpack (b + size2b);

  if (pn > pn_push)
    {
      pushmicro (addr);
    }
  else if (pn == pn_push && off > off_push)
    {
      pushmicro (addr);
    }
}

void
pushmicro (i4_t addr)
{
  struct msg_buf sbuf;

  sbuf.mtype = OUTDISK;
  t4bpack (addr, sbuf.mtext);
  __MSGSND(msqidm, &sbuf, size4b, 0,"BUF.msgsnd: OUTDISK");
  __MSGRCV(msqidm, &sbuf, 0, ANSMJ, 0,"BUF.msgrcv");
  pushaddmj = addr;
}
struct des_seg *
new_seg (void)			/* Create a segment */
{
  register i4_t i;
  struct des_seg *desseg;
  key_t key;
  i4_t shmid;

  for (i = 0; i < max_buffers_number; i++)
    if ((desseg = (struct des_seg *) tabdseg[i]) == NULL)
      {
        desseg = (struct des_seg *) get_empty (sizeof (struct des_seg));
        key = FSEGNUM + i;
        while ((shmid = shmget (key, BD_PAGESIZE,
                                IPC_CREAT | DEFAULT_ACCESS_RIGHTS)) < 0)
          {
            if ( (N_buf + 1) < N_opt)
              fprintf (stderr, "BUF.new_seg: number of buffers is "
                       "less optimal number of buffers\n");
            waitfor_seg (N_buf - 1);
            max_buffers_number = N_buf;
            /*            
            perror ("BUF.shmget");
            exit (1);
            */
          }
        desseg->keyseg = key;
        desseg->idseg = shmid;
        tabdseg[i] = (char *) desseg;
        N_seg++;
        return (desseg);        
      }
  return (NULL);  
}

void
del_seg (struct des_seg *desseg)
{
  struct shmid_ds shmstr;
  register i4_t i;  

  shmctl (desseg->idseg, IPC_RMID, &shmstr);
  for (i = 0; i < MAXNBUF; i++)
    if (desseg == (struct des_seg *) tabdseg[i])
      {
	tabdseg[i] = NULL;
        break;
      }
  xfree (desseg);
  N_seg--;
}

/*****************************************************************************

                                FIXATION
*/

void
inifixb (i4_t nop, i4_t ljadd)
{
  struct msg_buf sbuf;

  N_log = nop;
  if (N_op != N_log)
    fix_mode ();
  push_micro (cur_endmj);
  do_fix ();
  sbuf.mtype = DOFIX;		/* Fixation */
  t4bpack (ljadd, sbuf.mtext);
  t4bpack (PRXR, (char*)sbuf.mtext+size4b);
  __MSGSND(msqidm, &sbuf, size4b + 1, 0,"BUF.msgsnd: DOFIX");
  __MSGRCV(msqidm, &sbuf, 0, ANSMJ, 0,"BUF.msgrcv");
  I_fix = NO;
  N_op = 0;
  sbuf.mtype = ANSBUF;
  t4bpack (0, sbuf.mtext);
  __MSGSND(msqidbf, &sbuf, sizeof (i4_t), 0,"BUF.msgsnd: ANSLJ");
}

/**************************** real fixation *********************************/

extern struct BUFF *prios[PRIORITIES];	/* priority rings */

void
do_fix (void)
{
  u2_t prio;
  struct BUFF *buf;

  for (prio = 0; prio < PRIORITIES; prio++)
    if ((buf = prios[prio]) != NULL)
      do
	{
	  push_buf (buf);
	  buf = buf->b_next;
	}
      while (buf != prios[prio]);
  cur_endmj = NULL_MICRO;
  pushaddmj = NULL_MICRO;
}

/**************************** fixation mode *********************************/

void
fix_mode (void)
{
  I_fix = YES;
  while (N_op != N_log)
    msg_rcv ();
}

/*************************** end of operation *******************************/

void
end_op (void)
{
  N_op++;
}

/****************************** weak error **********************************/

void
weak_err (char *text)
{
  printf ("%s.\n", text);
}

/***************************** read/write buffer ********************************/

void
read_buf (struct BUFF *buf)
{
  u2_t segn, pn;
  char *shm;
  off_t n;
  i4_t fd;

  segn = buf->b_page->p_seg;
  pn = buf->b_page->p_page;

  if (fdseg[segn] < 0)
    if ((fdseg[segn] = open (seg_file_name[segn], O_RDWR, 0644)) < 0)
      {
        printf ("BUF.read_buf: seg_file_name = %s\n", seg_file_name[segn]);
	perror ("SEGFILE: open error");
	exit (1);
      }
  fd = fdseg[segn]; 
  if ((n = lseek (fd, BD_PAGESIZE * pn, SEEK_SET)) < 0)
    {
/*      printf ("BUF.read_buf: lseek error errno=%d, fd = %d\n", errno, fdseg[segn]);*/
      ADM_ERRFU (MERR);
    }
  if ((shm = shmat (buf->b_seg->idseg, NULL, 0)) == (char *) -1)
    {
      perror ("BUF.shmat");
      exit (1);
    }
  if (read (fd, shm, BD_PAGESIZE) < 0)
    {
      printf ("BUF.read_buf: read error errno=%d\n", errno);
      ADM_ERRFU (MERR);
    }
  if (shmdt (shm) < 0)
    {
      perror ("BUF.shmdt");
      exit (1);
    }
}

void
write_buf (struct BUFF *buf)
{
  u2_t segn, pn;
  char *shm;
  off_t n;
  i4_t fd;

  segn = buf->b_page->p_seg;
  pn = buf->b_page->p_page;

  if (fdseg[segn] < 0)
    if ((fdseg[segn] = open (seg_file_name[segn], O_RDWR, 0644)) < 0)
      {
        printf ("BUF.write_buf: sn = %d, seg_file_name = %s\n", segn, seg_file_name[segn]);
	perror ("SEGFILE: open error");
	exit (1);
      }
  fd = fdseg[segn];
  if ((n = lseek (fd, BD_PAGESIZE * pn, SEEK_SET)) < 0)
    {
      ADM_ERRFU (MERR);
    }
  if ((shm = shmat (buf->b_seg->idseg, NULL, 0)) == (char *) -1)
    {
      perror ("BUF.shmat");
      exit (1);
    }
  if (write (fd, shm, BD_PAGESIZE) != BD_PAGESIZE)
    ADM_ERRFU (MERR);
  if (shmdt (shm) < 0)
    {
      perror ("BUF.shmdt");
      exit (1);
    }
  buf->b_prmod = PRNMOD;
  buf->b_micro = NULL_MICRO;
}

void
ADM_ERRFU (i4_t p)
{
#if 1
  perror ("BUF. ERRFU");
#else
  struct msg_buf sbuf;
  sbuf.mtype = ERRFU;
  sbuf.mtext[0] = (char) p;
  sbuf.mtext[1] = BF_PPS;
  __MSGSND(msqida, &sbuf, 2, 0,"BUF. ERRFU");
#endif
}

void
finit (void)
{
  register i4_t i;
  struct shmid_ds shmstr;
  struct msg_buf sbuf;
  struct BUFF *buf;

  for (i = 0; i < MAXSEGNUM; i++)
    if (fdseg[i] >= 0)
      close (fdseg[i]);
  xfree ((char *) fdseg);
  PRINTF (("BUF.finit: segments are closed\n"));
  for (i = 0; i < MAXSEGNUM; i++)
    if (seg_file_name[i] != NULL)
      xfree ((char *) seg_file_name[i]);
  xfree ((char *) seg_file_name);
  PRINTF (("BUF.finit: seg_file_names are free \n"));
  for (i = 0; i < PRIORITIES; i++)
    {
      PRINTF (("BUF.finit: prios[%d]=%d\n", i, (i4_t) prios[i]));
      if ((buf = prios[i]) != NULL)
	do
	  {
	    if (buf->b_seg != NULL)
	      {
		PRINTF (("BUF.finit: idseg=%d\n", buf->b_seg->idseg));
		shmctl (buf->b_seg->idseg, IPC_RMID, &shmstr);
	      }
	  }
	while ((buf = buf->b_next) != prios[i]);
    }
  PRINTF (("BUF.finit.e: buftime=%ld(msec)\n",
           buffer.tms_utime * 1000 / CLK_TCK));

  sbuf.mtype = ANSBUF;
  sbuf.mtext[0] = 0;
  __MSGSND(msqidbf, &sbuf, 1, 0,"BUF.msgsnd: Answer ADM");
  sleep(1); /* allow adm to get a mesage before CHLD signal */
  exit(0);
}

void
waitfor_seg (i4_t buf_num)
{
  struct msg_buf rbuf;
  u2_t trnum, pn, segn;
  char *p, *p1;
  register i4_t op;
  i4_t admj;

  for (; (N_buf + 1) >= buf_num;)
    {
      rbuf.mtype = 0;
      
      PRINT ("BUF.waitfor_seg: msqidbf = %d\n", (i4_t)msqidbf);
      
      __MSGRCV(msqidbf, &rbuf, BD_PAGESIZE, -(ANSBUF - 1), 0,"BUF.msgrcv");
      
      PRINT ("BUF.waitfor_seg: rbuf.mtype = %d\n", rbuf.mtype);
      
      op = rbuf.mtype;
      p = rbuf.mtext;
      switch (op)
	{
	case INIFIXB:
	  inifixb (t4bunpack (p), t4bunpack (p + size4b));
	  break;
	case PUTPAGE:
          UNPACKMSG(p);
	  BUFUPACK(p,admj);
	  put (segn, pn, admj, *p);
	  user_p (trnum, 0);
	  break;
	case LOCKPAGE:
          UNPACKMSG(p);
	  if (segn >= (u2_t) MAXSEGNUM || seg_file_name[segn] == NULL)
	    user_p (trnum, -ER_NO_SUCH_SEG);
	  else if (buflock (trnum, segn, pn, *p, 0) == 0)
            user_p (trnum, 0);
	  break;
	case ENFORCE:
          UNPACKMSG(p);
	  if (enforce (trnum, segn, pn) == 0)
	    user_p (trnum, 0);
	  break;
	case UNLKPG:
          UNPACKMSG(p);
	  unlock (segn, pn, p);
	  user_p (trnum, 0);
	  break;
	case PUTUNL:
	  p1 = p + 2*size2b;
          UNPACKMSG(p);
	  BUFUPACK(p,admj);
	  put (segn, pn, admj, *p);
	  unlock (segn, 1, p1);
	  user_p (trnum, 0);
	  break;
	case ENDOP:
	  end_op ();
	  break;
	case BEGTACT:
	  tact ();
	  break;
	case OPTNUM:
	  optimal (t4bunpack (p));
          MAXTACT = t4bunpack (p + size4b);          
	  break;
	case FINIT:
	  finit ();
	  break;
	default:
	  printf ("BUF.waitfor_seg: The operation op=%d is disabled\n", op);
	  break;
	}
      tact ();
    }
}

/******************************** the end ***********************************/
