/*
 *  struct.c - special functions for dumping of internal commands to
 *             module and some additional functions for synthesator
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

/* $Id: struct.c,v 1.250 1998/09/16 22:51:36 kimelman Exp $ */
  
#include "global.h"
#include "syndef.h"
#include "engine/fieldtp.h"
#include "tassert.h"

extern i4_t   *chconstr_col_use;
extern i4_t    ch_col_use_num;
extern SH_Info *tr_arr;
extern i4_t tr_cnt, tr_max;

#define ADD_CUR_PG(d) { CurPg += d; ALIGN(CurPg,SZ_LNG); }

#define SET_ENUM(x) { *V_PTR(CurPg,enum CmdCode)=x; \
                      ADD_CUR_PG (sizeof(enum CmdCode)); }

/*
 * 'set_command' puts the command to module for interpreter 
 * returns VADR of the commad's structure     
 */

VADR
set_command (enum CmdCode code, char *com_adr, i4_t com_size)
{
  VADR adr_res;
  static i4_t min_rest_size = 3 * sizeof(i4_t) + SIZES (S_GoTo);
  /* check for room for "Goto ; next_vm_piece " block at the end
   * of currenly filled virtual memory portion.
   */
  
  if (MaxPg - CurPg < min_rest_size + com_size)
    {/* additional portion of virtual memory required */
      VADR NewPg = VMALLOC (INTERP_MODULE_DELTA, char);
      
      SET_ENUM (GoTo);
      Pr_GoTo (NewPg, CurPg);
      ADD_CUR_PG (SIZES (S_GoTo));
      SET_ENUM (next_vm_piece);
      CurPg = NewPg;
      ALIGN(CurPg,SZ_LNG);
      MaxPg = CurPg + INTERP_MODULE_DELTA;
    }      

  SET_ENUM(code);
  adr_res = CurPg;
  if (com_adr)           /* copy data , if present */
    bcopy (com_adr, V_PTR(CurPg,char), com_size);
  ADD_CUR_PG (com_size); /* shift & align free mem pointer */
  return adr_res;
} /* set_command */

void
Pr_Procedure (VADR Proc_Adr)
/* insertion to module of command for colling *
 * the procedure from address Proc_Adr        */
{
  struct S_Procedure D_Procedure;
  
  D_Procedure.ProcBeg = Proc_Adr;
  
  SET_CMD (Procedure);
}

void
Pr_SCAN (VADR Tid, VADR Scan, char OperType, char mode,
	 i4_t nr, VADR rlist, i4_t nl, VADR list, i4_t ic,
	 VADR range, i4_t nm, VADR mlist, VADR Ex, VADR CmdPlace)
{
  struct S_OPSCAN D_OPSCAN;

  /* table scan                            */
  D_OPSCAN.Scan = Scan;
  /* Tabid,Indid or Filid                  */
  D_OPSCAN.Tid = Tid;
  /* operation type of System of Memory    */
  /* and  Synchronization Managment        */
  D_OPSCAN.OperType = OperType;        
  /* open mode                             */
  D_OPSCAN.mode = mode;
  /* the number of columns which values will be selected */
  D_OPSCAN.nr = nr;
  /* array of column numbers which values  */
  /* will be selected                      */
  D_OPSCAN.rlist = rlist;           
  D_OPSCAN.nl = nl;
  /* array of VADR for trees of SP         */
  D_OPSCAN.list = list;
  D_OPSCAN.ic = ic;
  /* scan range - restricted form of       */
  /* conditions for columns included in    */
  /* idexes = idexes; array of VADR        */
  D_OPSCAN.range = range;           
  /* the number of columns which values    */
  /* will be modified                      */
  D_OPSCAN.nm = nm;
  /* array of column number which values   */
  /* will be modified                      */
  D_OPSCAN.mlist = mlist;
  /* exit in the case of                   */
  /* incompatibility of  SP                */

  D_OPSCAN.Exit = Ex;

  if (CmdPlace)
    PLACE_CMD (CmdPlace, OPSCAN);
  else
    SET_CMD (OPSCAN);
}

void
Pr_SORTTBL (VADR TidIn, VADR TidOut, i4_t ns, VADR rlist, char fl, char order)
{
  struct S_SORTTBL D_SORTTBL;

  D_SORTTBL.TidIn = TidIn;
  D_SORTTBL.TidOut = TidOut;
  D_SORTTBL.ns = ns; 
  D_SORTTBL.rlist = rlist; 
  D_SORTTBL.fl = fl; 
  D_SORTTBL.order = order;
  
  SET_CMD (SORTTBL);
}

