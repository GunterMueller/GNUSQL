/*  $Id: conn_handler.h,v 1.2 1998/08/21 00:29:12 kimelman Exp $
 *
 *  conn_handler.h - handle client/server transport connection,
 *                   slightly hide RPC layer and provide alternative
 *                   SYSV messages transport for gss c/s interaction.
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1998, Free Software Foundation, Inc
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
 *  $Log: conn_handler.h,v $
 *  Revision 1.2  1998/08/21 00:29:12  kimelman
 *  fix:SVC_UNREG for direct mode
 *
 *  Revision 1.1  1998/08/01 04:35:51  kimelman
 *  membrane for rpc connection
 *
 */

#ifndef __conn_handler_h__
#define __conn_handler_h__

#include "setup_os.h"

#ifndef   RPC_H_RPCGEN
# define  RPC_H_RPCGEN

# include <rpc/rpc.h>

#if TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else
#  if HAVE_SYS_TIME_H
#   include <sys/time.h>
#  else
#   include <time.h>
#  endif
#endif

#endif

#ifdef __cplusplus
extern "C"
{
#endif

  typedef bool_t (*xdr_func_t)(XDR*,void*,...);

  struct rpcgen_table {
	char	*(*proc)();
	xdrproc_t	xdr_arg;
	unsigned	len_arg;
	xdrproc_t	xdr_res;
	unsigned	len_res;
  };

  typedef struct {
    int               mode ;
    void             *handle;
  } gss_client_t;

  typedef void      *gss_server_t;

  char *         clnt_error_msg __P((void));
  void           store_err      __P((char *msg));

  gss_client_t  *get_gss_handle  (char *hostname,long svc_id, long vers_id,
                                  int timeout);
  void           drop_gss_handle ( gss_client_t *gc);
  enum clnt_stat gss_client_call (gss_client_t *rh, long proc,
                                  xdrproc_t xargs,caddr_t argsp,
                                  xdrproc_t xres,caddr_t resp,
                                  struct timeval timeout);

  void          gss_svc_unregister  (long prog, long vers);
  gss_server_t *allocate_gss_server (void);
  void         deallocate_gss_server(gss_server_t *server);
  int           register_gss_service(gss_server_t *server, int svr_port, int version,
                                     struct rpcgen_table *tbl, int tblsize);
  int           run_gss_server      (gss_server_t *server, int waitmode);

  
#ifdef __cplusplus
}
#endif

#endif
