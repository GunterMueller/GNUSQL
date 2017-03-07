/*
 *  dyn_funcs.c  -  functions of GNU dynamic SQL
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

/* $Id: dyn_funcs.c,v 1.249 1998/09/29 22:23:42 kimelman Exp $ */

#include "setup_os.h"
#ifdef HAVE_STDARG_H
# include <stdarg.h>
#endif
#include "gsqltrn.h"
#include "xmem.h"
#include "dyn_funcs.h"
#include "sql.h"
#include "global.h"

/* states of declared cursor */
#define CUR_CLOSED   0
#define HAS_CUR_ROW  1
#define NO_CUR_ROW   2

/* interpreter commands */
#define COM_START  0
#define COM_FETCH  1
#define COM_CLOSE  2
#define COM_DELETE 3

#define XFREE(v)                {if (v) xfree (v); v = NULL;     }
#define STRCPY(src, dest)	{if (src) dest = savestring(src);}

#define CHECK_FREE_DATA(val, cnt)		\
{						\
  int i;					\
  for (i = 0; i < (cnt); i++)			\
    if ((val)->data_allocated_size)		\
      XFREE ((val)->data);			\
}

struct s_cursor {
  char            *name;
  char            *stmt_name;
  char             status;
  struct s_stmt   *stmt_ptr;
  i4_t             segm; /* virtual address of made cursor segment */
  struct s_cursor *next;
};

struct s_descr {
  char           *name;
  SQL_DESCR       body_ptr;
  struct s_descr *next;
};

static struct s_stmt   *fir_stmt   = NULL;
static struct s_cursor *fir_cursor = NULL;
static struct s_descr  *fir_descr  = NULL;

i4_t SQL__prepare_stmt (char *stmt_text, char *stmt_name,
                         compiled_t **prep_res);
i4_t server_seg_del (i4_t seg_cnt, i4_t *seg_vadr, i4_t mem_ptr);
i4_t SQL__get_cursor_segm (char *cursor_name, struct s_stmt *stmt);

struct module interp_mdl;

#define typ_user2sql(user_type) get_sqltype_code(user_type)
#define typ_sql2user(sql_type)  get_dyn_sqltype_code(sql_type)

static char *
cat_strings (i4_t str_cnt, ...)
{
  i4_t i, len = 0;
  va_list args;
  char *str, *cur_pos, *res_str = NULL;

  va_start (args, str_cnt);
  for (i = 0; i < str_cnt; i++)
    {
      str = va_arg (args, char *);
      if (str)
        len += strlen (str);
    }
  va_end (args);
  
  if (len)
    {
      res_str = TYP_ALLOC (len+1, char);
      cur_pos = res_str;
      va_start (args, str_cnt);
      for (i = 0; i < str_cnt; i++)
        {
          str = va_arg (args, char *);
          if (str)
            {
              strcpy (cur_pos, str);
              cur_pos += strlen (str);
            }
        }
      cur_pos[0] = 0;
      va_end (args);
    }
      
  return res_str;
} /* cat_strings */

SQL_DESCR
SQL__allocate_descr (char *descr_name, int maximum)
{
  struct s_descr  *descr;
  SQL_DESCR        body;
  
  if (!descr_name || !(*descr_name))
    return NULL; /* incorrect name of descriptor */
  
  if (SQL__get_descr (descr_name))
    return NULL; /* there is already descriptor with this name */

  descr = TYP_ALLOC (1, struct s_descr);
  STRCPY (descr_name, descr->name);
  descr->body_ptr = body = TYP_ALLOC (1, sql_descr_t);
  descr->next = fir_descr;
  fir_descr = descr;

  body->maximum = maximum;
  if (maximum)
    {
      body->values = TYP_ALLOC (maximum, sql_descr_element_t);
      body->alloc_cnt = maximum;
    }
  
  return body;
} /* allocate_descr */

