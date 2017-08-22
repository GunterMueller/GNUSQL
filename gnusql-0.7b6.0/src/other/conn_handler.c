/*
 *  conn_handler.c -  connection/stubs & rpc/msg swither
 *  $Id: conn_handler.c,v 1.8 1998/08/22 04:20:09 kimelman Exp $
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
 *  $Log: conn_handler.c,v $
 *  Revision 1.8  1998/08/22 04:20:09  kimelman
 *  direct_mode timeout handling
 *
 *  Revision 1.7  1998/08/22 04:18:53  kimelman
 *  debug func addedd
 *
 *  Revision 1.6  1998/08/21 00:29:14  kimelman
 *  fix:SVC_UNREG for direct mode
 *
 *  Revision 1.5  1998/08/17 22:36:22  kimelman
 *  direct_mode : final bug fixes and throw away debugging stuff
 *
 *  Revision 1.4  1998/08/17 14:49:42  kimelman
 *  conn_fix
 *
 *  Revision 1.3  1998/08/15 02:01:23  kimelman
 *  debugging stuff flow
 *
 *  Revision 1.2  1998/08/12 22:31:23  kimelman
 *  direct_mode: debug phase
 *
 *  Revision 1.1  1998/08/01 04:38:32  kimelman
 *  rpc membrane
 *
 */

#include "setup_os.h"

#ifdef HAVE_UNISTD_H
#include  <unistd.h>
#endif


#ifdef  STDC_HEADERS        
#include  <string.h>
#else
#include  <strings.h>
#endif

#include <sys/types.h>

#ifdef HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif

#ifdef HAVE_SYS_MSG_H
#include <sys/msg.h>
#endif

#include <signal.h>

#include  "xmem.h"
#include  "conn_handler.h"

#include  <assert.h>

static struct {
  i4_t   used;
  char *err_message;
} rpc_err_msg = { 1, NULL };

void 
store_err(char *msg)
{
  if (rpc_err_msg.err_message)
    {
      xfree(rpc_err_msg.err_message);
      rpc_err_msg.err_message = NULL;
    }
  if (!msg)
    return;
  rpc_err_msg.err_message = savestring(msg);
  rpc_err_msg.used = 0;
}

char *
clnt_error_msg(void)
{
  if (rpc_err_msg.used)
    return "";
  rpc_err_msg.used = 1;
  return rpc_err_msg.err_message;
}

void
gss_debug_typestr(char *ttl,unsigned char *buf,int len)
{
  int i;
  fprintf(stderr,"\n%s(%d):'",ttl,len);
  for (i=0;i<len;i++)
    if ( buf[i] >=' ' && buf[i] < 127 )
      fprintf(stderr,"%c ",buf[i]);
    else
      fprintf(stderr,"0x%X ",buf[i]);
  fprintf(stderr,"'\n");
}

/*
 *  TYPEDEFs
 */

#ifdef	MSGMAX
#define MBUFSIZE MSGMAX/2
#else
#define MBUFSIZE 512
#endif

typedef struct {
  long mtype;            /* message type, must be > 0 */
  char mtext[MBUFSIZE];  /* message data              */
} mbuf_t;

typedef struct {
  long prog;
  long vers;
  long cmd;
  int  reply_id;
  int  msgsize;
} gss_cmd_t;

typedef struct {
  char *buf;
  int   buflen;
  int   start;
  int   stop;
} gss_cache_t;

/* a bit stupid rededfinition but we support only msg interface in direct mode */
typedef int gss_transport_t;

typedef struct {
  gss_cmd_t       reg;
  int             timeout;
  XDR             xdr;
  gss_cache_t     cache;
  gss_transport_t t;
} gss_conn_t;

/*----------------------------------------------------------
 * gss client handle processing
 ----------------------------------------------------------*/

static int
gss_msg_read(gss_cache_t *h,void *buf,int size)
{
  if ( size >  h->stop - h->start  )
    size = h->stop - h->start;
  if (size <=0)
    return -1;
  memcpy(buf,h->buf+h->start,size);
  h->start += size ;
  return size ;
}

