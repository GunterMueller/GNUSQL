/*  fdclsyn.h  - functions declarations of Synchronizer
 *               Kernel of GNU SQL-server. Synchronizer    
 *
 * This file is a part of GNU SQL Server
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

/* $Id: fdclsyn.h,v 1.248 1998/08/21 00:27:43 kimelman Exp $ */

#include "cmpdecl.h"
#include "totdecl.h"

/* dlock.c */
/*       9 */ void dlock(struct des_lock *anl);

/* incrs.c */
/*      14 */ void increase(struct des_tran *tr);

/* lock.c */
/*      17 */
CPNM lock(u2_t trnum, struct id_rel rel, COST cost, char lin, i4_t totsize, char *lc);
/*     162 */
struct des_rel *crtsrd(u2_t trnum, struct id_ob *udr, u2_t pn, u2_t ind);

/* shartest.c */
/*      11 */ i4_t shartest(struct des_lock *anl, i4_t size, char *con, struct des_lock *bl);
/*      96 */ void refrem(struct des_lock *a);

/* shtest1.c */
/*      15 */ i4_t shtest1(struct des_lock *anl, char *con, i4_t size, struct des_lock *bl);

/* snipc.c */
/*      31 */ void main(i4_t argc, char **argv);
/*     136 */ void answer_opusk(u2_t trnum, CPNM cpn);
/*     165 */ char *getpage(u2_t trn, u2_t segn, u2_t pn);
/*     215 */ void putpage(u2_t trn);

/* synpr.c */
/*      18 */ i4_t synstart(u2_t trnum);
/*      47 */ CPNM svpnt_syn(u2_t trnum);
/*      70 */ void unltsp(u2_t trnum, CPNM cpnum);
/*     100 */ void commit(u2_t trnum);
/*     116 */ void error(char *s);

/* unlock.c */
/*      10 */ void unlock(struct des_tran *tr, struct des_lock *alock);
/*     103 */ void lilore(struct des_rel *r, struct des_lock *a, struct des_lock *f, struct des_lock *b);