void
SQL__deallocate_descr (SQL_DESCR *descr_ptr)
{
  struct s_descr *cur_descr, *pred_descr;
  SQL_DESCR descr;

  if (!descr_ptr)
    return;

  descr = *descr_ptr;
  *descr_ptr = NULL;
  for (cur_descr = fir_descr, pred_descr = NULL; cur_descr;
       pred_descr = cur_descr, cur_descr = cur_descr->next)
    if (cur_descr->body_ptr == descr)
      break;

  if (cur_descr)
    {
      CHECK_FREE_DATA (descr->values, descr->alloc_cnt);
      XFREE (descr->values);
      XFREE (cur_descr->name);
      XFREE (cur_descr->body_ptr);

      if (pred_descr)
        pred_descr->next = cur_descr->next;
      else
        fir_descr = cur_descr->next;
      xfree (cur_descr);
    }
} /* deallocate_descr */

SQL_DESCR
SQL__get_descr (char *descr_name)
{
  struct s_descr *cur_descr = NULL;
  SQL_DESCR body_ptr = NULL;
  
  if (descr_name)
    for (cur_descr = fir_descr; cur_descr; cur_descr = cur_descr->next)
      if (!strcmp (descr_name, cur_descr->name))
        break;

  if (cur_descr)
    body_ptr = cur_descr->body_ptr;
  return body_ptr;
} /* get_descr */

static struct s_stmt *
SQL__find_stmt (char *stmt_name)
{
  struct s_stmt *cur_stmt = NULL;
  
  if (stmt_name)
    for (cur_stmt = fir_stmt; cur_stmt; cur_stmt = cur_stmt->next)
      if (!strcmp (stmt_name, cur_stmt->name))
        break;

  return cur_stmt;
} /* find_stmt */

static
struct s_cursor *
SQL__find_cursor (char *cursor_name)
{
  struct s_cursor *cur_cursor = NULL;
  
  if (cursor_name)
    for (cur_cursor = fir_cursor; cur_cursor;
         cur_cursor = cur_cursor->next)
      if (!strcmp (cursor_name, cur_cursor->name))
        break;

  return cur_cursor;
} /* find_cursor */

/* all following functions return error code = SQLCODE */

int
SQL__prepare (char *stmt_name, char *stmt_text)
{
  struct s_stmt *stmt = SQL__find_stmt (stmt_name);
  compiled_t *prep_res;
  i4_t err, do_out, n, i;
  sql_descr_element_t *val_to;
  prep_elem_t *val_from;
  
  if (stmt && (err = SQL__deallocate_prepare (stmt_name)))
    return err;
  
  /* statement creation */
  if (!stmt_name || !(*stmt_name))
    return -DS_NMARG;
  
  stmt = TYP_ALLOC (1, struct s_stmt);
  STRCPY (stmt_name, stmt->name);
  stmt->next = fir_stmt;
  fir_stmt = stmt;

  if ((err = SQL__prepare_stmt (stmt_text, stmt_name, &prep_res)))
    return err;

  /* filling of preparing result to statement descriptor */
  if (!prep_res->objects)
    {
      stmt->sectnum = -3;
      return 0;
    }
  stmt->sectnum = prep_res->objects->object;
  if (prep_res->objects->cursor_name)
    stmt->dep_on_cursor = SQL__find_cursor (prep_res->objects->cursor_name);
  if (!*prep_res->objects->table_name)
    stmt->table_name = NULL;
  else
    STRCPY (prep_res->objects->table_name, stmt->table_name);
  if (!*prep_res->objects->table_owner)
    stmt->table_owner = NULL;
  else
    STRCPY (prep_res->objects->table_owner, stmt->table_owner);
  
  if (prep_res->stored.comp_type == COMP_DYNAMIC_SIMPLE)
    stmt->segm = prep_res->stored.comp_data_t_u.segm;
  else
    stmt->mem_ptr = prep_res->stored.comp_data_t_u.seg_ptr;
  
  n = stmt->in_descr.count = prep_res->objects->descr_in.descr_t_len;
  val_from = prep_res->objects->descr_in.descr_t_val;
  val_to = stmt->in_descr.values = TYP_ALLOC (n, sql_descr_element_t);

  for (do_out = 1; do_out >= 0; do_out--)
    {
      for (i = 0; i < n; i++, val_to++, val_from++)
        {
          val_to->type = val_from->type;
          val_to->length = val_from->length;
          val_to->nullable = val_from->nullable;
          val_to->unnamed = (val_from->name) ? 0 : 1;
          STRCPY (val_from->name, val_to->name);
        }

      if (do_out)
        {
          n = stmt->out_descr.count = prep_res->objects->descr_out.descr_t_len;
          val_from = prep_res->objects->descr_out.descr_t_val;
          val_to = stmt->out_descr.values =
            TYP_ALLOC (n, sql_descr_element_t);
        }
    }
  return 0;
} /* prepare */

