/*
 *  int_cli.c  -  top level routine of GNU SQL client runtime library
 *
 *  This file is a part of GNU SQL Server
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

/* $Id: int_cli.c,v 1.250 1998/09/29 21:26:11 kimelman Exp $ */

#include "setup_os.h"
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "global.h"
#include "gsqltrn.h"
#include "sql.h"
#include "inprtyp.h"
#include "cl_lib.h"

struct sqlca gsqlca;
i4_t    static_int_fl = 1;

#define ANSWER_WAIT_TIME     200   /* time of waiting for a result  *
				   * from transaction & dispatcher */
#define TRN_CLIENT_WAIT_TIME 300  /* time of transaction's waiting *
				   * for client's request          */

/* definition for time (in seconds) of waiting *
 * for transaction work's finishing            */
#define FIRST_SLEEP 1
#define MAX_SLEEP   10

#define CNT_MAX_SLEEP 500 /* after sleeping with maximum delay (MAX_SLEEP)  *
			   * CNT_MAX_SLEEP times transaction will be killed */

#define MAX_TRN_WAIT_TIME (MAX_SLEEP*(CNT_MAX_SLEEP+1))

static i4_t prefetch_cache __P((struct module *mdl,gsql_parms *args,result_t **res));

static struct A {
  struct module *mdl;
  struct A      *next;
} *top = NULL;

int
SQL__disconnect(int commit)
{
  extern void SQL__disconnect_pass2 __P((i4_t));
  prefetch_cache(NULL,NULL,NULL); /* clear cache */
  while(top)
    {
      struct A *c = top;
      top = top->next;
      c->mdl->svc = NULL;
      xfree(c);
    }
  SQL__disconnect_pass2(commit);
  SQLCODE = 0;
  return 0;
}

int
SQL__connect(struct module *mdl,i4_t svc_type,
              i4_t answer_wait_time,i4_t client_wait_time)
{
  struct A    *cmdl;
  for(cmdl = top; cmdl; cmdl = cmdl->next)
    if (cmdl->mdl==mdl)
      break;
  if (!cmdl)
    {
      cmdl = (struct A*) xmalloc(sizeof(*cmdl));
      cmdl->mdl = mdl;
      cmdl->next = top;
      top = cmdl;
    }
  if (!mdl->svc)
    /* work with dispatcher & transaction initialization */
    {
      mdl->svc = (void*) create_service(mdl->db_host,svc_type,
                                        answer_wait_time,
                                        client_wait_time,
					-1);
      if (!mdl->svc)
	return -ER_SERV;
      mdl->segment = 0;
    }
  return 0;
}


/*
 * cache level 
 */

int
SQL__cache(int new_cache_limit)
{
  static i4_t cache_limit = 0;
  if (new_cache_limit==-1)
    return cache_limit;
  if (cache_limit <= new_cache_limit)
    {
      cache_limit = new_cache_limit;
      return 0;
    }
  assert(cache_limit>new_cache_limit);
  cache_limit = new_cache_limit;
  SQL__runtime (NULL, NULL);
  return SQLCODE;
}

/*
 * prefetch caching
 */
typedef struct cache {
  parm_row_t    *tbl;
  i4_t            count;
  i4_t            sqlcode;
  struct module *mdl;
  i4_t            sect;
  i4_t            current;
  struct cache  *next;
  struct cache  *prev;
} cache_t;

static int
purge_cache_tag(cache_t **fc,cache_t **top)
{
  i4_t i,l;
  cache_t *c = *fc;
  
  if (c->next)
    (*fc = c->next)->prev = c->prev;
  if (c->prev)
    (*fc = c->prev)->next = c->next;
  else if (top)
    *top = c->next;
  
  for ( i = 0 ; i < c->count ; i ++ )
    for ( l = 0 ; l < c->tbl[i].parm_row_t_len ; l ++ )
      if (c->tbl[i].parm_row_t_val[l].value.type == SQLType_Char)
        xfree(c->tbl[i].parm_row_t_val[l].value.data_u.Str.Str_val);
  xfree(c->tbl);
  
  xfree(c);
  return 0;
}

