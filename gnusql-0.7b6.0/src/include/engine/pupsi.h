/*
 *  pupsi.h  -  External definitions of Storage and Transaction
 *              Synchronization Management System of
 *              GNU SQL server
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

#ifndef __PUPSI_H__
#define __PUPSI_H__

/* $Id: pupsi.h,v 1.245 1997/03/31 03:46:38 kml Exp $ */

#define BD_PAGESIZE        1024

#ifndef GSQL_ROOT_DIR
#  define GSQL_ROOT_DIR "."
#endif 


# define SOC_BUF_SIZE    2*BD_PAGESIZE

enum {
  MERR   =1,
  HERR   =2,

  MJ_PPS =1,
  LJ_PPS =2,
  SN_PPS =3,
  BF_PPS =4,
  TR_PPS =5,
  SR_PPS =6,

  /* scan modes */
  RSC     =1,       /* read                       */
  MSC     =2,       /* modification               */
  DSC     =3,       /* deletion                   */
  WSC     =4,       /* modification and deletion  */

  DBL     =1,       /* duplicates are stayed      */
  NODBL   =0,       /* duplicates are deleted     */
  GROW    =1,       /* grow sorting direction     */
  DECR    =0       /* decrease sorting direction */
};

#endif
