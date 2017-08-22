/*
 *  mjipc.c  -  Interprocess communications of Microjournal
 *              Kernel of GNU SQL-server. Microjournal 
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

/* $Id: mjipc.c,v 1.247 1998/05/20 05:49:51 kml Exp $ */

#include "setup_os.h"
#include "xmem.h"
#include <sys/types.h>
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif
#if HAVE_SYS_MSG_H
#include <sys/msg.h>
#endif
#include "rnmtp.h"
#include "pupsi.h"
#include "strml.h"
#include "inpop.h"
#include "fdeclmj.h"

i4_t    msqidl, msqidm;
extern i4_t     fdmj;
extern i4_t     REDLINE;
extern i4_t     MPAGE;
extern struct  ADBL ABLOCK;

int
main (int argc, char **argv)
{
  i4_t op, rep;
  key_t keymj, keylj;
  struct msg_buf rbuf, sbuf;
  char *pnt, *mj_name;
  u2_t trnum, sz;
  struct ADBL admj;

  MEET_DEBUGGER;
  
  setbuf (stdout, NULL);
  mj_name = argv[1];
  sscanf (argv[2], "%d", &REDLINE);
  sscanf (argv[3], "%d", &MPAGE);
  {
    i4_t long_key;
    sscanf (argv[4], "%d", &long_key);
    keymj = long_key;
    sscanf (argv[5], "%d", &long_key);
    keylj = long_key;
    }
  PRINTF (("MJ.main: mj_name = %s, REDLINE = %d, MPAGE = %d\n",
           mj_name, REDLINE, MPAGE));
  rep = INI (mj_name);		/* MJ-file name */
  
  if ((msqidm = msgget (keymj, IPC_CREAT | DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("MJ.msgget: Queue for MJ");
      exit (1);
    }
  ans_mj (rep);
  PRINTF (("MJ.main: after ans_mj keymj = %d, keylj = %d\n", keymj, keylj));
  __MSGRCV(msqidm, &rbuf, 0, INIMJ, 0,"MJ.msgrcv: INI MJ");
  PRINTF (("MJ.main: before MSG_INIT mj_name = %s\n", mj_name));
  MSG_INIT (msqidl, keylj, "MJ");
  
  for (;;)
    {
      __MSGRCV(msqidm, &rbuf, RPAGE, -(ANSMJ - 1), 0,"MJ.msgrcv: Queue for MJ");
      op = rbuf.mtype;
      pnt = rbuf.mtext;
      switch (op)
	{
	case PUTBL:
	  BUFUPACK(pnt,trnum);
	  BUFUPACK(pnt,sz);
	  putbl (sz, pnt);
	  sbuf.mtype = trnum;
	  TPACK(ABLOCK,sbuf.mtext);
	  __MSGSND(msqidm, &sbuf, sizeof (struct ADBL), 0,
                   "MJ.msgsnd: Answer to TRN");
	  break;
	case GETBL:
	  BUFUPACK(pnt,trnum);
          bcopy (pnt, (char *) &admj, sizeof (struct ADBL));
	  get_rec (sbuf.mtext, admj, fdmj);
	  sbuf.mtype = trnum;
	  __MSGSND(msqidm, &sbuf, SZMSGBUF, 0,"MJ.msgsnd: Answer to TRN");
	  break;
	case OUTDISK:
	  outdisk (t2bunpack (pnt));
	  ans_buf ();
	  break;
	case DOFIX:
	  /*                     dofix((struct TOPJOUR *)pnt); */
	  dofix (pnt);
	  ans_buf ();
	  break;
	case STATEMJ:
/*        rep=ask();*/
	  ans_mj (rep);
	  break;
	case FINIT:
	  finit (mj_name);
	  break;
	default:
	  perror ("MJ.main: No such operation");
	  break;
	}
    }
  return 0; /* not reached */
}

void
LJ_ovflmj ()
{
  struct msg_buf sbuf;

  sbuf.mtype = OVFLMJ;
  __MSGSND(msqidl, &sbuf, 0, 0,"MJ.msgsnd: OVFLMJ");
}

void
ans_buf ()
{
  struct msg_buf sbuf;
  sbuf.mtype = ANSMJ;
  __MSGSND(msqidm, &sbuf, 0, 0,"MJ.msgsnd: Answer BUF");
}

void
ans_mj (char rep)
{
  struct msg_buf sbuf;
  sbuf.mtype = ANSMJ;
  sbuf.mtext[0] = rep;
  __MSGSND(msqidm, &sbuf, 1, 0,"MJ.msgsnd: Answer ADM");
}

i4_t
ask (void)
{
  i4_t rep;
  rep = 0;
  return (rep);
}

void
ADM_ERRFU (i4_t p)
{
#if 1 
  perror ("MJ. ERRFU");
#else
  struct msg_buf sbuf;
  sbuf.mtype = ERRFU;
  sbuf.mtext[0] = (char) p;
  sbuf.mtext[1] = MJ_PPS;
  __MSGSND(msqida, &sbuf, 2, 0,"MJ. ERRFU");
#endif
}

void
finit (char *mj_name)
{
  i4_t rep;

  mjini (mj_name);
  rep = close (fdmj);
  ans_mj (rep);
  exit(0);
}

void
mjini (char *mj_name)
{
  i4_t NV;
  static i4_t fdmj = 0;
  char page[RPAGE];

  /* the initialization of MJ */
  NV = 0;
  if (fdmj != 0)
    close (fdmj);
  unlink (mj_name);
  if ((fdmj = open (mj_name, O_RDWR | O_CREAT, DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("MJ: open error");
      exit (1);
    }
  t4bpack (NV, page);
  if (write (fdmj, page, RPAGE) != RPAGE)
    {
      perror ("MJ: write error");
      exit (1);
    }
}
