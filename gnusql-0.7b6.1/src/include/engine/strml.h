/*
 *  strml.h -  Structures of Joutnals
 *             Kernel of GNU SQL-server
 *                
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Vera Ponomarenko
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
 *  Contacts: gss@ispras.ru
 *
 */

/* $Id: strml.h,v 1.246 1998/09/29 21:26:08 kimelman Exp $ */

#ifndef __STRML_H__
#define __STRML_H__

#include "setup_os.h"
#include <sys/types.h>
#include <signal.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

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


#define RTPAGE  7           /* page-top       size (in bytes)*/
#define RTBLK   2           /* jour-block-top size (in bytes)*/
#define RTJOUR  5           /* MJ-journal-top size (in bytes)*/

#define RPAGE       (BD_PAGESIZE*2)

#define SIGN_CONT   1
#define SIGN_NOCONT 0

struct ADBL {
    u2_t npage;
    u2_t cm;
};
struct TOPJOUR {
    struct ADBL AKLJF;
    char pxr;
};
struct ADREC {
    u2_t razm;
    char block [RPAGE];
};

struct TOPPAGE {
    i4_t  version_number;
    u2_t lastb_off;
    char  sign_cont; 
};

#define ADM_SEND(tp, len, err_txt)  {		\
  sbuf.mtype = tp;                        	\
  if (msgsnd (msqida, (MSGBUFP)&sbuf, len, 0) < 0)  \
    { perror (err_txt); exit (1);  }           	\
  kill(parent, SIGUSR1);			\
}
                               
#define ANSW_WAIT 10000 /* maximum of time for waiting answer */
#define DEFAULT_ACCESS_RIGHTS 0660

#ifndef EXIT
#define EXIT exit (1)
#endif

#define MSG_INIT(id, key, str)              \
 {                                          \
  time_t tm = time (NULL);                  \
  while (1)                                 \
    {                                       \
      if ((id = msgget (key, DEFAULT_ACCESS_RIGHTS)) >= 0)   \
	break;                              \
      if (tm + ANSW_WAIT < time (NULL))     \
	{                                   \
	  perror (str);                     \
	  EXIT;                             \
	}                                   \
      sleep (1);                            \
    }                                       \
 }

#endif