void
Pr_MKUNION (VADR TidIn1, VADR TidIn2, VADR TidOut)
{
  struct S_MKUNION D_MKUNION;

  D_MKUNION.TidIn1 = TidIn1;       
  D_MKUNION.TidIn2 = TidIn2;       
  D_MKUNION.TidOut = TidOut;
  
  SET_CMD (MKUNION);
}

void
Pr_GROUP (VADR TidIn, VADR TidOut, i4_t ng, VADR glist,
	  char order, i4_t nf, VADR flist, VADR fl)
{
  struct S_MKGROUP D_MKGROUP;

  D_MKGROUP.TidIn = TidIn;     
  D_MKGROUP.TidOut = TidOut;    
  D_MKGROUP.ng = ng;        
  D_MKGROUP.glist = glist;     
  D_MKGROUP.order = order;     
  D_MKGROUP.nf = nf;        
  D_MKGROUP.flist = flist;     
  D_MKGROUP.fl = fl;        
		      
  SET_CMD (MKGROUP);
}

void
Pr_FUNC (VADR Tid, char OperType, i4_t nl, VADR list, i4_t ic, VADR range,
	 VADR colval, i4_t nf, VADR flist, VADR fl, VADR Exit, VCBREF FirClm)
{
  VADR cmd;
  struct S_FUNC D_FUNC;
  
  D_FUNC.flag = 'n';
  D_FUNC.Tid.off = Tid;      
  D_FUNC.OperType = OperType;
  D_FUNC.nl = nl;      
  D_FUNC.list.off = list;    
  D_FUNC.ic = ic;      
  D_FUNC.range.off = range;   
  D_FUNC.colval.off = colval;  
  D_FUNC.nf = nf;      
  D_FUNC.flist.off = flist;   
  D_FUNC.fl.off = fl;       
  D_FUNC.Exit.off = Exit;     
                      
  STEP_CMD  (cmd, FUNC);
  PLACE_CMD (cmd, FUNC);
  
  if (!Exit)
    {
      assert (!FuncExit);
      /* address of command "ERROR" in section must be saved in field Exit */
      FuncExit = &(V_PTR(cmd, struct S_FUNC)->Exit.off);
    }
}

void
Pr_FINDROW (VADR Scan, i4_t nr, VADR ColPtr, VADR Exit, VADR CmdPlace)
{
  struct S_FINDROW  D_FINDROW;

  D_FINDROW.flag = 'n';
  D_FINDROW.Scan.off = Scan;    
  D_FINDROW.nr = nr;      
  D_FINDROW.ColPtr.off = ColPtr;  
  D_FINDROW.Exit.off = Exit;
  
  if (CmdPlace)
    PLACE_CMD (CmdPlace, FINDROW);
  else
    SET_CMD (FINDROW);
} /* PrFINDROW */

void
Pr_READROW (VADR Scan, i4_t nr, VADR ColPtr, VADR rlist)
{
  struct S_READROW D_READROW;
  
  D_READROW.flag = 'n';        
  D_READROW.Scan.off = Scan;      
  D_READROW.nr = nr;        
  D_READROW.ColPtr.off = ColPtr;    
  D_READROW.rlist.off = rlist;     
                         
  SET_CMD (READROW);
} /* PrREADROW */

void
Pr_UNTIL (VADR cond, VADR ret, VADR CmdPlace)
{
  struct S_until D_until;
  
  D_until.flag     = 'n';
  D_until.cond.off = cond;
  D_until.ret.off  = ret;

  if (CmdPlace)
    PLACE_CMD (CmdPlace, until);
  else
    SET_CMD (until);
}

void
Pr_COND_EXIT (TXTREF Tree)
{
  MASK Depend;
  struct S_COND_EXIT D_COND_EXIT;
  
  D_COND_EXIT.flag = 'n';
  D_COND_EXIT.Tree.off = VNULL;

  SET (tr, D_COND_EXIT.Tree.off = 
       Tr_RecLoad (Tree, &Depend, FALSE, NULL), Depend);
 
  SET_CMD (COND_EXIT);
}

