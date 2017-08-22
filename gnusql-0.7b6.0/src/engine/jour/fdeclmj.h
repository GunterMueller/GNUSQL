/*
 *  fdeclmj.h  - Functions descriptions for helpfu.c, microj.c and mjipc.c
 *               Kernel of GNU SQL-server. Microjournal  
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
 *  Contacts: gss@ispras.ru
 *
 *  $Id: fdeclmj.h,v 1.246 1998/06/01 15:03:34 kimelman Exp $
 */


/* microj.c */
/*      36 */ i4_t INI (char *pnt);
/*      72 */ void putbl (u2_t razm, char *block);
/*     103 */ void dofix (char *pnt);
/*     124 */ void outdisk (u2_t n);
/*     130 */ void write_disk (i4_t i, i4_t c);

/* mjipc.c */
/*     128 */ void LJ_ovflmj (void);
/*     140 */ void ans_buf (void);
/*     165 */ void ans_mj (char rep);
/*     178 */ i4_t ask (void);
/*     186 */ void ADM_ERRFU (i4_t p);
/*     200 */ void finit (char *mj_name);

/*     211 */ void mjini (char *mj_name);

/* helpfu.c */
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

