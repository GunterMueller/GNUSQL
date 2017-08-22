/*
 *  db_cre_cli.c -  creating DB catalogs (client part)
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
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

/* $Id: db_cre_cli.c,v 1.246 1998/07/30 03:23:35 kimelman Exp $ */

#include "setup_os.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "cl_lib.h"
#include "gsqltrn.h"

#define ANSWER_WAIT_TIME     2000   /* time of waiting for a result  *
				     * from transaction & dispatcher */
#define TRN_CLIENT_WAIT_TIME 3000   /* time of transaction's waiting *
				     * for client's request          */
i4_t 
main (i4_t argc, char **argv)
{
  gss_client_t *cli_disp;
  result_t     *int_res;
  int           i;

  for(i=0;i<10;i++)
    {
      
      cli_disp = create_service(argv[1], BOOT_SVC, ANSWER_WAIT_TIME,
                                TRN_CLIENT_WAIT_TIME, -1);
      if (cli_disp)
        break;
      sleep(i+1);
    }

  if (!cli_disp)
    {
      printf ("DBcreate: %s\n",clnt_error_msg());
      exit (1);
    }
  
  int_res = db_create_1 (NULL, cli_disp);
  if (!int_res)
    printf ("Data Base creating fatal error\n");
  else if (int_res->sqlcode < 0)
    printf ("Data Base creating error \n");
  down_svc (cli_disp);
  return 0;
}
