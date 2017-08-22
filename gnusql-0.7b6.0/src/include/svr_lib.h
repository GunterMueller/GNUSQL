/*
 *  svr_lib.h  -  interface of high level library for GNU SQL RPC server 
 *                (compiler & interpretator)
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Michael Kimelman.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful 
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


#ifndef __SVR_LIB_H__
#define __SVR_LIB_H__

/* $Id: svr_lib.h,v 1.245 1997/03/31 03:46:38 kml Exp $ */

#include "gsqltrn.h"

extern i4_t rpc_program_id;

i4_t     server_init  __P((i4_t argc, char *args[]));
void    start_processing __P((void));
i4_t     finish_processing __P((void));
call_t *prepare_replacement __P((void));
void    describe_stmt __P((descr_t *d,TXTREF vcb,char mode));

/* transaction server methods prototypes */
result_t* init_compiler  __P((init_params_t*in, struct svc_req *rqstp));
result_t* compile        __P((stmt_info_t  *in, struct svc_req *rqstp));
result_t* del_segment    __P((seg_del_t    *in, struct svc_req *rqstp));
result_t* link_cursor    __P((link_cursor_t*in, struct svc_req *rqstp));
result_t* module_init    __P((string_t     *in, struct svc_req *rqstp));
result_t* execute_stmt   __P((insn_t       *in, struct svc_req *rqstp));
result_t* db_create      __P((void         *in, struct svc_req *rqstp));

#endif