void
Pr_GoTo (VADR Br, VADR CmdPlace)
{
  struct S_GoTo D_GoTo;

  D_GoTo.flag = 'n';
  D_GoTo.Branch.off = Br;

  if (CmdPlace)
    PLACE_CMD (CmdPlace, GoTo);
  else
    SET_CMD (GoTo);
}

VADR
SelArr (TXTREF SelNode, i4_t *Num)
/* results VADR of array of pointers (PSECT_PTR) to trees *
 * from SELECT & number of elements in this array to Num. */
{
  VADR PSect;
  i4_t i;
  TXTREF ColNode;
  MASK Depend;

  *Num = 0;
  ColNode = DOWN_TRN (SelNode);
  while (ColNode)
    {
      ColNode = COL_NEXT (ColNode);
      (*Num)++;
    }
  PSect = VMALLOC (*Num, PSECT_PTR);
  ColNode = DOWN_TRN (SelNode);
  for (i = 0; i < *Num; i++)
    {
      SET (tr, V_PTR (PSect, PSECT_PTR)[i].off =
	   Tr_RecLoad (ColNode, &Depend, FALSE, NULL), Depend);
      ColNode = COL_NEXT (ColNode);
    }
  return PSect;
}

void
Pr_RetPar (VADR OutList, i4_t OutNum, char ExitFlag)
{
  struct S_RetPar D_RetPar;
  
  D_RetPar.flag = 'n';
  D_RetPar.ExitFlag = ExitFlag;
  D_RetPar.OutNum = OutNum;  
  D_RetPar.OutList.off = OutList; 

  SET_CMD (RetPar);
}

void
Pr_INSROW (VADR Tbid, i4_t nv, VADR InsList, VCBREF FirClm,
	   VCBREF RealScan, i4_t nm, VADR V_mlist, VADR Scan)
{
  VADR V_Tp;
  sql_type_t *Tp;
  VCBREF CurClm;
  i4_t *mlist = NULL;
  struct S_INSROW D_INSROW;

  D_INSROW.flag = 'n';
  D_INSROW.Tid.off = Tbid;      
  D_INSROW.nv = nv;       
  D_INSROW.InsList.off = InsList;  
  D_INSROW.ClmTp.off = VNULL;    
  D_INSROW.Constr.off = VNULL;
  
  P_VMALLOC (Tp, nv, sql_type_t );
  for (CurClm = FirClm; CurClm; CurClm = COL_NEXT (CurClm))
    Tp [COL_NO (CurClm)] = COL_TYPE (CurClm);
  D_INSROW.ClmTp.off = V_Tp;
  if (RealScan)
    {
      if (nm)
        {
          mlist = TYP_ALLOC (nm, i4_t);
      
          TYP_COPY (V_PTR (V_mlist, i4_t), mlist, nm, i4_t);
        }
      D_INSROW.Constr.off =
	Pr_Constraints (RealScan, Scan, (nm) ? UPDATE : INSERT, nm, mlist);
      if (mlist)
        xfree (mlist);
    }
  else
    /* insertion into temporary table */
    {
      S_ConstrHeader *header;
      VADR V_header;
      
      P_VMALLOC (header, 1, S_ConstrHeader);
      header->tab_col_num = nv;
      D_INSROW.Constr.off = V_header;
    }
  SET_CMD (INSROW);
}

void
Pr_INSLIST (VADR Tbid, i4_t nv, VADR type_arr, VADR len,
	    VADR col_ptr, VADR credate, VADR cretime)
{
  struct S_INSLIST D_INSLIST;

  D_INSLIST.flag = 'n';
  D_INSLIST.Tid.off = Tbid;         
  D_INSLIST.nv = nv;          
  D_INSLIST.len.off = len;         
  D_INSLIST.col_types.off = type_arr;
  D_INSLIST.col_ptr.off = col_ptr;     
  D_INSLIST.credat_adr.off = credate;  
  D_INSLIST.cretime_adr.off = cretime; 
			     
  SET_CMD (INSLIST);
}

void
Pr_INSTAB (VADR TidFrom, VADR TidTo)
{
  struct S_INSTAB D_INSTAB;

  D_INSTAB.TidFrom = TidFrom;          
  D_INSTAB.TidTo = TidTo;
  
  SET_CMD (INSTAB);
}

