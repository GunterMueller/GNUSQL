/*
 *  trns_decl.c  - globals for transaction of any type -
 *                 regular or crash recovery
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
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
 *  Contacts:   gss@ispras.ru
 *
 */

/* $Id: trns_decl.c,v 1.1 1998/09/29 00:39:39 kimelman Exp $ */

#include "setup_os.h"
#include "destrn.h"

#define extern 
extern i4_t ljmsize;
extern struct ldesind **TAB_IFAM;
extern i4_t TIFAM_SZ;
extern char **scptab;
extern i4_t minidnt;
extern u2_t trnum;
extern u2_t S_SC_S;
extern i4_t msqidl, msqidm, msqidb;
