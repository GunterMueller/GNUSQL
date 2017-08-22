/*
 *  gen.c - contains function for tree building
 *
 * This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Software
 *  This file is written by Andrew Yahin.
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

/* $Id: gen.c,v 1.245 1997/03/31 03:46:38 kml Exp $ */

#include "global.h"
#include <stdarg.h>
#include "kitty.h"

TXTREF 
mk_vect(i4_t of_longs,i4_t len,...)
{
  va_list ap;
  TXTREF  v;
  register i4_t i;
  
  va_start(ap,len);
  v = gen_vect(len);
  for (i=0; i<len; i++)
    if (of_longs)
      VOP(v,i).l    = va_arg(ap,i4_t);
    else
      VOP(v,i).txtp = va_arg(ap,TXTREF);
  va_end(ap);
  return v;
}

TXTREF 
mk_node(enum token root,...)
{
  va_list ap;
  TXTREF return_trn;
  register i4_t i;
  enum token tmp_code;
  i4_t right_ptr=0,down_ptr=0;
  register char *format_ptr;
  
  va_start(ap,root);
  
  tmp_code = root;
  
  if (tmp_code == UNKNOWN)
    return 0;
  /* (NIL_CODE) stands for an expression that isn't there.  */
  if (tmp_code == NIL_CODE)
    return 0;
  
  format_ptr = FORMAT (tmp_code);
  
  return_trn = gen_node1(tmp_code,va_arg(ap,MASKTYPE));
  
  for (i = 0; i < LENGTH (tmp_code); i++,format_ptr++)
    switch (*format_ptr)
      {
	/* 0 means a field for internal use only.
	   Don't expect it to be present in the input.  */
      case '0':break;
      case 'r':
	right_ptr=1;
	break;
      case 'a':
	break;
      case 'd':
	down_ptr=1;
	break;
	
      case 'V': /* special vocabulary reference (backward link) */
	break;
      case 't':
      case 'v':
      case 'T':
      case 'L':
      case 'N':
	XTXT_TRN(return_trn, i) = va_arg(ap,TXTREF);
	break;
	
      case 'S':
	/* 'S' is an optional string: if a closeparen follows,
	   just store NULL for this element.  */
	
      case 's':
	XLTR_TRN(return_trn, i) = va_arg(ap,LTRLREF);
	break;
      case 'p':
      case 'y':
      case 'i':
      case 'x':
      case 'l':
      case 'R':
	XLNG_TRN(return_trn, i) = va_arg (ap,i4_t);
	break;
      case 'f':
	XFLT_TRN(return_trn, i) = va_arg (ap,double);
	break;
	
      default:
	fprintf (stderr,
		 "switch format wrong in mk_node(). format was: '%c'.\n",
		 *format_ptr);
	yyfatal("Abort");
      }
  
  if (TstF_TRN (return_trn, PATTERN_F))
    for(;i<LENGTH(tmp_code)+PTRN_ELEM(tmp_code);i++)
      XLNG_TRN(return_trn,i)=va_arg(ap,i4_t);
  
  if(down_ptr)
    {
      DOWN_TRN(return_trn)=va_arg(ap,TXTREF);
      arity_trn(return_trn);
    }
  
  return_trn = handle_types(return_trn,0);
  
  if(right_ptr)
    RIGHT_TRN(return_trn)=va_arg(ap,TXTREF);
  
  va_end(ap);
  return return_trn;
}