static i4_t
prefetch_cache(struct module *mdl,gsql_parms *args,result_t **res)
{
  static cache_t *cache = NULL;
  static result_t r;
  cache_t *c;

  /* global operation : freeing cache at all */
  if (mdl==NULL)
    {
      while (cache)
        purge_cache_tag(&cache,NULL);
      return 0;
    }
  /* find required prefetch */
  for(c = cache; c; c = c->next)
    if (c->mdl==mdl && c->sect==args->sectnum)
      break;
  if (res==NULL || args->command !=1) /* request to purge cache */
    {
      if (c) /* if there is some cache for that section */
        purge_cache_tag(&c,&cache);
      return 0;
    }
  assert(mdl && res && args->command==1);
  /* we check for cached data or probably save new data in cache */
  /*
   * we assume that request for storing data is impossible while we have some
   * cached data for the same cursor (section)
   */
  if (*res) /* request for storing data */ 
    {
      assert(c==0 /* there is no cached data for this section */);
      assert((*res)->info.rett==RET_TBL);
      c = (cache_t*) xmalloc( sizeof(cache_t) );
      /*
       * the following 3 lines is unreliable and potentially dangerous(!!!!)
       * way storing data, but i'm too lazy now to beautify it /kml
       */
      c->tbl     = (*res)->info.return_data_u.tbl.tbl_val;
      c->count   = (*res)->info.return_data_u.tbl.tbl_len;
      (*res)->info.rett=RET_VOID;
      /* ======== */
      c->sqlcode = (*res)->sqlcode;
      c->mdl     = mdl;
      c->sect    = args->sectnum;
      c->current = 0;
      /* data stored -- link it cache */
      c->prev = NULL;
      c->next = cache;
      cache = cache->prev =  c;
      return 1;
    }
  else if (c) /* if there is some cached data for this sectionr */ 
    {
      bzero(&r,sizeof(r));
      if (c->current<c->count)
        {
          r.info.return_data_u.row.parm_row_t_val = c->tbl[c->current].parm_row_t_val;
          r.info.return_data_u.row.parm_row_t_len = c->tbl[c->current].parm_row_t_len;
          r.info.rett = RET_ROW;
          if (++(c->current) == c->count && c->sqlcode!=100)
            {
              r.sqlcode = c->sqlcode;
              c->sqlcode = 0;
            }
        }
      else /* if there is no more data */
        {
          r.sqlcode = c->sqlcode;
          purge_cache_tag(&c,&cache);
          if (!r.sqlcode)
            return 0;
          assert(r.sqlcode==100);
        }
      *res = &r;
      return 1;
    }
  /* we checked for cached data but there no data in cache */
  return 0;
}

/*
 * error code procesinng
 */

static void
proc_ret (i4_t code, gss_client_t **svc)
{ 
  SQLCODE = code;
  if (SQLCODE < 0)
    { /* on error */
      if ( SQLCODE == -ER_SERV)
	gsqlca.errmsg = clnt_error_msg();
      else
	gsqlca.errmsg = SQL__err_msg(-SQLCODE);
      if ( SQLCODE == -ER_SERV || SQLCODE == -NOTRNANS)
        { /* down service if connection is lost                     */
          /* how about cache? Does it need to be purged here? Hmm...*/
          down_svc(*svc);
          *svc = NULL;
        }
    }
  else
    gsqlca.errmsg = "OK";
  return;
} /* proc_ret */

/*
 * client's part of interpretator 
 */

static int
execute_it(insn_t *insn,gss_client_t *svc,result_t **r)
{
  assert(r);
  *r = execute_stmt_1 (insn, svc);
  if (!*r)
    {
      i4_t ready = svc_ready(svc,FIRST_SLEEP,
			    MAX_SLEEP,MAX_TRN_WAIT_TIME);
      if (ready > 0)
	*r = retry_1 (NULL, svc);
      else if (ready <0)
        return -ER_SERV;
      else /* if (ready == 0) */
        return -NOTRNANS;
    }					
  if (!*r)
    return -NOTRNANS;
  return 0;
}