int
SQL__deallocate_prepare (char *stmt_name)
{
  struct s_stmt *stmt = NULL, *cur_stmt, *next_stmt, *pred_stmt = NULL;
  i4_t i, err, cursor_cnt = 0, *seg_vadr;
  sql_descr_element_t *in_values, *out_values;
  struct s_cursor *cur_cursor, *pred_cursor, *next_cursor;
  
  if (stmt_name)
    for (stmt = fir_stmt; stmt; pred_stmt = stmt, stmt = stmt->next)
      if (!strcmp (stmt_name, stmt->name))
        break;

  if (!stmt)
    return -DS_STMT;

  /* checking of cursors states */
  if (stmt->mem_ptr) /* statement can have cursors depended on it */
    {
      
      for (cur_cursor = fir_cursor; cur_cursor;
           cur_cursor = cur_cursor->next, cursor_cnt++)
        if (cur_cursor->stmt_ptr == stmt)
          if (cur_cursor->status != CUR_CLOSED)
            break;
      if (cur_cursor)
        return -DS_CURST;
    }
  if (stmt->segm)
    {
      seg_vadr = TYP_ALLOC (cursor_cnt + 1, i4_t);
      seg_vadr[0] = stmt->segm;
      if (cursor_cnt)
        for (i = 0, cur_cursor = fir_cursor, pred_cursor = NULL; cur_cursor;
             pred_cursor = cur_cursor, cur_cursor = next_cursor)
          {
            next_cursor = cur_cursor->next;
            if (cur_cursor->stmt_ptr == stmt)
              /* current cursor was declared for argument statement */
              {
                seg_vadr[++i] = cur_cursor->segm;
                /* making DEALLOCATE PREPARE for all prepared statement *
                 * that are being depended on current cursor            */
                for (cur_stmt = fir_stmt; cur_stmt; cur_stmt = next_stmt)
                  {
                    next_stmt = cur_stmt->next;
                    if (cur_stmt->dep_on_cursor == cur_cursor)
                      if ((err = SQL__deallocate_prepare (cur_stmt->name)) < 0)
                        return err;
                  }
                /* making 'deallocate' cursor */
                if (cur_cursor->stmt_name)
                  /* cursor made by DECLARE CURSOR */
                  {
                    cur_cursor->stmt_ptr = NULL;
                    cur_cursor->segm = 0;
                  }
                else
                  /* cursor made by ALLOCATE CURSOR */
                  {
                    XFREE (cur_cursor->name);
                    if (pred_cursor)
                      pred_cursor->next = next_cursor;
                    else
                      fir_cursor = next_cursor;
                    xfree (cur_cursor);
                  }
              }
          }
      if ((err = server_seg_del (cursor_cnt + 1, seg_vadr, stmt->mem_ptr)) < 0)
        return err;
      xfree (seg_vadr);
  
      in_values  = stmt->in_descr.values;
      out_values = stmt->out_descr.values;
  
      for (i = 0; i < stmt->in_descr.count; i++)
        {
          XFREE ((in_values[i]).name);
          XFREE ((in_values[i]).data);
        }
      for (i = 0; i < stmt->out_descr.count; i++)
        {
          XFREE ((out_values[i]).name);
          XFREE ((out_values[i]).data);
        }
      XFREE (stmt->table_name);
      XFREE (stmt->table_owner);
      XFREE (stmt->in_descr.values);
      XFREE (stmt->out_descr.values);
    }

  XFREE (stmt->name);
  
  if (pred_stmt)
    pred_stmt->next = stmt->next;
  else
    fir_stmt = stmt->next;
  xfree (stmt);
  return 0;
} /* deallocate_prepare */

