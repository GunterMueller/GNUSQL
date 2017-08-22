/*
 *  optimize.k  - handler of optimizer pass of GNU SQL compiler
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
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

/* $Id: optimize.k,v 1.249 1998/09/28 04:51:20 kimelman Exp $ */

#include "setup_os.h"
#include <stdio.h>
#include "cycler.h"
#include "kitty.h"
#include "trl.h"
#include "opt.h"
#include "tree_gen.h"
#include "cnf.h"
#include <assert.h>
#include "tassert.h"

static void create_selection_vcb  __P((TXTREF curs_root));

#define ALLOC_OPs(parent_node,number_of_OPs) 	\
TXTREF Op[number_of_OPs];		\
alloc_ops(DOWN_TRN(parent_node),Op,number_of_OPs);

static void
alloc_ops (TXTREF in_link_ptr, TXTREF * Ops, i4_t number_of_OPs)
{
  register TXTREF src;
  register i4_t i;
  for (i = 0, src = in_link_ptr; i < number_of_OPs && src;
       i++, src = RIGHT_TRN (src))
    Ops[i] = src;
  while (i < number_of_OPs)
    Ops[i++] = TNULL;
}

static void
process_pos_del(TXTREF del_stmt)
{
  TXTREF d, decl_curs;
  char *err = NULL;

  TASSERT(CODE_TRN(del_stmt)==DELETE,del_stmt);
  decl_curs = CUR_DECL(UPD_CURS(del_stmt));
  if(decl_curs==TNULL)
    {
      errors--;
      err="warning: positioned delete by unknown cursor; ";
    }
  else
    {
      decl_curs = DOWN_TRN(decl_curs);
      d = DOWN_TRN(decl_curs);
      if (RIGHT_TRN(d)!=TNULL)
        SetF_TRN(d,RO_F);
      if (TstF_TRN(d,RO_F)) /* if not updatable query */
        err="positional delete by nonupdatable query";
      else if (!trn_equal_p(DOWN_TRN(del_stmt) /* tblptr to modified table */,
                            /* tblptr from      updateble query */
                            DOWN_TRN (DOWN_TRN (d))))
        err="positional delete table is not the same as cursor's one";
      else /* if everything ok */
        SetF_TRN(decl_curs,DEL_CUR_F);
    }        
  if (err)
    {
      file_pos = LOCATION_TRN(del_stmt);
      yyerror(err);
    }
  free_tree(del_stmt);
}

static TXTREF 
invert(TXTREF n)
{
  TXTREF res = n, p;

  switch(CODE_TRN(n))
    {
    case AND: 
      CODE_TRN(n) = OR;
      for( p = DOWN_TRN(n); p ; p = RIGHT_TRN(p))
        p = invert(p);
      break;
    case OR:
      CODE_TRN(n) = AND;
      for( p = DOWN_TRN(n); p ; p = RIGHT_TRN(p))
        p = invert(p);
      break;
    case NOT:
      res = DOWN_TRN(n);
      RIGHT_TRN(res) = RIGHT_TRN(n); 
      free_node(n);
      break;
    case EQU: CODE_TRN(n) = NE ; break;
    case NE : CODE_TRN(n) = EQU; break;
    case LE : CODE_TRN(n) = GT ; break;
    case GE : CODE_TRN(n) = LT ; break;
    case LT : CODE_TRN(n) = GE ; break;
    case GT : CODE_TRN(n) = LE ; break;
    case ISNULL    : CODE_TRN(n) = ISNOTNULL ; break;
    case ISNOTNULL : CODE_TRN(n) = ISNULL    ; break;
    default:
      return TNULL;
    }
  return res;
}

#(Rule:static "Some2ExistWhere"
  ;; A op (select B from list where condition ) ==>
  (Op:0 "Is_Operation" { (Op:1) (SubQuery:2 { - (Result {(Op:4)}) (Where:3) }) })
  "TstF_TRN(Op[0],QUANTIF_F) && TstF_TRN(Op[0],SOME_F)"
  { 
    ;; ==> exists (select B from list where (A op B) and condition )
    (C_code "ClrF_TRN(Op[0],QUANTIF_F); ClrF_TRN(Op[0],SOME_F);")
    (Op:3 {                            ;; where (A=B) and condition
            (Run "nested_assotiative"  ;;
               (And   {                ;;
                  (Op:0 {
                          (Op:1)
                          (COPY (Op:4))
                       })              ;;
                  (DOWN (Op:3))        ;;
               } )                     ;;
            )                          ;;
          })                           ;;
    (Exists {(Op:2)})
} )

