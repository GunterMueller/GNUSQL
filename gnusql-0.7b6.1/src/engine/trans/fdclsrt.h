/*  fdclsrt.h - functions declarations of Sorter
 *              Kernel of GNU SQL-server. Sorter   
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
 *  Contacts:   gss@ispras.ru
 *
 */

/* $Id: fdclsrt.h,v 1.248 1998/09/29 21:25:30 kimelman Exp $ */

#include "f1f2decl.h"
/* cmpkey.c */
i4_t cmp_key(struct des_sort *sdes, char *pk1, char *pk2);

/* extsrt.c */
struct el_tree *extsort(i4_t M, struct des_sort *sdes,
                        struct el_tree *tree);

/* push.c */
void push(void (*putr1 )(), struct el_tree *tree, struct el_tree *q,
          struct des_sort *sdes, i4_t nbnum);
i4_t geteltr(i4_t nbnum, struct el_tree *q, i4_t i);
i4_t next_el_tree (i4_t nbnum, struct el_tree *q, i4_t i, char *pkr);

/* puts.c */
void putkr(char *pkr);
void putcrt(char *pkr);
void puttid(char *pkr);
void middle_put_tid (char *pkr);
void getptmp(void);

/* quicksor.c */
void quicksort(i4_t M, struct des_sort *sdes);

/* rkfrm.c */
void rkfrm(char *cort, u2_t pn, u2_t ind, struct des_sort *sdes,
           i4_t M, struct fun_desc_fields *desf);
void putkf(void);
char *remval(char *aval, char **a, u2_t type);

/* sort.c */
u2_t srt_trsort(u2_t *fpn, struct fun_desc_fields *desf,
                struct des_sort *sdes);
u2_t srt_flsort(u2_t segn, u2_t *fpn, struct fun_desc_fields *desf,
                struct des_sort *sdes);
u2_t tidsort(u2_t *fpn);
void addext(void);

/* tidsrt.c */
void quick_sort_tid(i4_t M);
void puts_tid(void);
struct el_tree *ext_sort_tid(i4_t M, struct el_tree *tree);
void push_tid(void (*pnt_puttid )(), struct el_tree *tree,
              struct el_tree *q, i4_t nbnum);
i4_t get_el_tr_tid(i4_t nbnum, struct el_tree *q, i4_t i);
void put_tid(char *pkr);
i4_t cmp_tid(char *pnt1, char *pnt2);