void
Pr_UPDATE (VADR Scan, i4_t nm, VADR OutList, VCBREF ScanNode, VADR V_mlist)
{
  VADR V_Tp = VNULL;
  sql_type_t *Tp = NULL;
  VCBREF CurClm, FirTabClm;
  i4_t i, *mlist = NULL;
  struct S_MODROW D_MODROW;
    
  D_MODROW.flag = 'n';
  D_MODROW.Scan.off = Scan;    
  D_MODROW.nl = nm;     
  D_MODROW.mlist.off = V_mlist;  
  D_MODROW.OutList.off = OutList; 
  D_MODROW.ClmTp.off = VNULL;
  D_MODROW.Constr.off = VNULL;
  
  if (nm)
    {
      mlist = TYP_ALLOC (nm, i4_t);
      TYP_COPY (V_PTR (V_mlist, i4_t), mlist, nm, i4_t);
      P_VMALLOC (Tp, nm, sql_type_t );
    }
  FirTabClm = TBL_COLS (COR_TBL (ScanNode));
  for (i = 0; i < nm; i++)
    {
      for (CurClm = FirTabClm; CurClm; CurClm = COL_NEXT (CurClm))
	if (COL_NO (CurClm) == mlist[i])
	  {
	    Tp [i] = COL_TYPE (CurClm);
	    break;
	  }
      if (!CurClm)
	yyfatal ("Synthes: UPDATE: There is not information about type of column ");
    }
  
  D_MODROW.ClmTp.off = V_Tp;
  D_MODROW.Constr.off = Pr_Constraints (ScanNode, Scan, UPDATE, nm, mlist);
  if (mlist)
    xfree (mlist);
  
  SET_CMD (MODROW);
}

void
Pr_DELETE (VADR Scan, VCBREF ScanNode)
{
  struct S_DELROW D_DELROW;

  D_DELROW.Scan = Scan;
  D_DELROW.Constr = Pr_Constraints (ScanNode, Scan, DELETE, 0, NULL);
  
  SET_CMD (DELROW);
}

void
Pr_CLOSE (VADR Scan)
{
  struct S_CLOSE D_CLOSE;
  
  D_CLOSE.Scan = Scan;

  SET_CMD (CLOSE);
}

void
Pr_DROP (i4_t untabid)
{
  struct S_DROPBASE D_DROPBASE;

  D_DROPBASE.untabid=untabid;

  SET_CMD (DROPBASE);
}

void
Pr_DROPTTAB (VADR Tbid)
{
  struct S_DROPTTAB D_DROPTTAB;
  
  D_DROPTTAB.Tid = Tbid;
  
  SET_CMD (DROPTTAB);
}

void
Pr_ERROR (i4_t cod)
{
  struct S_ERROR D_ERROR;
  
  D_ERROR.Err = cod;
  
  SET_CMD (ERROR);
}

void
Pr_CRETAB (VADR tabname, VADR owner, Segid segid, i4_t colnum,
	   i4_t nncolnum, VCBREF FirClm, VADR tabid)
{
  i4_t i;
  sql_type_t *arr;
  VADR V_arr;
  VCBREF CurClm = FirClm;
  
  struct S_CRETAB D_CRETAB;

  D_CRETAB.tabname = tabname;         
  D_CRETAB.owner = owner;           
  D_CRETAB.segid = segid;          
  D_CRETAB.colnum = colnum;          
  D_CRETAB.nncolnum = nncolnum;        
  D_CRETAB.coldescr = VNULL;        
  D_CRETAB.tabid = tabid;           

  P_VMALLOC (arr, colnum, sql_type_t );
  for (i = 0; CurClm; i++, CurClm = COL_NEXT (CurClm))
    type_to_bd( &COL_TYPE (CurClm), arr + i);
  D_CRETAB.coldescr = V_arr;
  
  SET_CMD (CRETAB);
}

void
Pr_CREIND (VADR tabid, VADR indid, char uniq_fl, i4_t colnum, VADR clmlist)
{
  struct S_CREIND D_CREIND;

  D_CREIND.tabid = tabid;           
  D_CREIND.indid = indid;           
  D_CREIND.uniq_fl = uniq_fl;         
  D_CREIND.colnum = colnum;          
  D_CREIND.clmlist = clmlist;
  
  SET_CMD (CREIND);
}

