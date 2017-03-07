/*
 *  sorter.k  - set of routines to  create sorters
 *              GNU SQL compiler
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Andrew Yahin.
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

/* $Id: sorter.k,v 1.246 1998/09/29 21:26:26 kimelman Exp $ */

#include <stdio.h>
#include "cycler.h"
#include "kitty.h"
#include "optutil.h"
#include "opt.h"
#include <assert.h>
#include "tassert.h"

TXTREF
find_sel(TXTREF qexpr)
{
  register TXTREF q = qexpr, s ;
  enum token code;
  while ( CODE_TRN(q) == UNION )
    q = DOWN_TRN(q);
  code = CODE_TRN(q);
  TASSERT( code == QUERY || code == SUBQUERY, qexpr);
  /*  selections from      query */
  s = RIGHT_TRN (DOWN_TRN (q));
  return s;
}

static void
ob_to_nums(TXTREF dc)
{
  TXTREF sel = TNULL , ob, e;
  i4_t    i;
  for ( ob = DOWN_TRN(RIGHT_TRN(DOWN_TRN(dc)));ob;ob = RIGHT_TRN(ob))
    switch(CODE_TRN(ob))
      {
      case SORT_COL:
        if (!sel)
          sel = DOWN_TRN (find_sel(DOWN_TRN(dc)));
        for ( i=0,e = sel; e ; e = RIGHT_TRN(e),i++)
          if ( trn_equal_p(e,SORT_CLM(ob)))
            {
              free_node(SORT_CLM(ob));
              CODE_TRN(ob) = SORT_POS;
              SORT_IND(ob) = i;
              break;
            }
        if ( !e )
          {
            char str[100];
            sprintf(str,"Attempt to sort by column(%s), unpresented in selection list",
                    STRING(COL_NAME(OBJ_DESC(SORT_CLM(ob)))));
            yyfatal(str);
          }
        break;
      case SORT_POS:
        break;
      default:
        yyfatal("unexpected node in order by clause");
      }
}

static int
is_empty_query(TXTREF qexpr)
{
  TXTREF sel_list,scol_list, col_list;
  TXTREF t,s;
  i4_t    i = 0;
  if ( CODE_TRN(qexpr) != QUERY )
    return 0;
  if (TstF_TRN(qexpr,EMPTY_F))
    return 1;
  /* we shall check for
     query
       from t (list) on (tbl (list))
       selection list
   */
#define CHECK(cond)    if (!(cond) ) return 0;
  CHECK (ARITY_TRN(qexpr) == 2);
  t = DOWN_TRN(qexpr);
  CHECK( (CODE_TRN(t)==FROM) && (ARITY_TRN(t)==1));
  s = TABL_DESC(DOWN_TRN(t));
  t = RIGHT_TRN(t);
  CHECK( (CODE_TRN(t)==SELECTION) && (ARITY_TRN(t) == TBL_NCOLS(COR_TBL(s))));
  sel_list  = DOWN_TRN(t);
  scol_list = COR_COLUMNS(s);
  col_list  = TBL_COLS(COR_TBL(s));
  while(col_list && scol_list && sel_list)
    {
      CHECK(OBJ_DESC(sel_list) == scol_list);
      CHECK(COL_NO(scol_list) == COL_NO(col_list));
      CHECK( COL_NO(scol_list) == i++);
      sel_list  = RIGHT_TRN(sel_list );
      scol_list = RIGHT_TRN(scol_list);
      col_list  = RIGHT_TRN(col_list );
    }

  CHECK(!(col_list || scol_list || sel_list));

  return 1;
#undef CHECK
}

