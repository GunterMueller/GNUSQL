/*
 * adfstr.h - the structure of administrative file
 *             Kernel of GNU SQL-server
 *
 * $Id: adfstr.h,v 1.247 1998/09/29 21:26:02 kimelman Exp $
 *
 * This file is a part of GNU SQL Server
 *
 * Copyright (c) 1996-1998, Free Software Foundation, Inc
 * Developed at the Institute of System Programming
 * This file is written by Vera Ponomarenko
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contacts: gss@ispras.ru
 *
 */


struct ADF {  
    i4_t   uid;
    i4_t   uidconst;
    i4_t   unname;
    u2_t   sizext;
    u2_t   maxtrans;
    u2_t   mjred;
    u2_t   ljred;
    u2_t   mjmpage;
    u2_t   ljmpage;
    u2_t   sizesc;
    i4_t   optbufnum;
    i4_t   maxnbuf;
    i4_t   maxtact;
    i4_t   fshmsegn;
  };
