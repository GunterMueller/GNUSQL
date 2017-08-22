/*
 *  gsqltrn.c  -  GNU SQL transaction client/server protocol interface
 *
 *  $Id: gsqltrn_lib.c,v 1.249 1998/09/29 00:39:40 kimelman Exp $ *
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
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

#ifndef __GSQLTRN_C__
#define __GSQLTRN_C__


#include "setup_os.h"
#include "gsqltrn.h"
#include "global.h"
#include "svr_lib.h"
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HEAVY_IPC_DEBUG
#define PRINT(x,y) PRINTF((x,y))
#else
#define PRINT(x,y)
#endif


i4_t run_server(void);
result_t gsqltrn_rc;
static i4_t bd_catalog_read = 0;

#if 0
program GSQL_TRN {
  version BETA0 {
    result_t    INIT_COMP(init_params_t)       = 1;
    result_t    COMPILE(stmt_info_t)           = 2;
    
    result_t    DEL_SEGMENT(seg_del_t)         = 3;
    result_t    LINK_CURSOR(link_cursor_t)     = 4;
    
    result_t    LOAD_MODULE(string_t)          = 5;
    result_t    EXECUTE_STMT(insn_t)           = 6;
    
    result_t    DB_CREATE(void)                = 7;
    
    result_t    RETRY(void)                    = 8;    /* repeat last answer */
    i4_t        IS_RPC_READY(void)             = 1000; /* return process id  */
  } = 1 ;
} = 0x40000010;
#endif

/*--------- shortcuts definition settings -----------------------------------*/

#if defined(SERVE_ALL)

#define init_comp_SUBST    1
#define compile_SUBST      1

#define del_segment_SUBST  1
#define link_cursor_SUBST  1

#define load_module_SUBST  1
#define execute_stmt_SUBST 1

#define db_create_SUBST    1
#endif

#if defined(SERVE_COMPILE)

#define init_comp_SUBST    1
#define compile_SUBST      1

#define del_segment_SUBST  1
#define link_cursor_SUBST  1

#endif

#if defined(SERVE_EXECUTE)

#define load_module_SUBST  1
#define execute_stmt_SUBST 1

#endif

#if defined(SERVE_BOOT)

#define db_create_SUBST    1

#endif


#if defined(db_create_SUBST)
#define CAN_BOOT
#endif

/*------------------------------------------------------------------------*/
static void
clr_return(void)
{
  XDR x;
  x.x_op=XDR_FREE;
  xdr_result_t(&x,&gsqltrn_rc);
  bzero(&gsqltrn_rc,sizeof(gsqltrn_rc));
  gsqltrn_rc.sqlcode = -MDLINIT;
}

#define UNSUPPORTED -ER_FATAL /* just some case of fatal error */

#define RPC_STUB(vers,ot,it,proc)      		\
RPC_SVC_PROTO(ot,it,proc,vers){         	\
  ot *rv = &gsqltrn_rc;   			\
  start_processing();				\
  clr_return();					\
  PRINT(" start '%s' \n",#proc);                \
  if(!bd_catalog_read && strcmp(#proc,"db_create")) \
    return rv;                                  \
  TRY {						\
    if(&gsqltrn_rc != proc##_SUBST(in,rqstp))   \
      EXCEPTION(UNSUPPORTED);			\
    if(!bd_catalog_read && strcmp(#proc,"db_create")) \
      bd_catalog_read = 1;                      \
  }						\
  CATCH {					\
  case UNSUPPORTED:				\
  case FATAL_EXIT_CODE:				\
    gsqltrn_rc.sqlcode = -ER_FATAL;     	\
    break;					\
  case -MDLINIT:				\
    break;					\
  default:					\
    gsqltrn_rc.sqlcode = catch_buffer_rc ;	\
  }						\
  END_TRY;					\
  						\
  PRINT(" end of '%s' \n",#proc);               \
  finish_processing();				\
  return rv;					\
}

#if !defined(init_comp_SUBST)
#define init_comp_SUBST(i,r) NULL
#elif init_comp_SUBST == 1
#undef init_comp_SUBST
#define init_comp_SUBST(i,r) init_compiler(i,r)
#endif
RPC_STUB( 1, result_t,  init_params_t,init_comp)

#if !defined(compile_SUBST)
#define compile_SUBST(i,r) NULL
#elif compile_SUBST == 1
#undef compile_SUBST
#define compile_SUBST(i,r) compile(i,r)
#endif
RPC_STUB( 1, result_t,  stmt_info_t,compile)

#if !defined(del_segment_SUBST)
#define del_segment_SUBST(i,r) NULL
#elif del_segment_SUBST == 1
#undef del_segment_SUBST
#define del_segment_SUBST(i,r) del_segment(i,r)
#endif
RPC_STUB( 1, result_t,  seg_del_t,del_segment)

#if !defined(link_cursor_SUBST)
#define link_cursor_SUBST(i,r) NULL
#elif link_cursor_SUBST == 1
#undef link_cursor_SUBST
#define link_cursor_SUBST(i,r) link_cursor(i,r)
#endif
RPC_STUB( 1, result_t,  link_cursor_t,link_cursor)

#if !defined(load_module_SUBST)
#define load_module_SUBST(i,r) NULL
#elif load_module_SUBST == 1
#undef load_module_SUBST
#define load_module_SUBST(i,r) module_init(i,r)
#endif
RPC_STUB( 1, result_t,  string_t,load_module)

#if !defined(execute_stmt_SUBST)
#define execute_stmt_SUBST(i,r) NULL
#elif execute_stmt_SUBST == 1
#undef execute_stmt_SUBST
#define execute_stmt_SUBST(i,r) execute_stmt(i,r)
#endif
RPC_STUB( 1, result_t,  insn_t,execute_stmt)

#if !defined(db_create_SUBST)
#  define db_create_SUBST(i,r) NULL
#elif db_create_SUBST == 1
#undef db_create_SUBST
#  define db_create_SUBST(i,r) db_create(i,r)
#endif
RPC_STUB(1,result_t,void,db_create)

RPC_SVC_PROTO(result_t,void,retry,1)
{
  if (!finish_processing())
    gsqltrn_rc.sqlcode = -NULLRES;
  return &gsqltrn_rc;
}

RPC_SVC_PROTO(i4_t,void,is_rpc_ready,1)
{
  static i4_t rc = 0;
  if (!rc)
    rc = getpid();
  finish_processing();
  return &rc;
}

int
main(i4_t argc, char *argv[])
{
  MEET_DEBUGGER;
  bd_catalog_read = (server_init(argc,argv) == 0);
#ifndef CAN_BOOT
  if (!bd_catalog_read)
    return FATAL_EXIT_CODE;
#endif
  run_server();
  /* unreachable code */
  return FATAL_EXIT_CODE;
} /* main */

#endif /* ifndef __GSQLTRN_C__ */
