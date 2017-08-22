/*
 *  sql.h  -  interface of GNU SQL run-time library    
 *                
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
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
 *  Contacs: gss@ispras.ru
 *
 */

#ifndef __SQL_H__
#define __SQL_H__

#include <gnusql/setup_os.h>
#include <assert.h>

#define DEF_ERR(CODE,ERRstr)  CODE,
typedef enum
{
#include "errors.h"
  ERR_LAST
} ERRORS;

#undef DEF_ERR


struct module {
  char   *db_host;
  void   *svc;
  i4_t   segment;
  char   *modname;
};

struct sqlca
{
  int   sqlcode;
  char *errmsg;
};

extern struct sqlca gsqlca;

typedef struct
{
  i4_t       type;
  i4_t       length;
  /*  i4_t       scale; */ /* to map Cobol & PL/1 data types */
  i4_t       flags;
  void      *valptr;
  i4_t      *indptr;
} gsql_parm;

typedef struct
{
  i4_t       count;
  i4_t       sectnum;
  i4_t       command;
  i4_t       options;
  gsql_parm *prm;
} gsql_parms;

#define _SQLCODE SQLCODE
#define SQLCODE gsqlca.sqlcode

/* some functions from other/sql_types.c : for client and server parts */


int  SQL__disconnect __P((int commit));
void SQL__runtime  __P((struct module *mdl,gsql_parms *args));
int  SQL__cache __P((int new_cache_limit));


#define _SQL_commit()   SQL__disconnect(-2)
#define _SQL_rollback() SQL__disconnect(0)

#endif
