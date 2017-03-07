/*
 *  page_op.c  - Operations with external storage pages (transaction)
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

/* $Id: page_op.c,v 1.252 1998/09/29 21:25:42 kimelman Exp $ */

#include "setup_os.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <assert.h>

#include "xmem.h"
#include "destrn.h"
#include "strml.h"
#include "fdcltrn.h"

extern u2_t trnum;
extern COST cost;
extern struct ADBL admj;
extern struct ADBL adlj;
extern struct ADREC blmj;
extern struct ADREC bllj;
extern i4_t msqidb, msqidm, msqidl;
extern i4_t N_AT_SEG;
extern char *bufmj;
extern char *pbuflj;
extern char *pbufmj;

#define COOKMSG(mcode,ptr)  {			\
  sbuf.mtype = mcode;				\
  ptr = sbuf.mtext;				\
  BUFPACK(trnum, ptr);				\
  BUFPACK(sn, ptr);				\
  BUFPACK(pn, ptr);				\
}

char *
getpg(struct A *ppage, u2_t sn, u2_t pn, i4_t type)
{
  struct msg_buf sbuf, rbuf;
  char *p;
  key_t keyseg;
  i4_t shmid;
  char *shm;

  COOKMSG(LOCKGET,p);
  *p = (type == 'x'? STRONG : WEAK );
  __MSGSND(msqidb, &sbuf, 3 * size2b + 1, 0,"TRN.msgsnd: LOCKGET to BUF");
  __MSGRCV(msqidb, &rbuf, sizeof (key_t), trnum, 0,
           "TRN.msgrcv: LOCKGET from BUF");
  TUPACK(rbuf.mtext, keyseg);
  if (keyseg == BACKUP)
    return (NULL);
  if (keyseg == -ER_NO_SUCH_SEG)
    {
      fprintf (stderr, "TRN.getpg: No such segment seg_num = %d\n", sn);
      /*      return (NULL);*/
      exit (0);
    }
  if ((shmid = shmget (keyseg, BD_PAGESIZE, DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("TRN.shmget");
      exit (1);
    }
  if ((shm = (char *) shmat (shmid, NULL, 0)) == (char *) -1)
    {
      printf ("TR.getpg: shmid=%d,sn=%d,pn=%d,errno=%d\n",
              shmid, sn, pn, errno);
      perror ("TRN.shmat");
      exit (1);
    }
  N_AT_SEG++;
  ppage->p_shm = shm;
  ppage->p_sn = sn;
  ppage->p_pn = pn;
  cost += 1;
  return (shm);
}

char *
getwl (struct A *ppage, u2_t sn, u2_t pn)
{
  struct msg_buf sbuf;
  char *p;
  i4_t shmid;
  char *shm;

  COOKMSG(GETPAGE,p);
  __MSGSND(msqidb, &sbuf, 3 * size2b, 0,
           "TRN.msgsnd: GETPAGE to BUF");
  __MSGRCV(msqidb, &sbuf, sizeof (key_t), trnum, 0,
           "TRN.msgrcv: GETPAGE from BUF");
  if ((shmid = shmget (*(key_t *) sbuf.mtext,
                       BD_PAGESIZE, DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("TRN.shmget");
      exit (1);
    }
  if ((shm = (char *) shmat (shmid, NULL, 0)) == (char *) -1)
    {
      printf ("TR.getwl: shmid=%d,sn=%d,pn=%d,errno=%d\n", shmid, sn, pn, errno);
      perror ("TRN.shmat");
      exit (1);
    }
  N_AT_SEG++;
  ppage->p_shm = shm;
  ppage->p_sn = sn;
  ppage->p_pn = pn;
  cost += 1;
  return (shm);
}

char *
getnew (struct A *ppage, u2_t sn, u2_t pn)
{
  struct msg_buf sbuf;
  i4_t shmid;
  char *shm, *p;

  COOKMSG(NEWGET,p);
  __MSGSND(msqidb, &sbuf, 3 * size2b, 0,"TRN.msgsnd: NEWGET to BUF");
  __MSGRCV(msqidb, &sbuf, sizeof (key_t), trnum, 0,
           "TRN.msgrcv: NEWGET from BUF");
  if ((shmid = shmget (*(key_t *) sbuf.mtext,
                       BD_PAGESIZE, DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("TRN.shmget");
      exit (1);
    }
  if ((shm = (char *) shmat (shmid, NULL, 0)) == (char *) -1)
    {
      perror ("TRN.shmat");
      exit (1);
    }
  N_AT_SEG++;
  ppage->p_shm = shm;
  ppage->p_sn = sn;
  ppage->p_pn = pn;
  cost += 1;
  return (shm);
}

static void
detach_page (struct A *ppage, char type,int msgtype)
{
  struct msg_buf sbuf;
  char *asp, *p;
  u2_t sn, pn;

  asp = ppage->p_shm;
  sn = ppage->p_sn;
  pn = ppage->p_pn;
  
  COOKMSG(msgtype,p);
  if (type == 'm')
    {
      if (sn == NRSNUM)
	{
          perror ("TRN.put_page: sn = NRSNUM");
          exit (1);
	}
      else
	{
	  BUFPACK(admj, p);
	  ((struct p_head *) asp)->csum = CCS (asp);
	}
      *p = (char) PRMOD;
    }
  else
    {
      bzero(p,size4b);
      p += size4b;
      *p = (char) PRNMOD;
    }
  __MSGSND(msqidb, &sbuf, 5 * size2b + 1, 0,"TRN.msgsnd: PUTPAGE to BUF");
  __MSGRCV(msqidb, &sbuf, sizeof (i4_t), trnum, 0,
           "TRN.msgrcv: PUTPAGE from BUF");
  shmdt (asp);
  ppage->p_shm = NULL;
  N_AT_SEG--;
  cost++;
}

void
putpg (struct A *ppage, i4_t type)
{
  if (ppage->p_sn == NRSNUM)
    {
      perror ("TRN.putpg: sn = NRSNUM");
      exit (1);
    }
  detach_page (ppage, type,PUTUNL);
}

void
putwul (struct A *ppage, i4_t type)
{
  detach_page (ppage, type,PUTPAGE);
}

void
BUF_unlock (u2_t sn, u2_t lnum, u2_t * mpn)
{
  struct msg_buf sbuf,rbuf;
  u2_t *p;
  u2_t i = 0;
  u2_t n, count = 0;

  if (lnum == 0)
    return;
  sbuf.mtype = UNLKPG;
  p = (u2_t *) sbuf.mtext;
  BUFPACK(trnum,p);
  BUFPACK(sn,p);
  n = (SZMSGBUF-sizeof(trnum)-sizeof(sn)-sizeof(n))/size2b;
  do
    {
      p = (u2_t *) sbuf.mtext + 2; 
      if ((count + n) > lnum)
        n = lnum - count;
      BUFPACK(n,p);
      for (i = 0; i < n; i++)
        *p++ = *mpn++;
      count += n;
      __MSGSND(msqidb, &sbuf, (3 + n) * size2b, 0,"TRN.msgsnd: UNLKPG to BUF");
      __MSGRCV(msqidb, &rbuf, sizeof (i4_t), trnum, 0,"TRN.msgrcv: UNLKPG from BUF");
      cost += 1;
    }
  while (count < lnum);
}

i4_t
BUF_lockpage (u2_t sn, u2_t pn, i4_t type)
{
  struct msg_buf sbuf;
  char *p;
  i4_t   answer;

  COOKMSG(LOCKPAGE,p);
  *p = (type == 'x' ? STRONG:WEAK);
  __MSGSND(msqidb, &sbuf, 3 * size2b + 1, 0,"TRN.msgsnd: LOCKPAGE to BUF");
  __MSGRCV(msqidb, &sbuf, sizeof (i4_t), trnum, 0,
           "TRN.msgrcv: LOCKPAGE from BUF");
  answer = t4bunpack( sbuf.mtext);
  cost += 1;
  if (answer == BACKUP)
    return ( -1);
  else
    return (0);
}

i4_t
BUF_enforce (u2_t sn, u2_t pn)
{
  struct msg_buf sbuf;
  i4_t    answer;
  char  *p;

  COOKMSG(ENFORCE,p);
  __MSGSND(msqidb, &sbuf, 3 * size2b, 0,"TRN.msgsnd: ENFORCE to BUF");
  __MSGRCV(msqidb, &sbuf, sizeof (i4_t), trnum, 0,
           "TRN.msgrcv: ENFORCE from BUF");
  answer = *(i4_t *) sbuf.mtext;
  if (answer == BACKUP)
    return ( -1);
  else
    return (0);
}

void
MJ_PUTBL (void)
{
  char *p;
  struct msg_buf sbuf;
  u2_t sz;

  sbuf.mtype = PUTBL;
  p = sbuf.mtext;
  BUFPACK(trnum, p);
  sz = blmj.razm = pbufmj - bufmj;
  BUFPACK(sz, p);
  bcopy (bufmj, p, sz);
  sz += 2 * size2b;
  assert (sz < SZMSGBUF);
  __MSGSND(msqidm, &sbuf, sz, 0,"TRN.msgsnd: MJ PUTBL");
  __MSGRCV(msqidm, &sbuf, adjsize, trnum, 0,"TRN.msgrcv: Answer from MJ");
  TUPACK(sbuf.mtext,admj);
  pbufmj = bufmj;
}

void
LJ_put (i4_t type)
{
  char *p;
  struct msg_buf sbuf;
  u2_t sz;

  sbuf.mtype = type;
  p = sbuf.mtext;
  BUFPACK(trnum, p);
  sz = bllj.razm;
  BUFPACK(sz, p);
  bcopy (pbuflj, p, sz);
  sz += 2 * size2b;
  assert (sz < SZMSGBUF);
  __MSGSND(msqidl, &sbuf, sz, 0,"TRN.msgsnd: LJ PUT");
  __MSGRCV(msqidl, &sbuf, adjsize, trnum, 0,"TRN.msgrcv: Answer from LJ");
  TUPACK(sbuf.mtext,adlj);
}
