/*
 *  f1f2decl.h  - Function declarations of Transaction library
 *                Kernel of GNU SQL-server 
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
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

#ifndef __f1f2decl_h__
#define __f1f2decl_h__

/* $Id: f1f2decl.h,v 1.247 1997/07/20 16:55:38 vera Exp $ */

#include "totdecl.h"
#include "cmpdecl.h"
#include "destrn.h"

/* libtran.c */
u2_t scscal __P((char *a));
i4_t    CCS __P((char *asp));
i4_t tab_difam __P((u2_t sn));
struct d_sc_i *  rel_scan  __P((struct id_ob *fullrn, char * ob, i2_t * n,
                           u2_t fnum, u2_t * fl, char * sc,
                           u2_t slsz, u2_t fmnum, u2_t * fml));
void sct __P((char **a, i4_t st, unsigned char t));
unsigned char selsc1 __P((char **a, i4_t st));
void delscan __P((i2_t scnum));
struct d_r_t *crtrd  __P((struct id_rel *idrel, char * a));
void crtid __P((struct ldesind *desind, struct d_r_t *desrel));
void dfunpack  __P((struct d_r_t *desrel, u2_t size, char *pnt));
void drbdunpack __P((struct d_r_bd * drbd, char *pnt));
char *lusc  __P((i2_t * num, u2_t size, char *aob, i4_t type, i4_t mode,
                 u2_t fn, u2_t * fl, char *selc, u2_t selsize, u2_t fmn, u2_t * fml, u2_t dsize));
i2_t lunt  __P((char ***tab, i2_t * maxn, i2_t delta));

#endif
