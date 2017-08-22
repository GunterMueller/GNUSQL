/*
 * procname.h  -   functions naming convention
 *                 GNU SQL-server compiler                          
 *
 *  This file is written by Michael Kimelman,1993
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contacts: gss@ispras.ru
 */

#ifndef __procname_h__
#define __procname_h__

/* $Id: procname.h,v 1.245 1997/03/31 03:46:38 kml Exp $ */

#define CALL_ROLLBACK_str      "_SQL_rollback();"
#define CALL_COMMIT_str        "_SQL_commit();"

#define COMMON_FUNC_str        "_SQL_func_call_no_"

#endif
