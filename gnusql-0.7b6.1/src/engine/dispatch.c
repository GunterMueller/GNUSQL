/*
 * dispatch.c - Storage and Transaction Synchrohization Management 
 *              System Administrator of GNU SQL server
 *
 * This file is a part of GNU SQL Server
 *
 * Copyright (c) 1996-1998, Free Software Foundation, Inc
 * Developed at the Institute of System Programming 
 * This file is written by Vera Ponomarenko
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU GeSETAneral Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contacts: gss@ispras.ru
 *
 */

/* $Id: dispatch.c,v 1.249 1998/09/29 21:24:57 kimelman Exp $ */

#include "setup_os.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 0xff) == 0)
#endif

/* for alarm() : */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "dispatch.h"
#include "global.h"
#include "inpop.h"
#include "totdecl.h"
#include <assert.h>

#define EXIT      finit(1)
#define RET(r)  { rest = (r); return &rest; }

i4_t default_num;

extern i4_t maxtrans, maxusetran;
extern struct des_trn *tabtr;
extern pid_t pidlj, pidmj, pidbf, pidsn, pidmcr;
extern i4_t msgida;
extern struct msg_buf rbuf, sbuf;
extern char nocrtr_fl, finit_fl, *fix_path, fix_lj_fl;
extern volatile i4_t finit_done;
extern volatile i4_t children;

#include "admdef.h"
void finit (i4_t err);

#define PRINT(x, y)  PRINTF((x, y))

/* child signals SIGUSR1 & SIGCHLD handler */