static int
gss_msg_write(gss_cache_t *h,void *buf,int size)
{
  int len = h->stop+size;
  if (h->buflen < len)
    {
      int newlen = ( 2*h->buflen < len ? len : 2*h->buflen) ;
      h->buf = xrealloc(h->buf,newlen);
      h->buflen = newlen ;
    }
  assert (h->buflen >= len);
  h->start = 0;
  memcpy(h->buf+h->stop,buf,size);
  h->stop += size ;
  return size ;
}

/*==============================================================================*/
/* CLIENT code                                                                  */
/*==============================================================================*/


gss_client_t *
get_gss_handle(char *hostname,long svc_id, long vers_id, int timeout)
{
  static int done=0;
  gss_client_t *gc;
  gc = xmalloc(sizeof(*gc));
  
#ifdef DIRECT_MODE
  if(strcmp(hostname,"localhost") == 0)
    {
      int      dispatch_id;
      if(!done)
        {
          signal(SIGALRM,SIG_IGN);
          done = 1;
        }
      dispatch_id = msgget(svc_id, 0);
      if (dispatch_id == -1 )
        {
          store_err(strerror(errno));
          xfree(gc);
          return NULL;
        }
      gc->mode = 1;
      {
        gss_conn_t *gch = (gss_conn_t *)xmalloc(sizeof(gss_conn_t));
        
        gch->t           = dispatch_id;
        
        gch->reg.prog    = svc_id;
        gch->reg.vers    = vers_id;
        gch->reg.reply_id= getpid();
        
        gch->timeout     = timeout;
        
        xdrrec_create (&(gch->xdr), MBUFSIZE,MBUFSIZE,
                      (void*)&(gch->cache),(void*)gss_msg_read,(void*)gss_msg_write);
        
        gc->handle = gch;
      }
    }
  else
#endif
    { /* if remote connnection */
      CLIENT *clnt = clnt_create(hostname, svc_id, vers_id,"tcp");
      if (clnt == (CLIENT *) NULL)
        {
          store_err(clnt_spcreateerror(hostname));
          xfree(gc);
          return NULL;
        }
      clnt->cl_auth = authunix_create_default();
      if (timeout > 0)
        {
          struct timeval tv;
          tv.tv_sec  = timeout;
          tv.tv_usec = 0;
          clnt_control(clnt,CLSET_TIMEOUT,(char*)&tv);
        }
      gc->handle = clnt ;
    }
  return gc ;
}
  
void
drop_gss_handle( gss_client_t *gc)
{
  assert(gc);
  assert(gc->handle);
#ifdef DIRECT_MODE
  if(gc->mode)
    {
      gss_conn_t *gch = (gss_conn_t *)gc->handle;
      
      xdr_destroy(&(gch->xdr));
      if ( gch->cache.buflen > 0 )
        xfree(gch->cache.buf);
      xfree(gch);
    }
  else
#endif
    {
      CLIENT *clnt = (CLIENT *)gc->handle;
      assert(gc->mode==0);
      auth_destroy(clnt->cl_auth);
      clnt_destroy(clnt);
    }
  xfree(gc);
}

#define STORE_ERR(arg) store_err(arg)
/*#define STORE_ERR(arg) fprintf(stderr,"%s\n",arg) */

