/*
 *  dyn_client.c  -  top level routine of GNU dynamic SQL
 *                   client runtime library
 *
 * This file is a part of GNU SQL Server
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

/* $Id: dyn_client.c,v 1.247 1998/09/29 21:26:18 kimelman Exp $ */

#define __CLIENT__

#include "setup_os.h"
#include "gsqltrn.h"

#include "global.h"
#include "sql.h"
#include "dyn_funcs.h"

#include "cl_lib.h"

#define DYN_MODE
#include "options.c"
#undef  DYN_MODE

#include "cs_link.c"            /* "cs" means client/server */

static char *server_host_name=NULL;

extern i4_t    static_int_fl;
extern struct module interp_mdl;


#define SERVER_PREP_CALL(res,routine,parmptr,message)	 \
  {						\
    i4_t err; 					\
    res = routine (parmptr, interp_mdl.svc);	\
    err = dyncall_epilogue(res,message);	\
    if(err)					\
      {						\
        SQLCODE=err;				\
        return SQLCODE;				\
      }						\
  }

#define SERVER_CALL(res,routine,parmptr,message)\
    {						\
      res = routine (parmptr, interp_mdl.svc);  \
      if (!res)					\
	{					\
	  fprintf(stderr,"%s:%s\n",message,clnt_error_msg());\
          SQLCODE=-ER_SERV;                     \
          return SQLCODE;			\
	}					\
    }

static i4_t Argc = 0;
static char **Argv = NULL;

int
server_start (void)
{
  init_params_t ip;
  result_t      *r;
  
  sql_prg = progname = "dynlog";

  static_int_fl = 0;
  interp_mdl.svc = create_service(server_host_name,DYNAMIC_SVC,-1,-1,-1);
  
  if (!interp_mdl.svc)
    yyfatal(clnt_error_msg());

  open_out_files();
  ip.init_params_t_len=Argc;
  ip.init_params_t_val=Argv;

  SERVER_CALL (r,init_comp_1,&ip,"Server initialization");
  return SQLCODE;
} /* server_start */

static int
dyncall_epilogue(result_t *res,char *message)
{
  if (!res)
    {
      if (svc_ready(interp_mdl.svc,0,0,0) > 0)
        res = retry_1 ((void*)1, interp_mdl.svc);
      else
        {
          yyerror (clnt_error_msg());
          return -ER_SERV;
        }
    }
  if (!res)
    {
      fprintf(stderr,"%s:%s\n",message,clnt_error_msg());
      return -ER_SERV;
    }
  if (res->info.rett == RET_COMP)
    put_files_bufs (res->info.return_data_u.comp_ret.bufs);
  SQLCODE = res->sqlcode;
  return 0;
}

int
SQL__prepare_stmt (char *stmt_text, char *stmt_name, compiled_t **prep_res)
{
  stmt_info_t st;
  result_t    *r;
  i4_t err;
  
  st.next      = NULL;
  st.stmt      = stmt_text;
  st.stmt_name = stmt_name;
  st.bline     = 0 /*bline*/;

  if (!interp_mdl.svc && (err = server_start ()) < 0)
    return err;
    
  SERVER_PREP_CALL (r, compile_1, &st,"Prepare statement request");
  *prep_res = &(r->info.return_data_u.comp_ret);
  return SQLCODE;
}

int
server_seg_del (i4_t seg_cnt, i4_t *seg_vadr, i4_t mem_ptr)
{
  seg_del_t seg_del;
  result_t *int_res;
  
  seg_del.segment = mem_ptr;
  seg_del.seg_vadr.seg_vadr_len = seg_cnt;
  seg_del.seg_vadr.seg_vadr_val = seg_vadr;
  
  SERVER_CALL (int_res, del_segment_1, &seg_del,
               "Deallocating segments' memory");
  return int_res->sqlcode;
} /* server_seg_del */

void
server_finish (void)
{
  close_out_files ();
  if (interp_mdl.svc)
    {
      down_svc (interp_mdl.svc);
      interp_mdl.svc = NULL;
    }
}

int
SQL__get_cursor_segm (char *cursor_name, struct s_stmt *stmt)
/* sends request to server to make segment for cursor           *
 * & returns (>0) virtual address of made segment or error (<0) */
{
  result_t *int_res;
  link_cursor_t mk_segm;

  if (!stmt || !(stmt->mem_ptr))
    return -DS_STMT;
  mk_segm.cursor_name = cursor_name;
  mk_segm.stmt_name   = stmt->name;
  mk_segm.segment     = stmt->mem_ptr;
  SERVER_CALL (int_res, link_cursor_1, &mk_segm, "Getting segment for cursor");
  return int_res->info.return_data_u.segid;
} /* get_cursor_segm */
