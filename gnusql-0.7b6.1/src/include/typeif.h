/*
 *  typeif.h  -  types, shared by interpretator and engine interface library
 *               of GNU SQL server
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Olga Dmitrieva
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

/* $Id: typeif.h,v 1.246 1998/09/29 21:26:00 kimelman Exp $ */

#ifndef __TYPEIF__
#define __TYPEIF__

#include "pupsi.h"

#define MAX_STR_LNG BD_PAGESIZE-256
#define MAX_USER_NAME_LNG 20

typedef struct 
{
  unsigned f4:4;
  unsigned f3:4;
  unsigned f2:4;
  unsigned f1:4;
}    Scale    ;

#include "sctp.h"

#define ENDSC4  0xF0     /* ENDSC<<4 */
#define ANY4    0xD0     /* ANY<<4 */

/*
  Selection condition has to include one element of scale
  for every field; if condition absence for some field
  then corresponding element of scale is equal ANY
  
struct Cond
{
  Scale  scale[n+1]; * array of four bit fields            *
                     * determining the compare operations  *
  char   values[ ] ; * constant's values        *
}

*/

typedef char *cond_buf_t; /* Actually pointer to buffer of conditions written
                             in the format decsribed above */

typedef  void  **Colval;

#endif