enum clnt_stat
gss_client_call (gss_client_t *rh, long proc,
              xdrproc_t xargs,caddr_t argsp,
              xdrproc_t xres,caddr_t resp,
              struct timeval timeout)
{
#ifdef DIRECT_MODE
  mbuf_t      msg;
  gss_conn_t *h;            /* connection handle */
#endif
  if (rh->mode==0)
    {
      enum clnt_stat rc = clnt_call((CLIENT*)(rh->handle), proc, xargs,
                                    argsp, xres, resp, timeout);
      STORE_ERR(clnt_sperror((CLIENT*)(rh->handle),""));
      return rc;
    }
#ifndef DIRECT_MODE
  abort();
#else
  h = (gss_conn_t *)(rh->handle);
  /* init cache */
  h->cache.start = h->cache.stop = 0;
  /*
   *  ENCODING structure to stream
   */
  h->xdr.x_op = XDR_ENCODE;
  if(!xargs(&h->xdr,argsp))
    {
      STORE_ERR("Arguments encoding failed");
      return RPC_CANTENCODEARGS;
    }
  if(!xdrrec_endofrecord(&h->xdr,1)) /* make sure we flushed the buffers */
    {
      STORE_ERR("endofrecord (flushing) failed");
      return RPC_FAILED;
    }
  /*
   * SEND request data to server
   */
  assert(h->cache.start == 0);
  msg.mtype = 1;
  /*
   * send request header
   */
  h->reg.cmd = proc;
  h->reg.msgsize = h->cache.stop;
  memcpy(msg.mtext,&(h->reg),sizeof(gss_cmd_t));
  if(msgsnd(h->t,(MSGBUFP)&msg,sizeof(gss_cmd_t),0))
    {
      STORE_ERR(strerror(errno));
      return RPC_CANTSEND ;
    }
  alarm(h->timeout); /* set timeout interval */
    /*
     * read entry port for arguments
     */
  {
    int         len;
    
    len = msgrcv(h->t,(MSGBUFP)&msg,sizeof(msg.mtext),h->reg.reply_id,0);
    alarm(0); /* set timeout interval */
    if ( len == -1 )
      if (errno == EINTR ) /* seems to be timeout */
        return RPC_TIMEDOUT;
      else
        return RPC_CANTRECV;
    if(len!=sizeof(long))
      {
        STORE_ERR("Unclear answer from server");
        return RPC_FAILED;
      }
    memcpy(&msg.mtype,msg.mtext,sizeof(long));
    if(msg.mtype<=0)
      {
        STORE_ERR("Servive unavailable");
        return RPC_FAILED;
      }
  }
  /*
   * send arguments buffer
   */
  while (h->cache.stop > h->cache.start)
    {
      int pos = 0;
      pos = gss_msg_read (&h->cache,msg.mtext,sizeof(msg.mtext));
      if (msgsnd (h->t,(MSGBUFP)&msg,pos,0))
        {
          STORE_ERR(strerror(errno));
          return RPC_CANTSEND ;
        }
    }
  assert(h->cache.start == h->cache.stop);
  h->cache.start = h->cache.stop = 0;
  /*
   * reading reply buffer from server
   */
  alarm(h->timeout); /* set timeout interval */
  while(1) 
    {
      int len ;
      len = msgrcv(h->t,(MSGBUFP)&msg,sizeof(msg.mtext),h->reg.reply_id,0);
      if ( len == -1 )
        switch(errno)
          {
          case EINTR : /* seems to be timeout */
            return RPC_TIMEDOUT;
          default:
            return RPC_CANTRECV;
          }
      gss_msg_write(&h->cache,msg.mtext,len);
      if (len< sizeof(msg.mtext))
        break;
    }
  alarm(0); /* clear timeout interval */
  assert(h->cache.start == 0);
  h->xdr.x_op = XDR_DECODE;
  xdrrec_skiprecord(&h->xdr);
  if(!xres(&h->xdr,resp))
    {
      STORE_ERR("Arguments decoding failed");
      return RPC_CANTDECODEARGS;
    }
  assert(h->cache.start == h->cache.stop);
  h->cache.start = h->cache.stop =  0;
#endif
  return RPC_SUCCESS;
}

/*==================================================================*/
/* SERVER code                                                      */
/*==================================================================*/

typedef struct gsrv_t {
  gss_conn_t           h      ;
  struct rpcgen_table *tbl    ;
  int                  tblsize;
  struct gsrv_t       *next   ;
} gss_service_t;

void
gss_svc_unregister(long prog, long vers)
{
#ifdef DIRECT_MODE
  int          dispatch_id = 0;
  dispatch_id = msgget(prog, 0);
  if(dispatch_id >= 0 )
    msgctl (dispatch_id, IPC_RMID, NULL);
#endif
 
#if defined(HAVE_SVC_UNREG)
  svc_unreg (prog, vers);
#elif defined(HAVE_SVC_UNREGISTER)
  svc_unregister (prog, vers);
#elif ! defined(DIRECT_MODE)
#  error "nothing have found for SVC_UNREG substitution"
#endif
}

