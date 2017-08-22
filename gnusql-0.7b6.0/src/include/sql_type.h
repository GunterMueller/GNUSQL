/*
 *  sql_type.h - SQL types support for GNU SQL compiler
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

/* $Id: sql_type.h,v 1.246 1997/03/31 11:04:06 kml Exp $ */
                
#ifndef __SQL_TYPE_H__
#define __SQL_TYPE_H__

#include "setup_os.h"
#include "dyn_funcs.h"
/*******==================================================*******\
*******            Type descriptions                       *******
\*******==================================================*******/
typedef struct {
        byte  code;
        byte  prec;
        i2_t len;
} sql_type_t;

#define US_NAME_LNG 18

#define DEF_SQLTYPE(CODE,SQLTN,CTN,DYN_ID)  SQLType_##CODE,

typedef enum {
#include "sql_type.def"
  SQLType_LAST
} SQLType;


#define TRANS_STRAIGHT 1 
#define TRANS_BACK     0

sql_type_t          pack_type  __P((SQLType tpname,i2_t lenght, i2_t prec)); 
int                 is_casted  __P((sql_type_t t1,sql_type_t t2));
i4_t                type2long  __P((sql_type_t t));
SQLType             get_sqltype_code  __P((dyn_sql_type_code_t t));
dyn_sql_type_code_t get_dyn_sqltype_code  __P((SQLType t));

#endif