/* In all following fuctions working with descriptors:              *
 * 'using_input' must be correctly filled accordingly description   *
 * of 'stmt_name' input.                                            *
 * Some fields of 'using_output' can be not filled. In this case    *
 * the function will make correspondent result value description.   *
 * In all cases function makes for output-descriptor memory         *
 * allocation for DATA. So we recommend for users of this functions *
 * don't use the same descriptor for input & output description.    */

int
SQL__describe (char *stmt_name, int is_input, SQL_DESCR descr)
/* for internal representation of descriptors (struct sql_descr_t)
   we have the same copies of pointers to strings (fields
   name & data in descriptor elements). But in realization
   of SQL dynamic operators 'GET & SET DESCRIPTOR' we must
   use user's places for strings or generate new ones              */
/* FOR USER of this function (instead of dynamic SQL operator):
   you can't use pointers to strings from descriptor after next
   statement 'stmt_name' preparing                              */
{
  struct s_stmt *stmt = SQL__find_stmt (stmt_name);
  SQL_DESCR      src;
  i4_t            i;

  if (!stmt)
    return -DS_STMT;
  if (!descr)
    return -DS_DESCR;
  src = (is_input) ? &(stmt->in_descr) : &(stmt->out_descr);
  if (src->count > descr->alloc_cnt)
    {
      if (descr->maximum)
        return -DS_DESCRLEN;
      else
        CHECK_ARR_SIZE (descr->values, descr->alloc_cnt,
                        src->count, sql_descr_element_t);
    }
  descr->count = src->count;
  CHECK_FREE_DATA (descr->values, descr->count);
  TYP_COPY (src->values, descr->values, src->count, sql_descr_element_t);
  for (i = 0; i < descr->count; i++)
    ((descr->values)[i]).type = typ_sql2user (((src->values)[i]).type);
  return 0;
} /* describe */

