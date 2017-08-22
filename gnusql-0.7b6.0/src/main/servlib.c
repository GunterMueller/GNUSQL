/*
 *  servlib.c  -  some functions from top level of GNU SQL compiler server
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

/* $Id: servlib.c,v 1.248 1998/09/12 21:46:03 kimelman Exp $ */

/*
 * Macro __SERVER__ defined here marks this file as a main file on
 * the server part of compiler. But in joined mode there is also defined
 * macro __CLIENT__.
 */

#define __SERVER__

#include "setup_os.h"
#include "gsqltrn.h"
#include "pupsi.h"
#include "sql_decl.h"
#include "svr_lib.h"
#include "global.h"
#include "tree_gen.h"

#include "options.c"
#include "cs_link.c"            /* "cs" means client/server */
#include "../semantic/seman.h"
#include "admdef.h"
#include <assert.h>

extern void restart_server_scanner  __P((char *stmt, i4_t bline));

int  compiler_state = 0;
int  fatal_error = 0;
VADR ld_segment  = 0;

/*
 * dump to 'fl' whole tree
 */

void 
trl_dump (FILE *fl)
{
  print_trl(TNULL,fl);
}

/*
 * dump only last operator of tree 
 */

static TXTREF tree_tail = TNULL;

void
trl_tail_dump (FILE *fl)
{
  if ( tree_tail == TNULL)
    print_trl(TNULL ,fl);
  else if ( RIGHT_TRN(tree_tail) )
    print_trl( RIGHT_TRN(tree_tail),fl);
}

int
chk_access(void)
{
  return access_main(ROOT);
}

int 
eval_types(void)
{
  handle_types (ROOT, 0);
  return 0;
}

int
synthesys (void)
{
  ld_segment = codegen ();
  return 0;
}

static int 
do_pass (int i)
{
#if NOT_DEBUG
#  define PRINT_DEB(str)  /**/
#else
#  define PRINT_DEB(str)  fprintf(stderr,str)
#endif
  
  PRINT_DEB ("\rPass ");
  {
    char s[50];
    sprintf(s,"%d",i+1);
    PRINT_DEB (s);
  }
  PRINT_DEB (": ");
  PRINT_DEB (compiler_passes[i].sdescr);
  PRINT_DEB ("... ");
  if (compiler_passes[i].skip)
    PRINT_DEB ("skipping...");
  else
    {
      if (compiler_passes[i].pass ())
        {
          fprintf (STDERR, "Error(s) in %s stage\n", compiler_passes[i].sname);
          if (!errors)
            errors += 1;
          return 0;
        }
    }
  if (compiler_passes[i].dump)
    {
      register Dumper dmpproc = compiler_passes[i].dumper;
      PRINT_DEB ("dumping...");
      if (dmpproc)
        dmpproc (compiler_passes[i].dmp_file);
      else
        PRINT_DEB ("impossible (no dumper) ");
    }
  PRINT_DEB (" Done\n");
  return 1;
}


static void 
compile_data (int stage,int *pass)
{
  int i;
  i = last_comp_pass;
  assert ((compiler_passes[i + 1].sname == NULL) && 
          (compiler_passes[i].sname != NULL));
  i = pass ? *pass + 1 : 0 ;
  if (stage == 0)
    {
      TXTREF r = ROOT;
      if (r)
	while(RIGHT_TRN(r))
	  r = RIGHT_TRN(r);
      tree_tail = r;
    }
  if (i < 0)
    i = 0;
  while (compiler_passes[i].stage < stage)
    i++;
  for (; compiler_passes[i].stage == stage; i++)
    {
      if (!do_pass (i))
        EXCEPTION(-ER_COMP_FATAL);
      if(pass)
        {
          *pass = i;
          return;
        }
      if (errors)
        break;
    }
  if(pass)
    *pass = -1;
  return;
}

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/

