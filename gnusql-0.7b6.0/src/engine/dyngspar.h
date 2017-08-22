/*
 *  dyngspar.h -  file of GNU SQL server dynamic parameters definitions
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Vera Ponomarenko.
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

#ifndef __dyngspar_h__
#define __dyngspar_h__

/* $Id: dyngspar.h,v 1.245 1997/03/31 03:46:38 kml Exp $ */

#define D_MAX_FREE_EXTENTS_NUM    64     /* max free extents number in Administrator     */
#define D_LJ_RED_BOUNDARY          2     /* max size of Logical Journal in journal pages */ 
#define D_OPT_BUF_NUM             64     /* optimal buffers number                       */
#define D_MAX_TACT_NUM             4     /* max tacts number used Buffer                 */

#endif
