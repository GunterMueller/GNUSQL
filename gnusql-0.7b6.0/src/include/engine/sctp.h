/*
 *  sctp.h  -  Condition Scale Types
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

#ifndef __sctp_H__
#define __sctp_H__

/* $Id: sctp.h,v 1.245 1997/03/31 03:46:38 kml Exp $ */

typedef enum {
  EQ =    1      /* = */,
  NEQ=    2      /* != */,
  SML=    3      /* < */,
  SMLEQ=  4      /* <= */,
  GRT  =  5      /* > */,
  GRTEQ=  6      /* >= */,
  SS   =  7      /* << */,
  SES  =  8      /* <=< */,
  SSE  =  9      /* <<= */,
  SESE = 10      /* <=<= */,
  EQUN = 11      /* = undefined value */,
  NEQUN= 12      /* != undefined value */,
  ANY  = 13,
  ENDSC= 15      /* end of scale */
} scale_t;

#endif