char*
store_module (void *ptr,i4_t size)
{
  extern char *current_user_login_name; /* in xmem.c */
  char  fn[256];
  FILE *f;
  
  assert(ptr && size );
  
  {
    char b[1024], *p;
    strcpy(b,progname);
    p = b + strlen (b) - 1;
    while ( p > b && *p != '.' && *p != '/' )
      p--;
    if (*p=='.')
      *p = 0;
    while ( p > b && *p != '/' )
      p--;
    if (*p=='/')
      p++;
    sprintf(fn,"%s/%s.%s",DBAREA,current_user_login_name,p);
  }  
  
  assert(fn);
  /* ? Can we create compiled module image entry in database */
  assert((f = fopen (fn, "w")));
  /* ? Can we write compiled module to database */
  assert(size == fwrite (ptr, 1, size, f));
  fclose (f);
  return savestring(fn);
}

/*----------------------------------------------------------------------------
 * dedine structure to hold stored modules descriptors
 */

typedef struct X2
{
  i4_t        id;
  void      *ptr;
  i4_t        size;
  struct X2 *next;
  struct X2 *prev;
} segment_t;

static segment_t* dyn_segms = NULL;

/*
 * append new module to list of stored segments
 */

static long
append_current_segm (void *m_ptr,i4_t size)
{
  segment_t *cs;

  cs = (segment_t*) xmalloc (sizeof (segment_t));
  cs->id   = size; /* I guess it's a cool value to be random */
  cs->ptr  = m_ptr;
  cs->size = size;
  cs->prev = NULL;
  cs->next = dyn_segms;
  if (cs->next)
    {
      cs->next->prev = cs;
      /* if choosen value is not so well as it worth to be */
      if (cs->id <= cs->next->id + 1 )
        cs->id = cs->next->id + 1 ;
    }
  dyn_segms = cs;
  return cs->id;
} /* append_current_segm */

/*
 * make segment for cursor & returns                 
 * virtual address of made segment (>0) or error (<0)
 */
result_t*
link_cursor(link_cursor_t *in,struct svc_req *rqstp)
{
  VADR       stmt_scan;
  segment_t *cs;

  gsqltrn_rc.sqlcode = 0;
  for (cs = dyn_segms; cs ; cs = cs->next )
    if ( cs->id == in->segment)
      {
        void *p = xmalloc(cs->size);
        i4_t segid;
        bcopy(cs->ptr,p,cs->size);
        segid = (i4_t)link_segment (p, cs->size);
        if (segid <= 0)
          gsqltrn_rc.sqlcode = -MDLINIT;
        else
          {
            gsqltrn_rc.info.rett = RET_SEG;
            gsqltrn_rc.info.return_data_u.segid = segid;
            stmt_scan = resolve_reference (in->stmt_name);
            register_export_address (stmt_scan, in->cursor_name);
          }
        break;
      }
  return &gsqltrn_rc;
} /* mk_cursor_segm */

/*
 * delete saved copy of module and unlink set of linked modules
 */

result_t*
del_segment(seg_del_t *in,struct svc_req *rqstp)
{
  i4_t i, *segs = (in->seg_vadr).seg_vadr_val;
  segment_t *cs;
  
  for (i = 0; i < (in->seg_vadr).seg_vadr_len; i++)
    if (segs[i])
      unlink_segment (segs[i], 1);
  if (in->segment)
    for ( cs = dyn_segms; cs ; cs = cs->next )
      if ( cs->id == in->segment)
        {
          xfree (cs->ptr);
          if (cs->prev) cs->prev->next = cs->next;
          if (cs->next) cs->next->prev = cs->prev;
          xfree(cs);
          break;
        }
  gsqltrn_rc.sqlcode = 0;
  return &gsqltrn_rc;
} /* seg_del */

/*-------------------------------------------------------------------------------*/

