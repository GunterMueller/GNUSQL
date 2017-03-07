/*
 *  gsql_cli.c  -  top level routine of GNU SQL compiler client
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Michael Kimelman.
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

/* $Id: gsql_cli.c,v 1.250 1998/09/30 02:39:07 kimelman Exp $ */

/*
 * Macro __CLIENT__ has to be defined here to mark this file as a main
 * file on the client part of compiler.  
 */

#define __CLIENT__

#include "setup_os.h"
#include <stdio.h>
#include "gsqltrn.h"
#include "sql_decl.h"
#include "xmem.h"
#include "cl_lib.h"
#include "sql.h"
#include <assert.h>

int scanner(void);

static gss_client_t *comp_srv         = NULL;
static char         *server_host_name = NULL;

#include "options.c"
#include "cs_link.c"            /* "cs" means client/server */

#define SERVER_CALL(res,routine,parmptr,message) \
  {						 \
    start_compiler_server();			 \
    res = routine (parmptr, comp_srv);	 	 \
    if (!call_epilogue(res,message))		 \
      return 0;					 \
  }

/*------------------------------------------------------------------------*/

static int          Argc       = 0;
static char       **Argv       = NULL;
static int          prep_done  = 0;
static stmt_info_t *st         = NULL;
static int          subst_to_c = 0;


static void
start_compiler_server (void)
{
  /* 
   * this call make connection to service server and register
   * reaction for unlinking it on exit 
   */
  if (!comp_srv)
    comp_srv = create_service(server_host_name,COMPILE_SVC,300,-1,-1);
  if (!comp_srv)
    yyfatal(clnt_error_msg());
}

static void 
disconnect_compiler_server (void)
{
  if (comp_srv)
    {
      down_svc (comp_srv);
      comp_srv = NULL;
    }
}

static void
close_all(void)
{
  close_out_files ();
  close_file(NULL,"c");
  while (st)
    {
      stmt_info_t *lst = st;
      st = st->next;
      free(lst);
    }
}


static int
call_epilogue(result_t *res,char *message)
{
  if (!res)
    {
      if (svc_ready(comp_srv,0,0,0) > 0)
        res = retry_1 ((void*)1, comp_srv);
      else
        {
          yyerror (clnt_error_msg());
          close_all();
          return 0;
        }
    }
  if (!res)
    {
      fprintf(stderr,"%s:%s\n",message,clnt_error_msg());
      close_all();
      return 0;
    }
  if (res->info.rett == RET_COMP)
    {
      put_files_bufs (res->info.return_data_u.comp_ret.bufs);
      errors += res->info.return_data_u.comp_ret.errors;
    }
  if (res->sqlcode<0)
    {
      /* yyerror ("gsqlca.errmsg); */
      close_all();
      return 0;
    }
  return 1;
}

/*------------------------------------------------------------------------*/

int
prepare_statement (char *stmt, i4_t bline, char **repl)
{
  stmt_info_t *lst;
  static char  replace[1024];
  lst = (stmt_info_t*) xmalloc(sizeof(*st));
  lst->stmt      = savestring ( stmt );
  lst->stmt_name = "";
  lst->bline     = bline;
  lst->next      = NULL;
  if (!st)
    st           = lst;
  else
    {
      stmt_info_t *st1 = st;
      while(st1->next)
        st1 = st1->next;
      st1->next  = lst;
    }
  if (repl)
    {
      assert(!subst_to_c);
      sprintf(replace,"SQL__subst_%d \n",prep_done);
      *repl = replace;
    }
  else if (prep_done==0)
    subst_to_c = 1;
  else
    {
      assert(subst_to_c);
    }
  prep_done++;
  return 1;
}/*-----------------------------------------------------------------------*/


static int
process_file (char *flname)
{
  progname = flname;
  prep_done = subst_to_c = 0;
  if (!progname)
    sql_prg = "gsql";
  else
    {
      char b[120];
      register char *p;
      strcpy (b, progname);
      p = b + strlen (b) - 1;
      while ( p > b && *p != '.' && *p != '/' )
        p--;
      if (*p=='.')
        *p = 0;
      while ( p > b && *p != '/' )
        p--;
      if (*p=='/')
        p++;
      sql_prg = savestring (p);
    }
  /*
   * Connection to gsql administrator and creating CLIENT handle to
   * started compiler server
   */
  open_out_files();
  if (scanner ())
    if (st)
      {
        init_params_t ip;
        result_t      *r;

        ip.init_params_t_len=Argc;
        ip.init_params_t_val=Argv;
        
        SERVER_CALL(r,init_comp_1,&ip,"server compiler initialization");
        SERVER_CALL(r,compile_1,st,"compile module request");
        if (errors==0)
          gen_substitution(subst_to_c,get_host(comp_srv),
                           &(r->info.return_data_u.comp_ret));
        while (st)
          {
            stmt_info_t *lst = st;
            st = st -> next;
            xfree (lst->stmt);
            xfree (lst);
          }
        disconnect_compiler_server();
      }
  close_all();
  return 1;
}/*-----------------------------------------------------------------------*/

int 
main (int argc, char **argv)
{
  int i, done, errs;

  read_options (argc, argv);
  Argc = argc;
  Argv = argv;
  done = 0;
  errs = 0;
  for (i = 1; i < argc; i++)
    if (argv[i][0] != '-')
      {
        i4_t rc;
	errors = 0;
        rc = process_file (argv[i]);
	errs += errors;
        if (!rc)
          goto FATAL_EXIT;
        done = 1;
      }
  if (done == 0)
    {
      fprintf(stderr,"Usage: gsql [options] prognames...\n");
    }
FATAL_EXIT:
  return errs || !done ? FATAL_EXIT_CODE : SUCCESS_EXIT_CODE;
}/*-----------------------------------------------------------------------*/
