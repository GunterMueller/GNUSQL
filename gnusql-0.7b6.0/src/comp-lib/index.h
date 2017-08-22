/*
 *  index.h
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
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

#ifndef __index_h__
#define __index_h__

/* $Id: index.h,v 1.245 1997/03/31 03:46:38 kml Exp $ */

extern Indid indid1,
             indid2,
             indidcol,
             indidcol2,
             sysindexind,
             sysauthindid,
             syscolauthindid,
             chconstrindid,
             chconstrtwoind,
             sysrefindid,
             sysrefindid1,
             viewsind
              ;   
/*-----------------------------------------*/
#define SYSTABLESIND1ID    &indid1
#define SYSTABLESIND2ID    &indid2
#define SYSCOLUMNSIND1ID   &indidcol
#define SYSCOLUMNSIND2ID   &indidcol2
#define SYSINDEXIND        &sysindexind
#define SYSTABAUTHIND1ID   &sysauthindid
#define SYSCOLAUTHIND1ID   &syscolauthindid
#define SYSREFCONSTRINDEX  &sysrefindid
#define SYSREFCONSTRIND1   &sysrefindid1
#define SYSCHCONSTRINDEX   &chconstrindid
#define SYSCHCONSTRTWOIND  &chconstrtwoind
#define SYSVIEWSIND        &viewsind

#endif