void
describe_stmt(descr_t *d,TXTREF vcb,char mode)
{
  register i4_t   i;
  TXTREF         cn;  /* current node */

  assert(d);
  /* travel throuth the vcb list and mark interesting parameters */
  for(i=0, cn = vcb; cn ; cn = RIGHT_TRN(cn))
    if (CODE_TRN(cn) == PARAMETER)
      switch(mode)
        {
        case 'I' /* interface */ :
          i++; SetF_TRN(cn,MARK_F); break;
        case '>' /* call input parameters */ :
        case 'i' /* object method input parameters */ :
          if ( !TstF_TRN(cn,OUT_F) && !TstF_TRN(cn,INDICATOR_F))
            { i++; SetF_TRN(cn,MARK_F); }
          break;
        case '<' /* call output parameters */ :
        case 'o' /* object method output parameters */ :
          if ( TstF_TRN(cn,OUT_F) && !TstF_TRN(cn,INDICATOR_F))
            { i++; SetF_TRN(cn,MARK_F); }
          break;
        default:
          lperror("Internal error: incorrect describe statement mode '%c'",
                  mode);
          return;
        }

  if (i==0) /* if we havn't found anything interesting */
    return;
  
  /*--- process choosen parameters ---*/
  d->descr_t_len += i;/*increase descriptor list(is it really smart idea?..Hmm)*/
  if (d->descr_t_len == i)
    d->descr_t_val = (prep_elem_t*)xmalloc(sizeof(prep_elem_t)*i);
  else /* there was already elements in descriptor list */
    d->descr_t_val =
      (prep_elem_t*)xrealloc((void*)d->descr_t_val,
                             sizeof(prep_elem_t)*(d->descr_t_len));
  
  for(cn = vcb ; cn && (i >= 0) ;cn = RIGHT_TRN(cn))
    if (TstF_TRN(cn,MARK_F))
      {
        prep_elem_t *e = &(d->descr_t_val[d->descr_t_len - i--]);
        sql_type_t  *t = node_type (cn);
        e->type        = t->code;
        if (t->code)
          {
            e->length      = sizeof_sqltype (*t);
          }
        e->ind_name    = e->name = NULL;
        
        if(bcmp(STRING(PAR_NAME(cn)),"SQL__",5))
          e->name = savestring(STRING(PAR_NAME(cn)));
        if (mode == 'I') /* if we process interface */
          { /* there must be no indicators          */
            /* assert(PAR_INDICATOR(cn)==0);        */
            e->nullable = (TstF_TRN(cn,OUT_F)?-1:0);
          }
        else
          {
            e->nullable = (PAR_INDICATOR(cn)==0?0:1);
            if (e->nullable==1)
              e->ind_name = savestring(STRING(PAR_NAME(PAR_INDICATOR(cn))));
          }
        if (!e->name)
          e->name     = savestring(""); /* to avoid sending NULL */ 
        if (!e->ind_name)
          e->ind_name = savestring(""); /* to avoid sending NULL */ 
        
        ClrF_TRN(cn,MARK_F);
      }
  return;
}

static void
check_descriptor(descr_t *proto,descr_t *call,stmt_info_t *stmt)
{
  i4_t i;
  if (proto->descr_t_len != call->descr_t_len)
    {
      lperror("Error: number of parameters mismatch in call(%d) and \n"
              "compiled code(%d) for statement :\n '%s'\n",
              call->descr_t_len,proto->descr_t_len,stmt->stmt);
      return;
    }
  for(i = 0; i < proto->descr_t_len; i++)
    {
      prep_elem_t *o = &(proto->descr_t_val[i]);
      prep_elem_t *c = &(call->descr_t_val[i]);
      if (c->type == o->type)
        continue;
      if (!c->type)
        {
          c->type = o->type;
          c->length = o->length;
        }
      else
        {
          lperror("Warning: call and query parameter types mismatch for "
                  "'%s'-'%s'\n in statement : '%s'\n",
                  c->name,o->name,stmt->stmt);
          errors--;
        }
    }
}

