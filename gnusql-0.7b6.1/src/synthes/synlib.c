/*
 *  synlib.c - codegenerator library of GNU SQL server
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
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
 *  Contacts: dkv@ispras.ru
 *            gss@ispras.ru
 *
 */

/* $Id: synlib.c,v 1.248 1998/09/29 22:23:50 kimelman Exp $ */

#include "global.h"
#include "syndef.h"

typedef struct {
  TXTREF tree;
  MASK   tree_deps;
  char   col_flag; /* = TRUE if this tree - contains only COLPTR & SCOLUMN  */
  char   sign;     /* = TRUE if sign of tree - '+' or FALSE in another case */
} add_elem;

static add_elem *add_elem_arr;
static i4_t       add_elem_max;
static i4_t       add_elem_num;

#define ARR_DELTA 5

MASK
see_deps (TXTREF Node, MASK *SQ_deps, float *SQ_cost)
/* Fuction returns dependency of the tree with root = Node              *
 * and put to SQ_deps - dependency of SubQuery (if exists in this tree) *
 * and to SQ_cost - estimation of the cost for this  SubQuery           */
{
  TXTREF cur_son;
  MASK cur_sq_deps, deps = 0, cur_deps;
  float cur_sq_cost;
 
  *SQ_deps = 0;
  *SQ_cost = 0.0;
  
  if (!Node)
    return 0;
  
  if (CODE_TRN (Node) == COLPTR)
    deps = COR_MASK (COL_TBL (OBJ_DESC (Node)));
  else if (Is_SQPredicate_Code (CODE_TRN (Node)))
    {
      Tr_RecLoad (Node, SQ_deps, 0, SQ_cost);
      deps = *SQ_deps;
    }
  else if (HAS_DOWN (Node))
    for (cur_son = DOWN_TRN (Node);
         cur_son; cur_son = RIGHT_TRN (cur_son))
      {
        cur_deps = see_deps (cur_son, &cur_sq_deps, &cur_sq_cost);
        deps = BitOR (deps, cur_deps);
        *SQ_deps = BitOR (*SQ_deps, cur_sq_deps);
        *SQ_cost += cur_sq_cost;
      }
  return deps;
} /* see_deps */

MASK
see_add_elems (TXTREF Node, char sign, MASK *SQ_deps, float *SQ_cost)
/* Is used for making of array 'add_elem_arr' -     *
 * dividing of tree on arguments of 'ADD' operation *
 * This function doesn't change tree-argument       *
 * Fuction returns the same results as  see_deps () */
{
  MASK cur_sq_deps, deps = 0, cur_deps;
  float cur_sq_cost;
  TXTREF cur_son;
  enum token code = CODE_TRN (Node);
  add_elem *elem;

  *SQ_deps = 0;
  *SQ_cost = 0.0;
  
  switch (code)
    {
    case ADD :
    case SUB :
      for (cur_son = DOWN_TRN (Node);
           cur_son; cur_son = RIGHT_TRN (cur_son))
        {
          cur_deps =
            see_add_elems (cur_son,
                           (code == ADD || cur_son == DOWN_TRN (Node)) ?
                           sign : 1 - sign, &cur_sq_deps, &cur_sq_cost);
          deps = BitOR (deps, cur_deps);
          *SQ_deps = BitOR (*SQ_deps, cur_sq_deps);
          *SQ_cost += cur_sq_cost;
        }
      break;
      
    case UMINUS :
      deps = see_add_elems (DOWN_TRN (Node), 1 - sign, SQ_deps, SQ_cost);
      break;
      
    case NOOP:
      deps = see_add_elems (DOWN_TRN (Node), sign, SQ_deps, SQ_cost);
      break;

    default :
      CHECK_ARR_ELEM (add_elem_arr, add_elem_max,
                      add_elem_num, ARR_DELTA, add_elem);
      elem = add_elem_arr + (add_elem_num++);
      elem->tree = Node;
      deps = elem->tree_deps = see_deps (Node, SQ_deps, SQ_cost);
      elem->col_flag = (code == COLPTR);
      elem->sign = sign;
    }
  
  return deps;
} /* see_add_elems */