void
sig_usr_hnd (i4_t sig_num)
{

  if (sig_num != SIGALRM)
    {
      PRINT ("ADM: signal %d received ", sig_num);
      PRINT ("(children = %d)\n",children);
    }
  
  switch (sig_num)
    {
    case SIGUSR1 :
      while (1)
	{
	  if (msgrcv (msgida, (MSGBUFP)&rbuf,
		      BD_PAGESIZE, 0 /*NUM*/, IPC_NOWAIT) < 0)
	    break;
	  PRINT ("ADM.sig_hnd:  msgreceived mtype = %d\n", rbuf.mtype);
	  realop (t2bunpack (rbuf.mtext), rbuf.mtype/*NUM*/,
		  rbuf.mtext + sizeof(u2_t));
	}
      break;
      
    case SIGCHLD :
      while(1)
        {
          pid_t  pid;
          char  *who = NULL;
          i4_t    i = -1;
          i4_t    status;
          i4_t    exit_code = 0;
          i4_t    exited = 0;
          pid_t *to_clear = &pid;
          char   str[100];
          debug_pid_t *pd = NULL;
          static i4_t subdebuggers = 0;

          pid = waitpid ((pid_t)-1,&status,WNOHANG);
          if (!pid)
            break;

#define CHK(p,name) \
  if((pid==pid##p)||(-pid==pid##p)){to_clear=&(pid##p);who=name;}
          CHK(lj,"LJ");
          CHK(mj,"MJ");
          CHK(bf,"BUF");
          CHK(sn,"SYN");
          CHK(mcr,"MCR");
#undef CHK

          strcpy(str,"ADM.WAIT: ");
        
          if(who)
            strcat(str,who);
          else
            {
              for (i = NUM_LJ; i <= maxusetran; i++)
                if ((((tabtr[i]).idprtr) == pid) || (((tabtr[i]).idprtr) == -pid) )
                  {
                    to_clear = &((tabtr[i]).idprtr);
                    sprintf(str+strlen(str),"trans %d",i);
                    break;
                  }
              if ( i == maxusetran+1) /* if it isn't transaction or system service */
                for (pd = debuggers_pids; pd ; pd = pd->next)
                  if (pd->debugger == pid)     /* it should be debugger proccess   */
                    {
                      if (pd->next)
                        pd->next->prev = pd->prev;
                      if (pd->prev)
                        pd->prev->next = pd->next;
                      else
                        debuggers_pids = pd->next;
                      xfree(pd);
                      sprintf(str+strlen(str),"debugger %d",(i4_t)pid);
                      i = -1;
                      to_clear = &pid;
                      break;
                    }
              if ( (i == maxusetran+1)  /*       if it isn't even debugger and     */
                   && (subdebuggers>0)) /* there can be some subdebugger process   */
                {
                  sprintf(str+strlen(str),"debugger %d",(i4_t)pid);
                  i = -1;
                  subdebuggers--;
                  children++;
                  to_clear = &pid;
                }
            }
          PRINT ("ADM.sig_usr_hnd: i = %d", i);
          PRINT ("  maxusetran = %d", maxusetran);
          PRINT (" who = %s\n", (who?who:"null"));
          assert((i<= maxusetran) || (who));
          /* let's kill debugger if debugger process died */
          for (pd = debuggers_pids; pd ; pd = pd->next)
            if (pd->to_debug == pid)        /* it should be debugged proccess      */
              {                             /*                                     */
                kill(pd->debugger,SIGKILL); /* kill debugger                       */
                subdebuggers++;             /* and wait subdebuger exit            */
              }                             /*                                     */

          if (WIFEXITED (status))
            {
              exit_code = WEXITSTATUS(status);
              exited = 1;
              sprintf(str+strlen(str)," exited code(%d)\n",exit_code);
            }
          else if (WIFSIGNALED (status))
            {
              char buf[256]; 
              sprintf(buf,"test ! -f core || mv core core.%ld\n",pid);
              exit_code = -1;
              exited = 1;
              strcat(str," signalled\n");
              system(buf);
            }
          else if (WIFSTOPPED (status))
            strcat(str," stopped\n");
          else
            strcat(str," did something strange\n");
        
          PRINTF (("%s",str));
          if (exited)
            {
              i4_t is_killed = (-pid == *to_clear);
              children--;
              PRINTF ((">> children = %d\n",children));
              *to_clear = (pid_t)0;  /* clear process entry */
              if ( i >= 0 )
                endotr (i,(is_killed? 0 : exit_code ));
              else if (!is_killed && who )  /* we didn't kill it */
                {
                  if (to_clear==&pidmcr) /* MCR exited */
                    {
                      if (exit_code)
                        {
                          printf ("ADM.wait: Crash recovery failed - STOP\n");
                          kill_all(0);
                        }
                      else
                        printf ("ADM.wait: MCR exit\n");
                    }
                  else if (!finit_done)
                    {
                      printf ("ADM.wait: %s crash -- "
                              "kill all & start recovery\n"
                              ,who);
                      finit(-1); /* total stop and kill everything */
                    }
                }
              if(!children)
                {
                  kill_all(0);
                  break;
                }
            }
        } /* case SIGCHLD    */
      break;
    }   /* switch(sig_num) */
} /* sig_usr_hnd */

/* Initialization of transaction & interpretator (what == INTERPRET) *
 * or transaction & compiler (what == COMPILER) process.             *
 * arg->wait_time - after this time (in seconds) after last          *
 * client calling created process must be finished.                  *
 * Returns created process identifier or error (rpc_id < 0)          */

RPC_SVC_PROTO(res,init_arg,create_transaction,1)
{
  static i4_t trn_num;
  static res rest;
  static i4_t id[2];

  PRINT ("ADM.create_transaction:  nocrtr_fl = %d\n", nocrtr_fl);
  trn_num = (nocrtr_fl) ? -NOCRTR : creatr (in);
  PRINT ("ADM.create_transaction:  2 trn_num = %d\n", trn_num);
  if ( trn_num < 0 )
    { /* on error */
      rest.proc_id.opq_len = 0;
      rest.proc_id.opq_val = NULL;
      rest.rpc_id = trn_num;
    }
  else
    {
      rest.proc_id.opq_len = 2 * SZ_LNG;
      id[0] = trn_num;
      id[1] = tabtr[trn_num].cretime;
      rest.proc_id.opq_val = (char *)id;
      rest.rpc_id = DEFAULT_TRN + trn_num;
    }
  return &rest;
} /* transaction_create */

/* Checking of interpretator state.     *
 * Returns 0 if interpretator is ready, *
 * NEED_WAIT or error (<0) else         */

RPC_SVC_PROTO(i4_t,opq,is_ready,1)
{
  static i4_t rest;
  i4_t trn_num;
  
  /* Attempt to receive messages by dispatcher */
  while (1)
    {
      if (msgrcv (msgida, (MSGBUFP)&rbuf,
		  BD_PAGESIZE, 0 /*NUM*/, IPC_NOWAIT) < 0)
	break;
      PRINT ("ADM.is_ready:  msgreceived mtype = %d\n", rbuf.mtype);
      realop (t2bunpack (rbuf.mtext), rbuf.mtype,
	      rbuf.mtext + sizeof(u2_t));
    }
  
  if (!in || in->opq_len != SZ_LNG * 2 || !(in->opq_val))
    RET (-ER_CLNT);
  
  trn_num = ((i4_t *)(in->opq_val))[0];
  if (trn_num > maxusetran || !(tabtr[trn_num].idprtr) ||
      tabtr[trn_num].cretime != ((i4_t *)(in->opq_val))[1])
    RET (-TRN_EXITED);
    
  PRINT ("ADM.IS_READY: request from trn %d\n", trn_num);
  
  switch (tabtr[trn_num].res_ready)
    { 
    case 0 : /* it's first request from client */
      kill ( tabtr[trn_num].idprtr, SIGUSR2);
      tabtr[trn_num].res_ready = 2;
      
    case 2 : /* it isn't first client's request & *
	      * interpretator isn't ready         */ 
      rest = NEED_WAIT;
      break;
      
    case 1 : /* interpretator is ready for sending of the result */
      tabtr[trn_num].res_ready = 0;
      rest = 0;
      break;
      
    }      
  
  return &rest;
} /* is_ready */

/* Compilation of SQL text.                       *
 * Returns error code in elem.indicator & (if not *
 * error) path to created module (header.seq)     */

/* Stop all works. All child process will be killed with  *
   making ROLLBACK()  & dispatcher will finish it's work. */

RPC_SVC_PROTO(i4_t,int,kill_all,1)
{
  static i4_t rest;
  
  finit(0); 
  RET (0);
}

/* Process of transaction (for interpretator *
 * or compilator) finishing                  */

RPC_SVC_PROTO(i4_t,opq,trn_kill,1)
{
  i4_t i;
  static i4_t rest = 0;
    
  if (!in || in->opq_len != SZ_LNG * 2 || !(in->opq_val))
    RET (-TRN_ID);
  
  i = ((i4_t *)(in->opq_val))[0];
  if (i <= maxtrans && tabtr[i].idprtr &&
      tabtr[i].cretime == ((i4_t *)(in->opq_val))[1])
    {
      TRN_SEND (FINIT, 0);
    }
  RET (0);
}

/* Waiting for all existing transactions finished &       *
   exiting. Dispatcher will not handle operation TRN_INIT */

RPC_SVC_PROTO(i4_t,int,disp_finit,1)
{
  static i4_t rest;
  
  nocrtr_fl = 1;
  finit_fl = 1;
  RET (0);
}

i4_t 
cp_lj_reg (i4_t to_sz, char *to)
{
  i4_t i, trn_exist = 0;
  
  fix_path = (char *) xmalloc (to_sz+1);
  bcopy (to, fix_path, to_sz);
  fix_path[to_sz] = 0;
  
  for (i = FIR_TRN_NUM; i <= maxusetran; i++)
    if (tabtr[i].idprtr)
      {
	trn_exist++;
	break;
      }
  if (trn_exist)
    {
      nocrtr_fl = 1;
      fix_lj_fl = 1;
    }
  else
    copylj();
  return 0;
}

/* Waiting for all existing transactions finished   *
 * & log journal fixing. Dispatcher will not handle *
 * operation TRN_INIT before it's fixing.           *
 * Returns 0 if it's O'K or < 0 if error            */
RPC_SVC_PROTO(i4_t,opq,copy_lj,1)
{
  static i4_t rest;
  
  RET (cp_lj_reg (in->opq_len, in->opq_val));
}

RPC_SVC_PROTO(i4_t,int,change_params,1)
{
  static i4_t int_res = 0;
  
  dyn_change_parameters();
  return &int_res;
}

