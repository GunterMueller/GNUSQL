/*
 *  syndef.h - file with constants, data structure definitions
 *  and functions prototypes that are used only by synthesator.
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
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

/* $Id: syndef.h,v 1.247 1998/09/29 00:39:42 kimelman Exp $ */
  
#ifndef __SYNDEF_H__
#define __SYNDEF_H__
#include "type_lib.h"
#include "vmemory.h"
#include <assert.h>
		       
typedef struct {
  i4_t    ColNum;    /* number of column for given SP & Table     */
  float  Sel;       /* selectivity of the predicate              */
  TXTREF tree;      /* tree for SP                               */
  char   IndExists; /* flag of index existing for SP's column    */
  char   category;  /* = 1 for SP with EQ and = 2 in other cases */
} SP_Ind_Info;

typedef struct {
  float Sel;           /* the best selectivity of SP for this column      */
  i4_t  num_categ_eq;  /* number of SP of first category for this column  */
  i4_t  num_categ_neq; /* number of SP of second category for this column */
} Col_Stat_Info;

/* pointer to beginning, current & maximum *
   available pointers in the current page  */
extern VADR ProcAdr, CurPg, MaxPg;

/* if != NULL => it's the place where to   *
 * put address of command "END" in section */
extern VADR *FuncExit;

#define CAN_BE_USED(PredicMask, AllPredTblMask)              \
        (BitOR (AllPredTblMask, PredicMask) == AllPredTblMask)

#define WHAT_IS_BEG_CMD(cmd_adr) (cmd_adr - sizeof(enum CmdCode))

#define PLACE_CMD(adr, cmnd) *V_PTR(adr,struct S_##cmnd)=D_##cmnd
#define SET_CMD(cmnd)       set_command (cmnd, (char *)(&D_##cmnd), sizeof (D_##cmnd))
#define STEP_CMD(adr, cmnd) adr = set_command (cmnd, NULL, SIZES (S_##cmnd))
#define SET_END             set_command (end, NULL, 0)

/* 31 tables have own numners (masks) and others have mask "1"      *
 * For temporary tables it is too "1" (excepting special situations *
 * like GROUP BY (in this case temp. tables has own mask too)       */

extern i4_t CurBit;			/* is used in operators */
#define NEXT_BIT ((CurBit==8*sizeof(i4_t)) ? 1 : 1L<<(CurBit++))

typedef struct
{
  VADR PrAdr, Place;
  MASK Depend;
} SH_Info;

/* During the tree handling addres of current procedure beginning, *
 * position of the tree and it's dependence are stored in the next *
 * element of tr_arr.                                              *
 * During the command SetHandle handling addres of current proce-  *
 * dure beginning, position of the command and bit (that is set by *
 * this command) are stored in the next element of sh_arr.         */

#define SET_ARR_DELTA 10

