/* 
 *  cl_lib.c -  rpc client support library
 *              of GNU SQL compiler
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
 *  Contacts: gss@ispras.ru
 *
 */

/* $Id: cl_lib.c,v 1.253 1998/08/15 01:59:53 kimelman Exp $ */

#include "setup_os.h"
#ifdef HAVE_UNISTD_H
#include  <unistd.h>
#endif

#ifdef  STDC_HEADERS        
#include  <string.h>
#else
#include  <strings.h>
#endif

#if HAVE_UNAME
#include <sys/utsname.h>
#endif

#include  "cl_lib.h"
#include  "xmem.h"
#include  "conn_handler.h"
#include  "errors.h"
#include  "dispatch.h"
#include  "sql_decl.h"
#include  "gsqltrn.h"
#include  "sql.h"

#define DEFAULT_TRN_CLIENT_WAIT_TIME   300  /* 5 min - wait between client calls */
#define DEFAULT_TOTAL_TRANSACTION_TIME 3600 /* 1 hour - whole transaction window */
#define DEFAULT_MIN_DELAY              1    
#define DEFAULT_MAX_DELAY              16
#define DEFAULT_MAX_WAIT_TIME          3600 /* max waiting for transaction response*/
#define DEFAULT_ANSWER_WAIT_TIME       20

int last_client_asked_for_gdb;

typedef struct A {
  struct A     *next;
  struct A     *prev;
  gss_client_t *svc;
  opq           proc_id;
  rpc_svc_t     svc_type;
} svc_t;

typedef struct B {
  struct B     *next;
  struct B     *prev;
  gss_client_t *adm ;
  svc_t        *svcs;
  char         *host;
} svr_t;

static svr_t *svrs = NULL;

char *
SQL__err_msg(i4_t code)
{
  static char *err_str[]=
  {
#define DEF_ERR(CODE,ERRstr)  ERRstr,
#include "errors.h"
#undef DEF_ERR
    NULL
  };
  if ( code < 0 )
    code = -code;
  if (code>=0 && code<sizeof(err_str)/sizeof(char*))
    return err_str[code];
  return "incorrect error code";
}

static void global_disconnect __P((void));
     
char *
choose_host(char *hostname)
{
  if ( !hostname )
    hostname = getenv("GSSHOST");
#if defined(HAVE_UNAME) && 0
  if ( !hostname )
    {
      static struct utsname buf;
      uname(&buf);
      hostname = buf.nodename; /*!!!!!!!!! for debugging purpose only      */
      /* there is a hole for procedure for finding the only working server */
      /* or something like it..............................................*/
    }
#endif
  if ( !hostname )
    hostname = "localhost";
  if ( !hostname)
    {
      store_err("Can't choose the host to connect to.");
      return NULL;
    }
  return hostname;
}

