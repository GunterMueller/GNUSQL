/*
 *  opt.h  - interface of functionalisation part of GNU SQL compiler
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Michael Kimelman.
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

/* $Id: opt.h,v 1.246 1998/09/29 21:26:24 kimelman Exp $ */

#ifndef __OPT_H__
#define __OPT_H__

#include <stdio.h>
#include "cycler.h"

TXTREF group_by_proc(TXTREF node,i4_t flag);
TXTREF sorting_proc (TXTREF res,i4_t f);
TXTREF process_insert(TXTREF insert_stmt);
TXTREF process_update(TXTREF upd);
TXTREF make_selection(TXTREF scan); /* produce selection list for given scan */
TXTREF make_scan( TXTREF list,TXTREF qexpr);
TXTREF query_via_tmp(TXTREF selection,TXTREF qexpr);
TXTREF get_up_nested_table(TXTREF node,i4_t flag);
void   adjust_parameter (TXTREF p, TXTREF c);


#endif
