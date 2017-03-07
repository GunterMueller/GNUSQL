/*  puprcv.h  - Directory names for Recovery utilities
 *              Kernel of GNU SQL-server. Recovery utilities    
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by  Vera Ponomarenko
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
 *  Contacts:   gss@ispras.ru
 *
 */

/* $Id: puprcv.h,v 1.246 1998/09/29 21:25:11 kimelman Exp $ */

#include "pupsi.h"
#ifndef DIR_SQLSER
  #define DIR_SQLSER     GSQL_ROOT_DIR 
#endif
#define DIR_REP_SEGS   DIR_SQLSER "/repository/segs"
#define DIR_REP_LJS    DIR_SQLSER "/repository/ljs"
#define DIR_SEGS       DIR_SQLSER "/segs"
#define DIR_JOUR       DIR_SQLSER "/jrnls"

#ifndef DIR_DUB_SEGS
  #define DIR_DUB_SEGS   DIR_SQLSER "/repository/dubsegs"
#endif
#ifndef ARCHIVE
  #define ARCHIVE        DIR_SQLSER "/jarchive"
#endif

#define CP             "/bin/cp"










