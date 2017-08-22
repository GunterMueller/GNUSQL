/*
 *  optim.c - optimization of input tree for synthesator
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Konstantin Dyshlevoi
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

/* $Id: optim.c,v 1.250 1998/06/09 17:47:58 kimelman Exp $ */

#include "global.h"
#include "syndef.h"
#include "inprtyp.h"
#include <assert.h>
#include "tassert.h"

#define USE_INDEXES   1
#define USE_SIMP_PRED 1

#define OPEN_SCAN_COST 10
#define DEFAULT_SEL    0.5

#define PRINT(x, y) /* PRINTF ((x, y)) */
#define PRINT_(x)   /* PRINTF ((x))    */

extern data_unit_t *data_st;
extern i4_t   dt_ind, dt_max;
extern SH_Info *tr_arr;
extern i4_t tr_cnt, tr_max;

static i4_t  MAX_LNG = ((u4_t)(-1))/2;
static float FLT_MAX_LNG = (float)(((u4_t)(-1))/2);

static MASK *TblBits, *TblAllBits, _AllTblMsk;
static TXTREF *tbl_ind;
static i4_t    number_of_tables;
static i4_t   *cur_tbl_order, *tab_col_num;
static i4_t   *number_of_rows;          /* number of rows for every table        */
static float  *number_of_scanned_rows;  /* estimated number of rows which will be scanned */
static float   the_best_cost;
/*
 * number of AND sons in WHERE           
 * ( or 1 if WHERE condition isn't AND)
 */
static i4_t number_of_disjuncts;

/*
 * Simple Predicates
 */
static i4_t            number_of_SPs;   /* number of SP */
static Simp_Pred_Info *SPs;          
static SP_Ind_Info   **SPs_indexes;

static Col_Stat_Info *col_stat;
static i4_t *fin_nums;

/* optimizator works only with input trees of procedurizator */

#define CHECK_SEL_ERR(er) { if (er < 0) yyfatal (" Can't get statistic info \n"); }

/*
 * estimate selectivity for predicate
 */
