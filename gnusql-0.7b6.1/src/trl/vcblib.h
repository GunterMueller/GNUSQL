/*
 * vcblib.h - interface of tree library of GNU SQL compiler
 *            Vocabulary support.
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming, 
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


#ifndef __VCBLIB_H__
#define __VCBLIB_H__

/* $Id: vcblib.h,v 1.246 1998/09/29 21:26:54 kimelman Exp $ */
#include "trl.h"

/*------------------------------------------------------------------------\
| Compiler vocabulary stores information which can be multiple  refered   |
| from the intermediate reprezentation tree. Generally speaking  vocabu-  |
| lary divided to the 2 part - Global vocabulary and local vocabularies.  |
|                                                                         |
| Global      --  information, which can be shared by all program state-  |
| vocabulary      ment, f.i. exported cursor names, persistent DB tables  |
|                 structures, DB VIEW information and, may be, some info  |
|                 about 'host variables' which should be  defined  as  a  |
|                 global in according with SQL standard.                  |
|                                                                         |
| Local       --  statement dependent information: scan (table instance), |
| vocabulary      parameters info ( 'host variables'  sturctures  if  it  |
|                 wasn't declared as global),                             |
|                                                                         |
|   Both Global and local vocabularies represented by 1-directional list  |
| and search of information will be executed in the  order  local-global  |
| (if it worth to find in both dictionaries of course). Column information|
| stored as a one-directional list under appropriate table or scan node.  |
|                                                                         |
\------------------------------------------------------------------------*/
VCBREF  find_info __P(( enum token code,    /* search parameters, scans and*/
                   LTRLREF l));             /* so on by only 1 name        */
VCBREF  find_table __P((enum token code,    /* search table and view by    */
                   LTRLREF l,LTRLREF l1));  /* 2 name identifier ( a.b )   */
CODERT  add_info __P((VCBREF v));           /*                             */
CODERT  del_info __P((VCBREF v));           /*                             */

/*
 * try to add info and get back the stored info reference, which could 
 * differs from given one if the same info is already stored 
 */
VCBREF  add_info_l __P((VCBREF v));  

/* search column of given table by name or by number */
VCBREF  find_column __P((TXTREF tbl,LTRLREF l));
VCBREF  find_column_by_number __P((TXTREF tbl,i4_t n));

CODERT  add_column __P((TXTREF tbl,LTRLREF l)); /* add column information      */
VCBREF  find_entry __P((VCBREF v));             /* search for equivalent       */
                                                /* vocabulary entry            */
void  check_scan_cols __P((TXTREF tblptr));
void  sort_list_by_col_no __P((TXTREF *list,TXTREF *l2, TXTREF *l3,
			  i4_t is_scols,char *msg2, char *msg3));
sql_type_t *node_type __P((TXTREF node));
i4_t         count_list __P((TXTREF list));

#endif

