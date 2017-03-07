/*
 *  fieldtp.h  - Types processed by kernel of GNU SQL-server
 *
 * This file is a part of GNU SQL Server
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

#ifndef __fieldtp_h__
#define __fieldtp_h__

/* $Id: fieldtp.h,v 1.246 1998/09/29 21:26:07 kimelman Exp $ */


enum {
  /* Field types in BD */

  T1B     =1,
  T2B     =2,
  T4B     =3,
  TFL     =4,
  TCH     =5,
  TFLOAT   =6
};

#endif
