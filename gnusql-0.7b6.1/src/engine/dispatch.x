/*
 *  dispatch.x  -  GNU SQL adminstrator protocol interface
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
 *  Contact: gss@ispras.ru
 *
 */


#ifdef RPC_HDR
%#include "setup_os.h"
%extern long adm_rpc_port;
%void fix_adm_port __P((char* opt));
/* renamed main() declaration */ 
%#ifdef RPCMAIN_PROTO
%int adm_rpc_start __P((int,char**));   
%#define ADM_RPC_START adm_rpc_start(argc,argv) 
%#else
%int adm_rpc_start __P((void));
%#define ADM_RPC_START adm_rpc_start() 
%#endif
#endif

#ifdef RPC_SVC
%#define main adm_rpc_start
%#define RPC_SVC_FG
#endif

enum rpc_svc_t {
  BOOT_SVC   = 0,   
  DYNAMIC_SVC,
  COMPILE_SVC = DYNAMIC_SVC,
  INTERPR_SVC = DYNAMIC_SVC
};

typedef opaque opq<>;

struct res
{
  opq    proc_id;
  int    rpc_id;
};

struct init_arg
{
  string    user_name<>;
  int       wait_time;  /* non call window */
  int       total_time; /* whole transaction time window - enforce to stop after */
  int       type;       /* type of requiered service */
  int       need_gdb;   
  string    x_server<>;
};

program  SQL_DISP {  
  version SQL_DISP_ONE {
    res  CREATE_TRANSACTION (init_arg) = 1;
    int  IS_READY           (opq)      = 2;
    int  KILL_ALL           (int)      = 3;
    int  TRN_KILL           (opq)      = 4;
    int  DISP_FINIT         (int)      = 5;
    int  COPY_LJ            (opq)      = 6;
    int  CHANGE_PARAMS      (int)      = 7;
  } = 1;
} = adm_rpc_port;

#ifdef RPC_HDR
%#define DEFAULT_TRN (SQL_DISP + 1)
#endif

#ifdef RPC_XDR
%long adm_rpc_port = 0x30001001;
%
%void fix_adm_port(char* opt)
%{
%  static int have_got = 0;
%  if (opt)
%    {
%      adm_rpc_port = atol(opt);
%      have_got = 1;
%    }
%  else if (!have_got)
%    {
%      char *s = getenv("GSSPORT");
%      if (s)
%        adm_rpc_port = atol(s);
%      have_got = 1;
%    }
%}
#endif
