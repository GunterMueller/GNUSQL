/*
 *  codegen.c - functions of synthesator 
 *              (last part of compilator: result module generation).
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

/* $Id: codegen.c,v 1.255 1998/09/29 00:39:41 kimelman Exp $ */
  
#include "global.h"
#include "syndef.h"
#include <assert.h>
#include "tassert.h"
#include "exti.h"
#include "inprtyp.h"
#include "const.h"
#include "kitty.h"

#define OPTIM           1
#define DROP_ARR_DELTA  5
#define SCAN_ARR_DELTA  5
#define SECT_ARR_DELTA  10
#define PROC_ARR_DELTA  5
#define TRPTR_ARR_DELTA 5

#define CHECK_TYPE_ARR(name)			\
{ if (!V_type_arr_##name) {			\
    sql_type_t *type_arr_##name;		\
    P_VMALLOC (type_arr_##name, name##_COLNO, sql_type_t); \
    bcopy (name##_COLTYPES, type_arr_##name,	\
           sizeof (sql_type_t) * name##_COLNO);	\
} }

extern data_unit_t *data_st;
extern i4_t   static_sql_fl;   /* == 0 for dynamic SQL */
extern i4_t  dt_ind, dt_max;
extern sql_type_t INDEXES_COLTYPES [INDEXES_COLNO];
extern sql_type_t CHCONSTR_COLTYPES [CHCONSTR_COLNO];
extern sql_type_t VIEWS_COLTYPES [VIEWS_COLNO];
extern sql_type_t CHCONSTRTWO_COLTYPES [CHCONSTRTWO_COLNO];
extern sql_type_t REFCONSTR_COLTYPES [REFCONSTR_COLNO];
extern sql_type_t TABLES_COLTYPES [TABLES_COLNO];
extern sql_type_t COLUMNS_COLTYPES [COLUMNS_COLNO];
extern sql_type_t TABAUTH_COLTYPES [TABAUTH_COLNO];
extern sql_type_t COLAUTH_COLTYPES [COLAUTH_COLNO];
VADR V_type_arr_INDEXES = VNULL;
VADR V_type_arr_CHCONSTR = VNULL;
VADR V_type_arr_VIEWS = VNULL;
VADR V_type_arr_CHCONSTRTWO = VNULL;
VADR V_type_arr_REFCONSTR = VNULL;
VADR V_type_arr_TABLES = VNULL;
VADR V_type_arr_COLUMNS = VNULL;
VADR V_type_arr_TABAUTH = VNULL;
VADR V_type_arr_COLAUTH = VNULL;

/* pointer to beginning, current & maximum *
   available pointers in the current page  */
VADR ProcAdr, CurPg, MaxPg;

/* if != NULL => it's the place where to   *
 * put address of command "END" in section */
VADR *FuncExit;
i4_t CurBit;			/* is used in operators */

i4_t PRCD_DEBUG = 0;
i4_t   *chconstr_col_use = NULL;
i4_t    ch_col_use_num;

/* pointer to beginning, current & maximum *
   available pointers in the current page  */
VADR ProcAdr, CurPg, MaxPg;

/* if != NULL => it's the place where to   *
 * put address of command "END" in section */
VADR *FuncExit;
i4_t CurBit;			/* is used in operators */

VADR *ProcTable; /* beginning of array of virtual       *
		  * addresses for additional procedures */
i4_t ProcMax = 0, ProcNum = 0;

VCBREF real_insert_scan = TNULL;
/* is used (!= TNULL) for operator INSERT */

VADR DelCurConstr = VNULL;
/* is used (!= VNULL) for DELETE CURRENT operator */

i4_t drop_max = 0;      /* current number of available elements in drop_arr */
i4_t drop_cnt = -1;      /* number of temporary tables in cursor          *
		        * this variable is used only if it != -1        */
VADR *drop_arr = NULL; /* array of addresses of Tabid for temp. tables  *
		        * in cursor (is being filled if drop_cnt != -1) */

i4_t scan_max = 0;      /* current number of available elements in scan_arr */
i4_t scan_cnt = -1;     /* number of scans in cursor -         *
		        * is used only if it != -1            */
VADR *scan_arr = NULL; /* array of addresses of scans         *
		        * (is being filled if scan_cnt != -1) */

SH_Info *sh_arr = NULL, *tr_arr = NULL;
i4_t tr_cnt = -1, tr_max = 0;		/* is used in operators */
i4_t sh_cnt = -1, sh_max = 0;		/* is used in operators */

i4_t uniqnm (void);

/*---------------------------------------------------------------------*/

VADR
codegen (void)
/* ROOT - pointer to entrance tree;                       *
 * Returns virtual address of section with module created */
{
  TXTREF statement;
  VADR SegBeg, SecTabOff;
  VADR *table_of_sections; /* beginning of array of virtual  *
		    * addresses for basic procedures */
  VADR *SectArr = NULL, V_table_of_sections;
  i4_t SectMax = 0, SectNum = 0; /* !! First segment number is 1 !! */
  MASK Depend = 0;
  
  if (!(statement = ROOT))
    return VNULL;
  
  InitDat;
/* work with segment initialization : */
  SegBeg = create_segment ();
  if ((SecTabOff = vmalloc (4)) != 4)
    yyfatal ("First vmalloc(4) doesn't return 4 !!");

  ProcAdr = VNULL;
  CurPg = VNULL;
  MaxPg = VNULL;
  FuncExit = NULL;
  V_type_arr_INDEXES = VNULL;
  V_type_arr_CHCONSTR = VNULL;
  V_type_arr_VIEWS = VNULL;
  V_type_arr_CHCONSTRTWO = VNULL;
  V_type_arr_REFCONSTR = VNULL;
  V_type_arr_TABLES = VNULL;
  V_type_arr_COLUMNS = VNULL;
  V_type_arr_TABAUTH = VNULL;
  V_type_arr_COLAUTH = VNULL;

  do
    {
      CurBit = 1;
      tr_cnt = 0;
      sh_cnt = 0;
      
      SectNum++;
      CHECK_ARR_ELEM (SectArr, SectMax, SectNum, SECT_ARR_DELTA, VADR);

      SectArr[SectNum] = ProcHandle (statement, VNULL, TNULL,
				     SectNum, &Depend, NULL, NULL);
      assert (!Depend);
    }
  while ((statement = RIGHT_TRN (statement)) != TNULL);

  P_VMALLOC (table_of_sections, SectNum + ProcNum + 2, VADR);
  TYP_COPY (SectArr, table_of_sections, SectNum + 1, VADR);
  if (ProcNum)
    {
      TYP_COPY (ProcTable, table_of_sections + SectNum + 1, ProcNum, VADR);
      xfree (ProcTable);
      ProcTable = NULL;
    }
  assert(ProcTable==NULL);
  xfree (SectArr);
  
  /* in the beginning of array table_of_sections is its length  *
   * the pointer to table_of_sections is in virtual address 4   */

  *((i4_t*)&table_of_sections[0]) = SectNum;
  *V_PTR (SecTabOff, VADR) = V_table_of_sections;

  return SegBeg;
}				/* GlobHandle */

/*---------------------------------------------------------------------*/

VADR
ProcHandle (TXTREF BegNode, VADR BO_Tid, VCBREF ResScan,
	    i2_t ProcN, MASK *Depend, float *sq_cost, i4_t *res_row_num)

/* procedure designing: for basic procedure ProcN >=1   *
 * & for auxiliary procedure ProcN = 0.                 *
 * Returns virtual address of procedure beginning       *
 * To Depend are putted MASK bits for all external      *
 * tables in current procedure.                         *
 * If sq_cost != NULL & BegNode - SubQuery => the       *
 * estimated cost of this SubQuery is putted to sq_cost */
{
  VADR OldProcAdr = ProcAdr, OldCurPg = CurPg, OldMaxPg = MaxPg;
  VADR BegPg, V_SetH = VNULL;
  i4_t i, j, trptr_cnt;
  i4_t Old_drop_cnt = drop_cnt, Old_scan_cnt = scan_cnt;
  i4_t Old_drop_max = drop_max, Old_scan_max = scan_max;
  VADR *Old_drop_arr = drop_arr, *Old_scan_arr = scan_arr;
  MASK CurMask, ExtDep, IntDep = 0;
  static PSECT_PTR *trptr_arr = NULL;
  static i4_t trptr_max = 0;
  
  drop_arr = scan_arr = VNULL; /* kml */
  drop_cnt = scan_cnt = -1;
  drop_max = scan_max = 0;

  /* CurPg is always suitable for type "i4_t" */
  ProcAdr = CurPg = BegPg = VMALLOC (INTERP_MODULE_DELTA, char);
  MaxPg = CurPg + INTERP_MODULE_DELTA;
  
  if (!ProcN)
    STEP_CMD (V_SetH, SetHandle);
  Handle (BegNode, BO_Tid, ResScan, sq_cost, res_row_num);

  SET_END;
  
  *Depend = 0;
  /* filling of SetHandle for trees with external references */
  if (!ProcN)
    {
#define SH  V_PTR (V_SetH, struct S_SetHandle)
      
      SH->flag = 'n';
      /* memory allocation for placing of tree pointers     *
       * (here we think that it is necessary for all trees) */
      SH->Bit = 0;
      
      for (i = 0; i < sh_cnt; i++)
	if (sh_arr[i].PrAdr == ProcAdr)
	  IntDep = BitOR (IntDep, sh_arr[i].Depend);
      /* IntDep contains bits for all internal tables for this procedure now */
      
      IntDep = BitNOT (IntDep);
      for (j = 0, trptr_cnt = 0; j < tr_cnt; j++)
	if ((tr_arr[j].PrAdr == ProcAdr) &&
	    (ExtDep = BitAND (IntDep, tr_arr[j].Depend)))
	  /* Putting of bits for external tables       *
	   * of j-th tree in the procedure to Depend : */
	  { 
	    *Depend = BitOR (*Depend, ExtDep);
            CHECK_ARR_ELEM (trptr_arr, trptr_max, trptr_cnt,
                            TRPTR_ARR_DELTA, PSECT_PTR);
            
	    trptr_arr[trptr_cnt++].off = tr_arr[j].Place;
	  }
      
      SH->TrCnt = trptr_cnt;
      if (trptr_cnt)
        {
          SH->Trees.off = VMALLOC (trptr_cnt, PSECT_PTR);
          TYP_COPY (trptr_arr, V_PTR (SH->Trees.off, PSECT_PTR),
                    trptr_cnt, PSECT_PTR);
        }
      
#undef SH      
    }

  /* making of all commands SetHandle in current procedure */
  for (i = 0; i < sh_cnt; i++)
    if (sh_arr[i].PrAdr == ProcAdr)
      {  
#define SH  V_PTR (sh_arr[i].Place, struct S_SetHandle)
	
	CurMask = sh_arr[i].Depend;
	SH->flag = 'n';
	SH->Bit = CurMask;
	
	for (j = 0, trptr_cnt = 0; j < tr_cnt; j++)
	  if ((BitAND (tr_arr[j].Depend, CurMask) == CurMask) &&
	      (tr_arr[j].PrAdr == ProcAdr))
            {
              CHECK_ARR_ELEM (trptr_arr, trptr_max, trptr_cnt,
                              TRPTR_ARR_DELTA, PSECT_PTR);
              trptr_arr[trptr_cnt++].off = tr_arr[j].Place;
            }
	SH->TrCnt = trptr_cnt;
        if (trptr_cnt)
          {
            SH->Trees.off = VMALLOC (trptr_cnt, PSECT_PTR);
            TYP_COPY (trptr_arr, V_PTR (SH->Trees.off, PSECT_PTR),
                      trptr_cnt, PSECT_PTR);
          }
	
#undef SH      
      }
  /* Bits for all external tables used in procedure are now in Depend */
  
  xfree (drop_arr);
  xfree (scan_arr);
  scan_cnt = Old_scan_cnt;
  scan_max = Old_scan_max;
  scan_arr = Old_scan_arr;
  drop_cnt = Old_drop_cnt;
  drop_max = Old_drop_max;
  drop_arr = Old_drop_arr;
  ProcAdr  = OldProcAdr;
  CurPg    = OldCurPg;
  MaxPg    = OldMaxPg;
  
  /* additional procedure registration */
  if (!ProcN)
    {
      CHECK_ARR_ELEM (ProcTable, ProcMax, ProcNum, PROC_ARR_DELTA, VADR);
      ProcTable[ProcNum++] = BegPg;
    }

  return BegPg;
}				/* ProcHandle */

/*---------------------------------------------------------------------*/

VADR
ScanHandle (VCBREF ScanNode, VADR * Scan, MASK * Msk, float *sq_cost, i4_t *res_row_num)
/* returns address of Tabid for table                        *
 *                 or NULL if Scanid was already filled      *
 * if Scan!=NULL && *Scan == NULL => address of Scanid is    *
 * putting to Scan. This putted address may be = VNULL if    *
 * there is not scanning table : result is a single row      *
 * if Msk!=NULL, mask for table is creating & pitting to Msk */
{
  VCBREF TblNode, CurClm;
  TXTREF TabQuery;
  VADR Tbid = VNULL;
  VADR V_sc;
  Scanid *sc;
  i4_t res = 0;
  char need_tbid = (!Scan || (*Scan == VNULL));
  
  TASSERT(ScanNode && (CODE_TRN (ScanNode) == SCAN),ScanNode);
  TblNode = COR_TBL (ScanNode);
  Tbid = COR_TID (ScanNode);
  
  if (!Tbid)
    {
      COR_TID (ScanNode) = Tbid = VMALLOC (1, UnId);
      if (CODE_TRN (TblNode) == TMPTABLE)
	{
	  TabQuery = VIEW_QUERY (TblNode);
          switch (CODE_TRN (TabQuery))
            {
            case SORTER:
            case MAKEGROUP:
              break;
              
            case UNION:
              TASSERT (TstF_TRN (TabQuery, ALL_F), TabQuery);
            case DELETE:
            case QUERY:
	      Pr_TMPCRE (Tbid, (TBL_COLS (TblNode)) ? TBL_COLS (TblNode) :
			 COR_COLUMNS (ScanNode), TBL_NNULCOL (TblNode));
	      if (drop_cnt >= 0)
		{
		  CHECK_ARR_ELEM (drop_arr, drop_max, drop_cnt, DROP_ARR_DELTA, VADR);
		  drop_arr[drop_cnt++] = Tbid;
		}
              break;
            default:
              lperror("Unexpected code (%s) of table expression",
                      NAME(CODE_TRN(TabQuery))); 
	    }
          
	  res = Handle (TabQuery, Tbid, ScanNode,
			&BUILD_COST (TblNode), &NROWS_EST (TblNode));
	  /* res == 1 <==> there is not scanning table : *
	   * result is a single row. This situation is   *
	   * fixed in COR_TID(ScanNode) :                */
	  if (res)
	    {
	      vmfree (Tbid);
	      COR_TID (ScanNode) = Tbid = 1;
	    }
	}
      else
	{
	  TASSERT (CODE_TRN (TblNode) == TABLE, ScanNode);
	  V_PTR (Tbid, UnId)->t = TBL_TABID (TblNode);
	  for (CurClm = COR_COLUMNS (ScanNode); CurClm; CurClm = COL_NEXT (CurClm))
	    if (!COL_ADR (CurClm))
	      COL_ADR (CurClm) = prepare_HOLE (COL_TYPE (CurClm));
  
	}
    }
      
  if (CODE_TRN (TblNode) == TMPTABLE)
    {
      if (sq_cost)
	*sq_cost = BUILD_COST (TblNode);
      if (res_row_num)
	*res_row_num = NROWS_EST (TblNode);
    }
      
  if (Msk)
    {
      if (!COR_MASK (ScanNode))
	COR_MASK (ScanNode) = NEXT_BIT;
      *Msk = COR_MASK (ScanNode);
    }
  
  if (Tbid == 1)
    {
      if (Scan && !(*Scan))
	*Scan = VNULL;
      return VNULL;
    }
  else
    if (Scan && !(*Scan))
      {
	if (!COR_SCANID (ScanNode))
	  {
	    P_VMALLOC (sc, 1, Scanid);
	    if (scan_cnt >= 0)
	      {
		CHECK_ARR_ELEM (scan_arr, scan_max, scan_cnt, SCAN_ARR_DELTA, VADR);
		scan_arr[scan_cnt++] = V_sc;
	      }
	    COR_SCANID (ScanNode) = V_sc;
	    *sc = -1; /* value before scan opening */
	  }
	*Scan = COR_SCANID (ScanNode);
      }
  
  return (need_tbid) ? Tbid : VNULL;
}				/* ScanHandle */

/*---------------------------------------------------------------------*/

VADR
CrInd (i4_t ind_num, char *ind_typ, VADR V_Tbd, VADR V_tabname,
       VADR V_owner, VCBREF Node, i4_t clmcnt)
/* handling of index creation for table with Tabid Tbd. *
 * (this index has number ind_num)                      *
 * Information for creation is from Node.               *
 * Returns virtual address of unindid of created index. */
{
    VCBREF Cur;
    VADR V_inxname, V_inxtype = VNULL, V_colno;
    VADR V_credate, V_cretime, V_ncol;
    VADR V_clmlist, V_indid;
    char *inxname, *inxtype;
    i2_t *colno;
    i2_t *ncol;
    VADR *colval, V_colval;
    UnId *indid, *sysind;
    VADR V_sysind;
    i4_t i, *clmlist, unindid_off;
    
    assert(clmcnt<=8);
    /* it may not be more than 8 columns for index */ 
    
    P_VMALLOC (sysind, 1, UnId);
    sysind->t=sysindtabid;
    
    P_VMALLOC (clmlist, clmcnt, i4_t);
    for (Cur=Node, i=0; Cur; i++, Cur=RIGHT_TRN(Cur))
	clmlist[i]=COL_NO( OBJ_DESC (Cur));
    
    P_VMALLOC (indid, 1, UnId);
    unindid_off = (char *)( &(indid->i.unindid)) - (char *)indid;
    
    Pr_CREIND (V_Tbd, V_indid, (ind_typ?1:0), clmcnt, V_clmlist);
    
    P_VMALLOC (inxname, 24, char);
    sprintf(inxname,"%sIND%d", V_PTR (V_tabname, char), ind_num);
    
    V_credate = VMALLOC (9, char);
    V_cretime = VMALLOC (9, char);
    P_VMALLOC (ncol, 1, i2_t);
    *ncol = clmcnt;
    
    V_colno = VMALLOC (clmcnt, i2_t);
    if (ind_typ)
      {
	P_VMALLOC (inxtype, 3, char);
	strncpy (inxtype, ind_typ, 2);
	inxtype[2] = 0;
      }

    /* adding record about index into SYSINDEXES */

    P_VMALLOC (colval, 16, VADR);
    
    colval[0] = V_Tbd;
    colval[1] = V_inxname;
    colval[2] = V_owner;
    colval[3] = (ind_typ) ? V_inxtype : VNULL;
    colval[4] = V_ncol;
    colval[13] = V_credate;
    colval[14] = V_cretime;
    colval[15] = V_indid + unindid_off;
    
    colno   = V_PTR (V_colno, i2_t);
    clmlist = V_PTR (V_clmlist, i4_t);
    for (i=0; i<clmcnt; i++)
      {
	colno[i] = clmlist[i];
	colval[5+i] = V_colno + i * SZ_SHT;
      }
    for (i=clmcnt+5; i<13; i++)
      colval[i]=VNULL;
    
    CHECK_TYPE_ARR (INDEXES);
    Pr_INSLIST (V_sysind, 16, V_type_arr_INDEXES, 
		VNULL, V_colval, V_credate, V_cretime);
    return colval[15]; /* address of unindid */
} /* CrInd */

void
make_str_parts (VADR V_id, i4_t is_cre_tab, char *seg_ptr, i4_t seg_lng)
/* preparing of table SYSCHCONSTRTWO (is_cre_tab == TRUE) *
   or SYSVIEWS (is_cre_tab == FALSE) filling              */
{
  i4_t *fragsize, i, rest_len;
  i2_t *fragno;
  char *end_ptr, *cur_ptr, *frag;
  VADR V_fragno, V_fragsize;
  VADR *colval, V_colval, V_len, V_into_tab, V_frag;
  UnId *into_tab;
  i2_t *len;

  P_VMALLOC (into_tab, 1, UnId);
  into_tab->t = (is_cre_tab == TRUE) ? chcontwotabid : viewstabid;
  
  end_ptr = seg_ptr + seg_lng;
  for (i = 0, cur_ptr = seg_ptr; cur_ptr < end_ptr; i++, cur_ptr += *fragsize)
    {
      rest_len = end_ptr - cur_ptr;
      P_VMALLOC (fragno, 1, i2_t);
      *fragno = i;
      
      P_VMALLOC (fragsize, 1, i4_t);
      *fragsize = (rest_len < MAX_STR_LNG) ? rest_len : MAX_STR_LNG;
      
      P_VMALLOC (frag, *fragsize, char);
      bcopy (cur_ptr, frag, *fragsize);
      
      P_VMALLOC (len, 4, i2_t);
      len[3] = *fragsize;
  
      P_VMALLOC (colval, 4, VADR);
      colval[0] = V_id;
      colval[1] = V_fragno;
      colval[2] = V_fragsize;
      colval[3] = V_frag;

      if (is_cre_tab == TRUE) /* table creation */
        CHECK_TYPE_ARR (CHCONSTRTWO)
      else                    /* view  creation */
        CHECK_TYPE_ARR (VIEWS);
        
      Pr_INSLIST (V_into_tab, 4, (is_cre_tab == TRUE) ?
                  V_type_arr_CHCONSTRTWO : V_type_arr_VIEWS,
                  V_len, V_colval, VNULL, VNULL);
    }
} /* make_str_parts */

/*---------------------------------------------------------------------*/

void
CrCheckConstr (VADR V_Tbd, i4_t tbl_col_num, TXTREF tree)
{
  i4_t *chconid, *seg_size, i, j;
  i2_t *ncols, *colno;
  VADR V_ncols, V_chconid, V_seg_size, V_cur_colno, V_colno;
  VADR cur_seg, seg, tree_ptr;
  MASK Depend;
  char *seg_ptr;
  VADR *colval, V_colval, V_syschconstr;
  UnId *syschconstr;
  
  P_VMALLOC (syschconstr, 1, UnId);
  syschconstr->t = chconstrtabid;
  
  P_VMALLOC (chconid, 1, i4_t);
  *chconid = uniqnm ();
  
  chconstr_col_use = TYP_ALLOC (tbl_col_num, i4_t);
  ch_col_use_num = 0;
  
  /* VM-segment with tree (check condition) creating */
  
  cur_seg = get_vm_segment_id (0);
  seg = create_segment ();
  switch_to_segment (seg);
  
  if ((tree_ptr = vmalloc (4)) != 4)
    yyfatal ("First vmalloc(4) doesn't return 4 !!");
  *V_PTR (tree_ptr, VADR) = Tr_RecLoad (tree, &Depend, 0, NULL);
  /* in new segment in virtual address 4 there is now VADR for tree beginning */
  switch_to_segment (cur_seg);

  P_VMALLOC (ncols, 1, i2_t);
  assert ((*ncols = ch_col_use_num) <= 8);
  
  P_VMALLOC (seg_size, 1, i4_t);
  seg_ptr = (char *) export_segment (seg, seg_size, 1);
  assert (seg_ptr && *seg_size);
  
  /* preparing of table SYSCHCONSTR filling : */
  
  P_VMALLOC (colval, 12, VADR);
  colval[0] = V_Tbd;
  colval[1] = V_chconid;
  colval[2] = V_seg_size;
  colval[3] = V_ncols;
  
  P_VMALLOC (colno, *ncols, i2_t);
  for (i = 0, j = 4, V_cur_colno = V_colno; i < tbl_col_num; i++)
    if (chconstr_col_use [i])
      {
	colno[j-4] = i;
	colval[j++] = V_cur_colno;
	V_cur_colno += SZ_SHT;
      }
  
  for (j = 4 + ch_col_use_num; j < 12; j++)
    colval[j] = VNULL;
  
  CHECK_TYPE_ARR (CHCONSTR);
  Pr_INSLIST (V_syschconstr, 12, V_type_arr_CHCONSTR,
	      VNULL, V_colval, VNULL, VNULL);
  
  /* preparing of table SYSCHCONSTRTWO filling */
  make_str_parts (V_chconid, TRUE, seg_ptr, *seg_size);
  
  xfree (chconstr_col_use);
  chconstr_col_use = NULL;
}
/* CrCheckConstr */

/*---------------------------------------------------------------------*/

void
CrRefConstr (VADR V_Tbd, i2_t colno, TXTREF BegClmFrom, TXTREF BegClmTo)
{
  VADR V_tabid_to, V_ind_to, V_sysrc;
  VADR *colval;
  i2_t *num_to, *num_from;
  VADR V_colval, V_num_to, V_num_from, V_cur_num;
  i2_t *ncols;
  VADR V_ncols;
  UnId *sysrc;
  i4_t i, j, *tabid_to, *ind_to;
  TXTREF CurClm, tbl_to;
  
  P_VMALLOC (sysrc, 1, UnId);
  sysrc->t=refconstrtabid;
  
  P_VMALLOC (ncols, 1, i2_t);
  *ncols = colno;
  
  V_num_to = VMALLOC (colno, i2_t);
  V_num_from = VMALLOC (colno, i2_t);
  
  tbl_to = COL_TBL (OBJ_DESC (BegClmTo));
  if ((TBL_TABID (tbl_to)).untabid)
    {
      /* this table already exists => we have Tabid */
      P_VMALLOC (tabid_to, 1, i4_t);
      *tabid_to = (TBL_TABID (tbl_to)).untabid;
    }
  else /* table from the same schema => Tabid is not created yet */
    V_tabid_to = TBL_VADR (tbl_to);
  P_VMALLOC (ind_to, 1, i4_t);
  *ind_to = 0;
  
  P_VMALLOC (colval, 20, VADR);
  
  colval[0] = V_Tbd;
  colval[1] = V_tabid_to;
  colval[2] = V_ind_to;
  colval[3] = V_ncols;
  
  num_from = V_PTR (V_num_from, i2_t);
  for (i = 4, CurClm = BegClmFrom, V_cur_num = V_num_from;
       i < colno + 4; i++, CurClm = RIGHT_TRN (CurClm))
    {
      TASSERT (CODE_TRN (CurClm) == COLPTR, CurClm);
      
      num_from[i-4] = COL_NO (OBJ_DESC (CurClm));
      colval[i] = V_cur_num;
      V_cur_num += SZ_SHT;
    }
  
  num_to = V_PTR (V_num_to, i2_t);
  for (i = 12, CurClm = BegClmTo, V_cur_num = V_num_to;
       i < colno + 12; i++, CurClm = RIGHT_TRN (CurClm))
    {
      TASSERT (CODE_TRN (CurClm) == COLPTR, CurClm);
      
      num_to[i-12] = COL_NO (OBJ_DESC (CurClm));
      colval[i] = V_cur_num;
      V_cur_num += SZ_SHT;
    }
  for (i = colno + 4, j = colno + 12; i < 12; i++, j++)
    colval[i] = colval[j] = VNULL;
  
  CHECK_TYPE_ARR (REFCONSTR);
  Pr_INSLIST (V_sysrc, 20, V_type_arr_REFCONSTR,
	      VNULL, V_colval, VNULL, VNULL);
} /* CrRefConstr */

/*---------------------------------------------------------------------*/

void
CrTab (VCBREF Tab)
/* handling of table or view creation */
{
  i4_t i, *nncolnum, colnum = TBL_NCOLS (Tab);
  VCBREF FirClm = TBL_COLS (Tab), Nd;
  VCBREF CurClm, Constr, NextEl, PredEl, Obj, Obj_rh, DefNd;
  char *ind_flag = GET_MEMC (char, colnum)
    /* ind_flag[i] = flag of index existing for only this (i) column */
    i4_t ind_cnt = 0;
  UnId *systab,*syscol;
  VADR V_systab, V_syscol, V_primindid=VNULL;
  VADR V_tabname, V_owner, V_defval, resindid, V_Tbd;
  VADR V_colval, V_tabtype, V_credate, V_cretime;
  VADR V_ncols, V_nrows, V_colname, V_colno, V_nncolnum;
  VADR V_coltype, V_coltype1, V_coltype2, V_defnull;
  VADR *colval;
  char *tabname, *owner, *defval;
  char *tabtype, *def_val_ptr;
  char *colname, *coltype, *defnull;
  i2_t *ncols, *colno;
  i4_t *nrows, *coltype1, *coltype2;
  i4_t ow_leng;
  i4_t prim_exist = 0;
  Tabid tbd;
  static i4_t nnul_arr_lng = 0;
  static char *nnul_fl = NULL;
  i4_t is_cre_tab = (CODE_TRN (Tab) == TABLE);

  assert ((V_Tbd = TBL_VADR (Tab)));
  CHECK_ARR_SIZE (nnul_fl, nnul_arr_lng, colnum, char);
  bzero (nnul_fl, colnum);
  
  P_VMALLOC (systab, 1, UnId);
  systab->t=systabtabid;
  P_VMALLOC (syscol, 1, UnId);
  syscol->t=syscoltabid;
  
  ow_leng = strlen (STRING (TBL_NAME (Tab)));
  if (ow_leng > MAX_USER_NAME_LNG)
    {
      lperror("Table name '%s' is too long",STRING (TBL_NAME (Tab)));
      errors--;
      ow_leng = MAX_USER_NAME_LNG;
    }
  P_VMALLOC (tabname, ow_leng+1, char);
  bcopy (STRING (TBL_NAME (Tab)), tabname, ow_leng);
  tabname[ow_leng] = 0;
  
  ow_leng = strlen (STRING (TBL_FNAME (Tab)));
  if (ow_leng > MAX_USER_NAME_LNG)
    {
      lperror("Authorization name '%s' is too long",
              STRING (TBL_FNAME (Tab))); 
      errors--;
      ow_leng = MAX_USER_NAME_LNG;
    }
  P_VMALLOC (owner, ow_leng+1, char);
  bcopy (STRING (TBL_FNAME (Tab)), owner, ow_leng);
  owner[ow_leng] = 0;

  P_VMALLOC (tabtype, 2, char);
  tabtype[0] = (is_cre_tab) ? 'B' : 'V';
  tabtype[1] = 0;
  
  V_credate = VMALLOC (9, char);
  V_cretime = VMALLOC (9, char);
  
  P_VMALLOC (ncols, 1, i2_t);
  *ncols = colnum;
  
  P_VMALLOC (nrows, 1, i4_t);
  *nrows = 0;
  
  P_VMALLOC (nncolnum, 1, i4_t);

  if (is_cre_tab) /* for CREATE TABLE */
    {
      /* finding the number of the beginning columns with NOT NULL predicate */
      for (Constr = TBL_CONSTR (Tab); Constr; Constr = RIGHT_TRN (Constr))
        if ((CODE_TRN (Constr) == CHECK) &&
            ( (CODE_TRN (Nd = (DOWN_TRN (Constr))) == ISNOTNULL) ||
              ( (CODE_TRN (DOWN_TRN (Constr)) == NOT) &&
                (CODE_TRN (Nd = DOWN_TRN (DOWN_TRN (Constr))) == ISNULL) ) )
            )
          {
            TASSERT (CODE_TRN (Nd = DOWN_TRN (Nd)) == COLPTR, Nd);
            nnul_fl[COL_NO (OBJ_DESC (Nd))] = TRUE;
          }
      for (*nncolnum = 0; *nncolnum < colnum; (*nncolnum)++)
        if (!nnul_fl[*nncolnum])
          break;
  
      /* removing NOT NULL predicates for first *nncolnum columns */
      for (Constr = TBL_CONSTR (Tab), PredEl = TNULL; Constr; Constr = NextEl)
        {
          NextEl = RIGHT_TRN (Constr);
          if ((CODE_TRN (Constr) == CHECK) &&
              ( (CODE_TRN (Nd = (DOWN_TRN (Constr))) == ISNOTNULL) ||
                ( (CODE_TRN (DOWN_TRN (Constr)) == NOT) &&
                  (CODE_TRN (Nd = DOWN_TRN (DOWN_TRN (Constr))) == ISNULL) ) ) &&
              (COL_NO (OBJ_DESC (Nd = DOWN_TRN (Nd))) < *nncolnum))
            {
              if (PredEl)
                RIGHT_TRN (PredEl) = NextEl;
              else
                TBL_CONSTR (Tab) = NextEl;
              RIGHT_TRN (Constr) = TNULL;
              free_tree (Constr);
            }
          else
            PredEl = Constr;
        }
  
      Pr_CRETAB (V_tabname, V_owner, 1, colnum, *nncolnum, FirClm, V_Tbd);
  
      ind_cnt=0;
      /* reorder indexes */
      {
        TXTREF prim,uniq,ind;
        TXTREF priml,uniql,indl;
        prim = uniq = ind = TNULL;
        priml = uniql = indl = TNULL;
        for (Constr = IND_INFO (Tab); Constr; Constr = RIGHT_TRN (Constr))
          switch (CODE_TRN (Constr))
            {
            case PRIMARY : 
              if (prim) priml = RIGHT_TRN(priml) = Constr ;
              else      priml = prim = Constr ;
              break;
            case UNIQUE  :
              if (uniq) uniql = RIGHT_TRN(uniql) = Constr ;
              else      uniql = uniq = Constr ;
              break;
            case INDEX   :
              if (ind)  indl = RIGHT_TRN(indl) = Constr ;
              else      indl = ind = Constr ;
              break;
            default :
              debug_trn(Constr);
              yyfatal("Unexpected index code");
            }
        if (indl)   RIGHT_TRN(indl)  = TNULL;
        if (uniql)  RIGHT_TRN(uniql) = ind;
        else uniq = ind;
        if (priml)  RIGHT_TRN(priml) = uniq;
        else prim = uniq; 
        IND_INFO (Tab) = prim;
      }

      /* indexes creation */
      for (Constr = IND_INFO (Tab); Constr; Constr = RIGHT_TRN (Constr))
        {
          char *ind_t = NULL;
          switch (CODE_TRN (Constr))
            {
            case PRIMARY : ind_t = "P"; break;
            case UNIQUE  : ind_t = "U"; break;
            case INDEX   :              continue;
              
            default :
              debug_trn(Constr);
              yyfatal("Unexpected index code");
            } /* switch: CODE_TRN (Constr) */
          assert( ind_t ); /* ununique index is not carefully tested yet */
          Obj = DOWN_TRN (Constr);
          if ((CODE_TRN (Obj) == COLPTR) && (!RIGHT_TRN (Obj)) &&
              ( ind_flag[COL_NO (OBJ_DESC (Obj))]++ ))
            continue; /* such index already exist */
          resindid = CrInd (++ind_cnt, ind_t, V_Tbd, V_tabname,
                            V_owner, Obj, ARITY_TRN(Constr));
          if (CODE_TRN (Constr) == PRIMARY)
            {
              V_primindid = resindid;
              prim_exist = 1;
            }
        }
      /* ind_cnt = count of indexes for table */
  
      for (Constr = TBL_CONSTR (Tab); Constr; Constr = RIGHT_TRN (Constr))
        switch (CODE_TRN (Constr))
          {
          case FOREIGN :
            /* (Foreign {
             *    (LocalList { (ColPtr ..) ...})
             *    (OPtr { 
             *      (Uniqie|Primary { (ColPtr ..) ...})
             *    })
             * }) 
             */
            Obj = DOWN_TRN (Constr);
            TASSERT (Obj && CODE_TRN (Obj) == LOCALLIST, Constr);
            Obj_rh = RIGHT_TRN (Obj);
            TASSERT (Obj_rh && CODE_TRN (Obj_rh) == OPTR, Constr);
            CrRefConstr (V_Tbd, ARITY_TRN (Obj),
                         DOWN_TRN (Obj), DOWN_TRN(OBJ_DESC(Obj_rh)));
            break;
	  
          case CHECK :
            CrCheckConstr (V_Tbd, TBL_NCOLS(Tab), DOWN_TRN (Constr));
            break;
	  
          default :
            debug_trn(Constr);
            yyfatal("Unexpected constraint code");
          } /* switch: CODE_TRN (Constr) */
    } /* if (is_cre_tab) */
  else /* for CREATE VIEW */
    {
      i4_t seg_size;
      TXTREF sav_segm, tr_segm, tree, lvcb = LOCAL_VCB_ROOT,
        query = TNULL, where;
      VADR   code_segm;
      char *seg_ptr;

      LOCAL_VCB_ROOT = TNULL;
      TASSERT (CODE_TRN (Tab) == VIEW && (query = VIEW_QUERY (Tab)), Tab);
      
      /*        Where    Selection    From    Query   */
      where = RIGHT_TRN (RIGHT_TRN (DOWN_TRN (query)));
      if (where)
        TASSERT (CODE_TRN (where) == WHERE, query);
      if (TstF_TRN (Tab, CHECK_OPT_F) && where)
        CrCheckConstr (V_Tbd,
                       /*          Table      Scan     TblPtr    From     Query    */
                       TBL_NCOLS (COR_TBL (TABL_DESC (DOWN_TRN (DOWN_TRN (query))))),
                       DOWN_TRN (where));
        
      code_segm = GET_CURRENT_SEGMENT_ID;
      sav_segm = get_current_tree_segment ();
      tr_segm  = create_tree_segment ();
      tree = copy_tree (query);
      register_export_address (tree, "VIEW_TREE");
         
      seg_ptr = extract_tree_segment (tr_segm, &seg_size);
      assert (seg_ptr && seg_size);
      set_current_tree_segment (sav_segm);
      switch_to_segment(code_segm);
      LOCAL_VCB_ROOT = lvcb;
      /* preparing of table SYSVIEWS filling */
      make_str_parts (V_Tbd, FALSE, seg_ptr, seg_size);
    }
  
  for (i=0; i < colnum; i++)
    ind_flag[i]=0;

  /* adding record about table into SYSTABLES */
  P_VMALLOC (colval, 13, VADR);
  
  colval[0] = V_tabname;
  colval[1] = V_owner;
  colval[2] = V_Tbd;
  colval[3] = (is_cre_tab) ?
    V_Tbd+((char *)(&tbd.segid) - (char *)(&tbd)) : VNULL;
  colval[4] = (is_cre_tab) ?
    V_Tbd+((char *)(&tbd.tabd) - (char *)(&tbd)) : VNULL;
  colval[5] = VNULL;
  colval[6] = (prim_exist) ? V_primindid : VNULL;
  colval[7] = V_tabtype;
  colval[8] = V_credate;
  colval[9] = V_cretime;
  colval[10] = V_ncols;
  colval[11] = V_nrows;
  colval[12] = V_nncolnum;
    
  CHECK_TYPE_ARR (TABLES);
  Pr_INSLIST (V_systab, 13, V_type_arr_TABLES,
	      VNULL, V_colval, V_credate, V_cretime);

  /* insertion information about table  *
   * columns into the table  SYSCOLUMNS */

  CurClm = FirClm;
  for (i = 0; i < colnum; i++, CurClm = COL_NEXT (CurClm))
    {
      if (strlen (STRING (COL_NAME (CurClm))) >= MAX_USER_NAME_LNG)
	yyfatal ("Length of column name is too big");
      P_VMALLOC (colname, MAX_USER_NAME_LNG, char);
      strcpy (colname, STRING (COL_NAME (CurClm)));
      
      P_VMALLOC (colno, 1, i2_t);
      *colno = COL_NO (CurClm);
      
      P_VMALLOC (coltype1, 1, i4_t);
      *coltype1 = (COL_TYPE (CurClm)).len;
      
      P_VMALLOC (coltype2, 1, i4_t);
      *coltype2 = (COL_TYPE (CurClm)).prec;
      
      P_VMALLOC (coltype, 2, char);
      coltype[0]  = (COL_TYPE (CurClm)).code;
      coltype[1]  = 0;
      
      P_VMALLOC (defnull, 2, char);
      defnull[0] = (nnul_fl[COL_NO (CurClm)]) ? '1' : '0';
      defnull[1] = 0;
      
      defval=NULL; V_defval=VNULL;
      if ((DefNd = COL_DEFAULT (CurClm)))
	{
	  if (CODE_TRN (DefNd) == CONST)
	    {
	      def_val_ptr = STRING (CNST_NAME (DefNd));
	      P_VMALLOC (defval, strlen(def_val_ptr)+1, char);
	      strcpy (defval, def_val_ptr);
	    }
	  else
	    if (CODE_TRN (DefNd) == USERNAME)
	      *V_PTR (V_coltype2, i4_t) = USERNAME;
	}
      
      P_VMALLOC (colval, 11, VADR);
      
      colval[0] = V_colname;
      colval[1] = V_Tbd;
      colval[2] = V_colno;
      colval[3] = V_coltype;
      colval[4] = V_coltype1;
      colval[5] = V_coltype2;
      colval[6] = (V_defval) ? V_defval : VNULL;
      colval[7] = V_defnull;
      colval[8] = VNULL;
      colval[9] = VNULL;
      colval[10] = VNULL;

      CHECK_TYPE_ARR (COLUMNS);
      Pr_INSLIST (V_syscol, 11, V_type_arr_COLUMNS,
		  VNULL, V_colval, VNULL, VNULL);
    }				/* for */
} /* CrTab */

void
GrantHandle (TXTREF GrNode)
{
  TXTREF Tbl_Ptr, Tbl, Priv, PrivCur, Grntee, GrCur;
  TXTREF FirUpdCol, FirRefCol;
  TXTREF UpdCur, RefCur;
  UnId *stabauth,*scolauth;
  VADR V_stabauth, V_scolauth;
  VADR V_untabid, V_chckauth = VNULL, V_tabauth;
  VADR V_colval, V_grantor, V_owner, V_colno; 
  VADR V_updcols = VNULL, V_refcols = VNULL, V_ur, V_grntee_nm;
  VADR *colval, u_r_ur[3];
  i2_t *colno;
  i4_t *untabid, updcnt = 0, refcnt = 0, col_max = -1, grntee_len;
  /* col_max - the biggest column's number in lists of UPDATE & REFERENCES */
  i4_t *updcols = NULL, *refcols = NULL, gr_len = strlen (STRING (GL_AUTHOR));
  i4_t i;
  char *owner;
  char auth[9], *chckauth, *tabauth, *grantor, *grntee_nm, n_own_fl;
  char *col_fl = NULL, *ur; /* ur will be used for 2 chars : 'U' and 'R' *
                             * for columns authorisation coding          */
  auth[0]   = 0;
  chckauth  = NULL;
  FirUpdCol = TNULL;
  FirRefCol = TNULL;
  TASSERT ((Tbl_Ptr = DOWN_TRN (GrNode)) && 
	   (CODE_TRN (Tbl_Ptr) ==TBLPTR), GrNode);
  TASSERT ((Tbl = TABL_DESC (Tbl_Ptr)) && 
	   ((CODE_TRN (Tbl) == TABLE) || (CODE_TRN (Tbl) == VIEW)), GrNode);
  TASSERT ((Priv = RIGHT_TRN (Tbl_Ptr)) && 
	   (CODE_TRN (Priv) == PRIVILEGIES), GrNode);
  TASSERT ((Grntee = RIGHT_TRN (Priv)) && 
	   (CODE_TRN (Grntee) == GRANTEES), GrNode);
  
  /* grantor != table owner ? */
  P_VMALLOC (owner, strlen (STRING (TBL_FNAME (Tbl))) + 1, char);
  strcpy (owner, STRING(TBL_FNAME (Tbl)));
  n_own_fl = strcmp (STRING (GL_AUTHOR), owner);
  
  P_VMALLOC (stabauth, 1, UnId);
  stabauth->t=tabauthtabid;
  P_VMALLOC (scolauth, 1, UnId);
  scolauth->t=colauthtabid;
    
  P_VMALLOC (ur, 7, char);
  /* putting to ur : U\0R\0UR\0 */
  ur[1] = ur[3] = ur[6] = 0;
  ur[0] = ur[4] = 'U';
  ur[2] = ur[5] = 'R';
  u_r_ur[0] = V_ur;     /* pointer to "U"  */
  u_r_ur[1] = V_ur + 2; /* pointer to "R"  */
  u_r_ur[2] = V_ur + 4; /* pointer to "UR" */
  
  P_VMALLOC (grantor, gr_len + 1, char);
  strcpy (grantor, STRING (GL_AUTHOR));
  if (gr_len > MAX_USER_NAME_LNG)
    grantor[MAX_USER_NAME_LNG] = 0;
  if (n_own_fl)
    P_VMALLOC (chckauth, 9, char);
  
  {
    i4_t un_id;
    if ( CODE_TRN(Tbl)==TABLE )
      un_id = (TBL_TABID (Tbl)).untabid;
    else
      un_id = VIEW_UNID(Tbl);
    
    if (un_id)
      {
        /* this table already exists => we have Tabid */
        P_VMALLOC (untabid, 1, i4_t);
        *untabid = un_id;
      }
    else /* table from the same schema => Tabid is not created yet */
      V_untabid = TBL_VADR (Tbl);
  }
  
  for (PrivCur = DOWN_TRN (Priv); PrivCur; PrivCur = RIGHT_TRN (PrivCur))
    switch (CODE_TRN (PrivCur))
      {
      case SELECT :
	sprintf (auth, "%sS", auth);
	break;
	
      case INSERT :
	sprintf (auth, "%sI", auth);
	break;
	
      case DELETE :
	sprintf (auth, "%sD", auth);
	break;
	
      case UPDATE :
	updcnt = ARITY_TRN (PrivCur);
	if (updcnt)
	  FirUpdCol = DOWN_TRN (PrivCur);
	else
	  sprintf (auth, "%sU", auth);
	break;
	
      case REFERENCE :
	refcnt = ARITY_TRN (PrivCur);
	if (refcnt)
	  FirRefCol = DOWN_TRN (PrivCur);
	else
	  sprintf (auth, "%sR", auth);
	break;
	  
      default :
	yyfatal ("unrecognized privilege code");
      }
  
  if (n_own_fl)
    sprintf (chckauth, "%sG", auth);
  P_VMALLOC (tabauth, 9, char);
  tabauth[0] = (TstF_TRN (GrNode, GRANT_OPT_F)) ? 'G' : 0;
  tabauth[1] = 0;
  sprintf (tabauth, "%s%s", tabauth, auth);
  
  if (updcnt) /* there is a list of columns for UPDATE */
    {
      if (n_own_fl)
	strcat(chckauth, "U");      
      sprintf (tabauth,  "%su", tabauth);      
      P_VMALLOC (updcols, updcnt, i4_t);
      for (i = 0, UpdCur = FirUpdCol; UpdCur; 
				      UpdCur = RIGHT_TRN (UpdCur), i++)
	{
	  updcols[i] = COL_NO (OBJ_DESC (UpdCur));
	  if (col_max < updcols[i])
	    col_max = updcols[i];
	}
    }
  
  if (refcnt) /* there is a list of columns for REFERENCES */
    {
      if (n_own_fl)
	strcat (chckauth, "R");
      tabauth = V_PTR (V_tabauth, char);
      sprintf (tabauth,  "%sr", tabauth);      
      P_VMALLOC (refcols, refcnt, i4_t);
      for (i = 0, RefCur = FirRefCol; RefCur; 
				      RefCur = RIGHT_TRN (RefCur), i++)
	{
	  refcols[i] = COL_NO (OBJ_DESC (RefCur));
	  if (col_max < refcols[i])
	    col_max = refcols[i];
	}
    }
      
  /* Strings of privelegies' codes for rights checking (chckauth) *
   * and adding the row to SYSTABAUTH (tabauth) are complited.    */ 
  Pr_PRIVLG (V_untabid, V_owner, V_grantor, V_chckauth,
	     updcnt, V_updcols, refcnt, V_refcols);
  
  if (col_max+1)
    {
      col_fl = (char *) xmalloc (col_max+1);
      updcols = V_PTR (V_updcols, i4_t);
      for (i = 0; i < updcnt; i++)
	col_fl[updcols[i]] = 1;
      refcols = V_PTR (V_refcols, i4_t);
      for (i = 0; i < refcnt; i++)
	col_fl[refcols[i]] |= 2;
    }
  
  for (GrCur = DOWN_TRN (Grntee); GrCur; GrCur = RIGHT_TRN (GrCur))
    {
      TASSERT (CODE_TRN (GrCur) == USERNAME, Grntee);
      
      grntee_len = strlen (STRING(USR_NAME (GrCur)));
      
      P_VMALLOC (grntee_nm, grntee_len+1, char);
      strcpy (grntee_nm, STRING(USR_NAME (GrCur)));
      if (grntee_len > MAX_USER_NAME_LNG)
	grntee_nm[MAX_USER_NAME_LNG] = 0;
      
      P_VMALLOC (colval, 4, VADR);
      /* data preparation for adding row to SYSTABAUTH */ 
      colval[0] = V_untabid;
      colval[1] = V_grntee_nm;
      colval[2] = V_grantor;
      colval[3] = V_tabauth;
	  
      CHECK_TYPE_ARR (TABAUTH);
      Pr_INSLIST (V_stabauth, 4, V_type_arr_TABAUTH,
		  VNULL, V_colval, VNULL, VNULL);
	  
      /* data preparation for adding rows to SYSCOLAUTH */ 
      for (i = 0; i <= col_max; i++)
	{
	  if (!col_fl[i]) /* this column isn't included to any       *
			   * columns' list (for UPDATE & REFERENCES) */
	    continue;
	  P_VMALLOC (colno, 1, i2_t);
	  *colno = i;
	  
	  P_VMALLOC (colval, 5, VADR);
	  colval[0] = V_untabid;
	  colval[1] = V_colno;
	  colval[2] = V_grntee_nm;
	  colval[3] = V_grantor;
	  colval[4] = u_r_ur[col_fl[i]-1];
	  
	  CHECK_TYPE_ARR (COLAUTH);
	  Pr_INSLIST (V_scolauth, 5, V_type_arr_COLAUTH,
		      VNULL, V_colval, VNULL, VNULL);
	}
    } /* for */
} /* GrantHandle */



void
ins_val ( Tabid *tid, i4_t clmcnt, TXTREF FirVal, VCBREF FirClm)
     /* operator INSERT VALUE handling :          *
      * INTO table tid with clmcnt; FirVal - list *
      * of trees of expressions to insert         */
{
  VADR Tbid, InsList;
  i4_t i;
  TXTREF CurVal;
  MASK Dep;
  
  InsList = VMALLOC (clmcnt, PSECT_PTR);
  Tbid = VMALLOC (1, UnId);
  V_PTR (Tbid, UnId)->t=*tid;
  
  for (CurVal = FirVal, i = 0; CurVal; CurVal = RIGHT_TRN (CurVal), i++)
    {
      /* column & insert constant descriptions must be present *
       * for all columns of table (where to insert)            */
      (V_PTR (InsList, PSECT_PTR)[i]).off =
	Tr_RecLoad (CurVal, &Dep, FALSE, NULL);  
    }
  
  assert (real_insert_scan);
  Pr_INSROW (Tbid, clmcnt, InsList, FirClm,
	     real_insert_scan, 0, VNULL, VNULL);
} /* ins_val */

void
SQ_Handle (TXTREF CmdNode, TXTREF SelectTree, int arity)
{
  VADR old_val_hole = VAL_HOLE (CmdNode);
  
  VAL_HOLE (CmdNode) = 1;
  switch (CODE_TRN (CmdNode))
    {
    case EXISTS :
      ARITY_TRN (CmdNode) = 0;
      DOWN_TRN (CmdNode) = TNULL;
      break;
      
    case SUBQUERY : /* comparison predicate */
      ARITY_TRN (CmdNode) = arity;
      DOWN_TRN (CmdNode) = SelectTree;
      break;
      
    default :
      lperror("Unexpected code of SubQuery = '%s'\n",NAME(CODE_TRN(CmdNode)));
      yyfatal("Aborted");
    }
  Pr_COND_EXIT (CmdNode);
  VAL_HOLE (CmdNode) = old_val_hole;
} /* SQ_Handle */

void
ScanTable (TXTREF CurTab, TXTREF UntilPtr, i4_t AndNum,
	   VADR *AndTree, MASK *AndMasks, MASK MskDone, 
	   TXTREF SelectNode, VADR BO_Tid, VCBREF ResScan, TXTREF CmdNode)
/* Table scanning forming .                                     *
 * CurTab - pointer to node TBLPTR for current table            *
 * UntilPtr - ponter to head of Until tree                      *
 * AndNum - number of elements in array AndTree                 *
 * AndTree - pointer to array of  subtrees - arguments of AND   *
 * ( some element = NULL when it is already used in UNTIL tree) *
 * AndMasks - array of masks for AND elements                   *
 * MskDone - masks of all tables handled before current table   *
 * BO_Tid - address of Tabid for temp. table, if result of this *
 *      query must be placed to this table (else == NULL)       *
 * If BO_Tid==NULL && CmdNode==(NULL || SELECT) => RetPar;      *
 *      else - CmdNode points to node UPDATE or  DELETE         *
 * ResScan - pointer to SCAN of result temporary table          *
 * (NULL -if not use)                                           */
{
  TXTREF SelItem;
  i4_t nr = 0, nm = 0, TreeN, ClmNum;
  VCBREF Tab, CurClm, ClmTo, ScanNode;
  i4_t *Rlist, *addrs;
  i4_t i, nl = 0, ic = 0, UntCnt = 0;
  VADR Ex, ExOp, TreeTab = VNULL;
  VADR OpScPoint = VNULL, SHPoint, FndrPoint= VNULL, UntPoint= VNULL;
  VADR WhereNode = VNULL, Tbid, Scan = VNULL, FirEl = VNULL, PredEl = VNULL;
  VADR V_Rlist = VNULL, V_addrs = VNULL, V_mlist = VNULL;
  MASK Msk, AllMsk, Depend, AllDep = 0;
  char sc_mode = RSC; /* READ */
  char del_cur_fl = 0;  /* = 1 if there is operator DELETE CURRENT OF CURSOR */
  char sel_stmt_fl = 0; /* = 1 if it's operator SELECT                   */
  
  ScanNode = TABL_DESC (CurTab);
  TASSERT(ScanNode!=TNULL,CurTab);
  Tab = COR_TBL (ScanNode);
  TASSERT(Tab!=TNULL,CurTab);
  ClmNum = TBL_NCOLS (Tab);
  
  if (TstF_TRN (CurTab, DEL_CUR_F))
    {
      sc_mode = DSC; /* DELETE CURRENT */
      del_cur_fl++;
    }
  
  if (CmdNode)
    switch (CODE_TRN (CmdNode))
      {
      case SELECT:
	sel_stmt_fl++;
	break;
	  
      case UPDATE:
	if (UPD_CURS (CmdNode))
          {
            char *cursor_name = STRING (CUR_NAME (UPD_CURS (CmdNode)));
            Scan = external_reference (cursor_name);
            UPD_CURS (CmdNode) = TNULL;
          }
	break;
	
      case DELETE:
	sc_mode = DSC;
	break;
	
      default:
	break;
      }
  
  Tbid = ScanHandle (ScanNode, &Scan, &Msk, NULL, NULL);
  
  /* if it's operator UPDATE CURRENT OF CURSOR =>          *
   *         Tbid = VNULL, Scan != VNULL;                  *
   * if there is not scanning table => Tbid = Scan = VNULL */
  
  if (del_cur_fl)
    /* DELETE CURRENT */
    {
      assert (!DelCurConstr);
      DelCurConstr = Pr_Constraints (ScanNode, Scan, DELETE, 0, NULL);
    }
  
  AllMsk = BitOR(MskDone, Msk); /* AllMsk - masks for current table & *
			         * for all tables handled before      */
  
  if (Scan)
    {
      CurClm = COR_COLUMNS (ScanNode);
      while (CurClm)
	{
	  nr++;
	  CurClm = COL_NEXT (CurClm);
	}

      V_Rlist = VMALLOC (nr, i4_t);
      V_addrs = VMALLOC (nr, i4_t);
      Rlist = V_PTR (V_Rlist, i4_t);
      addrs = V_PTR (V_addrs, i4_t);
      
      CurClm = COR_COLUMNS (ScanNode);
      for (i = 0; i < nr; i++)
	{
	  Rlist[i] = COL_NO (CurClm);
	  addrs[i] = COL_ADR (CurClm);
	  CurClm = COL_NEXT (CurClm);
	}

      nl = 0;
      ic = 0;
      if (COR_TAB_SP (ScanNode))
	{
	  VADR *Tab_SP = V_PTR (COR_TAB_SP (ScanNode), VADR);
	  
	  for (i = 0; i < ClmNum; i++)
	    if (Tab_SP[i])
	      nl = i;
	  nl++;
	}
      if (COR_IND_SP (ScanNode))
	{
	  VADR *Ind_SP = V_PTR (COR_IND_SP (ScanNode), VADR);
	  
	  for (i = 0; i < IND_CLM_CNT (ScanNode); i++)
	    if (Ind_SP[i])
	      ic = i;
	  ic++;
	}
      
      if (Tbid)
	{
	  assert (nr);
	  STEP_CMD (OpScPoint, OPSCAN);
	  STEP_CMD (FndrPoint, FINDROW);
	}
      else /* operator UPDATE CURRENT OF CURSOR */
	if (nr)
	  Pr_READROW (Scan, nr, V_addrs, V_Rlist);
    }

  STEP_CMD (SHPoint, SetHandle);
  SET (sh, SHPoint, Msk);
  
  /* UNTIL tree forming for current table : */
  for (i = 0; i < AndNum; i++)
    if (AndTree[i] /* element from AndTree (number i) was not used */ &&
	CAN_BE_USED (AndMasks[i], AllMsk) /* this element can be used now */)
      {
	if (FirEl)
	  V_PTR (PredEl, TRNode)->Rh.off = AndTree[i];
	else
	  FirEl = AndTree[i];
	PredEl = AndTree[i];
	AndTree[i] = VNULL;
	AllDep = BitOR (AndMasks[i], AllDep);
	/* UntCnt - number elements from AND for current position */
	UntCnt++; 
      }
  if (PredEl)
    V_PTR (PredEl, TRNode)->Rh.off = VNULL;
   
  if (UntCnt)
    {
      if (UntCnt == 1)
	WhereNode = FirEl;
      else /* there are some (more than 1) AND arguments */
	{
	  TRNode *and_trn;
	  
	  WhereNode = Tr_RecLoad(UntilPtr, &Depend, FALSE, NULL);
	  and_trn = V_PTR (WhereNode, TRNode);
	  and_trn -> Arity = UntCnt;
	  and_trn -> Dn.off = FirEl;
	  and_trn -> Depend = AllDep;
	}
      SET (tr, WhereNode, AllDep);
      if (FndrPoint)
	Pr_UNTIL (WhereNode, WHAT_IS_BEG_CMD (FndrPoint), VNULL);
      else
	STEP_CMD (UntPoint, until);
    }
  
  if (RIGHT_TRN (CurTab))
    ScanTable (RIGHT_TRN (CurTab), UntilPtr, AndNum, AndTree,
	       AndMasks, AllMsk, SelectNode, BO_Tid, ResScan, CmdNode);
  else
    {
      /* here is a body of all circles : codes for *
       * current query row handling                */
      if (!CmdNode || sel_stmt_fl || !Is_SQPredicate_Code (CODE_TRN (CmdNode)))
	TreeTab = SelArr (SelectNode, &TreeN);

      if (BO_Tid)
	{
	  if (CmdNode)
	    /* UPDATE operation was replaced by using of temporary table */
	    prepare_UPD (CmdNode, &nm, &V_mlist);
	  Pr_INSROW (BO_Tid, TreeN, TreeTab, TBL_COLS (COR_TBL (ResScan)) ?
		     TBL_COLS (COR_TBL (ResScan)) : COR_COLUMNS (ResScan),
		     real_insert_scan, nm, V_mlist, Scan);
	  /* columns descriptors ( for query result) are changing: */
	  SelItem = DOWN_TRN (SelectNode); /* current element of SELECT */
	  ClmTo = COR_COLUMNS (ResScan);   /* current column descriptor */
	  for (i = 0; ClmTo && (i < TreeN); i++)
	    {
	      if ( (CODE_TRN (SelItem) == COLPTR) &&
		   (COL_ADR (OBJ_DESC (SelItem))) )
		COL_ADR (ClmTo) = COL_ADR (OBJ_DESC (SelItem));
	      else
		COL_ADR (ClmTo) = prepare_HOLE (COL_TYPE (ClmTo));
	      ClmTo = COL_NEXT (ClmTo);
	      SelItem = RIGHT_TRN (SelItem);
	    }			/* for */
	}
      else
	if (!CmdNode || sel_stmt_fl)
	  Pr_RetPar (TreeTab, TreeN, !sel_stmt_fl);
      
      if (CmdNode && !sel_stmt_fl) 
	if (Is_SQPredicate_Code (CODE_TRN (CmdNode)))
	  SQ_Handle (CmdNode, DOWN_TRN (SelectNode), ARITY_TRN (SelectNode));
	else
	  switch (CODE_TRN (CmdNode))
	    {
	    case DELETE :
	      Pr_DELETE (Scan, (real_insert_scan) ? VNULL : ScanNode);
	      break;
	      
	    case UPDATE :
	      sc_mode = (sc_mode == DSC) ? WSC : MSC; /* MODIFICATION */
	      prepare_UPD (CmdNode, &nm, &V_mlist);
	      Pr_UPDATE (Scan, nm, TreeTab, ScanNode, V_mlist);
	      break;
	      
	    default :
	      TASSERT (0, CmdNode);
	    }
    }
  
  if (Tbid) /* There is table scanning */
    {
      Pr_GoTo (WHAT_IS_BEG_CMD (FndrPoint), VNULL);
      Ex = CurPg;
      Pr_CLOSE (Scan);
      ExOp = CurPg;
      
      if (CODE_TRN (COR_TBL (ScanNode)) == TMPTABLE)
	Pr_DROPTTAB (Tbid);
  
      Pr_SCAN (Tbid, Scan, COR_IND_SP (ScanNode) ? IND_VW : SEC_VW,
	       sc_mode , nr, V_Rlist, nl, COR_TAB_SP (ScanNode),
	       ic, COR_IND_SP (ScanNode), nm, V_mlist, ExOp, OpScPoint);

      Pr_FINDROW (Scan, nr, V_addrs, Ex, FndrPoint);
    }
  else
    if (!Scan)
      {
	if (FuncExit) /* it's the place where to put address     *
		       * of command "ERROR" (if exists) OR "END" */
	  {
	    *FuncExit = CurPg;
	    FuncExit = NULL;
	  }
	Pr_ERROR (-ER_EOSCAN);
	if (UntPoint)
	  Pr_UNTIL (WhereNode, *FuncExit, UntPoint);
      }
}				/* ScanTable */

/*---------------------------------------------------------------------*/
float
Query (TXTREF FromNode, VADR BO_Tid, VCBREF ResScan, TXTREF CmdNode, i4_t *res_row_num)

/* Query handling ( beginning - FROM) .                         *
 * BO_Tid - address of Tabid for temp. table, if result of this *
 * query must be placed to this table (else == NULL)            *
 * If BO_Tid==NULL && CmdNode==(NULL || SELECT)  =>  RetPar;    *
 *      else - CmdNode points to node UPDATE or  DELETE         *
 * ResScan - pointer to SCAN of result temporary table          *
 * (NULL -if not use)                                           *
 * Returns estimated cost of the query.                         *
 * If res_row_num != NULL => the estimated rows' number         *
 * in the result of Query is putted to *res_row_num.            */
{
  TXTREF TabPtr, SelectNode, WhereNode, CurAnd;
  i4_t i, AndNum = 0;
  VADR *AndArr= NULL, *Save_FuncExit = FuncExit;
  MASK Msk, AllMsk = 0, AllTrDep = 0, *AndMasks = NULL;
  float cost = 0;
  
  FuncExit = NULL;
  assert(FromNode);
  if (res_row_num)
    *res_row_num = 1;
  TASSERT (CODE_TRN (FromNode) == FROM, FromNode);
  TabPtr = DOWN_TRN (FromNode);
  TASSERT (TabPtr, FromNode);
  
  /* mask creating for tables & making places for columns' values : */
  do
    {
      TASSERT (CODE_TRN (TabPtr) == TBLPTR, TabPtr);
      ScanHandle (TABL_DESC (TabPtr), NULL, &Msk, NULL, NULL);
      AllMsk = BitOR (AllMsk, Msk);
    }
  while ((TabPtr = RIGHT_TRN (TabPtr)));
  /* Bits for all tables in current query are in AllMsk now */
  
  if (OPTIM)
    cost = opt_query (FromNode, res_row_num);
  
  SelectNode = RIGHT_TRN (FromNode);
  WhereNode = (SelectNode) ? RIGHT_TRN (SelectNode) : TNULL;
  if (WhereNode && CODE_TRN (WhereNode) != WHERE)
    {
      TASSERT (CODE_TRN (WhereNode) == INTO, WhereNode);
      fprintf (STDERR, "What is node INTO doing in QUERY ?\n");
      WhereNode = RIGHT_TRN (WhereNode);
    }
  TASSERT ((WhereNode==TNULL) || (CODE_TRN (WhereNode) == WHERE), WhereNode);
  
  if (WhereNode)
    WhereNode = DOWN_TRN (WhereNode);
  
  if (WhereNode)
    {
      AndNum = (CODE_TRN (WhereNode) == AND) ? ARITY_TRN(WhereNode) : 1;
      TASSERT (AndNum, WhereNode);
      AndArr = GET_MEMC(VADR, AndNum);
      AndMasks = GET_MEMC(MASK, AndNum);
      
      if (CODE_TRN (WhereNode) == AND)
	{
	  for (i = 0, CurAnd = DOWN_TRN (WhereNode); 
	       CurAnd; i++, CurAnd = RIGHT_TRN (CurAnd))
	    {
	      AndArr[i] = Tr_RecLoad (CurAnd, AndMasks + i, FALSE, NULL);
	      AllTrDep = BitOR (AllTrDep, AndMasks[i]);
	    }
	  ARITY_TRN (WhereNode) = 0;
	  DOWN_TRN (WhereNode) = TNULL;
	}
      else
	{
	  AndArr[0] = Tr_RecLoad (WhereNode, AndMasks, FALSE, NULL);
	  AllTrDep = BitOR (AllTrDep, AndMasks[0]);
	}
    }
  /* All dependence bits for until-tree are in AllTrDep now */
  
  /* For table handling all external tables are already handled */
     
  ScanTable (DOWN_TRN (FromNode), WhereNode, AndNum, AndArr, AndMasks, 
	     BitAND (AllTrDep, BitNOT (AllMsk)), SelectNode, BO_Tid, ResScan, CmdNode);
  
  FuncExit = Save_FuncExit;
  return cost;
}				/* Query */

/*---------------------------------------------------------------------*/

#define DR V_PTR (V_DR, struct S_Drop_Reg)

void
Fill_Drop_Reg (VADR V_DR)
{
  VADR V_arr, *arr;
  
  DR->TabdNum = drop_cnt;
  if (drop_cnt > 0)
    {
      P_VMALLOC (arr, drop_cnt, VADR);
      DR->Tabd = V_arr;
      TYP_COPY (drop_arr, arr, drop_cnt, VADR);
    }
	  
  DR->ScanNum = scan_cnt;
  if (scan_cnt > 0)
    {
      P_VMALLOC (arr, scan_cnt, VADR);
      DR->Scan = V_arr;
      TYP_COPY (scan_arr, arr, scan_cnt, VADR);
    }
} /* Fill_Drop_Reg */

#undef DR

/*---------------------------------------------------------------------*/

i4_t
Handle (TXTREF CurNode, VADR BO_Tid, VCBREF ResScan, float *sq_cost, i4_t *res_row_num)
/*
 * returns : 0 - if O'K, 1 - scanning result is single     
 * row (for example in FUNC) => there is not table-result, 
 * < 0 - error
 *    
 * work with nodes of tree (it is the working plan)        
 * BO_Tid - address of Tabid for temp. table, if it's      
 * known that this table must be created, else BO_Tid=NULL 
 * ResScan - pointer to SCAN of table-result
 * (NULL - if it is not used)
 * If CurNode - SubQuery =>
 * 1) if sq_cost != NULL => the estimated cost of this     
 *    SubQuery is putted to *sq_cost;
 * 2) if res_row_num != NULL => the estimated rows' number 
 *    in the result of SubQuery is putted to *res_row_num.
 */
{
  TXTREF DnNode, SQNode, CurQuery, CurTbl, CurEl, cre_obj;
  VCBREF ClmTo, ClmToBeg, ClmFrom, ClmBeg, ScNode;
  VCBREF Tab;
  i4_t i, j;
  i4_t clm_nmb, func_nmb;
  i4_t *clm_nums, *Tbd;
  VADR clm_arr, func_clm_arr;	/* i4_t [] */
  VADR func_arr;		/* char [] */
  VADR Tbid, V_colval = VNULL, V_Tbd, V_DR, V_CH;
  float cost = 0.0, cur_cost;
  i4_t row_num = 0, cur_row_num;
  VCBREF old_real_insert_scan;
  struct S_CursorHeader *CH;
  
  if (sq_cost)
    *sq_cost = 0.0;
  if (res_row_num)
    *res_row_num = 0;
  
  ClmToBeg = ClmTo = (ResScan) ? COR_COLUMNS (ResScan) : TNULL;
  DnNode = (CurNode && HAS_DOWN (CurNode)) ? DOWN_TRN (CurNode) : TNULL;
  switch (CODE_TRN (CurNode))
    {
    case CUR_AREA :
      /* copying pointer to cursor name */
      if (DnNode && CODE_TRN (DnNode) == DECL_CURS)
	SCAN_PTR (DnNode) = STMT_VCB (CurNode);
      Handle (DnNode, BO_Tid, ResScan, sq_cost, res_row_num);
      break;
      
    case DECL_CURS:
      STEP_CMD (V_CH, CursorHeader);
      STEP_CMD (V_DR, Drop_Reg);
      
      Pr_SavePar (STMT_VCB (CurNode));

      /* there may be no DECL_CURS, included in another DECL_CURS */
      assert (drop_cnt == -1 && scan_cnt == -1);
      drop_cnt = scan_cnt = 0;
      
      /* down to DECL_CURS are always : QUERY, FROM, TBLPTR, SCAN */
      if ( TstF_TRN (CurNode, DEL_CUR_F) ||
          (!static_sql_fl && !TstF_TRN(CurNode,RO_F)))
        {
          TXTREF NextDn = DnNode;
          
          TASSERT (CODE_TRN (NextDn) == QUERY, DnNode);
          NextDn = DOWN_TRN (NextDn);
          TASSERT (CODE_TRN (NextDn) == FROM , DnNode);
          NextDn = DOWN_TRN (NextDn);
          TASSERT (CODE_TRN (NextDn) == TBLPTR, DnNode);
          SetF_TRN (NextDn, DEL_CUR_F);
        }
      Handle (DnNode, BO_Tid, ResScan, sq_cost, res_row_num);
      
      /* in case of joining of some tables operation CLOSE CURSOR *
       * mean some scans closing & some temp. tables dropping     */

      TASSERT (scan_cnt > 0, CurNode);
      register_export_address (scan_arr[0],
                               STRING (CUR_NAME (SCAN_PTR (CurNode))));
      CH = V_PTR (V_CH, struct S_CursorHeader);
      CH->DelCurConstr = DelCurConstr;
      CH->Cur = NULL;
      CH->OpFl = 0;
      DelCurConstr = VNULL;
      Fill_Drop_Reg (V_DR);
      break;

    case INSERT:
      Pr_SavePar (STMT_VCB (CurNode));
      
      ScNode = SCAN_PTR (CurNode);
      TASSERT (CODE_TRN (ScNode) == SCAN && DnNode,CurNode);

      Tab = COR_TBL (ScNode);
      
      real_insert_scan = ScNode;
      switch (CODE_TRN (DnNode))
	{
	case IVALUES :
	  ins_val (&TBL_TABID (Tab), TBL_NCOLS (Tab),
		   DOWN_TRN (DnNode), TBL_COLS (Tab));
	  break;
	  
	case QUERY :
	  Tbid = ScanHandle (ScNode, NULL, NULL, sq_cost, res_row_num);
	  cost = Query (DOWN_TRN (DnNode), Tbid, ScNode, TNULL, res_row_num);
	  break;
	  
	case TBLPTR :
          {
            VADR tbid_to, tbid_from;
            VCBREF TmpTab;

            tbid_to = ScanHandle (ScNode, NULL, NULL, NULL, NULL);
            TmpTab = COR_TBL (TABL_DESC (DnNode));
            TBL_NNULCOL (TmpTab) = TBL_NNULCOL (Tab);
            TBL_COLS (TmpTab) = TBL_COLS (Tab);
            tbid_from = ScanHandle (TABL_DESC (DnNode), NULL, NULL, sq_cost, res_row_num);
            
            Pr_INSTAB (tbid_from, tbid_to);
            TBL_COLS (TmpTab) = TNULL;
            break;
          }
	  
	default :
	  TASSERT (0, CurNode);
	}
      real_insert_scan = TNULL;
      break;

    case SELECT:
      Pr_SavePar (STMT_VCB (CurNode));
      cost = Query (DnNode, VNULL, TNULL, CurNode, res_row_num);
      break;
      
    case DELETE:
    case UPDATE:
      Pr_SavePar (STMT_VCB (CurNode));
      cost = Query (DOWN_TRN (DnNode), BO_Tid, ResScan, CurNode, res_row_num);
      break;
      
    case UNION :
      if (TstF_TRN (CurNode, ALL_F))
	/* ALL TUPLES */
	for (CurQuery = DnNode; CurQuery; CurQuery = RIGHT_TRN (CurQuery))
	  {
	    Handle (CurQuery, BO_Tid, ResScan, &cur_cost, &cur_row_num);
	    cost    += cur_cost;
	    row_num += cur_row_num;
	  }
      else /* DISTINCT */
	{
          VADR tbid_out;
	  assert (BO_Tid && ClmTo);
	  TASSERT (DnNode && (CODE_TRN (DnNode) == TBLPTR), CurNode);
	  ScNode = TABL_DESC (DnNode);
	  tbid_out = ScanHandle (ScNode, NULL, NULL, &cur_cost, &cur_row_num);
	  cost +=  cur_cost;
	  row_num += cur_row_num;
	  
	  /* making information about result columns */
	  ClmFrom = COR_COLUMNS (ScNode);
	  while (ClmTo)
	    {
	      COL_ADR (ClmTo) = COL_ADR (ClmFrom);
	      ClmTo = COL_NEXT (ClmTo);
	      ClmFrom = COL_NEXT (ClmFrom);
	    }
	  
	  /* making commands for UNION */
	  for (CurTbl = RIGHT_TRN (DnNode); CurTbl;)
	    {
              VADR tbid_from;
	      TASSERT (CODE_TRN (CurTbl) == TBLPTR, CurTbl);
	      tbid_from = ScanHandle (TABL_DESC (CurTbl), NULL, NULL,
                                      &cur_cost, &cur_row_num);
	      cost +=  cur_cost;
	      row_num += cur_row_num;
	      
	      CurTbl = RIGHT_TRN (CurTbl);
	      Tbid = (CurTbl) ? VMALLOC (1, UnId) : BO_Tid;
	      assert (tbid_out && tbid_from && Tbid);
	      Pr_MKUNION (tbid_out, tbid_from, Tbid);
	      tbid_out = Tbid;
	    }
	  /* the estimation of rows' number in the result in case of "DISTINCT" *
	   * is dirty (as a sum of rows' numbers of all arg tables) because     *
	   * we can't predict the number of dublicates in tables                */
	}
      break;
      
    case SORTER:
      ScNode = TABL_DESC (CurNode);
      /* here we must to check background table */
      if (CODE_TRN (COR_TBL (ScNode)) == TABLE)
        {
          /* sorting of base table is not allowed by engine */
          extern TXTREF query_on_scan __P((TXTREF scan));
          extern TXTREF make_scan __P(( TXTREF list,TXTREF qexpr));
          ScNode = TABL_DESC(CurNode) = make_scan(TNULL,query_on_scan(ScNode));
        }
      
      Tbid = ScanHandle (ScNode, NULL, NULL, sq_cost, res_row_num);
      TASSERT (BO_Tid && Tbid, CurNode);
      
      /* making information about result columns */
      ClmFrom = COR_COLUMNS (ScNode);
      while (ClmTo)
	{
	  COL_ADR (ClmTo) = COL_ADR (ClmFrom);
	  ClmTo = COL_NEXT (ClmTo);
	  ClmFrom = COL_NEXT (ClmFrom);
	}
      prepare_SRT (CurNode, &clm_nmb, &clm_arr);
      Pr_SORTTBL (Tbid, BO_Tid, clm_nmb, clm_arr,
	       (TstF_TRN (CurNode, DISTINCT_F)) ? CH_UNIC : CH_ALL, 'A');
      if (CODE_TRN (COR_TBL (ScNode)) == TMPTABLE)
	Pr_DROPTTAB (Tbid);
      break;

    case QUERY:
      cost = Query (DnNode, BO_Tid, ResScan, TNULL, res_row_num);
      break;

    case MAKEGROUP: /* BO_Tid - address of Tabid for temp. table -       *
		     * result of GB. Columns descriptors for this        *
		     * table are used also for columns - arguments of GB */
      ScNode = TABL_DESC (CurNode);
      TASSERT (BO_Tid, CurNode);
      Tbid = ScanHandle (ScNode, NULL, NULL, sq_cost, res_row_num);
      prepare_MG (CurNode, &clm_nmb, &clm_arr,
		  &func_nmb, &func_arr, &func_clm_arr);
      
      if (!clm_nmb) /* there are only functions */
	{
	  assert (func_nmb);
	  V_colval = VMALLOC (func_nmb, VADR);
	}
      
      clm_nums = V_PTR (clm_arr, i4_t);
      ClmBeg = COR_COLUMNS (ScNode);
      for (i = 0; i < clm_nmb; i++)
	{
	  ClmFrom = ClmBeg;
	  for (j = 0; j < clm_nums[i]; j++)
	    ClmFrom = COL_NEXT (ClmFrom);
	  COL_ADR (ClmTo) = COL_ADR (ClmFrom);
	  ClmTo = COL_NEXT (ClmTo);
	}
      for (i = 0; i < func_nmb; i++)
	{
	  COL_ADR (ClmTo) = prepare_HOLE (COL_TYPE (ClmTo));
	  if (!clm_nmb)
	    V_PTR (V_colval, VADR)[i] = COL_ADR (ClmTo);
	  ClmTo = COL_NEXT (ClmTo);
	}
      
      if (clm_nmb) 
        /* there are GROUP BY columns */
	{
	  Pr_GROUP (Tbid, BO_Tid, clm_nmb, clm_arr, 'A',
		    func_nmb, func_clm_arr, func_arr);
	  if (CODE_TRN (COR_TBL (ScNode)) == TMPTABLE)
	    Pr_DROPTTAB (Tbid);
	}
      else /* there are only functions */
	{
	  Pr_FUNC (Tbid, CH_TAB, 0, VNULL, 0, VNULL, V_colval,
		   func_nmb, func_clm_arr, func_arr, VNULL, ClmToBeg);
	  if (res_row_num)
	    *res_row_num = 1;
	  return 1;
	  
	}
      break;

    case EXISTS:
    case SUBQUERY: /* comparison predicate */
      /*
        SQNode = (CODE_TRN (CurNode) == EXISTS) ? DnNode :
        RIGHT_TRN (DOWN_TRN (DnNode));
        */
      if (CODE_TRN (CurNode) == SUBQUERY)
	SQNode = CurNode;
      else
        {
          assert(CODE_TRN (CurNode) == EXISTS );
	  SQNode = DnNode;
        }
      TASSERT (CODE_TRN (SQNode) == SUBQUERY,SQNode);
      
      STEP_CMD (V_DR, Drop_Reg);

      assert (drop_cnt == -1 && scan_cnt == -1);
      drop_cnt = scan_cnt = 0;
      
      old_real_insert_scan = real_insert_scan;
      real_insert_scan = TNULL;
      
      /* down to predicate are here : SUBQUERY, FROM, TBLPTR, SCAN */
      cost = Query (DOWN_TRN (SQNode), BO_Tid, ResScan, CurNode, res_row_num);
      
      real_insert_scan = old_real_insert_scan;
      
      Fill_Drop_Reg (V_DR);
      break;

    case CREATE:
      /* virtual addresses of Tabids for all creating tables are  *
       * being saved for possible GRANT operators and references; *
       * unique identifiers for views are being created.          */
      if (!TBL_VADR (CREATE_OBJ (CurNode)))
	/* the first table is current */
	for (CurEl = CurNode; CurEl; CurEl = RIGHT_TRN (CurEl))
	  if (CODE_TRN (CurEl) == CREATE)
            {
              cre_obj = CREATE_OBJ (CurEl);
              if (CODE_TRN (cre_obj) == TABLE)
                V_Tbd = VMALLOC (1, UnId);
              else
                {
                  TASSERT (CODE_TRN (cre_obj) == VIEW, CurEl);
                  P_VMALLOC (Tbd, 1, i4_t);
                  *Tbd = uniqnm ();
                }
              TBL_VADR (cre_obj) = V_Tbd;
            }
      
      CrTab (CREATE_OBJ (CurNode));
      break;

    case DROP:
      Tab = CREATE_OBJ (CurNode);
      Pr_DROP ((CODE_TRN (Tab) == TABLE) ? (TBL_TABID (Tab)).untabid :
               VIEW_UNID (Tab));
      break;
      
    case GRANT:
      GrantHandle (CurNode);
      break;

    default:
      fprintf (stderr, "Unexpected node at %s:%d: \'%s\'\n", __FILE__, __LINE__,
	       NAME (CODE_TRN (CurNode)));
    }				/* switch */
  if (sq_cost && cost)
    *sq_cost = cost;
  if (res_row_num && row_num)
    *res_row_num = row_num;
  return 0;
}				/* Handle */
