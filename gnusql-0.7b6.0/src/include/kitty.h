/*
 *  kitty.h  - description of support library for generated functions
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Andrew Yahin
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
 */

/* $Id: kitty.h,v 1.245 1997/03/31 03:46:38 kml Exp $ */

#ifndef __KITTY_H__
#define __KITTY_H__

#include "global.h"

#define OPs_IND(x)       ( MASK_TRN(x) & ((1<<VCB_F)-1))-1
#define RULE_NAME(s)     STRING(XLTR_TRN(s,0))
#define RULE_PATTERN(r)  XTXT_TRN(r,5)
#define RULE_STR(s)      XLTR_TRN(s,6)


typedef i4_t (*Rule1_type) __P((TXTREF n,i4_t sel));


TXTREF mk_node __P((enum token root,...));
TXTREF mk_vect __P((i4_t of_longs,i4_t len,...));

i4_t    t_compare  __P((TXTREF x,TXTREF y,TXTREF *Ops /*[15]*/ ,Rule1_type Rule1));
void   arity_trn __P((TXTREF oper));
TXTREF rl_corrector __P((i4_t line,TXTREF node,TXTREF right_link));
TXTREF dl_corrector __P((i4_t line,TXTREF node,TXTREF addit_down_link));
TXTREF del_op __P(( TXTREF o,TXTREF p));
TXTREF replace_node __P((TXTREF s,TXTREF d,TXTREF p));
TXTREF image  __P((TXTREF p,TXTREF dest,TXTREF src));
TXTREF insert_node __P((i4_t before,TXTREF p,TXTREF dest,TXTREF src));
TXTREF copy_tree  __P((TXTREF src));

void   emit_trn_constructor __P((TXTREF s,i4_t look_to_the_right));

/* the following function are made for freeing patterns' memory */
void    register_pattern __P((TXTREF *pattern));
TXTREF *pattern_name __P((char *p_n));

#define free_patterns() register_pattern(NULL)
#define free_pat_mem()  register_pattern(NULL)

#define PATTERN(var,tree)     (var=(var?var:(register_pattern(&var),(tree))))
#define ANONYM_P(name,tree)   INIT_PATTERN(*pattern_name(name),tree)

#define SIMPLE2L(s)           ((i4_t)(s))
                         
#endif
