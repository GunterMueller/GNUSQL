/*
 *  gspstr.h -  structures of tuning parameters of GNU SQL server
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
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

#ifndef __gspstr_h___
#define __gspstr_h___

/* $Id: gspstr.h,v 1.246 1998/09/29 21:24:58 kimelman Exp $ */

#include "admdef.h"

#define DYNPARS         DBAREA "/dynparam"

struct GSPARAM {  
  i4_t    extent_size;         /* temporary segment extent size in BD pages   */
  i4_t    max_extents_num;     /* max extents number in Administrator         */
  i4_t    max_free_extents_num;/* max free extents number in Administrator    */
  i4_t    max_transactions_num;/* max transactions number in Server           */
  i4_t    mj_red_boundary;     /* max size of Microjournal in journal pages   */
  i4_t    lj_red_boundary;     /* max size of Logical Journal in journal pages*/
  i4_t    mj_add_pages;        /* ahead extention of Microjournal in journal  */
                              /*                                       pages */
  i4_t    lj_add_pages;        /* ahead extention of Logical Journal in       */
                              /*                               journal pages */
  i4_t    opt_buf_num;         /* optimal buffers number                      */
  i4_t    max_buf_num;         /* max buffers number                          */
  i4_t    max_tact_num;        /* max tacts number used Buffer                */
  i4_t   first_key_shm;       /* the first numeric name of the shared memory */
                              /*    segments for Server                      */
};

struct DGSPARAM {      /* structure of dynamic parameters                    */
  i4_t d_max_free_extents_num;/* max free extents number in Administrator     */
  i4_t d_lj_red_boundary;     /* max size of Logical Journal in journal pages */
  i4_t d_opt_buf_num;         /* optimal buffers number                       */
  i4_t d_max_tact_num;        /* max tacts number used Buffer                 */
};

#endif