#(Rule:static "Some2Exist"
  ;; A op (select B from list ) ==>
  (Op:0 "Is_Operation" { (Op:1) (SubQuery:2 { - (Result {(Op:4)}) }) })
  "TstF_TRN(Op[0],QUANTIF_F) && TstF_TRN(Op[0],SOME_F)"
  { 
    ;; ==> exists (select B from list where (A op B) )
    (Op:2 {
      (DOWN:list (Op:2))
      (Where {                            ;; where (A=B) and condition
         (Op:0 {
            (Op:1)
            (COPY (Op:4))
          })                  
      })
    })
    (Exists {(Op:2)})
} )

static TXTREF
process_q_predicate(TXTREF in)
{
  TXTREF s;
  TXTREF t;
  i4_t   ok;
  
  if (CODE_TRN(in)==EXISTS  || !HAS_DOWN(in) || ARITY_TRN(in) <2)
    return in; /* there is nothing to do for exist */

  t = DOWN_TRN(in);
  s = RIGHT_TRN(t);
  
  if (CODE_TRN(s) != SUBQUERY)
    s = TNULL;
      
  if(CODE_TRN(in)==IN && s == TNULL)
    {   /* column IN ( a,b,c,d ) */
      TXTREF v=DOWN_TRN(in);
      TXTREF val=RIGHT_TRN(v);
      if (!val || (CODE_TRN(val)!=VALUES)/* job has already done */ )
        return in;
      /*
       * (In { (ColPtr) (Values { (val_0) ... (val_n) }) }) ===>
       * ==> (In {(ColPtr) (val_0) ... (val_n) }) and (Values {}) --> xxxx
       */
      RIGHT_TRN(v)=DOWN_TRN(val);
      arity_trn(in);
      free_node(val); 
      return in;
    }

  if (s==NULL)
    return in;

  /* here we have (ARITY_TRN(in)==2 && CODE_TRN(s) == SUBQUERY) */
  assert(ARITY_TRN(in)==2);

  if (Is_Operation(in) && TstF_TRN(in,QUANTIF_F) && !TstF_TRN(in,SOME_F))
    { /* transform 'ALL' to 'SOME' */
      TXTREF inv = invert(in);
      assert(inv); 
      SetF_TRN(inv,SOME_F);
    }
  if (CODE_TRN(in) == IN)  /* transform 'IN' to 'SOME' */
    {
      CODE_TRN(in) = EQU ;
      SetF_TRN(in,QUANTIF_F);
      SetF_TRN(in,SOME_F);
    }
  ok = 0;
  in = rule_Some2ExistWhere (&ok, in);
  if (!ok)
    in = rule_Some2Exist (&ok, in);
  return in;
}

