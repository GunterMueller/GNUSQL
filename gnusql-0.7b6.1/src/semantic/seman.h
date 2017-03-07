/*   seman.h - common semantic interface for pass 2 of SQL compiler 
 *             GNU SQL server
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
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
 *  Contacts:  gss@ispras.ru
 *
 */

/* $Id: seman.h,v 1.246 1998/09/29 21:26:37 kimelman Exp $ */

#ifndef __SEMAN_H__
#define __SEMAN_H__

#include "sql_decl.h"

#define SEM_PRINT       fprintf
#define SEM_OUTFILE     STDERR

/* abnormally terminates compiling */
#define SEM_EXIT(code)  exit (code);

/********************************************************************/
#define SWARNING      {s_w_error++;SEM_PRINT (SEM_OUTFILE,"\nWarning: ");}
#define SERROR        {s_e_error++;SEM_PRINT (SEM_OUTFILE,"\nError: ");}

#define SFERROR_MSG(msg) {progname=__FILE__; file_pos=__LINE__; yyfatal(msg);}
#define SERROR_MSG(ef,l,msg) \
{ if (ef) s_e_error++; else s_w_error++; file_pos=l; yyerror(msg); }

#define SFATAL_ERROR  SFERROR_MSG("Fatal error: invalid structure of SQL-program")

#define SINT_ERROR    SFERROR_MSG("Internal error: error of SQL-compiler\n")
#define SMEM_ERROR    SFERROR_MSG("Not enough memory")

/********************************************************************/
extern i2_t s_w_error;
extern i2_t s_e_error;
/********************************************************************/
CODERT access_main (TXTREF);
i4_t    binding (void);
CODERT subq_main (TXTREF);
#endif