i4_t
SP_Extract (TXTREF Node, MASK *TblBits, i4_t TblNum, Simp_Pred_Info ** Result)
     /* making attempt to find interpretations of disjuncts *
      * as simple predicates and filling information about  *
      * disjunct into array *Result. This function returns  *
      * elements number in this array = number of disjuncts *
      * Important is that this function change where-tree   *
      * onto array *Result so that tree with root 'Node'    *
      * can't be used after this functin finished           */
{
  i4_t k, dis_num = 1, add_num, arr_num;
  TXTREF cur_dis = Node, next_dis, right_list,
    right_node, comp_node, left_node, col_tree;
  Simp_Pred_Info *arr;
  enum token code;
  char sign;
  MASK left_deps, right_deps, left_sq_deps, right_sq_deps, deps;
  float left_cost, right_cost;
  add_elem *elem;
  add_elem *old_add_elem_arr = add_elem_arr;
  i4_t       old_add_elem_max = add_elem_max;
  i4_t       old_add_elem_num = add_elem_num;

  enum token opp_code [] = { /* EQU */ EQU,
                             /* NE  */ NE,
                             /* LE  */ GE,
                             /* GE  */ LE,
                             /* LT  */ GT,
                             /* GT  */ LT  };
  
  if (!Node)
    {
      *Result = NULL;
      return 0;
    }

  add_elem_arr = NULL;
  add_elem_max = 0;
  
  if (CODE_TRN (Node) == AND)
    {
      dis_num = ARITY_TRN (Node);
      cur_dis = DOWN_TRN (Node);
    }
  arr = *Result = TYP_ALLOC (dis_num, Simp_Pred_Info);

  for (arr_num = 0; cur_dis; cur_dis = next_dis, arr_num++)
    {
      next_dis = RIGHT_TRN (cur_dis);
      RIGHT_TRN (cur_dis) =  TNULL;
      code = CODE_TRN (cur_dis);
      if (Is_Comp_Code (code) && code != NE)
        /* may be current disjunct can be considered as simple predicate */
        {
          add_elem_num = 0;
          left_deps = see_add_elems (DOWN_TRN (cur_dis), TRUE,
                                     &left_sq_deps, &left_cost);
          right_deps = see_add_elems (RIGHT_TRN (DOWN_TRN (cur_dis)),
                                      FALSE, &right_sq_deps, &right_cost);

          arr[arr_num].Depend = BitOR (left_deps, right_deps);
          arr[arr_num].SQ_deps = BitOR (left_sq_deps, right_sq_deps);
          arr[arr_num].SQ_cost = left_cost + right_cost;
          
          for (add_num = 0; add_num < add_elem_num; add_num++)
            if (add_elem_arr[add_num].col_flag)
              {
                deps = add_elem_arr[add_num].tree_deps;
                /* checking: if column of current add element *
                 * belongs to table from current query        */
                for (k = 0; k < TblNum; k++)
                  if (TblBits[k] == deps)
                    break;
                if (k == TblNum)
                  /* column of current add element belongs to external table */
                  continue;

                /* checking: if this column isn't used in other add elements */
                for (k = 0; k < add_elem_num; k++)
                  if (k != add_num && BitAND (deps, add_elem_arr[k].tree_deps))
                    /* there is another ADD argument (number k)  *
                     * depends on the same table as checking     *
                     * column (from add_num-th argument) is from */
                    break;
                if (k == add_elem_num)
                  /* checking column can be the base of simple predicate */
                  {
                    (arr[arr_num].SP_Num)++;
                    sign = add_elem_arr[add_num].sign;
                      
                    /* node for compare operation (comp_node) making */
                    comp_node = gen_node ((sign) ? code : opp_code [code - EQU]);
                    RIGHT_TRN (comp_node) = arr[arr_num].SP_Trees;
                    arr[arr_num].SP_Trees = comp_node;
                    OPRN_TYPE (comp_node) = OPRN_TYPE (cur_dis);
                    ARITY_TRN (comp_node) = 2;

                    /* left part of compare operation making */
                    col_tree = add_elem_arr[add_num].tree;
                    left_node = gen_node (NOOP);
                    DOWN_TRN  (left_node) = col_tree;
                    ARITY_TRN (left_node) = 1;
                    OPRN_TYPE (left_node) = COL_TYPE (OBJ_DESC (col_tree));
                    DOWN_TRN  (comp_node) = left_node;
                    RIGHT_TRN (col_tree) = TNULL;
                      
                    /* right part of compare operation making */
                    right_list = TNULL;
                    for (k = 0, elem = add_elem_arr; k < add_elem_num; k++, elem++)
                      if (k != add_num)
                        {
                          RIGHT_TRN (elem->tree) = TNULL;
                          right_node = gen_node ((sign - elem->sign) ? NOOP : UMINUS);
                          DOWN_TRN  (right_node) = elem->tree;
                          ARITY_TRN (right_node) = 1;
                          RIGHT_TRN (right_node) = right_list;
                          right_list = right_node;
                        }
                    if (add_elem_num > 2)
                      {
                        right_node = gen_node (ADD);
                        DOWN_TRN  (right_node) = right_list;
                        ARITY_TRN (right_node) = add_elem_num - 1;
                        right_list = right_node;
                      }
                    RIGHT_TRN (left_node) = right_list;
                  }
              }
          if (!(arr[arr_num].SP_Num))
            arr[arr_num].SP_Trees = cur_dis;
        }
      else
        /* current disjunct can't be considered as simple predicate */
        {
          arr[arr_num].Depend = see_deps (cur_dis, &(arr[arr_num].SQ_deps),
                                          &(arr[arr_num].SQ_cost));
          arr[arr_num].SP_Num = ((code == ISNULL) ||
                                 (code == ISNOTNULL)) ? 1 : 0;
          arr[arr_num].SP_Trees = cur_dis;
        }
    } /* for arr_num */
  
  add_elem_arr = old_add_elem_arr;
  add_elem_max = old_add_elem_max;
  add_elem_num = old_add_elem_num;

  return dis_num;
} /* SP_Extract */

#undef ARR_DELTA
