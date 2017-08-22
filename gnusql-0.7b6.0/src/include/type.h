/* 
 *  type.h - contains data types shared by plan generator and interpretator
 *           of GNU SQL server
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Konstantin Dyshlevoj 
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

/* $Id: type.h,v 1.245 1997/03/31 03:46:38 kml Exp $ */

#ifndef __TYPE_H__
#define __TYPE_H__

#include "engine/rnmtp.h" 

/* the following 3 declarations are used for debugging only */
typedef u2_t   Segid;
typedef i4_t    Tid;
typedef i4_t    Unid;

typedef  struct {
  /* untabid - the 1st element of structure  */
		Unid  untabid;/* unique identifier of relation  */
		Segid segid;  /* segment identifier in wich     */
			      /* table is located               */
		Tid   tabd;   /* tid of table   descriptor      */
                              /* in table-catalog of given      */
			      /* segment                        */
		       }   Tabid     ;

typedef  struct {
		Tabid tabid;    /* table identifier             */
		Unid  unindid;  /* indexe number                */
		      }      Indid ;

typedef   i4_t   Filid ;


typedef   i4_t   Scanid ;

#endif