void
SQL__runtime (struct module *mdl,gsql_parms *args)
{
  static int     arg_hole_cnt = 0;
  static parm_t *arg_hole     = NULL;
  
  static struct {
    insn_t       *head;
    insn_t       *tail;
    int           insns;
    gss_client_t *svc;
  } icache = { NULL, NULL, 0 , NULL };
  
  int         rc;
  insn_t     *i_arg = NULL;
  result_t   *i_res = NULL;
  gsql_parm  *parm = (args) ? args->prm : NULL;
  int        args_cnt = (args) ? args->count : 0;
  int        in_args_cnt, out_args_cnt, i, j, error;

#define RET(code) {        proc_ret (code,(gss_client_t**)&(mdl->svc)); return; }
#define CHKRC     { if(rc){proc_ret (rc  ,(gss_client_t**)&(mdl->svc)); return; }}

  if (!icache.tail && !mdl) /* if there is no cache and this call    */
    RET(0);                 /* invoked by cache change call - return */ 
  
  if (args->sectnum == -3) /* if nothing was prepared */
    RET(0);
  
  if (mdl)
    {
      rc = SQL__connect(mdl,INTERPR_SVC,ANSWER_WAIT_TIME,TRN_CLIENT_WAIT_TIME);
      CHKRC;
      /* module initialization */
      if (!mdl->segment)
        {
          result_t *res_init = load_module_1 (&(mdl->modname), mdl->svc);
          
          if (!res_init)
            RET (-ER_SERV);
          assert(res_init->info.rett == RET_SEG);
          if ((mdl->segment = res_init->info.return_data_u.segid) < 0)
            RET (res_init->sqlcode);
        }
    }
  /* Parameters handling : */
  
  for (in_args_cnt = i = 0; i < args_cnt; i++ )
    if (!parm[i].flags)
      in_args_cnt++;
  
  out_args_cnt = args_cnt - in_args_cnt;

  /* check for prefetched data */
  if (prefetch_cache(mdl,args,&i_res))
    goto read_answer;
  
  /* iput parameters processing */
  CHECK_ARR_SIZE (arg_hole, arg_hole_cnt, in_args_cnt, parm_t);
	    
  for (j = i = 0; i < args_cnt; i++)
    if (!parm[i].flags)
      if ((error = user_to_rpc (parm + i, arg_hole + (j++))) < 0)
	RET (error);
  
  /* forming instruction */
  if (mdl)
    {
      i_arg = (insn_t*)xmalloc(sizeof(insn_t));
      i_arg->parms.parm_row_t_len = in_args_cnt;
      i_arg->parms.parm_row_t_val = (in_args_cnt) ? arg_hole : NULL;
      i_arg->vadr_segm = mdl->segment;
      i_arg->sectnum = args->sectnum;
      i_arg->command = args->command;
      i_arg->options = gmin (args->options,SQL__cache(-1));
    }
  
  /* check for possibility to cache instruction */
  if ( SQL__cache(-1) > icache.insns && 
       (!icache.head || !mdl || icache.svc==mdl->svc) &&
       out_args_cnt == 0 &&
       (!args || args->command >=0)
       )
    {
      if (!mdl) /* if it's dummy command -- changing cache */
        RET(0);
      if (in_args_cnt) /* store parameters */
        {
          i4_t l = in_args_cnt * sizeof (parm_t);
          assert (i_arg != NULL);
          i_arg->parms.parm_row_t_val = (parm_t*) xmalloc(l);
          bcopy (arg_hole,i_arg->parms.parm_row_t_val,l);
          for ( l = 0 ; l < in_args_cnt ; l ++ )
            if (arg_hole[l].value.type == SQLType_Char)
              {
                parm_t *p = i_arg->parms.parm_row_t_val + l;
                char   *v = xmalloc(p->value.data_u.Str.Str_len);
                bcopy(p->value.data_u.Str.Str_val,v,p->value.data_u.Str.Str_len);
                p->value.data_u.Str.Str_val = v;
              }
        }
      if (icache.head)
        icache.tail = icache.tail->next = i_arg;
      else
        {
          icache.tail = icache.head       = i_arg;
          icache.svc = mdl->svc;
        }
      icache.insns ++;
      i_arg = NULL;
      if (SQL__cache(-1) > icache.insns)
        RET(0);
    }
  /* flush cache */
  if (icache.head)
    {
      rc = execute_it(icache.head,icache.svc,&i_res);
      /* free cache */
      while (icache.head) 
        {
          i4_t l;
          icache.tail = icache.head;
          icache.head = icache.head->next;
          for ( l = 0 ; l < icache.tail->parms.parm_row_t_len ; l ++ )
            if (icache.tail->parms.parm_row_t_val[l].value.type == SQLType_Char)
              xfree(icache.tail->parms.parm_row_t_val[l].value.data_u.Str.Str_val);
          xfree(icache.tail);
          icache.insns--;
        }
      icache.tail = icache.head = NULL;
      assert(icache.insns==0);
      CHKRC;
    }
  
  if (i_arg)
    {
      assert(i_res==NULL || i_res->info.rett==RET_VOID);
      rc = execute_it(i_arg,mdl->svc,&i_res);
      /* free in parms */
      xfree(i_arg);
      CHKRC;
    }

  if (i_res->info.rett==RET_VOID)
    {
      if ((i_res->sqlcode == 100) && out_args_cnt==0 &&
          args && args->command==0)
        RET (0); /* It's not CURSOR or SELECT but SQLCODE == 100 */
      
      RET (i_res->sqlcode); /* normal exit: may be on error */
    }

  assert(i_res->info.rett==RET_TBL || i_res->info.rett==RET_ROW);
  if (i_res->info.rett==RET_TBL)
    {
      assert(prefetch_cache(mdl,args,&i_res)); /* put table in cache */
      i_res = NULL;
      assert(prefetch_cache(mdl,args,&i_res)); /* read fist row      */
    }
  
read_answer:
  {
    parm_row_t *orow = &i_res->info.return_data_u.row;
    rc = i_res->sqlcode;

    if(rc==100 && args->command!=1) /* avoid eofscan report outside cursor */
      rc = 0;
    
    if (rc==100) /* SQLCODE : End Of Scan */
      RET (rc);
    
    /* checking of result data count */
    if (orow->parm_row_t_len != out_args_cnt)
      RET (-ER_CLNT);
    
    for (i = 0, j = 0; i < args_cnt; i++ )
      if (parm[i].flags)
        if ((error = rpc_to_user (orow->parm_row_t_val + (j++), parm + i)) < 0)
          RET (error);
  }
  RET (rc);
  
#undef RET
}