TXTREF
transformer(TXTREF in, i4_t f)
{
  i4_t ok;
  TXTREF res;

  /* set default result value */
  res = in;

  if(f) /* for external processors */
    {
      res = group_by_proc(res,f);  /* make group processing */
      res = sorting_proc(res,f);   /* sorter preparation    */ 
    }
  
  if (f == 0) /* initialization */
    {
      res = cycler (in, transformer, 
                    CYCLER_LD + CYCLER_DR + CYCLER_RL +
                    CYCLER_RLC + CYCLER_LN);
    }
  else if (f == CYCLER_LD)
    {
      if (Is_Statement(res)) 
        { /* clear checked flag of parameters */
          TXTREF p;
          for (p = STMT_VCB(res); p ; p = RIGHT_TRN(p))
            if (CODE_TRN(p) == PARAMETER)
              ClrF_TRN(p,CHECKED_F);
        }
      switch (CODE_TRN (res))
	{
	case GRANT:
          cycler_skip_subtree = 1;
          break;
	case TBLPTR:
          if (CODE_TRN(COR_TBL(TABL_DESC(res)))==VIEW)  /* get view in tree */
            {
              TXTREF t,c,tbl;
              tbl = COR_TBL(TABL_DESC(res));
              /* if some tmptable of view has the same atributes (names) as local tmptable
               * change it's name until it become unique */
              for (t=VIEW_VCB(tbl); t; t = RIGHT_TRN(t))
                while (find_entry(t)) /* check tmptable in statement local vcb */
                  {
                    static i4_t tmptbl_no = 100;
                    char   tn[100];
                    TXTREF lv = LOCAL_VCB_ROOT;
                    LTRLREF l;
                    LOCAL_VCB_ROOT = VIEW_VCB(tbl);
                    TASSERT(CODE_TRN(t)==TMPTABLE,t);
                    do  /* check for uniqueness in view vcb */
                      {
                        sprintf(tn,"%s_%d",STRING(TBL_NAME(t)),tmptbl_no++);
                        l = ltr_rec(tn);
                      }
                    while (find_table(TMPTABLE,TBL_FNAME(t),l));
                    TBL_NAME(t)= ltr_rec(tn);
                    LOCAL_VCB_ROOT = lv;
                  }
              c = #(TblPtr (Scan ));
              COR_TBL(TABL_DESC(c)) = tbl;
              check_scan_cols(c);
              t = gen_node(TMPTABLE);
              TBL_COLS(t)  = COR_COLUMNS(TABL_DESC(c));
              TBL_NCOLS(t) = TBL_NCOLS(tbl);
              TBL_FNAME(t) = TBL_FNAME(tbl);
              TBL_NAME(t)  = TBL_NAME(tbl);
              free_node(TABL_DESC(c));
              free_node(c);
              for(c=TBL_COLS(t);c;c=COL_NEXT(c))
                CODE_TRN(c) = COLUMN;
              add_info(t);
              VIEW_QUERY(t) = copy_tree(VIEW_QUERY(tbl));
              COR_VIEW(TABL_DESC(res)) = tbl;
              tbl = COR_TBL(TABL_DESC(res)) = t;
            }
          if (CODE_TRN(COR_TBL(TABL_DESC(res)))==TMPTABLE)  
            { /* go throuth nested query */
              TXTREF t = COR_TBL(TABL_DESC(res));
              VIEW_QUERY(t) = transformer(VIEW_QUERY(t),0);
            } 
          break;
        default: break;
        }
    }
  else if (f == CYCLER_DR)
    {
      /* let's try to make fast selection */
      switch (CODE_TRN (res))
	{
	case DECL_CURS:
          create_selection_vcb (res);
          break;
	case DELETE:
          if (!Is_Statement(res)) /* grant delete */
            break;  /* do nothing */
          res = get_up_nested_table(res,0);
          if (UPD_CURS(res)) /* positioned delete by UPD_CURS cursor */
            {
              process_pos_del(res);
              res = TNULL;
            }
          else /* searched only **** Ops     0     1     */
            {                  /****      TblPtr Where   */
              ALLOC_OPs (res, 2);
              DOWN_TRN (res) =
#               (Query {
 	          (From {(Op:0)})
                  (Selection)
	          (Op:1)
                } )
                ;
              ARITY_TRN (res) = 1;
            }
          break;
	case UPDATE:
          if (!Is_Statement(res)) /* grant update */
            break;  /* do nothing */
          res = process_update (get_up_nested_table(res,0));
	  break;
	case INSERT:
	  res = process_insert (get_up_nested_table(res,0));
	  break;
	case CREATE:
          if (CODE_TRN(CREATE_OBJ(res))==VIEW)
            { /* compute RO_F in view query */
              TXTREF view,lvcb;
              view = CREATE_OBJ(res);
              lvcb = LOCAL_VCB_ROOT;
              LOCAL_VCB_ROOT = VIEW_VCB(view);
              VIEW_QUERY(view) = transformer(VIEW_QUERY(view),0);
              VIEW_VCB(view) = LOCAL_VCB_ROOT;
              LOCAL_VCB_ROOT = lvcb;
              if (!TstF_TRN(VIEW_QUERY(view),RO_F))
                /*           colptrs  selection from     query              */
                for ( lvcb = DOWN_TRN(RIGHT_TRN(DOWN_TRN(VIEW_QUERY(view))));
                      lvcb;
                      lvcb = RIGHT_TRN(lvcb))
                  if (CODE_TRN(lvcb)!=COLPTR)
                    {
                      SetF_TRN(VIEW_QUERY(view),RO_F);
                      break;
                    }
            }
	  break;
	case UNION: /* processed in sorter */
          SetF_TRN(res,RO_F);
	  break;
	case WHERE:
	case HAVING:
	  DOWN_TRN(res) = rule_CNF (&ok, DOWN_TRN (res));
	  break;
	case BETWEEN:		/* OPs  0   1     2   */
	  {			/*    what left right */
	    ALLOC_OPs (res, 3);
            if (Is_Operation(Op[0]))
              {
                Op[0] = #(Noop 2 { (Op:0) });
                res = 
#                 (And
	            {
		      (Ge { (Noop  { (Op:0) }) (Op:1) })
		      (Le { (Noop  { (Op:0) }) (Op:2) })
                  } );
              }
            else
              {
                res =
#                 (And
	            {
		     (Ge { (Op: 0)        (Op:1) })
		     (Le { (COPY (Op: 0)) (Op:2) })
                    } );
              }
	    break;
	  }
	case NOT:
          {
            TXTREF to_del = res;
            res = invert(DOWN_TRN (to_del));
            if (res)
              free_node(to_del);
            else
              res = to_del;
            break;
          }
	default:
	  /* otherwise we have to do complex tree analysys */
	  res = process_q_predicate (res);
	}
    }
  else if (f == CYCLER_RL) {}

  return res;
}