void
emit_module_interface(stmt_info_t *in)
{
  TXTREF cn;
  i4_t    oid;
  compiled_object_t *last;
  
  assert( gsqltrn_rc.info.return_data_u.comp_ret.objects == NULL);
  last = NULL;
  for (cn = ROOT,oid = 0; cn ; cn = RIGHT_TRN(cn),oid ++)
    {
      TXTREF vcb; 
      compiled_object_t *current; 
      current = (compiled_object_t*) xmalloc ( sizeof (compiled_object_t));
      if ( CODE_TRN(cn) == CUR_AREA )
        {
          current->cursor_name = savestring(STRING(CUR_NAME(STMT_VCB(cn))));
          if (!TstF_TRN(DOWN_TRN (DOWN_TRN (cn)),RO_F))
            { /*              tblptr    from      query    decl_curs cur_area*/
              TXTREF tblptr = DOWN_TRN (DOWN_TRN (DOWN_TRN(DOWN_TRN (cn))));
              TXTREF tbl    = COR_TBL(TABL_DESC(tblptr));
              current->table_name  = savestring(STRING (TBL_NAME (tbl)));
              current->table_owner = savestring(STRING (TBL_FNAME (tbl)));
            }
          vcb = STMT_VCB(DOWN_TRN(cn));
        }
      else
        vcb = STMT_VCB(cn);
      if (!current->cursor_name)
        current->cursor_name  = savestring(""); /* to avoid sending NULL */ 
      if (!current->table_name)
        current->table_name  = savestring(""); /* to avoid sending NULL */ 
      if (!current->table_owner)
        current->table_owner  = savestring(""); /* to avoid sending NULL */ 
      current->object = oid;
      describe_stmt(&(current->descr_in),vcb,'i');
      describe_stmt(&(current->descr_out),vcb,'o');
      /* check call substittions */
      {
        call_t      *call = gsqltrn_rc.info.return_data_u.comp_ret.calls;
        stmt_info_t *stmt = in; 
        while(call)
          {
            assert(stmt);
            if (call->subst && call->subst->object == STMT_UID(cn))
              {
                call->subst->object = oid;
                file_pos = stmt->bline;
                if (*current->cursor_name) /* if multi-row query */
                  switch (call->subst->method)
                    {
                    case 0: /* open cursor */
                      check_descriptor(&(current->descr_in),
                                       &(call->subst->in_sql_parm),
                                       stmt);
                      if (call->subst->out_sql_parm.descr_t_len)
                        lperror("Error: output parameters found "
                                "in OPEN CURSOR '%s' statement",
                                current->cursor_name);
                      break;
                    case 1: /* fetch  */
                      if (call->subst->in_sql_parm.descr_t_len)
                        lperror("Error: input parameters in FETCH CURSOR"
                                " '%s' statement", current->cursor_name);
                      check_descriptor(&(current->descr_out),
                                       &(call->subst->out_sql_parm),
                                       stmt);
                      break;
                    case 2: /* close  */
                      if (call->subst->out_sql_parm.descr_t_len ||
                          call->subst->in_sql_parm.descr_t_len)
                        lperror("Error: parameters in CLOSE CURSOR '%s' statement",
                                current->cursor_name);
                      break;
                    case 3: /* delete */
                      if (!*current->table_name) /* if not updatable */
                        lperror("Attempt to positional delete on read-only "
                                "query by cursor '%s'",current->cursor_name);
                      if (call->subst->out_sql_parm.descr_t_len ||
                          call->subst->in_sql_parm.descr_t_len)
                        lperror("Error: parameters in "
                                "DELETE ... where current of CURSOR '%s' statement",
                                current->cursor_name);
                      break;
                    default: /* error */
                      lperror("Internal error: unexpected method required "
                              "on cursor '%s'",
                              current->cursor_name);
                      
                    }
                else /* other type of statement */
                  {
                    assert(call->subst->method==0);
                    check_descriptor(&(current->descr_in),
                                     &(call->subst->in_sql_parm),
                                     stmt);
                    check_descriptor(&(current->descr_out),
                                     &(call->subst->out_sql_parm),
                                     stmt);
                  }
              }
            call = call->next;
            stmt = stmt->next;
          }
      }
      if (last)
        last = last->next = current;
      else /* if the first object */
        last = gsqltrn_rc.info.return_data_u.comp_ret.objects = current;
    }
}

/*--------------------------------------------------------------------*
 *  External interfaces                                               *
 *--------------------------------------------------------------------*/

result_t*
init_compiler(init_params_t *in, struct svc_req *rqstp)
{
  static char du[1] = "";
  gsqltrn_rc.sqlcode = 0;
  finish(NULL);
  close_out_files ();
  if(progname && progname!=du)
    xfree(progname);
  progname = NULL;
  read_options (in->init_params_t_len, in->init_params_t_val);
  if(!progname)
    progname = du;
  sql_prg = savestring(tmpnam (NULL));
  fatal_error = 0;
  errors = 0;
  open_out_files ();
  if (install (NULL) != SQL_SUCCESS)
    gsqltrn_rc.sqlcode = -DS_SYNTER;
  return &gsqltrn_rc;
}

/*--------------------------------------------------------------------*/

