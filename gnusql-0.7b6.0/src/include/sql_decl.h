/*
 *  sql_decl.h  - GNU SQL compiler communication area
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
 */

/* $Id: sql_decl.h,v 1.245 1997/03/31 03:46:38 kml Exp $ */
                
#ifndef __SQL_DECL_H__
#define __SQL_DECL_H__

#include "setup_os.h"
#include <errno.h>
#include "trl.h"

extern VCBREF    local_vcb       ;

#define LOCAL_VCB_ROOT   local_vcb

extern char *progname;
extern i4_t   errors;
extern i4_t   under_dbg;
extern char *sql_prg;
extern i4_t   file_pos;
extern FILE *STDERR;
extern FILE *STDOUT;
extern FILE *sc_file;

/* options declaration */
typedef void (*Opt_processor) __P((i4_t *state,char *tail));
typedef i4_t  (*Comp_pass) __P((void));
typedef void (*Dumper) __P((FILE *f));

/* declarations for type compatibility */
#define DEF_OPTION(o_nm,var_nm,is_settled) \
     extern i4_t var_nm;
#define DEF_OPTION1(o_hd,o_proc) \
     void o_proc __P((i4_t *flag,char *tail));\
     extern i4_t o_proc##_state;
#define DEF_PASS(name,title,id,proc,st_id,dmp,skip,ext,dumper) \
     i4_t    proc __P((void));\
     void   dumper __P((FILE *f));

#include "options.def"

extern struct compiler_pass_descr 
{
  char     *sname,
           *sdescr,
            atr;
  Comp_pass pass;
  i4_t       skip,
            dump;
  char     *dumpext;
  FILE     *dmp_file;
  Dumper    dumper;
  i4_t       stage;
} compiler_passes[];

extern struct sql_options_info
{
  char          *name;
  i4_t           *ptr_to_var;
  Opt_processor  proc;  
} sql_options[];

void read_options __P((i4_t argc,char *argv[]));

void dump_modes __P((i4_t *p,char *s));
void skip_modes __P((i4_t *p,char *s));

#endif

