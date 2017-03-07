/*
 *  trl_macro.h  - macrodefinition version of tree processing funcvtions
 *                 GNU SQL compiler
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming, Russia
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
 *  Contact:  gss@ispras.ru
 *
 */

/* $Id: trl_macro.h,v 1.246 1998/09/29 21:26:52 kimelman Exp $ */

#ifndef __TRL_macro_H__
#define __TRL_macro_H__

#define Ptree(n)  ((trn)Pnode(n))

/********* direct node address ******************************************/
#ifdef CHECK_TRL
  trn   test_exist __P((trn node,i4_t n,char *f,i4_t l));
  trn   test_node __P((trn node,char *f,i4_t l));
  i4_t   tstv_exist __P((trvec vec,i4_t n,char *f,i4_t l));
  trvec tstv_vec __P((trvec vec,char *f,i4_t l));
  tr_union* xOp_parm  __P((trn node,i4_t n,char  c,char *f,i4_t l));
  tr_union* xOp_parms __P((trn node,i4_t n,char *s,char *f,i4_t l));
  i4_t       compatible_tok_fl  __P((enum token code, enum flags fcode));

#  define TEST_EXIST(ptr,n)   test_exist(ptr,n,__FILE__,__LINE__)
#  define TSTV_EXIST(ptr,n)   tstv_exist(ptr,n,__FILE__,__LINE__)
#  define TSTV_VEC(ptr)       tstv_vec(ptr,__FILE__,__LINE__)
#  define TSTV_VEC1(ptr)      tstv_vec(ptr,_FILE__,_LINE__)
#  define TST_NODE(ptr)       test_node(ptr,__FILE__,__LINE__)
#  define TST_NODE1(ptr)      test_node(ptr,_FILE__,_LINE__)
#  define xOp_PARMS(node,n,s) (*xOp_parms(node,n,s,__FILE__,__LINE__))
#  define xOp_PARM(node,n,c)  (*xOp_parm(node,n,c,__FILE__,__LINE__))

#  define IS_COMP(ptr,fl)     (compatible_tok_fl(CODe_TRN(ptr),fl)?ptr:TNULL)

#  define FMT_EQ(str,pat)     fmt_eq(str,pat)
#  define FMT_EQ1(str,pat)    fmt_eq(str,pat)

#else
#  define TEST_EXIST(ptr,n)   ptr
#  define TSTV_VEC(ptr)       ptr
#  define TSTV_VEC1(ptr)      ptr
#  define TST_NODE(node)      node
#  define TST_NODE1(node)     node
#  define xOp_PARMS(node,n,s) xOp_PARM(node,n,s)
#  define xOp_PARM(node,n,c)  (node)->operands[n]
#  define IS_COMP(ptr,fl)     ptr
#  define FMT_EQ(str,pat)     (bcmp(str,pat,strlen(pat))==0)
#  define FMT_EQ1(str,pat)    (*(char*)(str) == (char*)(pat))
#endif

#define   CODe_TRN1(node)       (TST_NODE1(node))->code

#define   CODe_TRN(node)        (TST_NODE(node))->code
#define   MASk_TRN(node)        (TST_NODE(node))->mask


#define   XOp_TRN(node,n)       (TEST_EXIST(node,n))->operands[n]
#define   XTXt_TRN(node,n)      XOp_TRN(node,n).txtp
#define   XVCb_TRN(node,n)      XTXt_TRN(node,n)
#define   XLTr_TRN(node,n)      XOp_TRN(node,n).ltrp
#define   XLNg_TRN(node,n)      XOp_TRN(node,n).l
#define   XFLt_TRN(node,n)      xOp_PARM(node,n,'f').f

#define   XREl_TRN(node,n)      *((Tabid*) &(xOp_PARMS(node,n,"RRR")) )

#define   LOCATIOn_TRN(n)       xOp_PARM(n,0,'p').pos
#define   RIGHt_TRN(n)          xOp_PARM(n,1,'r').txtp
#define   DOWn_TRN(n)           xOp_PARM(n,2,'d').txtp
#define   ARITy_TRN(n)          xOp_PARM(n,3,'a').l
#define   HAS_DOWn(n)           FMT_EQ(FORMAT(CODe_TRN(n))+2,"da")
#define   HAS_LOCATIOn(n)       FMT_EQ1(FORMAT(CODe_TRN(n)),"p")

#define   Setf_TRN(node,FCODE)  SetF(MASk_TRN(IS_COMP(node,FCODE)),FCODE)
#define   Clrf_TRN(node,FCODE)  ClrF(MASk_TRN(IS_COMP(node,FCODE)),FCODE)
#define   Tstf_TRN(node,FCODE)  TstF(MASk_TRN(IS_COMP(node,FCODE)),FCODE)


/********* TXTREF node address ******************************************/
#define   CODE_TRN(node)        CODe_TRN(Ptree(node))
#define   MASK_TRN(node)        MASk_TRN(Ptree(node))

#define   XOP_TRN(node,n)       XOp_TRN(Ptree(node),n)
#define   XTXT_TRN(node,n)      XTXt_TRN(Ptree(node),n)
#define   XVCB_TRN(node,n)      XVCb_TRN(Ptree(node),n)
#define   XLTR_TRN(node,n)      XLTr_TRN(Ptree(node),n)
#define   XLNG_TRN(node,n)      XLNg_TRN(Ptree(node),n)
#define   XFLT_TRN(node,n)      XFLt_TRN(Ptree(node),n)
#define   XREL_TRN(node,n)      XREl_TRN(Ptree(node),n)

#define   LOCATION_TRN(n)       LOCATIOn_TRN(Ptree(n))
#define   RIGHT_TRN(n)          RIGHt_TRN(Ptree(n))
#define   DOWN_TRN(n)           DOWn_TRN(Ptree(n))
#define   ARITY_TRN(n)          ARITy_TRN(Ptree(n))
#define   HAS_DOWN(n)           HAS_DOWn(Ptree(n))
#define   HAS_LOCATION(n)       HAS_LOCATIOn(Ptree(n))

#define   SetF_TRN(node,FCODE)  Setf_TRN(Ptree(node),FCODE)
#define   ClrF_TRN(node,FCODE)  Clrf_TRN(Ptree(node),FCODE)
#define   TstF_TRN(node,FCODE)  Tstf_TRN(Ptree(node),FCODE)

/********* VECTOR processing ******************************************/

#define Ptrvec(n)          ((trvec)Pnode(n))

#define NULL_TRVEC         (trvec)NULL

#define XVEc_TRN(node,n)   XTXt_TRN(node,n)
#define XVEC_TRN(node,n)   XTXT_TRN(node,n)

#define VLEn(vec)          (TSTV_VEC(vec))->num_elem
#define VLEn1(vec)         (TSTV_VEC1(vec))->num_elem
#define VLEN(vec)          VLEn(Ptrvec(vec))

#define VOp(vec,n)         (vec)->elem[TSTV_EXIST(vec,n)]

#define VOP(vec,n)         VOp(Ptrvec(vec),n)

#define XLEN_VEC(node,n)   VLEN(XVEC_TRN(node,n))
/* XOP_VEC(node,n,m)   node -- TXTREF pointer to trn           *\
*                      n -- number of param, what is vector     *
\*                     m -- number of vectors element          */
#define XOP_VEC(node,n,m)  VOP(XVEC_TRN(node,n),m)
#define XVECEXP(node,n,m)  VOP(XVEC_TRN(node,n),m).txtp

#endif
