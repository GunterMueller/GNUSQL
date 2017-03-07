/*  $Id: cl_lib.h,v 1.247 1998/09/29 21:25:50 kimelman Exp $
 *
 *  cl_lib.h -  rpc client support library interface
 *              of GNU SQL compiler
 *                
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
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
 *  Contacts: gss@ispras.ru
 *
 *  $Log: cl_lib.h,v $
 *  Revision 1.247  1998/09/29 21:25:50  kimelman
 *  copyright years fixed
 *
 *  Revision 1.246  1998/07/30 03:23:36  kimelman
 *  DIRECT_MODE
 *
 */

#ifndef __CL_LIB_H__
#define __CL_LIB_H__

#include "setup_os.h"
#include "conn_handler.h"

#include "dispatch.h"

gss_client_t *create_service __P((char *hostname,
		       rpc_svc_t required_service,
		       i4_t answer_wait_time, 
		       i4_t trn_client_wait_time,
		       i4_t total_transaction_time));

i4_t   svc_ready __P((gss_client_t *cl_handle,
		i4_t min_delay,
		i4_t max_delay,
		i4_t max_wait_time));

void  down_svc     __P((gss_client_t *svc     ));
char *get_host     __P((gss_client_t *cl      ));
char *choose_host  __P((char   *hostname));
char *SQL__err_msg __P((i4_t code));


#endif