void
Pr_PRIVLG (VADR untabid, VADR owner, VADR user, VADR privcodes, 
	   i4_t un, VADR ulist, i4_t rn, VADR rlist)
{
  struct S_PRIVLG D_PRIVLG;

  D_PRIVLG.untabid = untabid;            
  D_PRIVLG.owner = owner;               
  D_PRIVLG.user = user;               
			   
  D_PRIVLG.privcodes = privcodes;          
  D_PRIVLG.un = un;                 
  D_PRIVLG.ulist = ulist;              
  D_PRIVLG.rn = rn;                 
  D_PRIVLG.rlist = rlist;
  
  SET_CMD (PRIVLG);
}

void
Pr_SavePar (VCBREF Dict)
/* Dict - the beginning of operator vocabulary */
{
  VCBREF CurPar = Dict;
  VADR PlaceOff;
  i4_t n = 0, Num = 0;
  struct S_SavePar D_SavePar;

  D_SavePar.flag = 'n';
		      
  for(CurPar = Dict, Num = 0; CurPar ; CurPar = RIGHT_TRN (CurPar))
    if ((CODE_TRN (CurPar) == PARAMETER) &&
        !TstF_TRN (CurPar,INDICATOR_F) &&
        !TstF_TRN (CurPar, OUT_F))
      Num++;

  if (Num)
    {
      PlaceOff = VMALLOC (Num, PSECT_PTR);
      for( CurPar = Dict, n = 0; CurPar; CurPar = RIGHT_TRN (CurPar))
        if ((CODE_TRN (CurPar) == PARAMETER) &&
            !TstF_TRN (CurPar,INDICATOR_F) &&
            !TstF_TRN (CurPar, OUT_F))
          V_PTR (PlaceOff, PSECT_PTR)[n++].off =
            PAR_ADR (CurPar)                   =
            prepare_HOLE (PAR_TTYPE (CurPar));
      
      D_SavePar.ParamNum = Num;
      D_SavePar.PlaceOff.off = PlaceOff;
      
      SET_CMD (SavePar);
    }
}				/* PrSavePar */

void
Pr_TMPCRE (VADR Tbid, VCBREF FirClm, i4_t nnulnum)
{
  sql_type_t *arr;
  VADR V_arr;
  VCBREF CurClm;
  i4_t i, ColNum = 0;
  struct S_TMPCRE D_TMPCRE;

			   
  CurClm = FirClm;
  while (CurClm)
    {
      ColNum++;
      CurClm = COL_NEXT (CurClm);
    }
  P_VMALLOC (arr, ColNum, sql_type_t );
  
  for (i = 0, CurClm = FirClm; CurClm; i++, CurClm = COL_NEXT (CurClm))
    type_to_bd (&COL_TYPE (CurClm), arr + COL_NO (CurClm));
  
  D_TMPCRE.Tid = Tbid;
  D_TMPCRE.colnum = ColNum;
  D_TMPCRE.nnulnum = nnulnum;
  D_TMPCRE.coldescr = V_arr;
  
  SET_CMD (TMPCRE);
}				/* PrTMPCRE */

#define Mk_InitDt(d, t)  P_VMALLOC (InitDt, 1, PrepSQ); \
                         d = &(InitDt->ToDtStack.dat);  \
                         t = &(InitDt->ToDtStack.type)

#define Mk_dat_typ       NL_FL(dat) = REGULAR_VALUE;    \
                         typ->code = T_BOOL;            \
                         typ->len = 0

#define Mk_Call(x, prep) x->code    = CALLPR;           \
                         x->Dn.off  = V_InitDt;         \
                         x->Ptr.off = (prep) ?          \
			    prepare_HOLE (*typ) : VNULL;\
                         x->Depend  = *Dep;             \
                         x->Handle  = 1;                \
                         x->Arity   = 0;                \
                         x->Ty = *typ
			 
#define USE_HOLE     TRUE
#define NOT_USE_HOLE FALSE
			 
/* Next 3 functions form the tree in Query for SubQuery *
 * in Exists | comrapision | quantified predicate.      *
 * It return addr. of this tree (>0) or error (<0).     *
 * Nod - node for subquery in compilator's tree,        *
 * PTN - node formed in Tr_RecLoad.                     */
 