static float
estimate_selectivity (i4_t tbl_num, i4_t col_num, TXTREF col_node, TXTREF SP_Tree, i4_t prdc_num)
{
  TXTREF col_ptr   = DOWN_TRN (SP_Tree);
  TXTREF right_ptr = RIGHT_TRN (col_ptr);
  TXTREF cur_ptr;
  VCBREF tab, cur_col;
  float sel = DEFAULT_SEL, min_sel = 1.0 / ((float) number_of_rows[tbl_num]);
  i4_t sign = 0, err, op = CODE_TRN (SP_Tree);
  col_stat_info **col_info;
  
  if (op == EQU)
    return min_sel;
  
  if (SPs[prdc_num].SQ_cost || SPs[prdc_num].Depend != TblBits[tbl_num]
      || SPs[prdc_num].SP_Num != 1 || !Is_Comp_Code (op))
    return sel;
  
  for (cur_ptr = right_ptr; cur_ptr; cur_ptr = DOWN_TRN (cur_ptr))
    if (CODE_TRN (cur_ptr) == UMINUS)
      sign = BitNOT (sign);
    else
      if (CODE_TRN (cur_ptr) != NOOP)
	break;
  
  if (cur_ptr && CODE_TRN (cur_ptr) == CONST)
    /* SP has type : col_name 'op' constant */
    {
      VADR nd;
      data_unit_t cnst;
      
      nd = VMALLOC (1, TRNode);
      prepare_CONST (cur_ptr, nd);
      cnst = *V_PTR (V_PTR (nd, TRNode)->Ptr.off, data_unit_t);
      vmfree (nd);
      
      if (sign)
	{
	  DtPush;
	  DtCurEl = cnst;
	  err = oper (UMINUS, 1);
	  CHECK_SEL_ERR (err);
	  cnst = DtCurEl;
	  DtPop;
	}
      /* there is right value of SP in *const now */
      
      tab = COR_TBL (COL_TBL (col_node));
      if (CODE_TRN (tab) == TMPTABLE)
        return sel;
      for (cur_col = TBL_COLS (tab); cur_col; cur_col = RIGHT_TRN (cur_col))
	if (COL_NO (cur_col) == col_num)
	  break;
      assert (cur_col);
      
      col_info = (col_stat_info **)(&(COL_STAT (cur_col)));
      if (!(*col_info))
	/* making statistic info about column : min & max values */
	{
	  *col_info = TYP_ALLOC (1, col_stat_info);
	  err = get_col_stat ((TBL_TABID (tab)).untabid, col_num, *col_info);
	  CHECK_SEL_ERR (err);
	  
	  if ((*col_info)->min.dat.nl_fl == NULL_VALUE ||
	      (*col_info)->max.dat.nl_fl == NULL_VALUE)
	    {
	      i4_t spn, flist[2], nf = 0;
              i4_t min_change_fl = FALSE, max_change_fl = FALSE;
	      char fl[2];
	      data_unit_t *du[2];
	      col_change_info ch_info;
              
	      if ((*col_info)->min.dat.nl_fl == NULL_VALUE)
		{
		  du[nf] = &((*col_info)->min);
		  du[nf]->type = COL_TYPE (cur_col);
		  min_change_fl = TRUE;
		  flist[nf] = col_num;
		  fl[nf++] = FN_MIN;
		}
	      if ((*col_info)->max.dat.nl_fl == NULL_VALUE)
		{
		  du[nf] = &((*col_info)->max);
		  du[nf]->type = COL_TYPE (cur_col);
		  max_change_fl = TRUE;
		  flist[nf] = col_num;
		  fl[nf++] = FN_MAX;
		}
	      
	      err = func (&TBL_TABID (tab), &spn, 0, NULL, du, nf, flist, fl);
	      CHECK_SEL_ERR (err);

              ch_info.del_min.dat.nl_fl = UNKNOWN_VALUE;
              ch_info.del_max.dat.nl_fl = UNKNOWN_VALUE;
              
              if (min_change_fl)
                ch_info.ins_min = (*col_info)->min;
              else
                ch_info.ins_min.dat.nl_fl = UNKNOWN_VALUE;
              
              if (max_change_fl)
                ch_info.ins_max = (*col_info)->max;
              else
                ch_info.ins_max.dat.nl_fl = UNKNOWN_VALUE;
              
              put_col_stat ((TBL_TABID (tab)).untabid, col_num, &ch_info);
	    }
	}
      
      if ((*col_info)->min.dat.nl_fl == NULL_VALUE ||
	  (*col_info)->max.dat.nl_fl == NULL_VALUE)
	/* column is empty or consists only of NULL VALUES */
	sel = min_sel;
      else
	{
	  sel = sel_const (op, &cnst, &((*col_info)->min), &((*col_info)->max));
	  CHECK_SEL_ERR (sel);
	  if (!sel)
	    sel = min_sel;
	}
    }
  
  return sel;
} /* estimate_selectivity */

/*
 * create information about simple predicate
 * SP_tree and store it (info) to SPs_indexes for correspondent
 * table and predicate (its number = prdc_num).
 */
static void
collect_SP_info (TXTREF SP_tree, i4_t prdc_num)
{
  TXTREF col_ptr = DOWN_TRN (SP_tree), col_node, cur_ind;
  MASK depend;
  i4_t tbl_num;
  SP_Ind_Info *res;

  if (CODE_TRN (col_ptr) == NOOP)
    col_ptr = DOWN_TRN (col_ptr);
  assert (col_ptr && CODE_TRN (col_ptr) == COLPTR);
  col_node = OBJ_DESC (col_ptr);
  assert (col_node && CODE_TRN (col_node) == SCOLUMN);
  
  depend = COR_MASK (COL_TBL (col_node));
  
  /* finding of corresponding table : */
  for (tbl_num = 0; tbl_num < number_of_tables; tbl_num++)
    if (TblBits[tbl_num] == depend)
      break;
  assert (tbl_num < number_of_tables);
  
  res = SPs_indexes[tbl_num] + prdc_num;
    
  res->ColNum = COL_NO (col_node);
  res->category = (CODE_TRN (SP_tree) == EQU) ? 1 : 2;
  res->Sel = estimate_selectivity (tbl_num, res->ColNum, col_node, SP_tree, prdc_num);
  
  for (cur_ind = tbl_ind[tbl_num]; cur_ind; cur_ind = RIGHT_TRN (cur_ind))
    if (COL_NO (OBJ_DESC (DOWN_TRN (cur_ind))) == res->ColNum)
      break;
  res->IndExists = (cur_ind != TNULL);
  res->tree = SP_tree;
} /* collect_SP_info */

