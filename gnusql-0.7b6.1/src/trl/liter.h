/*
 *  liter.h  - interface of literal support library of GNU SQL compiler
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming, Russia
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
 *  Contact:  gss@ispras.ru
 *
 */

/* $Id: liter.h,v 1.246 1998/09/29 21:26:46 kimelman Exp $ */

#ifndef __LITER_H__
#define __LITER_H__

#include "vmemory.h"

typedef VADR  LTRLREF;

/* creating literal entry in hash table */
LTRLREF  ltr_rec           __P((char *  liter));
char    *ltrlref_to_string __P((LTRLREF l    ));
void     free_hash         __P((void         ));

#define STRING(l) ltrlref_to_string(l)

#endif
	
