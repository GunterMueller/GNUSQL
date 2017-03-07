/*
 *  fdecllj.h  - function declarations for helpfu.c, ljipc.c and logj.c
 *               Kernel of GNU SQL-server. Logical Journal  
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
 *  Contacts:  gss@ispras.ru
 *
 *  $Id: fdecllj.h,v 1.248 1998/09/30 02:39:45 kimelman Exp $
 */

#ifndef __fdecllj_h__
#define __fdecllj_h__

#include "setup_os.h"

/* logj.c */
/*      36 */ void INI (void);
/*      53 */ i4_t renew (void);
/*      73 */ void begfix (void);
/*      87 */ void PUTRC (u2_t razm, char *block);
/*     119 */ void putrec (u2_t razm, char *block);
/*     127 */ void putout (u2_t razm, char *block);
/*     135 */ void write_disk (i4_t i, i4_t c);
/*     150 */ void overflow_mj (void);

/* ljipc.c */
/*     133 */ i4_t BUF_INIFIXB (struct ADBL ADR, i4_t nop);
/*     158 */ void ADML_COPY (void);
/*     169 */ void ADM_ERRFU (i4_t p);
/*     183 */ void ans_adm (i4_t rep);
/*     196 */ void ans_trn (u2_t trnum, i4_t tpop);
/*     253 */ void push_microj ();

/* helpfu.c */
/*      ?? */ char **helpfu_init(void);
/*      25 */ void getrfile (i4_t fd);
/*      35 */ void get_last_page (i4_t fd);
/*      59 */ i4_t get_page (i4_t j, u2_t N, i4_t fd);
/*      76 */ void get_rec (char *pnt, struct ADBL adj, i4_t fd);
/*     122 */ void MOREFILE (i4_t n);
/*     130 */ void PICTURE (i4_t n);
/*     144 */ void out_push (i4_t i, i4_t c);
/*     159 */ void WAIT (i4_t i);
/*     166 */ void READPG (u2_t N, i4_t i, i4_t fd);
/*     183 */ void WRITEPG (i4_t i, i4_t c, i4_t fd);
/*     204 */ char *write_topblock (u2_t size, u2_t off, char *a);
/*     227 */ void do_cont (void);


#include "totdecl.h"  /* t4bpack etc */

#endif