static int
SQL_d_interpret (i4_t segm, int command, SQL_DESCR using_descr, struct s_stmt *stmt)
/* segm can be = 0 => command must be = -1 (rollback) or -2 (commit) */
{
  static gsql_parms  args        = { 0, 0, 0, 0, NULL};
  static int         args_max    = 0;
  static double     *mk_args     = NULL;
  static int         mk_args_max = 0;
  
  SQL_DESCR          stmt_descr = NULL;
  i4_t need_arg = 0, need_res = 0, need_cnt = 0;
  i4_t need_type, real_type, need_length, err = 0;
  sql_descr_element_t *val, *need_val;
  gsql_parm           *arg;
  int                  i;

  if (!segm)
    {
      switch (command)
        {
        case -2: /* commit   */
          _SQL_commit();
          break;
        case -1: /* rollback */
          _SQL_rollback();
          break;
        default:
          err = -ER_8;
        }
      return err;
    }
  
  interp_mdl.segment = segm;

  if (stmt)
    {
      if (command == COM_START)
        {
          stmt_descr = &(stmt->in_descr);
          need_cnt = stmt_descr->count;
          if (need_cnt)
            need_arg++;
        }
      else if (command == COM_FETCH)
        {
          stmt_descr = &(stmt->out_descr);
          need_cnt = stmt_descr->count;
          need_res++;
        }
    }
  if (using_descr && using_descr->maximum &&
      using_descr->count > using_descr->maximum)
    return -DS_CNT;

  CHECK_ARR_SIZE (args.prm, args_max, need_cnt+1, gsql_parm);
  
  if (need_arg)
    {
      CHECK_ARR_SIZE (mk_args, mk_args_max, need_cnt, double);
      if (!using_descr || !(using_descr->count))
        return -DS_NOPAR;
      if (using_descr->count != need_cnt)
        return -DS_BADPAR;
      for (i = 0, val = using_descr->values,
             need_val = stmt_descr->values, arg = &(args.prm[0]);
           i < need_cnt; i++, val++, need_val++, arg++)
        {
          /* using this descriptor as argument cancells all        *
           * allocations made in it when it was an result receiver */
          if (val->data_allocated_size)
            val->data_allocated_size = 0;
          
          need_type = need_val->type;
          if (val->indicator >=0)
            {
              real_type = typ_user2sql (val->type);
              if ((!real_type) || (real_type == T_STR && need_type != T_STR) ||
                  (real_type != T_STR && need_type == T_STR))
                return -DS_BADPAR;
              
              if (real_type != need_type)
                /* here real_type can't be T_STR */
                {
                  arg->valptr = mk_args + i;
                  if (put_dat (val->data, 0, real_type, REGULAR_VALUE,
                               arg->valptr, 0, need_type, NULL) < 0)
                    return -DS_BADPAR;
                }
              else
                arg->valptr = val->data;
            }
          arg->type = need_type;
          if (need_type == T_STR)
            arg->length = strlen (arg->valptr);
          else
             arg->length = 0;
          arg->flags = 0; /* in-flag */
          arg->indptr = &(val->indicator);
        }
    }
  
  if (need_res)
    {
      if (!using_descr)
        return -DS_NORES;
      if (!(using_descr->count))
        using_descr->count = need_cnt;
      else if (using_descr->count != need_cnt)
        return -DS_BADTAR;
          
      if (need_cnt > using_descr->alloc_cnt)
        {
          if (using_descr->maximum)
            return -DS_DESCRLEN;
          else
            CHECK_ARR_SIZE (using_descr->values, using_descr->alloc_cnt,
                            need_cnt, sql_descr_element_t);
        }
      for (i = 0, val = using_descr->values,
             need_val = stmt_descr->values, arg = &(args.prm[0]);
           i < need_cnt; i++, val++, need_val++, arg++)
        {
          if (!(val->data_allocated_size))
            val->data = NULL;
          
          need_type = need_val->type;
          real_type = typ_user2sql (val->type);
          if (!real_type)
            {
              val->type = typ_sql2user (need_type);
              real_type = need_type;
            }
          else
            if ((real_type == T_STR && need_type != T_STR) ||
                (real_type != T_STR && need_type == T_STR))
              return -DS_BADTAR;
              
          arg->type = real_type;
          if (need_type != T_STR || !(val->length))
            val->length = need_val->length;
          need_length =  val->length;
          arg->flags = 1; /* out-flag */
          arg->indptr = &(val->indicator);
          if (need_type == T_STR)
            need_length++;
          CHECK_ARR_SIZE (val->data, val->data_allocated_size,
                          need_length, char);
          arg->length = need_length;
          arg->valptr = val->data;
        }
    }
  
  if (using_descr && (!need_cnt))
    return -DS_SYNTER;
  
  args.count = need_cnt;
  args.sectnum = 0;
  args.command = command;
  if (command == COM_FETCH && (!stmt->table_name || !*stmt->table_name))
    args.options  = 50;
  else
    args.options  = 0;
  
  SQL__runtime (&interp_mdl, &args);
  return SQLCODE;
} /* SQL_d_interpret */

int
SQL__execute (char *stmt_name,
               SQL_DESCR using_input, SQL_DESCR using_output)
