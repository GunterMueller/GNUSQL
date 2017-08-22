/*
 *  sql_decl.c  -  GNU SQL compiler communication area
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

/* $Id: sql_decl.c,v 1.245 1997/03/31 03:46:38 kml Exp $ */

#include "xmem.h"
#include <string.h>
#include "sql_decl.h"
#include "trl.h"

VCBREF local_vcb = TNULL;
i4_t    errors    = 0;
i4_t    under_dbg = 0;
char  *progname  = NULL;
char  *sql_prg   = NULL;
i4_t    file_pos  = 0;
FILE  *STDERR    = stderr;
FILE  *STDOUT    = stdout;
FILE  *sc_file   = NULL;

#define DEF_OPTION(o_nm,var_nm,is_settled)	i4_t var_nm=is_settled;
#define DEF_OPTION1(o_hd,o_proc)	        i4_t o_proc##_state=0;

#include "options.def"
