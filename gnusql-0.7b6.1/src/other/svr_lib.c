/*
 *  svr_lib.c  -  high level support library of GNU SQL server
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Michael Kimelman
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
 *  Contact: gss@ispras.ru
 *
 */

/* $Id: svr_lib.c,v 1.248 1998/09/29 21:26:30 kimelman Exp $ */

#include "setup_os.h"


#include <sys/types.h>
#if HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif
#if HAVE_SYS_MSG_H
#include <sys/msg.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <signal.h>

#include "global.h"
#include "destrn.h"
#include "strml.h"
#include "inpop.h"
#include "totdecl.h"

static time_t last_req_tm;
static i4_t    max_wait;   /* if client hasn't sent any request last *
			   * max_wait seconds => exit               */

static struct msg_buf rbuf, sbuf;

#define ALARM { last_req_tm = time (NULL); if (max_wait > 0) alarm (max_wait); }

static volatile i4_t res_ready  = 0;
static volatile i4_t need_ans   = 0;
i4_t      rpc_program_id = 0;
i4_t      static_sql_fl   = 1;    /* == 0 for dynamic SQL */

extern pid_t     parent;
extern i4_t       msqida, msqidt;
extern i4_t       TRNUM;
extern i4_t      minidnt;
void trans_init (i4_t argc,char *args[]);

/* 
 * server signals SIGUSR1 && SIGUSR2 && SIGALRM handler 
 */

static RETSIGTYPE
sig_serv_hnd (i4_t sig_num
#if SA_SIGINFO
              , siginfo_t *usr_info, void *addp
#endif
              )
{
#if SA_SIGINFO
  if (usr_info)
    {
      if ( usr_info->si_pid != parent || 
	   ( (sig_num != SIGUSR1) && (sig_num != SIGUSR2)))
	return;
    }
  else if (sig_num != SIGALRM)
      return;
#endif
  
  switch (sig_num)
    {
    case SIGUSR1 : /* message from dispatcher is arrived */
      rbuf.mtype = 0;
      
      PRINTF (("sig_serv_hnd: Waiting for message from dispatcher\n"));
      
      while (msgrcv (msqidt, (MSGBUFP)&rbuf, sizeof(i4_t), 0, 0) < 0) {}
      
      PRINTF (("sig_serv_hnd: Message received mtype = %d\n",
               (int)rbuf.mtype));
      
      if (rbuf.mtype == GETMIT)
	minidnt = t4bunpack (rbuf.mtext);
      else /* rbuf.mtype == FINIT : finishing all works */
	exit(0);
      break;
      
    case SIGUSR2 : /* dispatcher asks to send RES_READY when processing *
                    * of current request'll be finished                 */
      if (res_ready)
	{
	  t2bpack (RES_READY, sbuf.mtext);
	  ADM_SEND (TRNUM, size2b, "TRN.msgsnd: RES_READY to ADM");
	  need_ans = 0;
	}
      else
	need_ans = 1;
      break;
      
    case SIGALRM :   /* client hasn't been working with  *
		      *	transaction for a i4_t time      */
      if (res_ready) /* i_send isn't working now */
	{
	  i4_t wt_rest = last_req_tm + max_wait - time (NULL);
	  
	  if (wt_rest > 0)
	    alarm ((unsigned int)wt_rest);
	  else
	    exit(0);
	}
    }
} /* sig_usr_hnd */

/* Server (transaction process) initialization */

/* returns 0 if O'K or <0 if base.dat was not readed */
int
server_init (i4_t argc, char *args[])
{
  struct sigaction act;
  sigset_t         set;
  i4_t             res = 0;
  
  sigemptyset (&set);
  sigaddset (&set, SIGUSR1);
  sigaddset (&set, SIGUSR2);

#ifdef SA_SIGINFO
  act.sa_handler = NULL;
  act.sa_sigaction = sig_serv_hnd;
  act.sa_mask = set;
  act.sa_flags = SA_SIGINFO;
#else
  act.sa_handler = sig_serv_hnd;
  act.sa_mask = set;
  act.sa_flags = 0;
#endif
  
  if (sigaction (SIGUSR1, &act, NULL) ||
      sigaction (SIGUSR2, &act, NULL) ||
      sigaction (SIGALRM, &act, NULL)   )
    exit (1);

  res = initbas();
  trans_init (argc, args);
  
  max_wait    = atoi(args[10]);
  ALARM;
  
  rpc_program_id = atoi(args[11]);
  if (argc > 12 )
    current_user_login_name = savestring(args[12]);
  return res;
} /* server_init */

void 
start_processing(void)
{
  res_ready = 0;
  need_ans  = 0;
  ALARM;
}

i4_t 
finish_processing(void)
{
  res_ready=1;
  ALARM;
  if (need_ans) /* client isn't waiting for result from i_send *
		 * client will take it from i_rcv              */
    {
      t2bpack (RES_READY, sbuf.mtext);
      ADM_SEND (TRNUM, size2b, "TRN.msgsnd: RES_READY to ADM");
      need_ans = 0;
      return 0;
    }
  return 1;
}

#undef is_ready_1
#undef is_ready_1_svc

RPC_SVC_PROTO(i4_t,void,is_started,1)
{
  static i4_t rc = 1;
  finish_processing();
  return &rc;
}