/*
 * Making estimation of query cost accordingly
 * current order of tables in query
 * Functin returns value < 0  if it decides not to
 * make all steps of estimation because the cost of
 * this order >> the_best_cost (if the_best_cost > 0)
 * In another case it returns estimated cost (>=0)
 */
static float
est_cost_order (i4_t *res_row_num)
{
  MASK Depend = _AllTblMsk;
  i4_t tbl_num, prdc_num, i, real_num, ColNum;
  float cost_all = 0.0, row_num = 1.0;
  float ind_best_sel, sel;
  SP_Ind_Info *cur_ind_info;
  
  /* estimation of the cost of tables scanning */
  for (tbl_num = 0; tbl_num < number_of_tables; tbl_num++)
    {
      ind_best_sel = 1.0;
      real_num = cur_tbl_order [tbl_num];
      TblAllBits[tbl_num] = Depend = BitOR (Depend, TblBits [real_num]);
      
      /* init of array for information about culumns */
      for (i = 0; i < tab_col_num[real_num]; i++)
	col_stat[i].Sel = 1.0;
      
      /* checking information about SPs for current table */
      for (prdc_num = 0; prdc_num < number_of_SPs; prdc_num++)
	if (!(SPs[prdc_num].flag) /* this predicate wasn't used yet */ &&
	    CAN_BE_USED (SPs[prdc_num].Depend, Depend)
	    /* predicate can be used now */)
	  {
	    SPs[prdc_num].flag++;
	    cur_ind_info = (SPs_indexes[real_num]) + prdc_num;
	    if (cur_ind_info->Sel)
	      { /* this predicate is SP for current table */
		ColNum = cur_ind_info->ColNum;
		if (col_stat[ColNum].Sel > cur_ind_info->Sel)
		  {
		    col_stat[ColNum].Sel = cur_ind_info->Sel;
		    if (cur_ind_info->IndExists /* there is index for the column of this SP */
			&& col_stat[ColNum].Sel < ind_best_sel)
		      ind_best_sel = col_stat[ColNum].Sel;
		  }
	      }
	  }
      
     /* finding of common selectivity of all simple predicates for current table */
      for (i = 0, sel = 1.0; i < tab_col_num[real_num]; i++)
	sel *=col_stat[i].Sel;
     
      /* adding of default selectivity for the rest of predicates */
      for (prdc_num = number_of_SPs; prdc_num < number_of_disjuncts; prdc_num++)
	if (!(SPs[prdc_num].flag) /* this predicate wasn't used yet */ &&
	    CAN_BE_USED (SPs[prdc_num].Depend, Depend)/* predicate can be used now */
	   )
	  {
	    SPs[prdc_num].flag++;
            sel *= DEFAULT_SEL;
          }
      
      number_of_scanned_rows [tbl_num] = number_of_rows[real_num] * ind_best_sel * row_num;
      /* number_of_scanned_rows [i] - estimated number of rows read from i-th table */
      cost_all += number_of_scanned_rows [tbl_num] + OPEN_SCAN_COST * row_num;
      row_num *= number_of_rows[real_num] * sel;
      
    } /* for tbl_num: tables handling */
      
  for (prdc_num = 0; prdc_num < number_of_disjuncts; prdc_num++)
    SPs[prdc_num].flag = 0;
      
  /* adding of the cost of all subqueries */
  for (prdc_num = 0; prdc_num < number_of_disjuncts; prdc_num++)
    {
      for (tbl_num = 0; tbl_num < number_of_tables; tbl_num++)
        if (CAN_BE_USED (SPs[prdc_num].SQ_deps, TblAllBits[tbl_num]))
          break;
      assert (tbl_num < number_of_tables);
      
      /* tbl_num - number of the last (in order) table *
       * that is referenced in the predicate           */
      cost_all += SPs[prdc_num].SQ_cost * number_of_scanned_rows [tbl_num];
    }
  
  *res_row_num = (row_num < 1.0) ? 1 :
    ((row_num < FLT_MAX_LNG) ? (i4_t)row_num : MAX_LNG);
  
  return cost_all;
} /* est_cost_order */

/*
 * rearrange cur_tbl_order to produce new order of "nested loop" join
 */