/* using_input & using_output are arguments here, they can be = NULL */
{
  struct s_stmt *stmt = SQL__find_stmt (stmt_name);
  i4_t segm, err, i;
  
  if (!stmt)
    return -DS_STMT;
  segm = stmt->segm;

  if (stmt->mem_ptr) /* select */
    {
      err = SQL_d_interpret (segm, COM_START, using_input, stmt);
      if (err) return err;
      for (i = 0; i < 2 && !err; i++)
        err = SQL_d_interpret (segm, COM_FETCH, using_output, stmt);
      if (err < 0) return err;
      SQL_d_interpret (segm, COM_CLOSE, NULL, stmt);
      if (err == 100)
        SQLCODE = (i == 1) ? 100 : 0;
      else
        {
          CHECK_FREE_DATA (using_output->values, using_output->count);
          SQLCODE = -DS_EXE;
        }
      err = SQLCODE;
    }
  else
    {
      if (using_output)
        return -DS_SYNTER;
      err = SQL_d_interpret (segm, stmt->sectnum, using_input, stmt);
    }
  return err;
} /* execute */

int
SQL__execute_immediate (char *stmt_text)
{
  compiled_t *prep_res;
  i4_t err;
  
  if ((err = SQL__prepare_stmt (stmt_text,"", &prep_res)))
    return err;
  if (!prep_res->objects)
    return 0;
  if ( (prep_res->stored.comp_type != COMP_DYNAMIC_SIMPLE) || 
      *prep_res->objects->table_name || *prep_res->objects->table_owner ||
      prep_res->objects->descr_in.descr_t_len ||
      prep_res->objects->descr_out.descr_t_len ||
      *prep_res->objects->cursor_name )
    return -DS_SYNTER;

  return SQL_d_interpret (prep_res->stored.comp_data_t_u.segm,
                      prep_res->objects->object, NULL, NULL);
} /* execute_immediate */

int
SQL__declare_cursor (char *stmt_name, char *cursor_name)
/* only associates cursor & statement names: *
 * pointed statement can be not prepared yet */
{
  struct s_cursor *cursor = SQL__find_cursor (cursor_name);

  if (cursor)
    return -DS_CURDECL;
  if (!stmt_name || !(*stmt_name))
    return -DS_STMT;

  cursor = TYP_ALLOC (1, struct s_cursor);
  STRCPY (cursor_name, cursor->name);
  STRCPY (stmt_name, cursor->stmt_name);
  cursor->status = CUR_CLOSED;
  cursor->next = fir_cursor;
  fir_cursor = cursor;
  return 0;
} /* declare_cursor */

int
SQL__allocate_cursor (char *stmt_name, char *cursor_name)
/* associates cursor name with prepared statement */
{
  struct s_cursor *cursor = SQL__find_cursor (cursor_name);
  struct s_stmt   *stmt   = SQL__find_stmt   (stmt_name);

  if (cursor)
    return -DS_CURDECL;
  if (!stmt /* incorrect statemnet name */
      || !(stmt->mem_ptr) /* prepared statement is not 'cursor specification' */)
    return -DS_STMT;

  cursor = TYP_ALLOC (1, struct s_cursor);
  STRCPY (cursor_name, cursor->name);
  cursor->stmt_ptr = stmt;
  cursor->status = CUR_CLOSED;
  cursor->next = fir_cursor;
  fir_cursor = cursor;
  cursor->segm = SQL__get_cursor_segm (cursor_name, stmt);
  return (cursor->segm < 0) ? cursor->segm : 0;
} /* allocate_cursor */