/* X = 'sh' or 'tr' */
#define SET(X,off,dep)				\
{						\
  CHECK_ARR_ELEM (X##_arr, X##_max, X##_cnt, SET_ARR_DELTA, SH_Info); \
  X##_arr[X##_cnt].PrAdr=ProcAdr;		\
  X##_arr[X##_cnt].Place=off;			\
  X##_arr[X##_cnt ++].Depend=dep;		\
}

#define INTERP_MODULE_DELTA 512
/* #define MAX_CMD_SZ (20*SIZES(S_OPSCAN)) */

VADR ProcHandle (TXTREF BegNode, VADR BO_Tid, VCBREF ResScan,
		 i2_t ProcN, MASK *Depend, float *sq_cost, i4_t *res_row_num);
VADR ScanHandle (VCBREF ScanNode, VADR * Scan, MASK * Msk,
		 float *sq_cost, i4_t *res_row_num);
float Query (TXTREF FromNode, VADR BO_Tid, VCBREF ResScan,
	     TXTREF CmdNode, i4_t * res_row_num);
i4_t Handle (TXTREF CurNode, VADR BO_Tid, VCBREF ResScan,
	    float *sq_cost, i4_t *res_row_num);

void tree_statistics (TXTREF in, i4_t *scan, i4_t *table, i4_t *tmp);
void prepare_MG (TXTREF ptr, i4_t *g_clm_nmb, VADR * g_clm_arr,
		 i4_t *func_nmb, VADR * func_arr, VADR * func_clm_arr);
void prepare_SRT (TXTREF ptr, i4_t *clm_nmb, VADR * clm_arr);
void prepare_UPD (TXTREF ptr, i4_t *clm_nmb, VADR * clm_arr);
VADR prepare_HOLE (sql_type_t typ);
i4_t SP_Extract (TXTREF Node, MASK *TblBits,
                i4_t TblNum, Simp_Pred_Info ** Result);

void ModDump (FILE * out);

VADR set_command (enum CmdCode code, char *com_adr, i4_t com_size);
void Pr_Procedure (VADR Proc_Adr);
void Pr_SCAN (VADR Tid, VADR Scan, char OperType, char mode, i4_t nr,
	      VADR rlist, i4_t nl, VADR list, i4_t ic,
	      VADR range, i4_t nm, VADR mlist, VADR Ex, VADR CmdPlace);
void Pr_SORTTBL (VADR TidIn, VADR TidOut, i4_t ns, VADR rlist, char fl,
                 char order);
void Pr_MKUNION (VADR TidIn1, VADR TidIn2, VADR TidOut);
void Pr_FUNC (VADR Tid, char OperType, i4_t nl, VADR list,
	      i4_t ic, VADR range, VADR colval, i4_t nf,
	      VADR flist, VADR fl, VADR Exit, VCBREF FirClm);
void Pr_GROUP (VADR TidIn, VADR TidOut, i4_t ng, VADR glist,
	       char order, i4_t nf, VADR flist, VADR fl);
void Pr_FINDROW (VADR Scan, i4_t nr, VADR ColPtr, VADR Exit, VADR CmdPlace);
void Pr_READROW (VADR Scan, i4_t nr, VADR ColPtr, VADR rlist);
void Pr_UNTIL (VADR PTree, VADR ret, VADR CmdPlace);
void Pr_COND_EXIT (TXTREF Tree);
void Pr_GoTo (VADR Br, VADR CmdPlace);
VADR SelArr (TXTREF SelNode, i4_t *Num);
void Pr_RetPar (VADR OutList, i4_t OutNum, char ExitFlag);
void Pr_INSROW (VADR Tbid, i4_t nv, VADR InsList, VCBREF FirClm,
		VCBREF RealScan, i4_t nm, VADR V_mlist, VADR Scan);
void Pr_INSLIST (VADR Tbid, i4_t nv, VADR type_arr, VADR len,
		 VADR col_ptr, VADR credate, VADR cretime);
void Pr_INSTAB (VADR TidFrom, VADR TidTo);
void Pr_UPDATE (VADR Scan, i4_t nm, VADR OutList,
                VCBREF ScanNode, VADR V_mlist);
void Pr_DELETE (VADR Scan, VCBREF ScanNode);
void Pr_CLOSE (VADR Scan);
void Pr_DROP (i4_t untabid);
void Pr_DROPTTAB (VADR Tid);
void Pr_ERROR (i4_t cod);
void Pr_CRETAB (VADR tabname, VADR owner, Segid segid, i4_t colnum,
		i4_t nncolnum, VCBREF FirClm, VADR tabid);
void Pr_CREIND (VADR tabid, VADR indid, char uniq_fl, 
		i4_t colnum, VADR clmlist);
void Pr_SavePar (VCBREF Dict);
void Pr_TMPCRE (VADR Tid, VCBREF FirClm, i4_t nnulnum);
void Pr_PRIVLG (VADR untabid, VADR owner, VADR user, VADR privcodes, 
		i4_t un, VADR ulist, i4_t rn, VADR rlist);
VADR Pr_Constraints (VCBREF ScanNode, VADR Scan,
                     i2_t oper, i4_t nm, i4_t *mlist);
VADR Tr_RecLoad (TXTREF Nod, MASK * Depend, i4_t RhNeed, float *sq_cost);
float opt_query (TXTREF FromNode, i4_t *res_row_num);
#endif