static int
next_order_of_tables_scan (void)
{
  i4_t i, j, bound_vl, bound_num;
  
  for ( i = number_of_tables - 1, fin_nums[0] = cur_tbl_order[i]; i; i--)
    if (cur_tbl_order[i - 1] < cur_tbl_order[i])
      break;
    else
      fin_nums[number_of_tables - i] = cur_tbl_order[i - 1];
  
  if (!i)
    return 0; /* all orders was generated */
  
  bound_num = i - 1;
  bound_vl = cur_tbl_order[bound_num];
  
  for (j = 0; fin_nums[j] < bound_vl; j++)
    cur_tbl_order[i++] = fin_nums[j];
  
  cur_tbl_order[bound_num] = fin_nums[j++];
  cur_tbl_order[i++] = bound_vl;
    
  for (; i < number_of_tables; i++, j++)
    cur_tbl_order[i] = fin_nums[j];
  
  return 1;
} /* next_order_of_tables_scan */

float
opt_query (TXTREF FromNode, i4_t *res_row_num)
     /* returns the cost of query */
{
  TXTREF sp_tree, fir_el, last_el = TNULL, cur_ind, Ind, cur_col_of_ind;
  TXTREF ScanNode, Tab, SelectNode, WhereNode = TNULL, WhereTree = TNULL;
  i4_t i, j, k, ColNum, tbl_num, prdc_num;
  i4_t sp_clm_tab_num; /* number of columns for which there are simple *
                       * predicates only for table (not for index)    */
  Simp_Pred_Info TmpEl;
  VADR *Tab_SP, *Ind_SP;
  VADR Tree;
  MASK Depend, tmp_dep, *CurTblBits;
  i4_t real_num, number_of_tables_in_query;
  i4_t *best_tbl_order;
  TXTREF *tblptr, cur_tblptr;

  SP_Ind_Info *cur_ind_info, *Heap = NULL;
  Col_Stat_Info best_ind_info;
  i4_t max_col_num = 0, unused_prdc, Cur_in_Heap;
  MASK AllTblMsk;
  float cost, sel, tmp_build_cost = 0.0;
  TRNode *trnode_dn;
  VADR trnode_dn_rh;
  i4_t categ_eq, categ_neq;
  i4_t row_num, best_row_num = 0;
  
  SelectNode = RIGHT_TRN (FromNode);
  WhereNode = (SelectNode) ? RIGHT_TRN (SelectNode) : TNULL;
  if (WhereNode)
    {
      if (CODE_TRN (WhereNode) != WHERE)
	WhereNode = RIGHT_TRN (WhereNode);
    }
  
  number_of_tables_in_query = ARITY_TRN (FromNode);
  assert (number_of_tables_in_query > 0);
  CurTblBits   = TYP_ALLOC (number_of_tables_in_query, MASK);
  for (tbl_num = 0, cur_tblptr = DOWN_TRN (FromNode);
       tbl_num < number_of_tables_in_query; tbl_num++, cur_tblptr = RIGHT_TRN (cur_tblptr))
    CurTblBits[tbl_num] = COR_MASK (TABL_DESC (cur_tblptr));
  
  if (WhereNode)
    {
      Simp_Pred_Info *SP_temp;

      WhereTree = DOWN_TRN (WhereNode);
      number_of_disjuncts = SP_Extract (WhereTree, CurTblBits, number_of_tables_in_query, &SP_temp);
      /* after function SP_Extract tree WhereTree can't be used */
      SPs = SP_temp;
    }
  else
    {
      number_of_disjuncts = 0;
      SPs = NULL;
    }
  
  number_of_SPs        = 0;
  number_of_tables       = number_of_tables_in_query;
  TblBits      = CurTblBits;
  fin_nums     = TYP_ALLOC (number_of_tables, i4_t);
  best_tbl_order = TYP_ALLOC (number_of_tables, i4_t);
  cur_tbl_order  = TYP_ALLOC (number_of_tables, i4_t);
  tab_col_num  = TYP_ALLOC (number_of_tables, i4_t);
  tblptr       = TYP_ALLOC (number_of_tables, TXTREF);
  SPs_indexes   = TYP_ALLOC (number_of_tables, SP_Ind_Info *);
  number_of_rows      = TYP_ALLOC (number_of_tables, i4_t);
  TblAllBits   = TYP_ALLOC (number_of_tables, MASK);
  tbl_ind      = TYP_ALLOC (number_of_tables, TXTREF);
  number_of_scanned_rows  = TYP_ALLOC (number_of_tables, float);
  
  /* filling of statistic information about tables */
  for (tbl_num = 0, cur_tblptr = DOWN_TRN (FromNode), AllTblMsk = 0;
       tbl_num < number_of_tables; tbl_num++, cur_tblptr = RIGHT_TRN (cur_tblptr))
    {
      ScanNode = TABL_DESC (cur_tblptr);
      tblptr[tbl_num] = cur_tblptr;
      Tab = COR_TBL (ScanNode);
      tab_col_num[tbl_num] = ColNum = TBL_NCOLS (Tab);
      tbl_ind[tbl_num] = (CODE_TRN (Tab) == TABLE) ? IND_INFO (Tab) : TNULL;
      if (max_col_num < ColNum)
	max_col_num = ColNum;
      if (CODE_TRN (Tab) == TABLE)
	{
          i4_t rc = get_nrows ((TBL_TABID (Tab)).untabid);
          number_of_rows[tbl_num] = 2; /* for statistic using purpose */
          if (rc > 0)
            number_of_rows[tbl_num] = rc;
          else if (rc < 0) /* if error was registered */
            {
              yyerror("Required 'nrows' data was not found"); /*!!!!!*/
              errors --;
            }
 	}
      else
	{
	  assert (CODE_TRN (Tab) == TMPTABLE);
	  number_of_rows[tbl_num] = NROWS_EST (Tab);
	  tmp_build_cost += BUILD_COST (Tab);
	}
      AllTblMsk = BitOR (AllTblMsk, TblBits[tbl_num]);
    }
  _AllTblMsk = BitNOT (AllTblMsk);
  col_stat = (max_col_num) ? TYP_ALLOC (max_col_num, Col_Stat_Info): NULL;

  if (WhereNode)
    {
      assert (number_of_disjuncts);
      /* all SP are moving to the beginning of SPs */
      for (i = 0, j = number_of_disjuncts - 1; i < j;)
	{
	  while (SPs[i].SP_Num && i < j)
	    i++;
	  while (!(SPs[j].SP_Num) && i < j)
	    j--;
	  if (i < j)
	    /* i-th and j-th elements are wrapped */
	    {
	      TmpEl = SPs[i];
	      SPs[i] = SPs[j];
	      SPs[j] = TmpEl;
	    }
	}
      assert (i == j);
      number_of_SPs = (SPs[i].SP_Num) ? i + 1 : i;
      /* there are (number_of_SPs) Simple Predicates & (number_of_disjuncts - number_of_SPs) others */

      if (number_of_SPs)
	{
	  Heap  = TYP_ALLOC (number_of_tables * number_of_SPs, SP_Ind_Info);
	  for (tbl_num = 0, Cur_in_Heap = 0; tbl_num < number_of_tables;
               tbl_num++, Cur_in_Heap += number_of_SPs)
	    {
	      SPs_indexes[tbl_num] = Heap + Cur_in_Heap;
            }
	  for (prdc_num = 0; prdc_num < number_of_SPs; prdc_num++)
	    for (sp_tree = SPs[prdc_num].SP_Trees;
		 sp_tree; sp_tree = RIGHT_TRN (sp_tree))
	      /* for all interpretations of prdc_num-th disjunct as SP : */
	      collect_SP_info (sp_tree, prdc_num);
	}
    } /* if (WhereNode) */
  /* End of working structures filling */

  /* initial order of tables */
  for (tbl_num = 0; tbl_num < number_of_tables; tbl_num++)
    best_tbl_order[tbl_num] = cur_tbl_order[tbl_num] = tbl_num;
  the_best_cost = -0.0001;

  /* optimization of table order in query */
  PRINT_ ("\n");
  do {
    /* est_cost_order returns value < 0  if this function decided
       not to make all steps of estimation because the cost of
       this order >> the_best_cost */
    cost = est_cost_order (&row_num);
    
    PRINT_ ("\ntable order: (");
    for (i = 0; i < number_of_tables; i++)
      { PRINT ("%d ", cur_tbl_order[i]); }
    PRINT (") cost = %f", cost);
    
    if (cost > 0 && (cost < the_best_cost || the_best_cost<0))
      {
	for (tbl_num = 0; tbl_num < number_of_tables; tbl_num++)
	  best_tbl_order[tbl_num] = cur_tbl_order[tbl_num];
	the_best_cost = cost;
	best_row_num = row_num;
      }
  } while (next_order_of_tables_scan ());
  PRINT_ ("\n");
  assert (best_row_num);
  
  the_best_cost += tmp_build_cost;
  if (res_row_num)
    *res_row_num = best_row_num;
  
  /* changing of tables' order in query */
  for (tbl_num = 0, DOWN_TRN (FromNode) = tblptr[best_tbl_order[0]];
       tbl_num < number_of_tables - 1; tbl_num++)
    RIGHT_TRN (tblptr[best_tbl_order[tbl_num]]) =
      tblptr[best_tbl_order[tbl_num+1]];
  RIGHT_TRN (tblptr[best_tbl_order[tbl_num]]) = TNULL;

  unused_prdc = number_of_disjuncts;
  if (USE_INDEXES)
    /* indexes choosing and forming of          *
     * Simple Predicates for tables and indexes */
    for (Depend = _AllTblMsk, tbl_num = 0; tbl_num < number_of_tables; tbl_num++)
      {
        sp_clm_tab_num = 0;
        real_num = best_tbl_order [tbl_num];
        ScanNode = TABL_DESC (tblptr [real_num]);
        assert(ScanNode);
        Tab = COR_TBL (ScanNode);
        Depend = BitOR (Depend, TblBits [real_num]);

        /* @ if (CODE_TRN (Tab) != TABLE) continue; : can we use SPs for TMPTABLEs ? @ */

        COR_TAB_SP (ScanNode) = VMALLOC (TBL_NCOLS (Tab), VADR);
      
        /* init of array for information about columns */
        for (i = 0; i < tab_col_num[real_num]; i++)
          col_stat[i].Sel = 1.0;

        /* checking information about SPs and compiling SPs in Tab_SP */
        for (prdc_num = 0; prdc_num < number_of_SPs; prdc_num++)
          if (!(SPs[prdc_num].flag) /* this SP wasn't used yet */ &&
              CAN_BE_USED (SPs[prdc_num].Depend, Depend)
              /* SP can be used now */                               &&
              (SPs_indexes[real_num][prdc_num]).Sel
              /* this predicate is SP for current table */             )
            {
              SPs[prdc_num].flag++;
              unused_prdc--;
              cur_ind_info = (SPs_indexes[real_num]) + prdc_num;
	    
              sp_tree = cur_ind_info->tree;
              Tree = Tr_RecLoad (sp_tree, &tmp_dep, FALSE, NULL);
	    
              /* saving dependency of right part of SP */
              assert ((trnode_dn = NextDn (V_PTR (Tree, TRNode))));
              trnode_dn_rh = (trnode_dn->Rh).off;
              if (trnode_dn_rh)
                SET (tr, trnode_dn_rh, NextRh (trnode_dn)->Depend);
	    
              ColNum = cur_ind_info->ColNum;

              if (col_stat[ColNum].Sel > cur_ind_info->Sel)
                col_stat[ColNum].Sel = cur_ind_info->Sel;
              if (cur_ind_info->category == 1)
                (col_stat[ColNum].num_categ_eq)++;
              else
                (col_stat[ColNum].num_categ_neq)++;

              Tab_SP = V_PTR (COR_TAB_SP (ScanNode), VADR);
              if (Tab_SP[ColNum])
                (V_PTR (Tree, TRNode)->Rh).off = Tab_SP[ColNum];
              else
                sp_clm_tab_num++;
              Tab_SP[ColNum] = Tree;
            }		/* if */

        /* choose index for current table */
        if (tbl_ind[real_num]) /* if there are some indexes for current table */
          {
            Ind = TNULL;
            best_ind_info.Sel = 1.0;
            
            for (cur_ind = tbl_ind[real_num]; cur_ind; cur_ind = RIGHT_TRN (cur_ind))
              {
                sel = 1.0; /* in sel we accumulate selectivity *
                            * for current index (cur_ind)       */
                for (cur_col_of_ind = DOWN_TRN (cur_ind);
                     cur_col_of_ind; cur_col_of_ind = RIGHT_TRN (cur_col_of_ind))
                  {
                    ColNum = COL_NO (OBJ_DESC (cur_col_of_ind));
                    if (col_stat[ColNum].Sel < 1.0)
                      {
                        sel *= col_stat[ColNum].Sel;
                        if (!(col_stat[ColNum].num_categ_eq))
                          break;
                      }
                    else
                      break;
                  }
                
                ColNum = COL_NO (OBJ_DESC (DOWN_TRN (cur_ind)));
                categ_eq = col_stat[ColNum].num_categ_eq;
                categ_neq = col_stat[ColNum].num_categ_neq;
                if ((best_ind_info.Sel > sel) ||
                    ( (best_ind_info.Sel == sel) &&
                      ( (best_ind_info.num_categ_eq < categ_eq) ||
                        ((best_ind_info.num_categ_eq == categ_eq) &&
                         (best_ind_info.num_categ_neq < categ_neq)) ) ))
                  {
                    Ind = cur_ind;
                    best_ind_info.Sel = sel;
                    best_ind_info.num_categ_eq = categ_eq;
                    best_ind_info.num_categ_neq = categ_neq;
                  }
              }

            /* Ind - pointer to found index descriptor */
            if (!Ind && COR_IND_SP (ScanNode))
              {
                Ind = COR_IND_SP (ScanNode);
                COR_IND_SP (ScanNode) = TNULL;
              }
            
            if (Ind)
              {
                if (!COR_TID (ScanNode))
                  COR_TID (ScanNode) = VMALLOC (1, UnId);
                V_PTR (COR_TID (ScanNode), UnId)->i = IND_INDID (Ind);
                IND_CLM_CNT (ScanNode) = ARITY_TRN (Ind);
                COR_IND_SP (ScanNode) = VMALLOC (ARITY_TRN (Ind), VADR);
                Ind_SP = V_PTR (COR_IND_SP (ScanNode), VADR);
                
                for (cur_col_of_ind = DOWN_TRN (Ind), k = 0;
                     cur_col_of_ind; cur_col_of_ind = RIGHT_TRN (cur_col_of_ind), k++)
                  {
                    ColNum = COL_NO (OBJ_DESC (cur_col_of_ind));
                    Tab_SP = V_PTR (COR_TAB_SP (ScanNode), VADR);
                    if (Tab_SP[ColNum])
                      /* there are simple predicates for k-th column of index */
                      {
                        sp_clm_tab_num--;
                        Ind_SP[k] = Tab_SP[ColNum];
                        Tab_SP[ColNum] = VNULL;
                        if (!(col_stat[ColNum].num_categ_eq))
                          /* for index's column there isn't SP 'EQU' */
                          break;
                      }
                    else
                      break;
                  }
              } /* if (Ind) */
		      
            if (!sp_clm_tab_num && COR_TAB_SP (ScanNode))
              /* all simple predicates was moved for index */
              {
                vmfree (COR_TAB_SP (ScanNode));
                COR_TAB_SP (ScanNode) = VNULL;
              }
          } /* if (tbl_ind[real_num]) :            *
             * there are indexes for current table */
      } /* for : tbl_num - tables handling finished */

  if (number_of_disjuncts)
    if (!unused_prdc)	/* WHERE must be deleted */
      {
        free_tree (RIGHT_TRN (SelectNode));
        RIGHT_TRN (SelectNode) = TNULL;
      }
    else
      { /* all unused disjuncts must be saved in WHERE */
        for (fir_el = TNULL, prdc_num = 0; prdc_num < number_of_disjuncts; prdc_num++)
          if (!SPs[prdc_num].flag)
            /* this predicate was not used as simple one */
            if (fir_el)
              last_el = RIGHT_TRN (last_el) = SPs[prdc_num].SP_Trees;
            else
              fir_el = last_el = SPs[prdc_num].SP_Trees;
        
        assert (fir_el && last_el);
        RIGHT_TRN (last_el) = TNULL;
        
        if (unused_prdc > 1)
          {
            WhereNode = DOWN_TRN (WhereNode);
            TASSERT (CODE_TRN (WhereNode) == AND, WhereNode);
          }
        else
          if (CODE_TRN (WhereTree) == AND)
            free_node (WhereTree);
        
        DOWN_TRN (WhereNode) = fir_el;
      }
  
  if (col_stat)
    xfree (col_stat);
  if (SPs)
    xfree (SPs);
  if (Heap)
    xfree (Heap);
  
  xfree (fin_nums);
  xfree (best_tbl_order);
  xfree (cur_tbl_order);
  xfree (tab_col_num);
  xfree (tblptr);
  xfree (SPs_indexes);
  xfree (number_of_rows);
  xfree (TblBits);
  xfree (TblAllBits);
  xfree (tbl_ind);
  xfree (number_of_scanned_rows);

  return the_best_cost;
}     /* opt_query */