static VADR
SQ_Exists (TXTREF Nod, VADR V_PTN, MASK *Dep, float *sq_cost)
{
  VADR V_InitDt, V_res;
  PrepSQ *InitDt;
  TpSet  *dat;
  sql_type_t   *typ;
   
  Mk_InitDt (dat, typ);
  Mk_dat_typ;
  InitDt->GoUpNeed = 0;
  LNG_VL(dat) = FALSE;
  
  if ((V_res = ProcHandle (Nod, VNULL, TNULL, 0, Dep, sq_cost, NULL)) < 0)
    return V_res;
  V_PTR (V_InitDt, PrepSQ)->ProcAdr = V_res;
  
  Mk_Call (V_PTR (V_PTN, TRNode), USE_HOLE);
  
  return V_PTN;
} /* SQ_Exists */

static VADR
SQ_Comp (TXTREF Nod, VADR V_PTN, MASK *Dep, float *sq_cost)
     /* Nod - SUBQUERY */
{
  VADR V_InitDt, V_res;
  PrepSQ *InitDt;
  TpSet  *dat;
  sql_type_t   *typ;
  
  Mk_InitDt (dat, typ);
  InitDt->GoUpNeed = 0;
  NL_FL(dat) = UNKNOWN_VALUE;
  V_PTR (V_PTN, TRNode)->Ty = *typ = OPRN_TYPE (Nod);
  
  if ((V_res = ProcHandle (Nod, VNULL, TNULL, 0, Dep, sq_cost, NULL)) < 0)
    return V_res;
  V_PTR (V_InitDt, PrepSQ)->ProcAdr = V_res;
  
  Mk_Call (V_PTR (V_PTN, TRNode), USE_HOLE);
  
  return V_PTN;
} /* SQ_Comp */

#undef Mk_InitDt
#undef Mk_dat_typ
#undef Mk_Call

VADR
Tr_RecLoad (TXTREF Nod, MASK * Depend, i4_t RhNeed, float *sq_cost)
/* Recursive tree loading : to Depend is putted         *
 * common dependence MASK of this tree (& all trees     *
 * that are right from current if RhNeed == 1)          *
 * RhNeed = 1 if there is needed to handle right        *
 * reference of tree's header.                          *
 * If sq_cost != NULL & Nod - SubQuery => the estimated *
 * cost of this SubQuery is putted to sq_cost           */
{
  VCBREF obj;
  TRNode *PTN;
  VADR V_PTN, res, V_Rh, V_Dn;
  MASK DnDepend = 0, RhDepend = 0;
  
  if (PRCD_DEBUG)
    printf ("Struct:i4_t TrRecLoad()\n");
  
  *Depend = 0;
  
  if (!Nod)
    return VNULL;
  
  P_VMALLOC (PTN, 1, TRNode);
  switch (CODE_TRN (Nod))
    {
    case COLPTR:
      obj = OBJ_DESC (Nod);
      PTN->code = CODE_TRN (obj);
      PTN->Ty = COL_TYPE (obj);
      if (PTN->code == SCOLUMN && !chconstr_col_use)
	{
	  *Depend = COR_MASK (COL_TBL (obj));
	  PTN->Ptr.off = COL_ADR (obj);
	}
      else
	{
          i4_t colno = COL_NO (obj);
          
	  TASSERT (PTN->code == COLUMN || chconstr_col_use, Nod);
	  *Depend = 0;
	  PTN->Ptr.off = colno;
	  if (!chconstr_col_use [colno])
	    {
	      chconstr_col_use [colno] ++;
	      ch_col_use_num ++;
	    }
	}
      break;

    case PARMPTR:
      PTN->code = PARAMETER;
      obj = OBJ_DESC (Nod);
      PTN->Ty = PAR_TTYPE (obj);
      PTN->Ptr.off = PAR_ADR (obj);
      break;

    case CONST:
    case NULL_VL:
      prepare_CONST (Nod, V_PTN);
      break;

    case USERNAME:
      prepare_USERNAME (PTN);
      break;

    default:			/* operation */
      if (Is_SQPredicate_Code (CODE_TRN (Nod)) && !VAL_HOLE (Nod))
	{
	  if (CODE_TRN (Nod) == SUBQUERY)
	    res = SQ_Comp (Nod, V_PTN, Depend, sq_cost);
	  else
            {
              assert (CODE_TRN (Nod) == EXISTS);
              res = SQ_Exists (Nod, V_PTN, Depend, sq_cost);
            }
	  return (VAL_HOLE (Nod) = res);
	}
	  
      /* this tree could be loaded before */
      if (VAL_HOLE (Nod) > 1)
	{
	  vmfree (V_PTN);
          /* in multiused nodes field Depend may be = 0 *
           * in this case dependency is in field Handle */
	  *Depend = V_PTR (VAL_HOLE (Nod), TRNode) -> Depend;
          if (!(*Depend))
            *Depend = V_PTR (VAL_HOLE (Nod), TRNode) -> Handle;
	  return VAL_HOLE (Nod);
	}
  
      PTN->code = CODE_TRN (Nod);
      PTN->Arity = ARITY_TRN (Nod);
      PTN->Ty = OPRN_TYPE (Nod);
      PTN->Ptr.off = VNULL;
    }				/* switch */

  V_Dn = (HAS_DOWN (Nod)) ? 
    Tr_RecLoad (DOWN_TRN (Nod), &DnDepend, TRUE, NULL) : VNULL;
  V_Rh = (RhNeed) ? 
    Tr_RecLoad (RIGHT_TRN (Nod), &RhDepend, TRUE, NULL) : VNULL;
  
  PTN = V_PTR (V_PTN, TRNode);
  PTN->Dn.off = V_Dn;
  PTN->Rh.off = V_Rh;
  PTN->Depend = BitOR (*Depend, DnDepend);
  *Depend = BitOR (PTN->Depend, RhDepend);
  PTN->Handle = 0;

  return V_PTN;
}				/* TrRecLoad */

