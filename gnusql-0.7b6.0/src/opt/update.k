/*
 * update.k  -    convert update statements tree to functional oriented
 *                form. 
 *                                  
 * This file is a part of GNU SQL Server
 *
 * Copyright (c) 1996, 1997, Free Software Foundation, Inc
 * Developed at the Institute of System Programming
 * This file is written by Andrew Yahin, 1995.
 * Rewritten by Michael Kimelman. 1996
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contacts: gss@ispras.ru
 *
 */

/* $Id: update.k,v 1.245 1997/03/31 03:46:38 kml Exp $ */

#include "tree_gen.h"
#include "opt.h"
#include "tassert.h"
#include <assert.h>

/*===========================================================

  These routines transform update trees to universal form:

 1. Positioned update
    (Update <cursor_scan_ptr> [] {
      (TblPtr (Scan (Table "..." "update_on_this_table")))
      (AssList {
        (Assign { (ColPtr (Scolumn 0 ...)) (expr0) })            . . .
        (Assign { (ColPtr (Scolumn 3 ...)) (expr3) })
      } )
    } )
      
 ==>>
    (Update <cursor_scan_ptr> [ 0 ... 3 ] {
      (Query {
        (From (TblPtr ..on_this_table..))
        (Selection {
          (expr0)
          . . .
          (expr3)
        } )
    } )

-----------------------------------------------------------
 2. Searced update
    (Update <0> [] {
      (TblPtr (Scan (Table "..." "update_on_this_table")))
      (AssList {
        (Assign { (ColPtr .c0.) (expr0) })
             . . .
        (Assign { (ColPtr .c3.) (expr3) })
      } )
      ? (Where {...})
    } )

 ==>>
    (Insert <update_on_this_table> {
      (TblPtr (Scan (TmpTable 
        (Delete
          (Query {
            (From (TblPtr ....))
            (Selection {
              (ColPtr .0.)
              (expra0)
               . . .
               (expra3)
               (ColPtr .n.)
            } )
            (Where {...}) ;; optional
          } )
        )
      )))
    } )
    
===========================================================*/

/*
 * 'update_list' creates array of number of updated columns and list
 *  of expressions to modify them.
 */

static TXTREF
update_list(TXTREF upd)
{
  TXTREF list=TNULL,arr,assign,set_list;
  i4_t i;
  set_list = RIGHT_TRN(DOWN_TRN(upd));
  TASSERT(CODE_TRN(set_list)==ASSLIST,upd);
  arr=gen_vect(ARITY_TRN(set_list));
  assign=DOWN_TRN(set_list);
  for(i=0;assign;i++)
    {
      TXTREF cn, colptr;
      cn = assign;
      assign = RIGHT_TRN(assign);
      TASSERT(CODE_TRN(cn)==ASSIGN,cn);
      colptr   = DOWN_TRN(cn);
      free_node(cn);
      TASSERT(CODE_TRN(colptr)==COLPTR,colptr);
      VOP(arr,i).l = COL_NO(OBJ_DESC(colptr));
      list   = join_list(list,RIGHT_TRN(colptr));
      adjust_parameter(RIGHT_TRN(colptr),colptr); /* change param name and nullability */
      free_node(colptr);
    }
  free_node(set_list);
  UPD_CLMNS(upd)=arr;
  return list;
}

static TXTREF
extend_update(TXTREF updlist,TXTREF upvec, TXTREF scan)
{
  TXTREF newlist, scol, csel, *ue ;
  i4_t    ue_len, i,l;

  l = VLEN(upvec);
  ue_len = TBL_NCOLS(COR_TBL(scan));
  ue = (TXTREF*) xmalloc(ue_len * sizeof(TXTREF));
  for( i = 0, csel = updlist; csel; i++, csel = RIGHT_TRN(csel))
    {
      assert ( i < l);
      ue[VOP(upvec,i).l] = csel;
    }
  for (i=0,scol = COR_COLUMNS(scan);scol;scol = RIGHT_TRN(scol),i++)
    if (ue[i]== TNULL)
      {
        TASSERT(COL_NO(scol)==i,scol);
        ue[i] = #(ColPtr (C_code "scol"));
      }      
  for(i = 1; i < ue_len; i++)
    RIGHT_TRN(ue[i-1]) = ue[i];
  RIGHT_TRN(ue[ue_len-1]) = TNULL;
  newlist = ue[0];
  xfree(ue);
  return newlist;
}

TXTREF
process_update(TXTREF upd)
{
  TXTREF tblptr=DOWN_TRN(upd);
  TXTREF where=RIGHT_TRN(RIGHT_TRN(tblptr));
  TXTREF updlist=update_list(upd);

  TASSERT(CODE_TRN(tblptr)==TBLPTR,upd);

  if ( UPD_CURS(upd) )
    {
      DOWN_TRN(upd) =
#       (Query
         {
           (From {(C_code "tblptr")})
           (Selection {(C_code:list "updlist")})  
           (C_code "where")
         } ) ;
      ARITY_TRN(upd) = 1;
    }
  else
    {
      /* special processing of update to avoid engine cycling and phantoms */
      TXTREF scan, ss;
      check_scan_cols(tblptr); /* uploading to scan all table columns... */
      ss = TABL_DESC(tblptr);
      scan = gen_node(SCAN);
      COR_TBL(scan) = COR_TBL(ss);
      add_info(scan);
      TABL_DESC(tblptr) = scan ; /* a bit stupid action because of 'check_scan_cols' interface */
      check_scan_cols(tblptr);   /* uploading to scan all table columns... */
      TABL_DESC(tblptr) = ss;    /* recovering */

      updlist = extend_update(updlist,UPD_CLMNS(upd),ss);
      ss =
#       (Delete {
           (Query
            {
              (From {(C_code "tblptr")})
              (Selection {(C_code:list "updlist")})  
              (C_code "where")
            } )
        } ) ;

      UPD_CLMNS (ss) = UPD_CLMNS (upd);
      
      CODE_TRN(upd) = INSERT;
      SCAN_PTR(upd) = scan;
      DOWN_TRN(upd) = #(TblPtr (C_code "make_scan(updlist,ss)"));
      ARITY_TRN(upd) = 1;
    }
  return upd;
}