gss_client_t *
create_service(char *hostname, 
	       rpc_svc_t required_service,
	       i4_t answer_wait_time,
	       i4_t trn_client_wait_time,
	       i4_t total_transaction_time)
{
  static i4_t atexit_registered = 0;
  svr_t *csvr;
  svc_t *csvc;

  if (answer_wait_time == -1 )
    answer_wait_time = DEFAULT_ANSWER_WAIT_TIME;
  if ( !atexit_registered)
    {
      ATEXIT(global_disconnect);
      atexit_registered = 1;
    }
  if (hostname)
    {
      static char *host=NULL;
      char *p = hostname;
      while ( *p && *p != ':' ) p++;
      if ( *p == ':' ) /* then hostname contained portid */
        {
          i4_t l = p - hostname + 1;
          if ( l > 1 )
            {
              host = xrealloc (host, l);
              bcopy (hostname, host, l);
              host[l]=0;
              hostname=host;
            }
           fix_adm_port(p+1);
        }
    }
  hostname = choose_host(hostname);
  /* change adm port in according to environment, options and defaults */
  fix_adm_port(NULL); 
  for ( csvr = svrs; csvr ; csvr = csvr->next )
    if ( !strcmp(csvr->host,hostname) )
      break;
  if (!csvr) /* if we don't have a handle to desired host */
    {
      gss_client_t *clnt = get_gss_handle(hostname,SQL_DISP,SQL_DISP_ONE,answer_wait_time*10);

      if(!clnt)
        {
          store_err("Service/host unavailable");
          return NULL;
        }
      
      csvr = xmalloc(sizeof(svr_t));
      
      csvr->prev = NULL;
      csvr->next = svrs;
      csvr->adm  = clnt;
      csvr->svcs = NULL;
      csvr->host = savestring(hostname);
      if (svrs)
	svrs->prev = csvr;
      svrs = csvr;
    }
  /* csvr points now to the handle of required server */
  for ( csvc = csvr->svcs; csvc ; csvc = csvc->next )
    if ( csvc->svc_type == required_service )
      break;
  
  if (!csvc) /* if we aren`t currently connected to the given host */
    {
      gss_client_t    *clnt;
      res       *result;
      init_arg   i_arg;
      
      if ( trn_client_wait_time < 0 )
	trn_client_wait_time = DEFAULT_TRN_CLIENT_WAIT_TIME;
      if ( total_transaction_time < 0 )
	total_transaction_time = DEFAULT_TOTAL_TRANSACTION_TIME;
      
      i_arg.user_name         = get_user_name();
      i_arg.wait_time         = trn_client_wait_time ;
      i_arg.total_time        = total_transaction_time ;
      i_arg.type              = required_service;
      i_arg.need_gdb          = 0;
      i_arg.x_server          = ":0.0";
#ifndef NOT_DEBUG
      {
	i4_t c = 0;
        char *s=NULL;
        s = getenv("NEEDGDB");
        if (!s)
          {
            fprintf(stderr,"Do you want to run gdb?");
            do
              c=getchar();
            while (c=='\n');
          }
        else if (*s=='y')
          c = 'y';
        
	if ( c == 'y' )
	  {
	    char buf[100], *b=getenv("DISPLAY");
	    i_arg.need_gdb = 1;
	    if ( b )
	      strcpy(buf,b);
	    else
	      {
                printf("Enter X display (host:0) >>");
                scanf("%s",buf);
	      }
	    i_arg.x_server = savestring(buf);
	  }
      }
#endif
      last_client_asked_for_gdb = i_arg.need_gdb;
      for(;;)
        {
          result = create_transaction_1(&i_arg,csvr->adm);
          if ( !result)
            return NULL;
          if ( result->rpc_id > 0)
            break;
          if ( result->rpc_id != -NOCRTR )
            {
              store_err(SQL__err_msg(NOCRTR));
              return NULL;
            }
          sleep(1);
        }
      clnt =  get_gss_handle(hostname,result->rpc_id,BETA0,
                             (i_arg.need_gdb?DEFAULT_MAX_WAIT_TIME:answer_wait_time));
      
      if(!clnt)
        {
          store_err("Service start failed");
          return NULL;
        }
      
      csvc = xmalloc(sizeof(svc_t));

      csvc->prev = NULL;
      csvc->next = csvr->svcs;
      csvc->svc  = clnt;
      csvc->svc_type = required_service;
      csvc->proc_id.opq_len = result->proc_id.opq_len;
      csvc->proc_id.opq_val = (char*) xmalloc(result->proc_id.opq_len);
      bcopy(result->proc_id.opq_val,
	    csvc->proc_id.opq_val,
	    result->proc_id.opq_len);
      if (csvr->svcs)
	csvr->svcs->prev = csvc;
      csvr->svcs = csvc;
    }
  return csvc->svc;
}

static int
find_svc(gss_client_t *cl,svr_t **svrp,svc_t **svcp)
{
  register svc_t *csvc;
  register svr_t *csvr;
  for ( csvr = svrs; csvr; csvr = csvr->next )
    for ( csvc = csvr->svcs ; csvc ; csvc = csvc->next )
      if (csvc->svc == cl)
	{
	  *svrp = csvr;
	  *svcp = csvc;
	  return 1;
	}
  *svrp = NULL;
  *svcp = NULL;
  return 0;
}

static int
valid_info(svr_t *svr, svc_t *svc)
{
  svc_t *csvc;
  svr_t *csvr;
  
  if (!find_svc(svc->svc,&csvr,&csvc))
    return 0; /* it has already destroyed */
  
  assert( svr == csvr && svc == csvc);
  return 1;
}

