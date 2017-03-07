/* 
 *  dyn_funcs.h  - setup global directives to current operating system 
 *                 and program environment 
 * 
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Konstantin Dyshlevoj, 1995
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

#ifndef _DYN_FUNCS_H_
#define _DYN_FUNCS_H_

/* $Id: dyn_funcs.h,v 1.248 1998/09/29 21:25:52 kimelman Exp $ */

#include <gnusql/setup_os.h>

#define DEF_SQLTYPE(CODE,SQLTN,CTN,DYN_ID)  SQL__##CODE = DYN_ID,

typedef enum {
#include "sql_type.def"
  SQL__LAST
} dyn_sql_type_code_t;

typedef struct
{
  int   type; /* type code from SQL standard                 *
               * in user's  descriptor or SQL-server's type  *
               * code in internal  descriptors               */
  int   length;
  int   returned_length;
  int   nullable;
  int   indicator;
  int   unnamed;
  char *name;
  void *data;
  int   data_allocated_size; /* not for user */
} sql_descr_element_t;

typedef struct
{
  int                    count;
  int                    maximum;
  int                    alloc_cnt; /* not for user */
  sql_descr_element_t   *values; 
} sql_descr_t;
  
typedef sql_descr_t *SQL_DESCR;

/****************************** not for user ****************************/
struct s_stmt {
  char *name;
  int   sectnum;
  int   segm;       /* virtual address of made segment for dynamic *
                     * nonselect operators. May be = 0 only for    *
                     * DELETE positioned & cursor specification    */
  int   mem_ptr;    /* for SELECT statement : phisical address in  *
                     * server of segment placed to piece of memory */
  int   mem_size;
  char *table_name; /* this & next fields identify the table   *
                     * if prepared statement - updatable query */
  char *table_owner;
  
  sql_descr_t in_descr;
  sql_descr_t out_descr;
  struct s_cursor *dep_on_cursor; /* points to correspondent cursor if this  *
                                   * statement - UPDATE or DELETE positioned */
  struct s_stmt *next;
};
/***********************************************************************/

SQL_DESCR SQL__allocate_descr    __P((char *descr_name, int maximum));
void      SQL__deallocate_descr  __P((SQL_DESCR *descr_ptr));
SQL_DESCR SQL__get_descr         __P((char *descr_name));

/* all following functions return error code = SQLCODE */

int       SQL__prepare  __P((char *stmt_name, char *stmt_text));
int       SQL__deallocate_prepare  __P((char *stmt_name));

/* In all following fuctions working with descriptors:              *
 * 'using_input' must be correctly filled accordingly description   *
 * of 'stmt_name' input.                                            *
 * Some fields of 'using_output' can be not filled. In this case    *
 * the function will make correspondent result value description.   *
 * In all cases function makes for output-descriptor memory         *
 * allocation for DATA. So we recommend for users of this functions *
 * don't use the same descriptor for input & output description.    */

int       SQL__describe  __P((char *stmt_name, int is_input, SQL_DESCR descr));
/* all values NAME & DATA in descriptor elements are being allocated *
 * here. So user can only read this values & can't use them after    *
 * reprepareing of statement 'stmt_name'                             */

int       SQL__execute  __P((char *stmt_name,SQL_DESCR using_input, SQL_DESCR using_output));
int       SQL__execute_immediate  __P((char *stmt_text));
int       SQL__declare_cursor  __P((char *stmt_name, char *cursor_name));
/* function only associates cursor & statement names: *
 * pointed statement can be not prepared yet          */

int       SQL__allocate_cursor  __P((char *stmt_name, char *cursor_name));
/* associates cursor name with prepared statement */

int       SQL__open_cursor  __P((char *cursor_name, SQL_DESCR using_input));
int       SQL__fetch  __P((char *cursor_name, SQL_DESCR using_output));
int       SQL__close_cursor  __P((char *cursor_name));
int       SQL__delete  __P((char *cursor_name, char *table_name, char *table_owner));
int       SQL__update  __P((char *cursor_name, char *table_name, char *table_owner, char *set_clause));
/* set_clause = "SET ..." */

#endif
