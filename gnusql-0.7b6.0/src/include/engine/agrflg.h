/* 
 *  agrflg.h -  Aggregate function masks
 *                
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Vera Ponomarenko
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

/* $Id: agrflg.h,v 1.245 1997/03/31 03:46:38 kml Exp $ */

#ifndef __agrflg_H__
#define __agrflg_H__

enum {
FN_COUNT   = 0x1,
FN_AVG     = 0x2,
FN_MAX     = 0x3,
FN_MIN     = 0x4,
FN_SUMM    = 0x5,
FN_DT_COUNT= 0x6,
FN_DT_AVG  = 0x7,
FN_DT_SUMM = 0xa
};

#endif
