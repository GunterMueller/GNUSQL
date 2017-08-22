/*  snipc.c - Interprocess communication of Synchronizer
 *            Kernel of GNU SQL-server. Synchronizer    
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

/* $Id: snipc.c,v 1.251 1998/09/28 06:09:07 kimelman Exp $ */

#include "setup_os.h"
#include <sys/types.h>
#include <sys/times.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>

#include "xmem.h"
#include "dessnch.h"
#include "inpop.h"
#include "f1f2decl.h"
#include "fdclsyn.h"
#include "strml.h"

#ifndef CLK_TCK
#define CLK_TCK 60
#endif

static struct tms buffer;
static i4_t tt, syntime;

long msqid, msqidb;

extern char *segadd;
struct A page;
#define AR     256

static void
opusk_tr (u2_t trnum)
{
  struct msg_buf sbuf;
  
  sbuf.mtype = trnum;
  __MSGSND(msqid, &sbuf, 0, 0,"SYN.msgsnd");
}

static void
finit (void)
{
  PRINTF (("SYN.finit: syntime=%ld(msec)\n", (long)syntime * 1000 / CLK_TCK));
  exit (0);
}

void
main (i4_t argc, char **argv)
{
  i4_t op =-1 , sz;
  key_t keysn, keybf;
  struct msg_buf rbuf;
  struct id_rel idrel;
  u2_t trnum;
  CPNM cpn;
  char *p, lin;
  COST cost;

  MEET_DEBUGGER;

  setbuf (stdout, NULL);
  times (&buffer);
  tt = buffer.tms_utime;
  sscanf (argv[1], "%d", &keysn);
  sscanf (argv[2], "%d", &keybf);
  if ((msqid = msgget (keysn, IPC_CREAT | DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("SN.msgget");
      exit (1);
    }
  MSG_INIT (msqidb, keybf, "BUF");
  for (;;)
    {
      __MSGRCV(msqid, &rbuf, BD_PAGESIZE, -(ANSSYN - 1), 0,"SN.msgrcv");
      times (&buffer);
      tt = buffer.tms_utime;
      op = rbuf.mtype;
      p = rbuf.mtext;
      PRINTF (("SYN:::: rcv %d \n",op));
      switch (op)
	{
	case START:
	  trnum = t2bunpack (p);
	  synstart (trnum);
	  break;
	case LOCK:
          BUFUPACK(p,trnum);
          BUFUPACK(p,idrel);
          BUFUPACK(p,cost);
	  lin = *p++;
          BUFUPACK(p,sz);
	  cpn = lock (trnum, idrel, cost, lin, sz, p);
	  break;
	case SVPNT:
	  trnum = t2bunpack (p);
	  cpn = svpnt_syn (trnum);
	  answer_opusk (trnum, cpn);
	  break;
	case UNLTSP:
          BUFUPACK(p,trnum);
          bcopy (p, (char *) &cpn, sizeof (CPNM));
	  unltsp (trnum, cpn);
	  opusk_tr (trnum);
	  break;
	case COMMIT:
	  trnum = t2bunpack (p);
          PRINTF(("SYN:::: run COMMIT\n"));
	  commit (trnum);
	  opusk_tr (trnum);
	  break;
	case FINIT:
	  finit ();
	  break;
	default:
	  perror ("SYN.main: No such operation");
	  break;

	}
      times (&buffer);
      tt = buffer.tms_utime - tt;
      syntime += tt;
      PRINTF(("SYN:::: rcv %d end \n",op));
    }
}

void
answer_opusk (u2_t trnum, CPNM cpn)
{
  /* this function looks to be totally undebugged */
  struct msg_buf sbuf;
  
  sbuf.mtype = trnum;
  /* make sure we don't receive this message by ourself */
  assert(trnum > FINIT ); 
  *(CPNM *) sbuf.mtext = cpn;
  __MSGSND(msqid, &sbuf, sizeof (CPNM), 0,"SYN.msgsnd");
}

char *
getpage (u2_t trn, u2_t segn, u2_t pn)
{
  struct msg_buf sbuf;
  char *p;
  i4_t shmid;
  key_t keyseg;
  char *shm;

  sbuf.mtype = LOCKGET;
  p = sbuf.mtext;
  BUFPACK(trn,p);
  BUFPACK(segn,p);
  BUFPACK(pn,p);
  *p = WEAK;
  keyseg = BACKUP;
  while (keyseg == BACKUP)
    {
      __MSGSND(msqidb, &sbuf, 3 * size2b + 1, 0,"SYN.msgsnd: LOCKGET to BUF");
      __MSGRCV(msqidb, &sbuf, sizeof (key_t), trn, 0,"SYN.msgrcv: LOCKPAGE from BUF");
      keyseg = *(key_t *) sbuf.mtext;
    }
  if ((shmid = shmget (keyseg, BD_PAGESIZE, DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("SYN.shmget");
      exit (1);
    }
  if ((shm = shmat (shmid, NULL, 0)) == (char *) -1)
    {
      perror ("SYN.shmat");
      exit (1);
    }
  page.p_shm = shm;
  page.p_sn = segn;
  page.p_pn = pn;
  return (shm);
}

void
putpage (u2_t trn)
{
  struct msg_buf sbuf;
  char *p;

  sbuf.mtype = PUTUNL;
  p = sbuf.mtext;
  BUFPACK(trn,p);
  BUFPACK(page.p_sn, p);
  BUFPACK(page.p_pn, p);
  t4bpack (0, p);
  p += size4b;
  *p++ = PRNMOD;
  __MSGSND(msqidb, &sbuf, p - sbuf.mtext, 0,"SYN.msgsnd: PUTUNL to BUF");
  __MSGRCV(msqidb, &sbuf, sizeof (i4_t), trn, 0,"SYN.msgrcv: PUTUNL from BUF");
  shmdt (page.p_shm);
}