static TXTREF
make_sorter(TXTREF qexpr,TXTREF sortlist,i4_t is_unique)
{
  TXTREF scan, sorter, sv, ce, sel ;
  i4_t i;

  sel = find_sel(qexpr);
  if (!sortlist)
    {
      sv = gen_vect(ARITY_TRN(sel));
      for( i = 0; i < VLEN(sv); i++)
        VOP(sv,i).l = i;
    }
  else
    {
      sv = gen_vect(ARITY_TRN(sortlist));
      for (i=0,ce = DOWN_TRN(sortlist);ce; ce = RIGHT_TRN(ce),i++)
        VOP(sv,i).l = SORT_IND(ce);
    }
  if ( is_empty_query(qexpr))
    {
      TXTREF tbl; 
      scan = COL_TBL(OBJ_DESC(DOWN_TRN(find_sel(qexpr))));
      tbl = COR_TBL(scan);
      if ( CODE_TRN(tbl) == TMPTABLE && CODE_TRN(VIEW_QUERY(tbl)) == SORTER )
        { /* let's try to join nested sorter with required one */
          i4_t j,k;
          sorter = VIEW_QUERY(tbl);
          ce = XTXT_TRN(sorter,0);
          if (!sortlist && TstF_TRN(sorter,EMPTY_F))
            { /* if order is unimportant now - swap sort vectors */
              TXTREF cc = ce;
              ce = sv;
              sv = cc;
            }
          k = VLEN(ce);
          for(i=VLEN(sv);i--;)
            for(j=VLEN(ce);j--;)
              if (VOP(ce,i).l == VOP(ce,j).l)
                {
                  VOP(ce,j).l = -1;
                  k--;
                }
          if (k)
            {
              i = VLEN(sv);
              sv = realloc_vect(sv, i + k);
              for( j = 0; j < VLEN(ce); j++)
                if ( VOP(ce,j).l >=0 )
                  {
                    VOP(sv,i).l = VOP(ce,j).l;
                    i++;
                  }
              assert(i == VLEN(sv));
            }
          XTXT_TRN(sorter,0) = sv; /* set new sort order to old sorter */
          if (is_unique)
            SetF_TRN(sorter,DISTINCT_F);
          if (sortlist)
            ClrF_TRN(sorter,EMPTY_F);
          free_vect(ce);
        }
      else
        {
          if(CODE_TRN(tbl) == TABLE)
            {
              /* if we need to sort base table lets check for
               * adequate index - may be it will much more usefull.
               */
              TXTREF ind;
              for(ind = IND_INFO(tbl);ind;ind = RIGHT_TRN(ind))
                if (!is_unique || (CODE_TRN(ind)==PRIMARY) ||(CODE_TRN(ind)==UNIQUE))
                  {
                    TXTREF colptr;
                    i4_t    i;
                    for(colptr = DOWN_TRN(ind),i=0; colptr;
                        i++,colptr= RIGHT_TRN(colptr))
                      if (COL_NO(OBJ_DESC(colptr)) != VOP(sv,i).l)
                        break;
                    if(colptr==TNULL)
                      break;
                  }
              if(ind) /* if there is usefull index - use it */
                {
                  COR_IND_SP (scan) = ind;
                  return qexpr;
                }
              else /* organize explicit sorting of the whole table */
                {
                  /* query (empty) { ..{..(scan "a" (table "A" ) )...}}
                   * + (sorter (scan "b" (tmptable "B" (nil))))
                   * ==>
                   *    query (empty) { ..{.. (scan "a" (tmptable "B" (sorter
                   *           (scan "b" (table "A" ))))) ...}}  
                   */
                  TXTREF s0,s1,tmp;
                  sorter = #(Sorter <sv> <make_scan(DOWN_TRN(sel),TNULL)> );
                  s0 = TABL_DESC(sorter);
                  /*   scan      tblptr   from     query   */
                  s1 = TABL_DESC(DOWN_TRN(DOWN_TRN(qexpr)));
                  tmp = COR_TBL(TABL_DESC(sorter));
                  COR_TBL(s0) = tbl;
                  VIEW_QUERY(tmp) = sorter;
                  /* table */
                  COR_TBL   (s1) = tmp;
                  { /* just to accurate scan naming */
                    LTRLREF l = COR_NAME(s0);
                    COR_NAME(s0) = COR_NAME(s1);
                    COR_NAME(s1) = l;
                  }
                }
            }
          else /* TMPTABLE */
            {              
              /* query (empty) { ..{.. (tmptable "A" (queryA)) ...}}
               * + (sorter (scan (tmptable "B" (nil))))
               * ==>
               *    query (empty) { ..{.. (tmptable "A" (sorter
               *           (scan(tmptable "B" (queryA))))) ...}}  
               */
              sorter = #(Sorter <sv> <make_scan(DOWN_TRN(sel),TNULL)> );
              VIEW_QUERY(COR_TBL(TABL_DESC(sorter))) = VIEW_QUERY(tbl);
              VIEW_QUERY(tbl) = sorter;
            }
          
          if (is_unique)
            SetF_TRN(sorter,DISTINCT_F);
          if (!sortlist)
            SetF_TRN(sorter,EMPTY_F);
        }
      return qexpr;
    }
  else
    {
      TXTREF new_query;
      scan = make_scan(DOWN_TRN(sel),qexpr);
      sorter = #(Sorter <sv> <scan> ) ;
      if (is_unique)
        SetF_TRN(sorter,DISTINCT_F);
      if (!sortlist)
        SetF_TRN(sorter,EMPTY_F);
      new_query = query_via_tmp(sel,sorter);
      if(CODE_TRN(qexpr)==SUBQUERY)
        {
          TXTREF sel;
          
          CODE_TRN(new_query) = SUBQUERY;
          sel = RIGHT_TRN(DOWN_TRN(new_query));
          TASSERT(CODE_TRN(sel)==SELECTION,new_query);
          CODE_TRN(sel)=RESULT;
          
          CODE_TRN(qexpr) = QUERY;
          sel = RIGHT_TRN(DOWN_TRN(qexpr));
          TASSERT(CODE_TRN(sel)==RESULT,qexpr);
          CODE_TRN(sel) = SELECTION;
          
        }
      return new_query;
    }
  /* UNREACHABLE */
  return TNULL;
}