int
register_gss_service(gss_server_t *server, int svr_port, int version,
                     struct rpcgen_table *tbl, int tblsize)
{  /* allocate service */
  gss_service_t* svc;
  int          dispatch_id = 0;

  /* check for this svc_port in other services list */
  for (svc=*server;svc;svc=svc->next)
    {
      if (svc->h.reg.prog == svr_port)
        {
          dispatch_id = svc->h.t;
          break;
        }
    }
  if (dispatch_id == 0) /* if we have not allocated this service yet */
    {
      dispatch_id = msgget(svr_port, 0);
      if (dispatch_id != -1 )
        {
          fprintf(stderr,
                  "Service message for key (%d) has already created (and used)\n",
                  svr_port);
          return -1;
        }
      dispatch_id = msgget(svr_port, IPC_CREAT | 0600);
      if (dispatch_id == -1 )
        {
          fprintf(stderr,"Can't create service message for key (%d)\n",svr_port);
          return -1;
        }
    }
  
  svc = xmalloc(sizeof(*svc));
  /* fill service */
  
  svc->h.t        = dispatch_id;
  
  svc->h.reg.prog = svr_port;
  svc->h.reg.vers = version;
  
  svc->tbl      = tbl;
  svc->tblsize  = tblsize;
  
  svc->next = *server;
  
  xdrrec_create (&(svc->h.xdr), MBUFSIZE,MBUFSIZE,
                 (void*)&(svc->h.cache),(void*)gss_msg_read,(void*)gss_msg_write);
  xdrrec_skiprecord(&svc->h.xdr);
  
  *server   =  svc ;
  return RPC_SUCCESS; /* 0 */
}

void
deallocate_gss_service(gss_service_t *svc)
{
  xdr_destroy(&svc->h.xdr);
  if (svc->h.cache.buflen > 0 )
    xfree(svc->h.cache.buf);
  SVC_UNREG(svc->h.reg.prog,svc->h.reg.vers);
  xfree(svc);
}

gss_server_t *
allocate_gss_server (void)
{
  return xmalloc(sizeof(gss_server_t));
}

void
deallocate_gss_server(gss_server_t *server)
{
  while(*server)
    {
      gss_service_t* svc = *server;
      *server = svc->next;
      deallocate_gss_service(svc);
    }
  assert(*server == NULL);
  xfree(server);
}

static gss_service_t*
gss_select(gss_service_t* *server, int waitmode,   gss_cmd_t    *hdr)
{
  gss_service_t* svc;
  int          no_wait = 1;
  mbuf_t       b;

  assert(server);
  if (*server==NULL)
    {
      fprintf(stderr,"%s:%d: gss_select failed: no services registered\n",__FILE__,__LINE__);
      return 0;
    }
  while(waitmode!=-1)
    {
      /* check message queues */
      for (svc = *server; svc ; svc = svc->next )
        {
          int len;
          if(no_wait)
            {
              len = msgrcv(svc->h.t,(MSGBUFP)&b,sizeof(b.mtext),1,IPC_NOWAIT);
              if (len == -1 && errno == ENOMSG)
                continue;
            }
          else
            {
              alarm(1);
              len = msgrcv(svc->h.t,(MSGBUFP)&b,sizeof(b.mtext),1,0);
              alarm(0);
              no_wait++; /* check other queues */
              if (len==-1 && errno == EINTR)
                {
                  if(waitmode>=0)
                    waitmode--;
                  continue;
                }
            }
          if (len == -1)
            {
              perror("gss_select");
              return NULL;
            }
          if(len == 0)
            continue;
          assert(len==sizeof(gss_cmd_t));
          /* read header */
          memcpy(hdr,b.mtext,sizeof(*hdr));
          /* check - if we have this program/version/function &
           * probably look for another - more suitable svc
           */
          {
            gss_service_t* svc1 = svc;
            
            while (svc1 && hdr->prog != svc1->h.reg.prog &&
                   hdr->vers != svc1->h.reg.vers)
              svc1=svc1->next;
            /* return READY_TO_GET */
            {
              long id = -1;
              if ( svc && svc->tblsize > hdr->cmd )
                id = getpid();
              b.mtype = hdr->reply_id;
              memcpy(b.mtext,&id,sizeof(id));
              assert(sizeof(b.mtype)==sizeof(id));
            }
            if(msgsnd(svc->h.t,(MSGBUFP)&b,sizeof(b.mtype),0))
              {
                fprintf(stderr,"%s:%d: %s\n\n",__FILE__,__LINE__,strerror(errno));
                return 0;
              }
            if(svc1)
              return svc1;
          }
          no_wait++; /* start select from the very beginning */
          break;
        }
      /* nothing is ready for reading */
      no_wait--;
    }
  return NULL;
}

