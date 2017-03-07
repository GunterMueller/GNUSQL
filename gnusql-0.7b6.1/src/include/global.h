/*
 *  global.h  - general purpose main include file of GNU SQL compiler
 *
 * This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
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

/* $Id: global.h,v 1.247 1998/09/29 21:25:54 kimelman Exp $ */

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include "setup_os.h"

#include <string.h>


#ifndef TRUE
#  define TRUE  (1)
#endif
#ifndef FALSE
#  define FALSE (0)
#endif

/*******==================================================*******\
*******            Type descriptions                       *******
\*******==================================================*******/
#include "sql_type.h"

/*******==================================================*******\
*******           Engine descriptions                      *******
\*******==================================================*******/
#include "type.h"

/*******************************************************************\
*                     XMEM declaration                              *
\*******************************************************************/
#include "xmem.h"

/*******************************************************************\
*            Tree languge definition                                *
\*******************************************************************/
#ifndef __UNLINK_TRL__
#  include "trl.h"
#endif

/*******************************************************************\
*           Bison and Flex declaration                              *
\*******************************************************************/

typedef struct yyltype
    {
      i4_t timestamp;
      i4_t first_line;
      i4_t first_column;
      i4_t last_line;
      i4_t last_column;
      char *text;
   }
  yyltype;

#define YYLTYPE yyltype
extern YYLTYPE  yylloc;

/*******************************************************************\
*                 FINAL stage declaration                           *
\*******************************************************************/

/* include synthesys pass interface & macrodefinitions */
#include "pr_glob.h"

/*******************************************************************\
*          Common compiler declaration (communication zone)         *
\*******************************************************************/
#include "sql_decl.h"

/*******************************************************************\
*                  transaction server interface                     *
\*******************************************************************/
#include "funall.h"

/*******************************************************************\
*           Passes declaration                                      *
\*******************************************************************/
int     yyparse __P((void));
i4_t     monitor __P((void));
TXTREF  handle_types  __P((TXTREF cur_node, i4_t f));
VADR    codegen __P((void));

#endif
