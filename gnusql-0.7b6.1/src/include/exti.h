/*
 *  exti.h  - contains descriptions of variables for work with system
 *            tables and indexes
 *
 * This file is a part of GNU SQL Server
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
 *  Contacts: gss@ispras.ru
 */

/* $Id: exti.h,v 1.246 1998/09/29 21:25:53 kimelman Exp $ */

#ifndef __exti_h__
#define __exti_h__

#include "type.h"

#ifdef   MAIN
#define  EXTERNAL
#else
#define  EXTERNAL  extern
#endif

/*----- for system tables and indices ------------------*/
EXTERNAL Indid   indid1,indid2,indidcol,indidcol2,sysindexind,
                 sysauthindid, indid, syscolauthindid,
                 chconstrindid, sysrefindid, sysrefindid1,
                 chconstrtwoind, viewsind;

EXTERNAL Tabid   tabid, systabtabid,syscoltabid,sysindtabid,
                 viewstabid,tabauthtabid,colauthtabid,
                 refconstrtabid,chconstrtabid,chcontwotabid ;
/*-----------------------------------------------------*/

#endif
