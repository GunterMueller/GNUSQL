/*
 *  ljipc.c  -  Interprocess communications of Logical Journal
 *              Kernel of GNU SQL-server. Journals 
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

/* $Id: ljipc.c,v 1.250 1998/09/29 22:23:35 kimelman Exp $ */

#include "setup_os.h"
#include "xmem.h"

#include <sys/types.h>
#ifdef HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif
#ifdef HAVE_SYS_MSG_H
#include <sys/msg.h>
#endif
#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <assert.h>

#include "pupsi.h"
#include "strml.h"
#include "inpop.h"
#include "fdecllj.h"

void INI (void);

i4_t msqida, msqidl, msqidbf, msqidm;
pid_t parent;
extern i4_t fdlj;
extern i4_t REDLINE;
extern i4_t MPAGE;
extern struct ADBL ABLOCK;

#define PRINT(x, y)  /*PRINTF((x, y))*/

int
main (int argc, char **argv)
{
  i4_t op, rep;
  key_t keyadm, keylj, keybf, keymj;
  char *pnt;
  u2_t trnum, sz;
  struct ADBL adlj;

  MEET_DEBUGGER;

  parent = getppid();
  setbuf (stdout, NULL);
  sscanf (argv[1], "%d", &fdlj);
  sscanf (argv[2], "%d", &REDLINE);
  sscanf (argv[3], "%d", &MPAGE);
  {
    i4_t long_key;
    sscanf (argv[4], "%d", &long_key);
    keylj = long_key;
    sscanf (argv[5], "%d", &long_key);
    keybf = long_key;
    sscanf (argv[6], "%d", &long_key);
    keyadm = long_key;
    sscanf (argv[7], "%d", &long_key);
    keymj = long_key;
  }
  
  if ((msqidl = msgget (keylj, IPC_CREAT | DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("LJ.msgget: Queue for LJ");
      exit (1);
    }
  MSG_INIT (msqidbf, keybf, "BUF");
  MSG_INIT (msqida, keyadm, "ADM");
  MSG_INIT (msqidm, keymj, "MJ");
  INI ();
  for (;;)
    {
      static struct msg_buf rbuf, sbuf;
      struct msg_buf *mp = &rbuf;
      __MSGRCV(msqidl, mp, 2 * RPAGE, -(ANSLJ - 1), 0,"LJ.msgrcv");
      op = mp->mtype;
/*      PRINT ("LJ: msgrcv op = %d\n", op);*/
      pnt = mp->mtext;
      switch (op)
	{
	case RENEW:
	  BUFUPACK(pnt,fdlj);
	  BUFUPACK(pnt,REDLINE);
	  rep = renew ();
	  ans_adm (rep);
	  break;
	case PUTOUT:
          BUFUPACK(pnt,trnum); 
          BUFUPACK(pnt,sz); 
	  putout (sz, pnt);
	  ans_trn (trnum, PUTOUT);
	  break;
	case PUTREC:
          BUFUPACK(pnt,trnum); 
          BUFUPACK(pnt,sz); 
	  putrec (sz, pnt);
	  ans_trn (trnum, PUTREC);
	  break;
	case PUTHREC:
          BUFUPACK(pnt,trnum); 
          BUFUPACK(pnt,sz);
	  PUTRC (sz, pnt);
	  ans_trn (trnum, PUTHREC);
	  break;
	case GETREC:
          BUFUPACK(pnt,trnum); 
          bcopy (pnt, (char *) &adlj, sizeof (struct ADBL));
	  get_rec (sbuf.mtext, adlj, fdlj);
	  sbuf.mtype = trnum;
	  __MSGSND(msqidl, &sbuf, SZMSGBUF, 0,
                   "LJ.msgsnd: Answer to TRN on GETREC");
	  break;
	case BEGFIX:
          PRINT ("LJ.main: before begfix op = %d\n", op);                    
	  begfix ();
	  break;
	case OVFLMJ:
          PRINT ("LJ.main: before overflowmj op = %d\n", op);          
	  overflow_mj ();
	  break;
        case INILJ:
          TUPACK(pnt,trnum);
	  INI ();
          sbuf.mtype = trnum;
	  __MSGSND(msqidl, &sbuf, 0, 0,
                   "LJ.msgsnd: Answer to MCR on INILJ");
	  break;
	case STATE:
          /*                        ask(); */
	  break;
        case FINIT:
          ans_adm (1);
          exit(0);
          break;
	default:
	  perror ("LJ.main: No such operation");
	  break;
	}
    }
}

i4_t
BUF_INIFIXB (struct ADBL addr_lj, i4_t nop)
{
  char *p;
  struct msg_buf mbuf;
  i4_t rep;

  PRINT ("LJ: BUF_INIFIXB: before connection to buf: msqidbf = %d\n", msqidbf);
  mbuf.mtype = INIFIXB;
  p = mbuf.mtext;
  t4bpack (nop, p);
  p += sizeof(nop);
  bcopy ((char *) &addr_lj, p, sizeof (struct ADBL));
  __MSGSND(msqidbf, &mbuf, sizeof(nop) + sizeof (struct ADBL), 0,
           "LJ.msgsnd: INIFIXBUF");
  /*  __MSGRCV(msqidbf, &mbuf, sizeof(i4_t), ANSBUF, 0,"LJ.msgrcv: INIFIXBF") */
  
  for (;;)
    {
      if (msgrcv (msqidl, (MSGBUFP)&mbuf, 2 * RPAGE, PUTHREC, IPC_NOWAIT) >=0 )
        {
          u2_t trnum, sz;
          p = mbuf.mtext;
          BUFUPACK(p,trnum); 
          BUFUPACK(p,sz);
	  PUTRC (sz, p);
	  ans_trn (trnum, PUTHREC);
        }
      if (msgrcv (msqidbf, (MSGBUFP)&mbuf, sizeof(rep), ANSBUF, IPC_NOWAIT) > 0)
        break;
    }
  rep = t4bunpack (mbuf.mtext);
  PRINT ("LJ: BUF_INIFIXB: after connection to buf: rep = %d\n", rep);
  return (rep);
}

void
ADML_COPY (void)
{
  struct msg_buf sbuf;
  t2bpack (COPY, sbuf.mtext);
  ADM_SEND (NUM_LJ /* LJ number in TABTR */, sizeof(u2_t), "LJ.msgsnd: COPY");
}

void
ADM_ERRFU (i4_t p)
{
#if 1
  perror ("LJ. ERROR");
#else
  struct msg_buf sbuf;
  sbuf.mtype = ERRFU;
  sbuf.mtext[0] = (char) p;
  sbuf.mtext[1] = LJ_PPS;
  __MSGSND(msqida, &sbuf, 1, 0,"LJ. ERROR");
#endif
}

void
ans_adm (i4_t rep)
{
  struct msg_buf sbuf;
  sbuf.mtype = ANSLJ;
  *sbuf.mtext = (char)rep;
  __MSGSND(msqidl, &sbuf, 1, 0,"LJ.msgsnd: Answer to ADM");
}

void
ans_trn (u2_t trnum, i4_t tpop)
{
  struct msg_buf sbuf;
  i4_t len;

  sbuf.mtype = (i4_t) trnum;
  if (tpop == PUTOUT)
    len = 0;
  else
    {
      len = sizeof (struct ADBL);
      TPACK(ABLOCK, sbuf.mtext);
    }
  __MSGSND(msqidl, &sbuf, len, 0,"LJ.msgsnd: Answer to TRN");
}

void
push_microj ()
{
  struct msg_buf sbuf;

  sbuf.mtype = OUTDISK;
  TPACK(ABLOCK, sbuf.mtext);
  __MSGSND(msqidm, &sbuf, sizeof(ABLOCK), 0, "LJ.msgsnd: OUTDISK"); /* Push MJ */
  __MSGRCV(msqidm, &sbuf, 0, ANSMJ, 0, "LJ.msgrcv");
}
