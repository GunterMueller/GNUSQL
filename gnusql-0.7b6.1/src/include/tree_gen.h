/* 
 * tree_gen.h  - set of tree constructors (especially usefull during 
 *               parsing) of GNU SQL compiler               
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Michael Kimelman, 1992 
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

/* $Id: tree_gen.h,v 1.246 1998/09/29 21:25:58 kimelman Exp $ */


#ifndef __TREE_GEN_H__
#define __TREE_GEN_H__

#define __USE_PARSER__

#include "global.h"

typedef struct tblname{
  LTRLREF          a,n;
  YYLTYPE          p;
} TN;

void    set_location __P((TXTREF n,YYLTYPE p));
TXTREF  join_list __P((TXTREF lst,TXTREF e));
TXTREF  get_param __P((LTRLREF l,i4_t enableadd,YYLTYPE p));
TXTREF  get_parm_node __P((LTRLREF l,YYLTYPE p,i4_t enableadd));
TXTREF  get_ind __P((TXTREF n,TXTREF n1,YYLTYPE p));
TXTREF  gen_column __P((TN *t,LTRLREF l,YYLTYPE p));
TXTREF  gen_colptr __P((VCBREF col));
TXTREF  gen_func __P((enum token code,YYLTYPE p,TXTREF obj,i4_t set_dist));
TXTREF  gen_oper __P((enum token code,TXTREF l,TXTREF r,YYLTYPE p));
TXTREF  gen_parent __P((enum token code,TXTREF list));
TXTREF  gen_up_node __P((enum token code,TXTREF child,YYLTYPE p));
TXTREF  gen_object __P((enum token code,YYLTYPE p));
TXTREF  gen_scanptr __P((TXTREF tbl,LTRLREF nm,YYLTYPE p));
TXTREF  coltar_column __P((TXTREF cc));

char   *get_param_list __P((TXTREF vcb,i4_t usetype));

TXTREF  last_parameter __P((TXTREF parent));
void    add_child __P((TXTREF parent,TXTREF child));

#endif
