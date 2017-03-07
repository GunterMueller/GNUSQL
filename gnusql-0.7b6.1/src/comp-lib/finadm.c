/*
 *  finadm.c -  test of existence of GNU SQL server and downloading it
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

/* $Id: finadm.c,v 1.248 1998/09/30 02:39:02 kimelman Exp $ */

#include "setup_os.h"
#include "cl_lib.h"
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "dispatch.h"

int 
main (int argc, char **argv)
{
  gss_client_t  *cli_disp;
  static i4_t    int_arg = 0;
  char          *hostname = NULL;
  
  hostname = choose_host( argc > 1 ?argv[1]:NULL);
  fix_adm_port(NULL);
  if ( !hostname)
    {
      puts(clnt_error_msg());
      exit(1);
    }
  if (!(cli_disp = get_gss_handle (hostname, SQL_DISP, SQL_DISP_ONE, 1)))
    {
      puts(clnt_error_msg());
      exit (1); 
    }
  kill_all_1 (&int_arg, cli_disp);
  return 0;
}

