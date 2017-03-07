/*
 *  fdclrcv.h  -  recovery functions interface 
 *              Kernel of GNU SQL-server. 
 *
 * This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  Generated automatically by mkptypes.
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


#ifndef __fdclrcv_h__
#define __fdclrcv_h__

/* $Id: fdclrcv.h,v 1.246 1998/09/29 21:25:09 kimelman Exp $ */


#include "f1f2decl.h"
#include "fdcltrn.h"

/* librcv.c */
/*      96 */ struct ADBL frwdmtn();
/*     201 */ u2_t LJ_next (i4_t fd, struct ADBL *adj, char **pnt, char *mas);
/*     256 */ void getmint(char *a, u2_t n);
/*     326 */ void r_tr(struct ADBL cadlj);
/*     348 */ struct ADBL bmtn_feot(i4_t fd, struct ADBL cadlj);
/*     389 */ void backactn(u2_t blsz, char *a, char type);
/*     587 */ u2_t LJ_prev(i4_t fd, struct ADBL *adj, char **pnt, char *mas);
/*     645 */ void rcv_wmlj(struct ADBL *cadlj); 
/*     738 */ void rcv_LJ_GETREC(struct ADREC *bllj, struct ADBL *pcadlj);
/*     766 */ i4_t dir_copy(char *dir_from, char *dir_to);
/*     791 */ void copy(char *name, char *dir_from, char *dir_to);
/*     820 */ i4_t LOGJ_FIX(void);
/*     838 */ void read_page(i4_t fd, u2_t N, char *buf);
/*     935 */ void dubl_segs(void);
/*     264 */ void forward_action (char *a, char type, u2_t n);
/*     834 */ void rcv_ini_lj(void);
#endif