int
SQL__open_cursor (char *cursor_name, SQL_DESCR using_input)
/* using_input is argument, it can be = NULL */
{
  struct s_cursor *cursor = SQL__find_cursor (cursor_name);
  i4_t err;
  
  if (!cursor)
    return -DS_CUR;
  
  if (!(cursor->segm))
    {
      cursor->stmt_ptr = SQL__find_stmt (cursor->stmt_name);
      if ((cursor->segm =
           SQL__get_cursor_segm (cursor_name, cursor->stmt_ptr)) < 0)
        return cursor->segm;
    }

  if ((err = SQL_d_interpret (cursor->segm, COM_START,
                          using_input, cursor->stmt_ptr)) < 0)
    return err;
  cursor->status = NO_CUR_ROW;
  return 0;
} /* open_cursor */

int
SQL__fetch (char *cursor_name, SQL_DESCR using_output)
/* using_output is the result, after correct fininshing
   of function's work using_output != NULL */
{
  struct s_cursor *cursor = SQL__find_cursor (cursor_name);
  i4_t err;
  
  if (!cursor)
    return -DS_CUR;
  if (cursor->status == CUR_CLOSED)
    return -DS_CURST;
  
  if ((err = SQL_d_interpret (cursor->segm, COM_FETCH,
                          using_output, cursor->stmt_ptr)) < 0)
    return err;
  if (!err)
    cursor->status = HAS_CUR_ROW;
  return 0;
} /* fetch */

int
SQL__close_cursor (char *cursor_name)
{
  struct s_cursor *cursor = SQL__find_cursor (cursor_name);
  i4_t err;
  
  if (!cursor)
    return -DS_CUR;
  if (cursor->status == CUR_CLOSED)
    return -DS_CURST;
  
  if ((err = SQL_d_interpret (cursor->segm, COM_CLOSE, NULL, NULL)) < 0)
    return err;
  cursor->status = CUR_CLOSED;
  return 0;
} /* close_cursor */

int
SQL__delete (char *cursor_name, char *table_name, char *table_owner)
/* table_name is used here for checking of DELETE statement using */
{
  struct s_cursor *cursor = SQL__find_cursor (cursor_name);
  struct s_stmt   *stmt;
  
  if (!cursor)
    return -DS_CUR;
  if (cursor->status != HAS_CUR_ROW)
    return -DS_CURST;

  stmt = cursor->stmt_ptr;
  if (!table_name || strcmp (table_name, stmt->table_name) ||
      !table_owner || strcmp (table_owner, stmt->table_owner))
    return -TNERR;

  cursor->status = NO_CUR_ROW;
  return SQL_d_interpret (cursor->segm, COM_DELETE, NULL, NULL);
} /* delete */

int
SQL__update (char *cursor_name, char *table_name,
              char *table_owner, char *set_clause)
/* table_name is used here for checking of UPDATE statement using */
/* set_clause = "SET ..." */
{
  struct s_cursor *cursor = SQL__find_cursor (cursor_name);
  struct s_stmt   *stmt;
  compiled_t      *prep_res;
  char            *stmt_text;
  i4_t              err;
  
  if (!cursor)
    return -DS_CUR;
  if (cursor->status != HAS_CUR_ROW)
    return -DS_CURST;

  stmt = cursor->stmt_ptr;
  if (!table_name || strcmp (table_name, stmt->table_name) ||
      !table_owner || strcmp (table_owner, stmt->table_owner))
    return -TNERR;

  stmt_text = cat_strings (8, "UPDATE ", table_owner, ".",
                           table_name, " ", set_clause,
                           " WHERE CURRENT OF ", cursor_name);
  err = SQL__prepare_stmt (stmt_text, NULL, &prep_res);
  xfree (stmt_text);
  if (err)
    return err;
  
  if (prep_res->stored.comp_type!=COMP_DYNAMIC_SIMPLE ||
      !(*prep_res->objects->cursor_name) ||
      prep_res->objects->descr_in.descr_t_len ||
      prep_res->objects->descr_out.descr_t_len)
    return -DS_SYNTER;
    
  return SQL_d_interpret (prep_res->stored.comp_data_t_u.segm,
                      COM_START, NULL, NULL);
} /* update */
