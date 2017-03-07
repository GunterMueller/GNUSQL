/* 
 *  inprtyp.h - file with definitions for interpretator only
 *              (GNU SQL compiler)
 *                
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Konstantin Dyshlevoj 
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

/* $Id: inprtyp.h,v 1.247 1998/09/29 21:25:54 kimelman Exp $ */

#ifndef __intprtyp_h__
#define __intprtyp_h__

#include "type_lib.h"

#define P_ALLOC(n,typ) V_PTR(VMALLOC(n,typ),char)
#define Tid_V_ADR(ptr,x) (&(V_ADR(ptr,x,UnId)->t))
#define Iid_V_ADR(ptr,x) (&(V_ADR(ptr,x,UnId)->i))
#define Fid_V_ADR(ptr,x) (&(V_ADR(ptr,x,UnId)->f))

#define Tid_ADR(ptr) (&(((UnId *)(ptr->Tid.adr))->t))
#define Iid_ADR(ptr) (&(((UnId *)(ptr->Tid.adr))->i))
#define Fid_ADR(ptr) (&(((UnId *)(ptr->Tid.adr))->f))

#define SCN(ptr)   (*((Scanid *)(ptr->Scan.adr)))
#define V_SCN(ptr) (*V_ADR(ptr,Scan,Scanid))

#define PTR(s,p,n,ty) ((ty *)( ( ((PSECT_PTR *)((s->p).adr))[n] ).adr ))
#define TYP(s,p,n) (PTR(s,p,n,TRNode)->Ty)
#define LEN_I(from,pointer)  ( ((sql_type_t *)(from->pointer.adr))[i].len)
#define TYP_I(from,pointer)  ( ((sql_type_t *)(from->pointer.adr))[i].code )

/* in two next definitions x- pointerc to tree */
#define LEN_STR(x) ((x)->Ty.len)
#define VAL_TYPE(x) ((x)->Ty.code)

typedef struct
{
  TRNode *TP;
  i4_t flg;    /* =1, if the first operand of TP->code  *
	       * operation is already handled. Else =0 */
  i4_t art;
} StackUnit;


typedef struct
{
  data_unit_t Left, Right;
  sql_type_t Type;
  char LeftCode, RightCode, NullCode;
} SP_Cond;

#define RhExist(x) (x->Rh.off != VNULL)
#define NextRh(x) ((TRNode *)ADR(x,Rh))
#define NextDn(x) ((TRNode *)ADR(x,Dn))
#define Leaf(x)   (x->Dn.off == VNULL)

#define CONDBUF(bufsz,bufpt,cmd,num,field)                                 \
                  CondBuf(bufsz,bufpt,cmd,num,                             \
			  (VADR *)(cmd->field.adr),cmd->Exit.adr)
#define V_CONDBUF(bufsz,bufpt,cmd,num,field)                               \
                  CondBuf(bufsz,bufpt,cmd,num,                             \
			  V_ADR(cmd,field,VADR),V_ADR(cmd,Exit,char))
#define CondBuf(bufsz,bufpt,cmd,num,list,ex) {                             \
                              bufsz=0; bufpt=NULL;                         \
                              if (cmd->num)                                \
                                {                                          \
			          bufsz=condbuf(&bufpt,cmd->num,list);     \
			          if ( bufsz<0 )                           \
				    CHECK_ERR(bufsz);                      \
		 /* checking of the compatibility of columns' conditions */\
			          if (!bufsz)                              \
		 /* columns' conditions aren't compatible */               \
				    {                                      \
				      CHECK_ERR(-ER_EOSCAN);               \
				      cur=ex;                              \
			              break;                               \
			            }                                      \
		                }  }

#define MAX_STACK 64

#define InitDat     { data_st=xrealloc(data_st,MAX_STACK*SZ_DU);   \
                      dt_max=MAX_STACK;                            \
                      dt_ind=-1; }

#define InitStack   { stack=xrealloc(stack,MAX_STACK*SZ_SU);       \
                      st_max=MAX_STACK;                            \
                      st_ind=-1; }

#define DtPush    { if (dt_ind==dt_max) {                               \
                      dt_max+=MAX_STACK;                                \
                      data_st=(data_unit_t *)xrealloc(data_st,dt_max*SZ_DU);\
                    }                                                   \
                    dt_ind++; }

#define DtPop       dt_ind--;

#define StPush    { if (st_ind==st_max) {                            \
                      st_max+=MAX_STACK;                             \
                      stack=(StackUnit *)xrealloc(stack,st_max*SZ_SU);\
                    }                                                \
                    st_ind++; }

#define StPop       st_ind--;

#define DtCurEl       (data_st[dt_ind])
#define DatCur        (DtCurEl.dat)
#define StrCur        (STR_PTR  (&DatCur))
#define VStrCur       (STR_VPTR (&DatCur))
#define SrtCur        (SRT_VL   (&DatCur))
#define IntCur        (INT_VL   (&DatCur))
#define LngCur        (LNG_VL   (&DatCur))
#define FltCur        (FLT_VL   (&DatCur))
#define DblCur        (DBL_VL   (&DatCur))
#define NlFlCur       (DatCur.nl_fl)
#define TypCur        (DtCurEl.type)
#define LenCur        (TypCur.len)
#define CodCur        (TypCur.code)

#define DtPredEl      (data_st[dt_ind-1])
#define DatPred       (DtPredEl.dat)
#define StrPred       (STR_PTR (&DatPred))
#define SrtPred       (SRT_VL (&DatPred))
#define IntPred       (INT_VL (&DatPred))
#define LngPred       (LNG_VL (&DatPred))
#define FltPred       (FLT_VL (&DatPred))
#define DblPred       (DBL_VL (&DatPred))
#define NlFlPred      (DatPred.nl_fl)
#define LenPred       (DtPredEl.type.len)
#define CodPred       (DtPredEl.type.code)
#define StcCur        (stack[st_ind])

#define PtrDtNextEl   (data_st+dt_ind+1)
#define DtNextEl      (data_st[dt_ind+1])
#define DatNext       (DtNextEl.dat)
#define LngNext       (LNG_VL   (&DatNext))

/* function from interpr.c : */
int proc_work  __P((char *begin, char **Exi, MASK Handle));

/* following functions are in intlib.c : */

int handle_statistic  __P((i4_t untabid, i4_t tab_col_num, i4_t mod_col_num,
                      i4_t *mod_col_list, i4_t use_values));
int change_statistic  __P((void));
int check_and_put  __P((i4_t mod_col_num, i4_t *mod_col_list, TRNode **tree_ptr_arr,
		   data_unit_t **res_data_arr, S_ConstrHeader *header));
int chck_err  __P((int cd));
void off_adr  __P((PSECT_PTR * arr_ptr, i4_t nr));
int calculate  __P((TRNode * tr, void *res_ptr, i4_t res_size,
                    byte res_type, char *null_fl));
int condbuf  __P((char **bufptr, i4_t ClmCnt, VADR * Trees));

#endif