int
run_gss_server (gss_server_t *server, int waitmode)
{
  gss_service_t* svc;
  gss_cmd_t    hdr;
  long         id = getpid(); /* must be the same type as msg.mtype */
  
  /* listening loop */
  while ((svc = gss_select((gss_service_t**)server,waitmode, &hdr)) != NULL)
    {
      struct rpcgen_table *e = &(svc->tbl[hdr.cmd]);
      mbuf_t               msg;
      /* got message */
      
      svc->h.cache.start = svc->h.cache.stop = 0;
      /* reading reply */
      while(svc->h.cache.stop < hdr.msgsize )
        {
          int len ;
          alarm(1); /* set timeout interval */
          errno=0;
          len = msgrcv(svc->h.t,(MSGBUFP)&msg,sizeof(msg.mtext),id,0);
          if ( len == -1 )
            if (errno != EINTR)
              return 0;
            else
              continue;
          gss_msg_write(&(svc->h.cache),msg.mtext,len);
        }
      alarm(0); /* clear timeout interval */
      assert(svc->h.cache.start == 0);
      if(svc->h.cache.stop < hdr.msgsize )
        {
          svc->h.cache.stop = 0;
          svc->h.cache.start = 0;
          continue;
        }
      /* decode argument */
      {
        char *argp = NULL;
        char *res;
        svc->h.xdr.x_op = XDR_DECODE;
        if (e->len_arg)
          {
            argp = xmalloc(e->len_arg);
            if(!e->xdr_arg(&svc->h.xdr,argp))
              {
                fprintf(stderr,"Arguments decoding failed\n");
                return 0;
              }
            xdrrec_skiprecord(&svc->h.xdr);
          }
        svc->h.cache.start = svc->h.cache.stop =  0;
        /* dispatch call using table */
        res = e->proc(argp,NULL);
        /* encode result */
        svc->h.xdr.x_op = XDR_ENCODE;
        if(!e->xdr_res(&svc->h.xdr,res))
          {
            fprintf(stderr,"Results decoding failed\n");
            return 0;
          }
        /* make sure we flushed the buffers */
        if(!xdrrec_endofrecord(&svc->h.xdr,1)) 
          {
            fprintf(stderr,"endofrecord (flushing) failed\n");
            return 0;
          }
        /* free arguments */
        if(argp)
          {
            svc->h.xdr.x_op = XDR_FREE;
            if(!e->xdr_arg(&svc->h.xdr,argp))
              {
                fprintf(stderr,"Arguments deallocating failed\n");
                return 0;
              }
            /* xfree(argp); */
          }
      }
      /* send reply */
      assert(svc->h.cache.start==0);
      msg.mtype = hdr.reply_id;
      while (svc->h.cache.stop > svc->h.cache.start)
        {
          int pos = 0;
          pos = gss_msg_read (&svc->h.cache,msg.mtext,sizeof(msg.mtext));

          
          if (msgsnd (svc->h.t,(MSGBUFP)&msg,pos,0))
            {
              perror("run_gss_server");
              return 0;
            }
        }
      /* in case of void result - send just empty buffer */
      if (svc->h.cache.stop == 0)
        if (msgsnd (svc->h.t,(MSGBUFP)&msg,0,0))
          {
            perror("run_gss_server");
            return 0;
          }
      assert(svc->h.cache.start == svc->h.cache.stop);
      svc->h.cache.start = svc->h.cache.stop = 0;
    }
  /* END of listening loop */
  return 1;
}
