/*
 * admdef.h - Decarations for Storage and Transaction Synchrohization
 *            Management System Administrator of GNU SQL server
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


#ifndef __ADMDEF_H__
#define __ADMDEF_H__

/* $Id: admdef.h,v 1.247 1998/09/29 21:24:56 kimelman Exp $ */

#include <pupsi.h>

#define SETUPFL         GSQL_ROOT_DIR "/setup.txt"

#ifndef SERVBIN
#define SERVBIN         GSQL_ROOT_DIR "/bin"
#endif

#ifndef DBAREA
#define DBAREA          GSQL_ROOT_DIR "/db"
#endif

/*
 * the following declaration MUST be adequate to *_SVC identifiers
 * in dispatch.x and to names of associated programs -- see .../main/makefile
 */
#define SVC_FILES  {  		\
  SERVBIN "/gsqlt-boot",	\
  SERVBIN "/gsqlt-dyn" }

#define BUF             SERVBIN "/buf"
#define SYN             SERVBIN "/syn"
#define MJ              SERVBIN "/mj"
#define LJ              SERVBIN "/lj"
#define MCR             SERVBIN "/rcvmc"

#define MJFILE          DBAREA "/mjour"
#define LJFILE          DBAREA "/ljour"
#define ADMFILE         DBAREA "/admfile"
#define SEG1            DBAREA "/seg1"

#ifndef __gspstr_h___

void realop  __P((i4_t op, u2_t num, char *b));
void endotr  __P((i2_t num, i4_t exit_code));
void copylj  __P((void));
void dyn_change_parameters  __P((void));
void ini_adm_file  __P((char *));
void ini_lj  __P((char *));
void ini_mj  __P((char *));
void ini_BD_seg  __P((char *, u2_t BD_seg_scale_size));

#ifdef __dispatch_h__
i4_t  creatr  __P((init_arg *arg));
#endif

typedef struct debug_pid_t
{
  pid_t                debugger;
  pid_t                to_debug;
  struct debug_pid_t  *next;
  struct debug_pid_t  *prev;
} debug_pid_t;

extern debug_pid_t volatile *debuggers_pids;

struct des_trn
{
  i4_t   uidtr;
  pid_t  idprtr;
  i4_t   msgidtrn;
  time_t cretime;
  char   res_ready;
};

#define FIRSTMQN           2000
#define SEND_WAIT          60
#define TRN_SEND(code, sz) trn_send(code, sz, i)

void trn_send  __P((i4_t code, i4_t sz, i4_t trn_num));

#ifdef TRN_SEND_NEED

void
trn_send (i4_t code, i4_t sz, i4_t trn_num)
{
  i4_t rest_time = SEND_WAIT;
  
  if (tabtr[trn_num].idprtr && tabtr[trn_num].cretime)
    {
      kill (tabtr[trn_num].idprtr, SIGUSR1);
      sbuf.mtype = code;
      while (1)
	{
	  if (msgsnd (tabtr[trn_num].msgidtrn, (MSGBUFP)&sbuf, sz, 0) >= 0)
	    break;
	  if (--rest_time)
	    sleep (1);
	  else
	    {
	      perror ("ADM.msgsnd");
	      break;
	    }
	}
    }
}

#endif

#endif

#endif