static void
destroy_svc_info(svr_t *csvr, svc_t *csvc)
{
  if (!valid_info(csvr,csvc))
    return ; /* it has already destroyed */
  
  if ( csvc->next )
    csvc->next->prev = csvc->prev; 
  if ( csvc->prev )
    csvc->prev->next = csvc->next;
  else
    csvr->svcs = csvc->next;

  drop_gss_handle(csvc->svc);
  xfree(csvc->proc_id.opq_val);
  xfree(csvc);
}

static void  
destroy_svr_info(svr_t *csvr)
{
  while(csvr->svcs)
    destroy_svc_info(csvr,csvr->svcs);
  if ( csvr->next )
    csvr->next->prev = csvr->prev; 
  if ( csvr->prev )
    csvr->prev->next = csvr->next;
  else
    svrs = csvr->next;
  
  drop_gss_handle(csvr->adm);
  xfree(csvr->host);
  xfree(csvr);
}

static commit_down_mode = 0;

static int
down_service(svr_t *csvr, svc_t *csvc)
{
  int *res;

  if (!valid_info(csvr,csvc))
    return 1; /* it has already destroyed */
  
  if (commit_down_mode)
    {
      result_t  *ret;
      insn_t     arg;

      bzero(&arg,sizeof(arg));
      arg.sectnum = commit_down_mode;
  
      ret = execute_stmt_1 (&arg, csvc->svc);
      
      if (!ret)
        {
          i4_t ready = svc_ready(csvc->svc,1,4,16);
          if (ready > 0)
            ret = retry_1 (NULL, csvc->svc);
        }
      if(!ret)
        store_err("service crash");
    }
  if (!valid_info(csvr,csvc))
    return 1; /* it has already destroyed */
  res = trn_kill_1(&(csvc->proc_id),csvr->adm);
  /* If server not respond - unlink it */
  if ( !res )
    {
      store_err("Server crash");
      destroy_svr_info(csvr);
      return 0;
    }
  else
    destroy_svc_info(csvr,csvc);
  return 1;
}

static void
down_server(svr_t *csvr)
{
  while (csvr->svcs)
    if( !down_service(csvr,csvr->svcs) )
      {
	fprintf(STDERR,clnt_error_msg());
	return;
      }
  destroy_svr_info(csvr);
}

static void 
global_disconnect(void)
{
  while(svrs)
    down_server(svrs);
}

void  
down_svc(gss_client_t *svc)
{
  svc_t *csvc;
  svr_t *csvr;
  i4_t    rc = 0;
  
  if (find_svc(svc,&csvr,&csvc))
    rc = down_service(csvr,csvc);
  if (!rc)
    fprintf(STDERR,clnt_error_msg());
}

int
svc_ready(gss_client_t *cl_handle, i4_t min_delay, i4_t max_delay, i4_t max_wait_time)
{
  svc_t *csvc;
  svr_t *csvr;
  register int    cdelay;
  register int    waited;
  
  if (!find_svc(cl_handle,&csvr,&csvc))
    {
      store_err("incorrect service required");
      return -1;
    }
  if ( min_delay <= 0 )
    min_delay = DEFAULT_MIN_DELAY;
  if ( max_delay <= 0 )
    max_delay = DEFAULT_MAX_DELAY;
  if ( max_wait_time <= 0 )
    max_wait_time = DEFAULT_MAX_WAIT_TIME;
  
  for ( cdelay  = waited = 0; waited < max_wait_time; waited += cdelay )
    {
      int *ready;
      ready = is_ready_1(&(csvc->proc_id), csvr->adm);
      if (!ready)
	{
	  destroy_svr_info(csvr);
	  return -1;
	}
      if (*ready < 0 ) /* if error occured */
	{
 	  store_err("service crash");
          destroy_svc_info(csvr,csvc);
	  return -1;
	}
      if (*ready == 0 ) /* result is ready */
	return 1;
      if (cdelay < min_delay ) cdelay = min_delay;
      else                    cdelay *= 2;
      if (cdelay > max_delay ) cdelay = max_delay;
      sleep (cdelay);
    }
  return 0;
}

void
SQL__disconnect_pass2(int commit)
{
  commit_down_mode = commit;
  global_disconnect();
  commit_down_mode = 0;
}

char *
get_host(gss_client_t *cl)
{
  svc_t *csvc;
  svr_t *csvr;
  
  if (!find_svc(cl,&csvr,&csvc))
    return NULL; /* it has already destroyed */
  
  return csvr->host;
}