VADR
Pr_Constraints (VCBREF ScanNode, VADR Scan, i2_t oper, i4_t nm, i4_t *mlist)
     /* makes the structure S_ConstrHeader in          *
      * virtual memory for constraints 	checking       *
      * accordingly operation (INSERT, DELETE, UPDATE) *
      * and mlist -columns' numbers (for UPDATE)       */
{
  Constr_Info *constr_info, *cur_constr, *next_constr, *last_constr;
  char constr_need, *mod_cols;
  i4_t i, j, all_cnstr_cnt = 0, cur_constr_cnt = 0, tab_colno, read_cnt = 0;
  static i4_t ptrs_cnt = 0, mlist_fl_cnt = 0, read_fl_cnt = 0;
  static VADR *ptrs = NULL;
  static char *mlist_fl = NULL, *read_fl = NULL;
  i2_t  constr_type;
  VADR V_header = VNULL;
  S_ConstrHeader *header = NULL;
  VCBREF CurClm, Tab, View;
  VADR V_constr_arr, *constr_arr, V_nums, V_types;
  i4_t *nums;
  sql_type_t *types;
  
  last_constr = NULL;
  if (!ScanNode)
    return VNULL;
  Tab = COR_TBL (ScanNode);
  if (!Tab)
    return VNULL;
  
  P_VMALLOC (header, 1, S_ConstrHeader);
  header->tab_col_num = tab_colno = TBL_NCOLS (Tab);
  header->scan = Scan;
  header->untabid = (TBL_TABID (Tab)).untabid;
  
  CHECK_ARR_SIZE (mlist_fl, mlist_fl_cnt, tab_colno, char);
  if (oper == UPDATE)
    {
      bzero (mlist_fl, tab_colno);
      for (i = 0; i < nm; i++)
	mlist_fl[mlist[i]] = TRUE;
      read_cnt = nm;
    }
  else
    {
      if (oper == DELETE)
        read_cnt = tab_colno;
      for (i = 0; i < tab_colno; i++)
        mlist_fl[i] = TRUE;
    }
  
  CHECK_ARR_SIZE (read_fl,  read_fl_cnt,  tab_colno, char);
  if (oper == INSERT)
    bzero (read_fl,  tab_colno);
  else
    /* old values are needed for filling of statistic about columns */
    bcopy (mlist_fl, read_fl, tab_colno);
  
  if (!CONSTR_INFO (Tab))
    {
      *(Constr_Info **) (&CONSTR_INFO (Tab)) =
	get_constraints ((TBL_TABID (Tab)).untabid, tab_colno);
      if (!CONSTR_INFO (Tab))
	CONSTR_INFO (Tab) = 1;
    }
  constr_info = (CONSTR_INFO (Tab) == 1) ? NULL :
    *(Constr_Info **) (&CONSTR_INFO (Tab));
  
  for (cur_constr = constr_info; cur_constr; cur_constr = next_constr)
    {
      all_cnstr_cnt++;
      if (!(next_constr = cur_constr->next))
        last_constr = cur_constr;
    }

  /* checking for VIEW constraints (it can exist  *
   * if there is CHECK OPTION in VIEW definition) */
  if ((View = COR_VIEW (ScanNode)))
    {
      Constr_Info *constr_view;
      
      TASSERT (CODE_TRN (View) == VIEW, View);
      if (!CONSTR_INFO (View))
        {
          *(Constr_Info **) (&CONSTR_INFO (View)) =
            get_constraints (VIEW_UNID (View), tab_colno);
          if (!CONSTR_INFO (View))
            CONSTR_INFO (View) = 1;
        }
      if (CONSTR_INFO (View) != 1)
        {
          constr_view = *(Constr_Info **) (&CONSTR_INFO (View));
          for (cur_constr = constr_view; cur_constr;
               cur_constr = cur_constr->next)
            {
              all_cnstr_cnt++;
              assert (cur_constr->constr_type == CHECK);
              cur_constr->constr_type = VIEW;
            }
          
          if (last_constr)
            last_constr->next = constr_view;
          else
            constr_info = constr_view;
        }
    }
  
  CHECK_ARR_SIZE (ptrs, ptrs_cnt, all_cnstr_cnt, VADR);
  
  for (cur_constr = constr_info; cur_constr; cur_constr = cur_constr->next)
    {
      constr_need = FALSE;
      constr_type = cur_constr->constr_type;
      mod_cols = cur_constr->mod_cols;
      
      if (oper == UPDATE)
	for (i = 0; i < nm; i++)
          {
            if (mod_cols[mlist[i]])
              {/* this operation updates column from constraint */
                constr_need = TRUE;
                break;
              }
          }
      else
	if ( (oper == DELETE && constr_type == REFERENCE) ||
             (oper == INSERT && constr_type != REFERENCE))
	  constr_need = TRUE;
      
      if (constr_need)
	{
	  if (! (cur_constr->constr))
	    {
	      char *string = get_chconstr (cur_constr->conid, cur_constr->consize);
	      VADR constr_ptr, cur_seg, str_seg;
              TRNode *t0;
	      
	      assert (constr_type == CHECK || constr_type == VIEW);
	      constr_ptr = VMALLOC (1, S_Constraint);
	      cur_constr->constr = constr_ptr;
	      V_PTR (constr_ptr, S_Constraint)->constr_type = constr_type;
		
	      cur_seg = get_vm_segment_id (0);
	      str_seg = link_segment (string, cur_constr->consize);
              switch_to_segment (str_seg);
              t0 = V_PTR (*V_PTR (4, VADR), TRNode);
              switch_to_segment (cur_seg);
              V_PTR (constr_ptr, S_Constraint)->object = load_tree (t0, cur_seg, str_seg);
	    }
	  
	  ptrs[cur_constr_cnt++] = cur_constr->constr;
	  
	  /* checking what columns must be readed from *
	   * scan to check current constraint :        */
	  for (i = 0; i < tab_colno; i++)
	    if (read_fl[i] < mod_cols[i] &&
		(!(mlist_fl[i]) || constr_type == REFERENCE))
	      {
		read_cnt++;
		read_fl[i] = mod_cols[i];
	      }
	}
    } /* for: see all constraints */
  
  if (cur_constr_cnt)
    {
      P_VMALLOC (constr_arr, cur_constr_cnt, VADR);
      bcopy (ptrs, constr_arr, cur_constr_cnt * sizeof (VADR));
      header = V_PTR (V_header, S_ConstrHeader);
      header->constr_num = cur_constr_cnt;
      header->constr_arr = V_constr_arr;
    }

  /* if there aren't any constraints arrays with info      *
   * about old values may be needed for statistic handling */
  
  V_nums = V_types = VNULL;
  if (read_cnt)
    {
      P_VMALLOC (nums, read_cnt, i4_t);
      P_VMALLOC (types, read_cnt, sql_type_t );
      for (CurClm = TBL_COLS (Tab), j = 0; CurClm; CurClm = RIGHT_TRN (CurClm))
        if (read_fl[i = COL_NO (CurClm)])
          {
            types[j] = COL_TYPE (CurClm);
            nums[j++] = i;
          }
      header = V_PTR (V_header, S_ConstrHeader);
    }
  header->old_vl_cnt = read_cnt;
  header->old_vl_nums = V_nums;
  header->old_vl_types = V_types;
  
  return V_header;
} /* Pr_Constraint */

