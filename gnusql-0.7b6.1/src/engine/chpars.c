/*
 *  chpars.c -  change parameters of GNU SQL server at runtime
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Vera Ponomarenko.
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

/* $Id: chpars.c,v 1.249 1998/09/30 02:39:03 kimelman Exp $ */

#include "setup_os.h"
#include <sys/types.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "engine/dispatch.h"
#include "dyngspar.h"
#include "gspstr.h"

#define RESPONSE     \
      scanf ("%c", input); \
      if (input[0] == 'y') {\
        printf ("Enter  ");\
        scanf ("%d", &in_num);\
      }
      
static void
write_new_pars(void)
{
  i4_t fddp, length;
  struct DGSPARAM dyn_par;
  char input[10];
  i4_t in_num;

  printf ("DO you want to change max free extents in System? y/n"
          "  (recommend = %d)\n", D_MAX_FREE_EXTENTS_NUM);
  RESPONSE;
  if (in_num >= 0 && in_num <= D_MAX_FREE_EXTENTS_NUM && input[0] == 'y')
    dyn_par.d_max_free_extents_num = in_num;
  else
    dyn_par.d_max_free_extents_num = D_MAX_FREE_EXTENTS_NUM;
  
  printf ("DO you want to change size of Logical "
          "journal in journals pages? y/n (recommend = %d)\n",
          D_LJ_RED_BOUNDARY);
  scanf ("%c", input);
  RESPONSE;  
  if (in_num > 0 && input[0] == 'y')
    dyn_par.d_lj_red_boundary = in_num;
  else
    dyn_par.d_lj_red_boundary = D_LJ_RED_BOUNDARY; 
  
  printf ("DO you want to change optimal buffers "
          "number in System? y/n (recommend = %d)\n", D_OPT_BUF_NUM);
  scanf ("%c", input);  
  RESPONSE;
  if (in_num > 0 && input[0] == 'y')
    dyn_par.d_opt_buf_num = in_num;
  else
    dyn_par.d_opt_buf_num = D_OPT_BUF_NUM;
    
  printf ("DO you want to change max tacts number for"
          " Buffer in System? y/n (recommend = %d)\n", D_MAX_TACT_NUM);
  scanf ("%c", input);  
  RESPONSE;
  if (in_num > 0 && input[0] == 'y')
    dyn_par.d_max_tact_num = in_num;
  else
    dyn_par.d_max_tact_num = D_MAX_TACT_NUM;
  
  unlink (DYNPARS);
  if ( (fddp = open (DYNPARS, O_RDWR|O_CREAT, 0660)) <0)
    {
      perror ("DYNPARS: open error");
      exit (1);
    }

  length = sizeof (struct DGSPARAM);
  if (write (fddp, (char *)&dyn_par, length) != length) {
    perror ("DYNPARSE: write error");
    exit (1);
  } 
  close (fddp);
}

int 
main (int argc, char **argv)
{
  gss_client_t   *cli_disp;
  static i4_t     int_arg = 0;
  char           *hostname = NULL;

  setbuf (stdout, NULL);
  
  if( argc > 1 )
    hostname = argv[1];
  if ( !hostname )
    hostname = getenv("GSSHOST");
  if ( !hostname )
    hostname = "localhost";
  if ( !hostname)
    {
      printf("Can't choose the host to connect to.\n"
             "Usage %s db_host_name\n",argv[0]);
      exit(1);
    }
  if (!(cli_disp = get_gss_handle (hostname, SQL_DISP, SQL_DISP_ONE, 1)))
    {
      printf("%s:%s\n",hostname,clnt_error_msg());
      exit (1);
    }
  
  write_new_pars();
  change_params_1 (&int_arg, cli_disp);
  drop_gss_handle(cli_disp);
  return 0;
}