i4_t 
transform (void)
{
  ROOT = get_up_nested_table(transformer(ROOT,0),-CYCLER_LN);
  free_patterns();
  return 0;
}

int
assotiative_operation (TXTREF node)
{
  switch (CODE_TRN (node))
    {
    case ADD:case OR:case MULT:case AND:
      return 1;
    default: break;
    }
  return 0;
}

int
the_same_code (TXTREF node)
{
  static i4_t odd = 0;
  static enum token code = UNKNOWN;
  register enum token ccode = CODE_TRN (node);

  if (!assotiative_operation (node))
    return 0;
  odd = 1 - odd;
  if (odd)
    code = ccode;
  return (code == ccode);
}

void
adjust_parameter (TXTREF p, TXTREF c)
{
  TXTREF parm = p,col = c ;
  
  if (CODE_TRN(parm) == PARMPTR)
    parm = OBJ_DESC(parm);
  if ( CODE_TRN(parm) != PARAMETER )
    return;
  
  if (CODE_TRN(col) == COLPTR)
    col = OBJ_DESC(col);
  switch (CODE_TRN(col))
    {
    case SCOLUMN:
      {
        TXTREF tbl = COR_TBL(COL_TBL(col));
        if (CODE_TRN(tbl) != TABLE)
          goto default_l;
        col = find_column(tbl,COL_NAME(col));
      }
    case COLUMN:
      if (!TstF_TRN(parm,CHECKED_F))
        PAR_NAME(parm) = COL_NAME(col);
      if (COL_DEFAULT(col) == TNULL)    /* if it's not null value        */
        PAR_INDICATOR(parm) = TNULL;
      else if (PAR_INDICATOR(parm) == TNULL && !TstF_TRN(parm,CHECKED_F)) 
        {/* if parameter can be NULL and parameter-indicator doesn't exist */
          TXTREF pi;
          char   b[100];
          sprintf(b,"ind_of_%s",STRING(PAR_NAME(parm)));
          pi = #(Parameter:indicator_f <ltr_rec(b)> Int[0,0] Int[0,0] );
          if (TstF_TRN(parm,OUT_F))
            SetF_TRN(pi,OUT_F);
          add_info(pi);
        }
      break;
    default:
    default_l:
      {
        char b[100];
        sprintf(b,"SQL__%d",(i4_t)(parm&0x7fff));
        PAR_NAME(parm) = ltr_rec(b);
      }      
      break;
    }
  SetF_TRN(parm,CHECKED_F);
}

static void
create_selection_vcb (TXTREF curs_root)
{
  register TXTREF vp = TNULL;
  register TXTREF cn, cp;
  register i4_t i,under_union = 0;
  TXTREF   query = curs_root;
  
  while ((CODE_TRN(query)==CUR_AREA) ||
         (CODE_TRN(query)==DECL_CURS))
    query = DOWN_TRN(query);
  while (CODE_TRN(query)==UNION)
    { query = DOWN_TRN(query); under_union = 1; }
  
  TASSERT(CODE_TRN(query) == QUERY,curs_root);
  vp = LOCAL_VCB_ROOT; 
  /*        1st sel_elm  selection  from      query */
  for (cn = DOWN_TRN    (RIGHT_TRN (DOWN_TRN (query))), i = 0;
       cn;
       cn = RIGHT_TRN (cn), i++ )
    {
      sql_type_t type = *node_type(cn);
      char pn[100];
      /* skip everything till parameter:out_f  */
      while (vp &&
             (CODE_TRN(vp)!=PARAMETER || !TstF(vp,OUT_F)  ||
              TstF(vp,INDICATOR_F) ))
        vp = RIGHT_TRN(vp);
      if (vp)
        cp = vp;
      else
        cp = #(Parameter:out_f - <type> <type>);
      if (CODE_TRN(cn)==COLPTR || under_union == 0)
        adjust_parameter(cp,cn);
      else
        {
          sprintf(pn,"SQL__%s_%d",NAME(CODE_TRN(cn)),i);
          PAR_NAME(cp) = ltr_rec(pn);
          SetF_TRN(cp,CHECKED_F);
        }
          
      if(vp) /* if we updated existed node */
        vp = RIGHT_TRN(vp); 
      else  /* if it was new node */
        add_info(cp);
    }
  return;
}

