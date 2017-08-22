/*
 * optutil.c  -   several utilitis for imr tree transformation 
 *
 * This file is a part of GNU SQL Server
 *
 * Copyright (c) 1996, 1997, Free Software Foundation, Inc
 * Developed at the Institute of System Programming
 * This file is written by Andrew Yahin.
 * Modifyed by Michael Kimelman.
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

/* $Id: optutil.k,v 1.246 1997/03/31 11:03:23 kml Exp $ */

#include "opt.h"

static LTRLREF
gen_name(char *str, i4_t no)
{
  char name[25];
  sprintf(name,"%s%d",str,no);
  return ltr_rec(name);
}

static TXTREF
add_scolumn(VCBREF scan, i4_t cno,sql_type_t tp,LTRLREF l)
{
  TXTREF scol;
  l = (l?l:gen_name("SQL__col_",cno)); 
  scol= #(SColumn <l> <cno> <tp> <scan> ) ;
  add_info(scol);
  return scol;
}

TXTREF
make_selection(TXTREF scan) /* produce selection list for given scan */
{
  TXTREF t,t1,res,scol;
  
  t1=t=res=TNULL;
  for (scol=COR_COLUMNS(scan);scol;scol=RIGHT_TRN(scol),t1=t)
    {
      t = #(ColPtr (C_code "scol")) ;
      if(res)  RIGHT_TRN(t1) = t;
      else     res           = t;
    }
  return res;
}

/* create scan & scolumns by selection list of underlied query */
TXTREF
make_scan( TXTREF list,TXTREF qexpr)
{
  extern TXTREF find_sel __P((TXTREF qexpr));
  TXTREF scan,tmpt;
  i4_t count=0; 
  
  scan = #(Scan "" <-1> (Tmptable) );
  add_info(tmpt=COR_TBL(scan));   /* Tmptable */
  add_info(scan);                 /* Scan     */

  VIEW_QUERY(tmpt) = qexpr;
  {
    char *str;
    str = xmalloc(100+strlen(STRING(TBL_FNAME(tmpt)))+strlen(STRING(TBL_NAME(tmpt))));
    sprintf(str,"scan_%s_%s_%d",
            STRING(TBL_FNAME(tmpt)),
            STRING(TBL_NAME(tmpt)),
            COR_NO(scan));
    COR_NAME(scan) = ltr_rec(str);
    xfree(str);
  }
  
  if(!list)
    list = DOWN_TRN(find_sel(qexpr));
  
  while(list)
    {
      LTRLREF l;
      
      switch(CODE_TRN(list))
        {
        case COLUMN: case SCOLUMN: l=COL_NAME(list); break;
        case COLPTR: l = COL_NAME(OBJ_DESC(list));   break;
        default:     l = 0;
        }
      add_scolumn (scan, count++, *node_type (list),l);
      list=RIGHT_TRN(list);
    }
  TBL_NCOLS(tmpt) = count;

  return scan;
}

TXTREF
query_on_scan(TXTREF scan)
{
  return 
#    (Query:empty_f {  
       (From { (TblPtr <scan>  ) } )
       (Selection { (C_code:list "make_selection(scan)") } )
     } )
  ;
}

TXTREF
query_via_tmp(TXTREF selection,TXTREF qexpr)
{
  /* scan for upper selection made from selection of the first low query 
   *   query
   *     from
   *     selection
   *       sel_list 
   *        ...
   *     where(?)
   */
  
  return query_on_scan(make_scan(DOWN_TRN(selection),
                                 qexpr));
}

