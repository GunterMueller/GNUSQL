/*
 *  fdeclbuf.h  - This file contains functions declarations of Buffer
 *                Kernel of GNU SQL-server. Buffer 
 *
 *    The functions deal with three parts of information. Page table
 *  contains pages' descriptors including all information concerning
 *  satisfied locks and presence of pages in the pool of buffers. Hash
 *  table serves a fast search of a page descriptor by the corresponding
 *  page number. Queue table contains all waiting locks.
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
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

#ifndef __fdeclbuf_h__
#define __fdeclbuf_h__

/* $Id: fdeclbuf.h,v 1.248 1998/09/30 02:39:03 kimelman Exp $ */

#include "setup_os.h"
#include <sys/types.h>
#ifdef HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif
#include "totdecl.h"
#include "bufdefs.h"


/* buf.c */
/*      23 */ void set_prio __P((struct BUFF *buf, u2_t new_prio));
/*      52 */ void unset_prio __P((struct BUFF *buf));
/*      71 */ void change_prio __P((struct BUFF *buf, u2_t new_prio));
/*      82 */ struct BUFF *find_buf __P((void));
/*     100 */ struct BUFF *get_buf __P((void));
/*     145 */ void del_buf __P((struct BUFF *buf));
/*     159 */ void push_buf __P((struct BUFF *buf));
/*     176 */ void optimal __P((u2_t num));
/*     191 */ char *get_empty __P((unsigned size));

/* bufipc.c */
/*     135 */ void msg_rcv __P((void));
/*     300 */ void buf_to_user __P((u2_t trnum, key_t keys));
/*     317 */ void user_p __P((u2_t conn, i4_t type));
/*     337 */ void push_micro __P((i4_t addr));
/*     361 */ void pushmicro __P((i4_t addr));
/*     381 */ struct des_seg *new_seg __P((void));
/*     413 */ void del_seg __P((struct des_seg *desseg));
/*     433 */ void inifixb __P((i4_t nop, i4_t ljadd));
/*     473 */ void do_fix __P((void));
/*     493 */ void fix_mode __P((void));
/*     503 */ void end_op __P((void));
/*     511 */ void weak_err __P((char *text));
/*     520 */ void read_buf __P((struct BUFF *buf));
/*     554 */ void write_buf __P((struct BUFF *buf));
/*     587 */ void ADM_ERRFU __P((i4_t p));
/*     602 */ void finit __P((void));
/*     645 */ void wait_free __P((void));
/*     650 */ void waitfor_seg __P((i4_t buf_num));

/* buflock.c */
/*      30 */ void tactup __P((void));
/*      39 */ void tact __P((void));
/*      78 */ i4_t buflock __P((u2_t conn, u2_t segn, u2_t pn,
                           char type, u2_t prget));
/*     119 */ i4_t enforce __P((u2_t conn, u2_t segn, u2_t pn));
/*     146 */ void unlock __P((u2_t segn, u2_t lnum, char *p));

/* get_put.c */
/*      18 */ struct BUFF *get __P((u2_t sn, u2_t pn, char pr));
/*      47 */ void put __P((u2_t sn, u2_t pn, i4_t address, char prmod));

/* page.c */
/*      22 */ void init_hash __P((void));
/*      31 */ u2_t hash __P((u2_t page));
/*      44 */ struct PAGE *new_page __P((u2_t seg_num, u2_t page_num));
/*      77 */ void del_page __P((struct PAGE *page));
/*      99 */ struct PAGE *find_page __P((u2_t n_seg, u2_t n_page));

/* queue.c */
/*      24 */ struct WAIT *new_wait __P((u2_t conn, u2_t type, u2_t tact, u2_t prg));
/*      38 */ void into_wait __P((struct PAGE *page, struct WAIT *wait));
/*      51 */ void out_first __P((struct PAGE *page));
/*      65 */ void set_weak_locks __P((struct PAGE *page));

#endif
