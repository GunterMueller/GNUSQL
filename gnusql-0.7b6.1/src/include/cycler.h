/* 
 *  cycler.h  - interface of cycle processors library of
 *              GNU SQL compiler  
 *
 * This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Michael Kimelman
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

/* $Id: cycler.h,v 1.246 1998/09/29 21:25:51 kimelman Exp $ */

#ifndef __CYCLER_H__
#define __CYCLER_H__

#include "global.h"
extern i4_t    cycler_skip_subtree;

#define LPROC(lv,RNODE) for(lv=RNODE;lv;lv=RIGHT_TRN(lv))

typedef TXTREF  (*PROC) __P((TXTREF r,i4_t flags));
TXTREF  cycler __P((TXTREF root,PROC exec,i4_t flags));
/*
  i4_t flags; - mask of bits determining the order and conditions of processing
  
    0 -  processing arguments (before step down)
    1 -  processing on Left-Down  way
    2 -  processing on Down-Right way 
    3 -  processing on Right-Left way 
    4 -  'root' is the left end of line
    5 -  correct right link
    6 -  it's vocabulary node - for internal purpose of cycler only
*/

#define CYCLER_OPER  1
#define CYCLER_LD    2
#define CYCLER_DR    4
#define CYCLER_RL    8
#define CYCLER_LN    0x10
#define CYCLER_RLC   0x20
#define CYCLER_VCB   0x40

#define CYC(fl)  CYCLER_##fl

/*************************************************************************\
**   STACK's macro declarations                                          **
\*************************************************************************/

#define STACK_PORTION  100

struct Stack_portion
{
  i4_t                   used;
  struct Stack_portion *nxt;
  void                 *inf;
};

  void st_push __P((struct Stack_portion **hdr,void *ptr,i4_t size));
  void st_pop __P((struct Stack_portion **hdr,void *ptr,i4_t size,char *f,i4_t l));
  i4_t  st_depth __P((struct Stack_portion *hd));
  void *st_get __P((struct Stack_portion *hd,i4_t size,i4_t shift,char *f,i4_t l));

#define DECL_STACK(SN,VTYPE) \
    typedef VTYPE SN##_EL_TYPE;\
    static struct Stack_portion *SN##_stack=NULL;\
    static SN##_EL_TYPE SN##_buffer;


#define PUSHS(SN,VALUE) { SN##_buffer=VALUE;\
    st_push(&SN##_stack,&SN##_buffer,sizeof(SN##_buffer));}

#define POPS(SN,LVALUE) \
  { st_pop(&SN##_stack,&SN##_buffer,sizeof(SN##_buffer),__FILE__,__LINE__);\
    (LVALUE) = SN##_buffer;}

/* st_pop(&SN##_stack,&LVALUE,sizeof(SN##_buffer),__FILE__,__LINE__); */

#define IS_ST_EPMTY(SN)         (SN##_stack==NULL)

#define STACK_DEPTH(SN,lv)      lv=st_depth(SN##_stack)

#define GET_STACK(SN,shift,lv) lv=*((SN##_EL_TYPE*)\
    st_get(SN##_stack,sizeof(SN##_buffer),shift,__FILE__,__LINE__))

#define SPTR(SN,shift)          ((SN##_EL_TYPE*)\
    st_get(SN##_stack,sizeof(SN##_buffer),shift,__FILE__,__LINE__))

#define TOP_STACK(SN) *((SN##_EL_TYPE*)\
    st_get(SN##_stack,sizeof(SN##_buffer),0,__FILE__,__LINE__))

#define TOP_ST                  TOP_STACK(SNAME)
#define IS_ST_EMP               IS_ST_EPMTY(SNAME)
#define DECL_ST(VTYPE)          DECL_STACK(SNAME,VTYPE)
#define PUSH(VALUE)             PUSHS(SNAME,VALUE)
#define POP(LVALUE)             POPS(SNAME,LVALUE)
#define ST_DEPTH(lv)            STACK_DEPTH(SNAME,lv)
#define GET_ST(shift,lv)        GET_STACK(SNAME,shift,lv)

#endif  /* __CYCLER_H__ */