static void
check_union_selections(TXTREF un)
{
  TXTREF selection1, selection2, query;
  /*           sel_list  selection  from      1st query  union   */
  selection1 = DOWN_TRN (RIGHT_TRN (DOWN_TRN (DOWN_TRN  (un))));
  for (query = DOWN_TRN(un); query; query = RIGHT_TRN(query))
    {
      TXTREF s1,s2;
      i4_t    i; 
      /*           sel_list  selection  from      1st query  union   */
      selection2 = DOWN_TRN (RIGHT_TRN (DOWN_TRN (DOWN_TRN  (un))));
      for(s1=selection1,s2=selection2, i=1 ;
          s1 && s2;
          s1 = RIGHT_TRN(s1), s2 = RIGHT_TRN(s2), i++)
        if (!(is_casted(*node_type(s1),*node_type(s2)) &&
              is_casted(*node_type(s2),*node_type(s1))))
          {
            char err[100];
            sprintf(err,
                    "types of %d element of union selection lists mismath",
                    i);
            file_pos = LOCATION_TRN(s2);
            yyerror(err);
          }
      if(s1 || s2)
          {
            char err[100];
            sprintf(err,"number of elements in union selection list mismath");
            file_pos = LOCATION_TRN(selection2);
            yyerror(err);
          }
    }
}

#define LD (f == CYCLER_LD)
#define DR (f == CYCLER_DR)
#define RL (f == CYCLER_RL)

TXTREF
sorting_proc(TXTREF n,i4_t f)
{
  switch(CODE_TRN(n))
    {
    case DECL_CURS:
      if (LD && (ARITY_TRN(n) == 2))
        ob_to_nums(n);
      else if (DR && (ARITY_TRN(n) == 2))
        {
          TXTREF ob;
          ob = RIGHT_TRN(DOWN_TRN(n));
          RIGHT_TRN(DOWN_TRN(n)) = TNULL;
          DOWN_TRN(n) = make_sorter(DOWN_TRN(n),ob,0);
          ARITY_TRN(n) = 1;
          free_tree(ob);
        }
      break;
    case QUERY:
    case SUBQUERY:
      if (DR && TstF_TRN(n,DISTINCT_F))
        {
          ClrF_TRN(n,DISTINCT_F);
          n = make_sorter (n, TNULL, 1);
        }
      break;
    case UNION:
      if(!DR)
        break;
      check_union_selections(n);
      if (!TstF_TRN(n,ALL_F)) /* if sorting required */
        {
          /*----------------------------------------------------*
           *   (union { (queryA) ... (queryZ)} )                *
           * ==>                                                *
           *   (sorter (union:all { (queryA) ... (queryZ) } ))  *
           * or                                                 *
           *   (union {                                         *
           *     (tblptr (scan (tmptable (sorter (queryA)))))   *
           *     ...                                            *
           *     (tblptr (scan (tmptable (sorter (queryZ)))))   *
           *   } )                                              *
           *----------------------------------------------------*/
          if(1)
            {
              SetF_TRN(n,ALL_F);
              n = make_sorter (n, TNULL, 1);
            }
          else
            {
              TXTREF q, s = find_sel(n);
              for( q = DOWN_TRN(n); q; q = RIGHT_TRN(q))
                {
                  TXTREF r = RIGHT_TRN(q);
                  TXTREF sc;
                  TASSERT(CODE_TRN(q) == QUERY,n);
                  q = make_sorter(q,TNULL,1);
                  /*   scan      tblptr   from     query  */
                  sc = TABL_DESC(DOWN_TRN(DOWN_TRN(q    )));
                  free_tree(q);
                  q =   #(TblPtr <sc>) ;
                  TASSERT(CODE_TRN(q) == TBLPTR,n);
                  RIGHT_TRN(q) = r;
                }
              n = query_via_tmp( s, n);
            }
        }
    default: break;
    }
  return n;
}
