/*
 * insert.k  -   insert statements functionalization. 
 *                                  
 * This file is a part of GNU SQL Server
 *
 * Copyright (c) 1996-1998, Free Software Foundation, Inc
 * Developed at the Institute of System Programming
 * This file is written by Andrew Yahin. 1994
 * Rewritten by Michael Kimelman. 1995
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

/* $Id: insert.k,v 1.246 1998/09/29 21:26:24 kimelman Exp $ */

#include "trl.h"
#include "opt.h"
#include <assert.h>
#include "tassert.h"

static void
add_defaults(TXTREF tblptr,TXTREF into,TXTREF ins_list)
{
  TXTREF cc[3],prev[3];
  i4_t    i;
  
  cc[0] = COR_COLUMNS(TABL_DESC(tblptr));
  cc[1] = DOWN_TRN(into);
  cc[2] = DOWN_TRN(ins_list);
  for(i=0;i<3;i++) prev[i]=TNULL;
  while(cc[0])
    {
      if ( cc[1]==TNULL || COL_NO(cc[0]) < COL_NO(OBJ_DESC(cc[1])) )
        {
	  /* add default value for skipped column */
	  TXTREF newptr;
	  
	  /* add new colptr node in 'into' list */
	  newptr= #(ColPtr <cc[0]>) ;
	  RIGHT_TRN(newptr)=cc[1];
	  if(prev[1]) RIGHT_TRN(prev[1]) = newptr;
	  else        DOWN_TRN(into)     = newptr;
	  cc[1]=newptr;
	  ARITY_TRN(into) ++;
	  
	  /* add default value in values list */
          if (COL_DEFAULT(cc[0])==TNULL)
            {
              lperror("Column '%s.%s.%s' has no default value and "
                      "omitted in insert list",
                      STRING(TBL_FNAME(COR_TBL(COL_TBL(cc[0])))),
                      STRING(TBL_NAME(COR_TBL(COL_TBL(cc[0])))),
                      STRING(COL_NAME(cc[0])));
              newptr = #(Null_vl <COL_TYPE(cc[0])>) ;
            }
          else
            newptr=copy_trn(COL_DEFAULT(cc[0]));
	  RIGHT_TRN(newptr)=cc[2];
	  if(prev[2]) RIGHT_TRN(prev[2]) = newptr;
	  else        DOWN_TRN(ins_list) = newptr;
	  cc[2]=newptr;
	  ARITY_TRN(ins_list) ++;
	}
      adjust_parameter(cc[2],cc[1]); /* change param name and nullability */
      for(i=0;i<3;i++)
        {
          prev[i]=cc[i];
          cc[i] = RIGHT_TRN(cc[i]);
	}
    }
}

TXTREF 
process_insert(TXTREF insert_stmt)
{
  TXTREF tblptr,into,vals;
  TXTREF ins_list;
  enum token    code;
  
  TASSERT(CODE_TRN(insert_stmt) == INSERT, insert_stmt);
  tblptr = DOWN_TRN(insert_stmt);
  TASSERT(CODE_TRN(tblptr)==TBLPTR,insert_stmt);
  /*
   * at first let's check scan column list - 
   * are there all table columns 
   */
  check_scan_cols(tblptr); /* uploading to scan all table columns... */
  /*
   * now there are all columns in scan and we can add default values in 
   * insert list
   */
  into = RIGHT_TRN(tblptr);
  code = CODE_TRN(into);
  if( code != INTO)
    {
      TASSERT(code==QUERY || code==IVALUES,insert_stmt);
      vals = into;
      into = TNULL;
      if(code==QUERY)
        {
          TXTREF selection = RIGHT_TRN(DOWN_TRN(vals));
          TASSERT(CODE_TRN(selection) == SELECTION,insert_stmt);
          ins_list = selection;
        }
      else
        ins_list = vals;
    }
  else /* IF  code==INTO */
    {
      vals = RIGHT_TRN(into);
      code = CODE_TRN(vals);
      TASSERT(code==QUERY || code==IVALUES,insert_stmt);       
      /* We have now the list of values to initialize given column list */
      /* We have to complete this list by default values for all column, which 
       * was absent in given list of columns 
       */
      /* sort 'into' and 'selection' list */
      /* add additional nodes to into & selection lists */
      if(code==QUERY)
        {
          TXTREF selection = RIGHT_TRN(DOWN_TRN(vals));
          TASSERT(CODE_TRN(selection) == SELECTION,insert_stmt);
          ins_list = selection;
          sort_list_by_col_no(&(DOWN_TRN(into)),&(DOWN_TRN(selection)),NULL,0,
                              "column and inserted value are of incompatible types",
                              NULL);
        }
      else
        {
          ins_list = vals;
          sort_list_by_col_no(&(DOWN_TRN(into)),&(DOWN_TRN(vals)),NULL,0,
                              "column and inserted value are of incompatible types",
                              NULL);
        }
      add_defaults(tblptr,into,ins_list);
    }
  /* We have now the list of values to initialize all column of the table */
  /* Let's check if they are coherent */
  sort_list_by_col_no(&(COR_COLUMNS(TABL_DESC(tblptr))),
                      &(DOWN_TRN(ins_list)), NULL,1,
                      "column and inserted value are of incompatible types",
                      NULL);
    
  SCAN_PTR(insert_stmt) = TABL_DESC(tblptr);
  free_node(tblptr);
  if(into)
    free_tree(into);
  {
    TXTREF v, tbl,scan;
    scan = SCAN_PTR(insert_stmt);
    tbl = COR_TBL(scan);
    for ( v= LOCAL_VCB_ROOT; v; v = RIGHT_TRN(v))
      if ( (v!= scan) && (CODE_TRN(v) == SCAN) && (COR_TBL(v) == tbl))
        break;
    if (v)
      vals = #(TblPtr <make_scan(DOWN_TRN(RIGHT_TRN(DOWN_TRN(vals))),vals)> ) ;
  }
  DOWN_TRN(insert_stmt)  = vals;
  ARITY_TRN(insert_stmt) = 1;
  return insert_stmt;
}

