/*
 *  runadm.c  -  test of existence of DB Dispatcher and run it if absent
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Konstantin Dyshlevoi.
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

/* $Id: runadm.c,v 1.248 1998/09/30 02:39:02 kimelman Exp $ */

#include "setup_os.h"
#include <stdio.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "pupsi.h"
#include "dispatch.h"

#define CREATE_WAIT_TIME 60

int
main (int argc, char **argv)
{
  CLIENT *clnt;
  pid_t pid;
  i4_t rest_time = CREATE_WAIT_TIME;
  
  if ((clnt = clnt_create ("localhost", SQL_DISP, SQL_DISP_ONE, "tcp")))
    {
      clnt_destroy (clnt);
      return 0; /* Dispatcher already exists */
    }

  SVC_UNREG(SQL_DISP, SQL_DISP_ONE);
  
  if ((pid = fork ())< 0)
    {
      perror ("fork adm");
      exit (1);
    }
  if (pid == 0)
    {
      printf ("TESTADM: before execvp()\n");
      argv[0] = GSQL_ROOT_DIR "/gsqls";
      argv[1] = NULL;
      execvp (*argv, argv);
    }
  else
    while (1)
      {
	if ((clnt = clnt_create ("localhost", SQL_DISP, SQL_DISP_ONE, "tcp")))
	  {
	    clnt_destroy (clnt);
	    break;
	  }
	if (--rest_time)
	  sleep (1);
	else
	  {
	    printf ("Can't connect to server");
	    break;
	  }
	
	printf ("Wait_create: sleep");
      }
  exit (0);
}