#define COMPILED gsqltrn_rc.info.return_data_u.comp_ret
#define STORED   COMPILED.stored.comp_data_t_u

#define BREAK_COMPILATION -101
result_t*
compile(stmt_info_t *in, struct svc_req *rqstp)
{
  TXTREF  o_root    = TNULL;
  
  gsqltrn_rc.sqlcode = 0;
  o_root = ROOT;
  ROOT   = TNULL;
  TRY {
    ld_segment = VNULL;
    gsqltrn_rc.info.rett = RET_COMP;
    /* parse statements one by one */
    {
      call_t *last_call = NULL;
      stmt_info_t *stmt = in;
      while (stmt)
        {
          extern char *dyn_sql_stmt_name;
          dyn_sql_stmt_name = (*stmt->stmt_name?stmt->stmt_name : NULL);
          restart_server_scanner (stmt->stmt, stmt->bline);
          compile_data (0,NULL) ;
          /* get replacement from parse and intelligently process it */
          if (last_call==NULL)
            last_call = COMPILED.calls = prepare_replacement();
          else
            last_call = last_call->next = prepare_replacement();
          stmt = stmt->next;
        }
      if(last_call)
        last_call->next = NULL;
    }
    if (errors) EXCEPTION(BREAK_COMPILATION);
    /* compile whole module as a piece */
    if (ROOT) /* if there is something to compile */
      {
        compile_data (1, NULL); /* process it */
        if (errors) EXCEPTION(BREAK_COMPILATION);
        emit_module_interface(in);
      }
    { /* store compiled data and settings */
      void * ptr;
      i4_t    size;
      if (*progname) /* if static compilation */
        {
          ptr = export_segment (ld_segment,&size, 0);
          STORED.module = store_module(ptr,size);
          COMPILED.stored.comp_type = COMP_STATIC;
        }
      else /* if dynamic compilation */
        if (ROOT == TNULL ) /* if compiled something like commit or rollback */
          { /* */
            compiled_object_t *cr;
            /* we assume that there still no objects */
            assert(COMPILED.objects == NULL);
            /* and just one call                     */
            assert(COMPILED.calls != NULL);
            assert(COMPILED.calls->next == NULL);
            if (COMPILED.calls->subst) /* if it has meaningful  substituion */
              {
                assert(COMPILED.calls->subst );
                /* and this call is of special type      */
                assert(COMPILED.calls->subst->object < 0);
                cr = (compiled_object_t*) xmalloc(sizeof(compiled_object_t));
                cr->cursor_name  = savestring(""); /* to avoid sending NULL */ 
                cr->table_name   = savestring(""); /* to avoid sending NULL */ 
                cr->table_owner  = savestring(""); /* to avoid sending NULL */ 
                cr->object = COMPILED.calls->subst->object;
                COMPILED.objects = cr;
              }
          }
        else /* if compiled something really compilable */
          {
            assert ( RIGHT_TRN(ROOT) == TNULL);
            switch(CODE_TRN(ROOT))
              {
              case CUR_AREA: /* if cursor compiled */
                ptr = export_segment (ld_segment,&size, 0);
                STORED.seg_ptr = append_current_segm (ptr,size);
                COMPILED.stored.comp_type = COMP_DYNAMIC_CURSOR;
                break;
              case CREATE: /* if create table or view */
              case DROP:
              case ALTER:
                /*         - don't remember them as checked in compiler tree */
                if (CREATE_OBJ(ROOT))
                  {
                    TXTREF tbl = CREATE_OBJ(ROOT);
                    free_line(ROOT);
                    ROOT = TNULL;
                    del_info(tbl);
                  }
              default:
                STORED.segm = ld_segment;
                COMPILED.stored.comp_type = COMP_DYNAMIC_SIMPLE;
              }
          }
    }
    ROOT = join_list(o_root,ROOT);
  }
  CATCH {
  case -ER_COMP_FATAL: /* fatal error encountered */
    gsqltrn_rc.sqlcode = -ER_COMP_FATAL;
  case BREAK_COMPILATION: /* errors in compilations  */
    ROOT = o_root;
  }
  END_TRY;
  COMPILED.errors = errors;
  COMPILED.bufs   = get_files_bufs ();
  return &gsqltrn_rc;
}

/*--------------------------------------------------------------------------*/
