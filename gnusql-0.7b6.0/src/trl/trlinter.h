/*
 *  trlinter.h  - internal interface of tree library of GNU SQL compiler
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
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

/* $Id: trlinter.h,v 1.245 1997/03/31 03:46:38 kml Exp $ */

#ifndef __TRLINTER_H__
#define __TRLINTER_H__

#include "trl.h"

/*******==================================================*******\
*******====================================================*******
*******=======           ATTENTION!!!               =======*******
*******=======  Available only inside tree library  =======*******
*******====================================================*******
\*******==================================================*******/

struct tree_root {      /****************************************/
  TXTREF   root;        /* root of the grammar tree             */
  VCBREF   vcbroot;     /* global vocabulary list reference     */
  LTRLREF  author;      /* user name (authorization identifier) */
  TXTREF   hash_tbl;    /* hash table reference                 */
};                      /****************************************/

#ifndef VCB_ROOT
# define VCB_ROOT  *(Root_ptr(Vcb_Root))
#endif

#endif
