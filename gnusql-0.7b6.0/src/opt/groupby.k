/*
 * groupby.c  -   'group by' transformation  
 *
 * This file is a part of GNU SQL Server
 *
 * Copyright (c) 1996, 1997, Free Software Foundation, Inc
 * Developed at the Institute of System Programming
 * This file is written by Michael Kimelman.
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

/* $Id: groupby.k,v 1.248 1997/04/24 18:41:28 kml Exp $ */

#include "cycler.h"
#include "opt.h"
#include "cnf.h"
#include "tree_gen.h"
#include "tassert.h"
#include <assert.h>

/*
  step in - save from in stack 
  check where for correctness
  save group_by list
  check having and selection (if there are any aggregate function appliable
                              to this level)
  if MGoperation requred, create 3 lists (selection list for inner scan,
  upper query scan column list and list of MAKEGROUP operation - 
  [col0, col1, Avg(col2), Max(col2)... ] )
	    
  from: 
    DR- store context in stack and put 'selection' behind 'having';
    RL - clear stack, generate 2-level query with makegroup (if needed) and replace
         selection to the original place;
  where,having,selection:
    LD - set subtree_code;
    DR - clear subtree_code
  group by: move subtree to inner selection list & create sort list 
  Aggregate functions: 
    LD - set flag af+ ;
    DR - clear flag af-, and replace node by colptr of tmp table
  colptr - check for availability (!single column in having) and if af+ check for
  one level columns in agr_func. columns from group by replaced by new colptrs
*/

#define COL_N(c) XLNG_TRN(c,0)

typedef struct {
  enum token   query_code;
  enum token   subtree_code;
  i4_t          is_col_in_selection;
  i4_t          one_row_groups;
  TXTREF       sort_list; 
  TXTREF       scol_list;
  TXTREF       from;
  TXTREF       selection;
  TXTREF       into;      /* specially for SELECT statement */
  TXTREF       where;
  TXTREF       group_by;
  TXTREF       having;
} query_level_t;
 
DECL_STACK(gb,query_level_t); /* ';' added to make indent & emacs happy */

/* the following two routines was wriiten to use from debugger */
     
static void
     debug_frame(query_level_t *t)
{
#define F         fprintf
#define E         STDERR
#define BRANCH(v) F(E,"\n%s:",#v)
  BRANCH(subtree_code);        F(E,"%s\n",NAME(t->subtree_code)); 
  BRANCH(is_col_in_selection); F(E,"%d\n",t->is_col_in_selection);
  BRANCH(one_row_groups     ); F(E,"%d\n",t->one_row_groups     );
  BRANCH(sort_list);           debug_trn(t->sort_list);
  BRANCH(scol_list);           debug_trn(t->scol_list);
  BRANCH(from);                debug_trn(t->from);
  BRANCH(selection);           debug_trn(t->selection);
  BRANCH(where);               debug_trn(t->where);
  BRANCH(group_by);            debug_trn(t->group_by);
  BRANCH(having);              debug_trn(t->having);
#undef F
#undef E
#undef BRANCH
}

void
debug_gb_stack(i4_t i)
{
  i4_t depth;
  query_level_t *st_frame;
  
  for(depth=0;depth < st_depth(gb_stack) && depth < i ;depth++)
    {
      fprintf(STDERR,
              "---------------------------%d-%d-%d-%d-------------\n",
              depth,depth,depth,depth);
      st_frame=SPTR(gb,depth);
      debug_frame(st_frame);
    }
}

static query_level_t *
find_frame(TXTREF colptr)
{
  i4_t depth;
  query_level_t *st_frame;
  TXTREF scan = TNULL,ctblptr,t;
  switch(CODE_TRN(colptr))
    {
    case COLPTR:
      t = OBJ_DESC(colptr);
      TASSERT(CODE_TRN(t)==SCOLUMN,colptr);
      scan = COL_TBL(t); /* pointer to scan */
      break;
    case TBLPTR:
      scan = TABL_DESC(colptr);
      break;
    default:
      debug_trn(colptr);
      yyfatal("Unexpected node in find_frame");
    }
  TASSERT(CODE_TRN(scan)==SCAN,colptr);
  for(depth=0;depth < st_depth(gb_stack);depth++)
    {
      st_frame=SPTR(gb,depth);
      for( ctblptr = DOWN_TRN(st_frame->from);
           ctblptr;
	   ctblptr = RIGHT_TRN(ctblptr))
        if (TABL_DESC(ctblptr) == scan)
	  return st_frame;
    }
  debug_trn(colptr);
  yyfatal("Can't find given column in the frame list...");
  return NULL;
}

static int
is_unique(TXTREF clist)
{
  TXTREF tbl;
  TXTREF ind,col,scan;
  
  TASSERT (CODE_TRN(clist)==COLPTR,clist);
  /*     scan    scolumn  colptr */
  scan = COL_TBL(OBJ_DESC(clist));
  tbl  = COR_TBL(scan);
  switch(CODE_TRN(tbl))
    {
    case TABLE:
      ind=IND_INFO(tbl);
      break;
    case TMPTABLE:
    case SCAN:
    default:
      return 0;
    }
  
  while(ind)
    {
      if (CODE_TRN(ind)==UNIQUE || CODE_TRN(ind)==PRIMARY)
        {
          TXTREF clm;
          TASSERT(ARITY_TRN(ind) > 0 && DOWN_TRN(ind) != TNULL, ind);
          for (clm = DOWN_TRN(ind); clm ; clm = RIGHT_TRN(clm))
            {
              i4_t    id = COL_NO(OBJ_DESC(clm));
              for (col = clist; col ; col = RIGHT_TRN(col))
                {
                  TXTREF scol = OBJ_DESC(col);
                  if ( COL_NO(scol) == id && COL_TBL(scol) == scan )
                    break;
                }
              if (!col) /* if not found */
                break;
            }
          if (!clm) /* unique key found for given list */
            return 1;
        }
      ind = RIGHT_TRN(ind);
    }
  return 0;
}

#define FOR(link_name,cnt) if(!frame->link_name) \
  frame->link_name = gen_node(NOOP); \
  for(cnt=0,cptr=DOWN_TRN(frame->link_name);cptr;cptr=RIGHT_TRN(cptr),cnt++)

#define CHECK_ERROR(msg) if(!cptr) { \
  /*debug_trn(colptr);*/ yyfatal(msg); return TNULL;}

static TXTREF 
replace_function (TXTREF func  ,query_level_t *frame  ) 
{
  TXTREF cptr;
  i4_t    counter,cno,dummy_cntr,count_all=0;
  
  TASSERT(Is_Function(func),func);

  if (TstF_TRN(func,DISTINCT_F) && is_unique(DOWN_TRN(func)))
    {
      ClrF_TRN(func,DISTINCT_F);
      if (CODE_TRN(func)==COUNT)
        {
          free_tree(DOWN_TRN(func));
          ARITY_TRN(func) = 0;
          DOWN_TRN(func) = TNULL;
        }
    }
  
  cptr = DOWN_TRN(func);
  if (frame->one_row_groups==1) /* if we have a fictive selctions */
    {
      if (CODE_TRN(func)==COUNT)
        cptr = #(Const "1" Int[10,0] Int[10,0] ) ;
      else
        cptr = DOWN_TRN(func);
      free_node(func);
      return cptr;
    }
  if(!cptr)
    {
      /* It looks to be 'count(*)' */
      TASSERT(CODE_TRN(func)==COUNT && !TstF_TRN(func,DISTINCT_F),func);
      if(!frame->group_by)
        frame->group_by = gen_node(NOOP);
      count_all = 1;
      counter = 0;
    }
  else
    {
      FOR(group_by,counter)
	if(trn_equal_p(cptr,DOWN_TRN(func)))
	  break;
      if(!cptr)
	{
          TASSERT(RIGHT_TRN(DOWN_TRN(func))==TNULL,func);
	  add_child(frame->group_by,DOWN_TRN(func));
          SetF_TRN(DOWN_TRN(func),EXACT_F);
	}
    }
  FOR(sort_list,cno)
    if(CODE_TRN(cptr)==CODE_TRN(func) && MASK_TRN(cptr)==MASK_TRN(func) &&
       ( count_all == 1 || COL_N(DOWN_TRN(cptr))==counter) )
      break;
  if(!cptr)
    {
      DOWN_TRN(func) = #(Col <counter> ) ;
      RIGHT_TRN(func) = TNULL;
      ARITY_TRN(func) = 1;
      add_child(frame->sort_list,func);
      cno++;
    }
  FOR(scol_list,dummy_cntr)
    if(COL_NO(cptr)==cno /* || dummy_cntr==cno */ )
      break;
  if(!cptr)
    {
      char nm[100];
      sprintf(nm,"%s_%d",NAME(CODE_TRN(func)),counter);
      cptr =  #(SColumn <ltr_rec(nm)> <dummy_cntr> <OPRN_TYPE(func)> ) ;
      add_child(frame->scol_list,cptr);
      cno++;
    }
  return #(ColPtr <cptr> ) ;
}

static TXTREF 
replace_clmn     (TXTREF colptr,query_level_t *frame) 
{
  TXTREF cptr;
  i4_t    counter,cno;
  char   msg[100],cn[100];

  if (frame->one_row_groups==1) /* if we have a fictive grouping */
    return colptr;

  cptr = COR_TBL(COL_TBL(OBJ_DESC(colptr)));
  sprintf(cn,"%s", STRING(COL_NAME(OBJ_DESC(colptr))));
  sprintf(msg,"column '%s' isn't in `group by' clause",cn);
  
  FOR(group_by,counter) /* check column in group by list - selection clause
			   of inner query */
    if(trn_equal_p(cptr,colptr))
      break;
  
  CHECK_ERROR(msg);
  if (TstF_TRN(cptr,EXACT_F))
    yyfatal(msg);
  FOR(sort_list,cno)    /* find reference to this column in sort list */
    if(CODE_TRN(cptr)==COL && COL_N(cptr)==counter)
      break;
  CHECK_ERROR(msg);
  FOR(scol_list,counter) /* find column of sorted scan */
    if(COL_NO(cptr)==cno /* || counter==cno */ )
      break;
  CHECK_ERROR("Internal error: sort list and scan column list mismatch");
  OBJ_DESC(colptr)=cptr;
  return colptr;
}

static void
process_gb_clmn  (TXTREF group_by,query_level_t *frame) 
{
  i4_t     counter;
  TXTREF  cptr;
  /* add new node in sort list and add column in scol_list */
  TASSERT(CODE_TRN(group_by) == GROUP,group_by);
  if (ARITY_TRN(frame->from)==1 && is_unique(DOWN_TRN(group_by)))  
    { /* if we'll got just 1 row after selection */
      frame->one_row_groups=1; /* if we have a fictive grouping */
      return;
    }
  
  FOR(group_by,counter)
    {
      TXTREF newscol,col=OBJ_DESC(cptr);
      TASSERT(CODE_TRN(col)==SCOLUMN,group_by);
      
      COPY_NODE(SCOLUMN,col,newscol);
      COL_TBL(newscol)=TNULL;
      COL_NO(newscol)=counter;
      COL_DEFAULT(newscol) = copy_tree(COL_DEFAULT(newscol));
      
      col = #(Col <counter>);
      
      if (!frame->scol_list)
        frame->scol_list = gen_node(NOOP);
      if (!frame->sort_list)
        frame->sort_list = gen_node(NOOP);
      
      add_child(frame->scol_list,newscol);
      add_child(frame->sort_list,col);
    }
}

/*
 * the extract_regular_query forms reqular query from query
 * with group by when group by condition leds to only one row
 * groups
 */

static TXTREF 
extract_regular_query   (TXTREF query,query_level_t *frame)
{
  TXTREF Op[4];
  i4_t    kitty_lcc; /* it looks to be need by (Run "..." ) */

  TASSERT(frame->sort_list==TNULL,query);

  Op[0] = query;
  free_tree(frame->group_by);  /* erase fictive group list */

  if (!frame->having)
    Op[1] = frame->where;
  else if (!frame->where)
    {
      Op[1] = frame->having;
      CODE_TRN(Op[1]) = WHERE;
    }
  else
    {
      TASSERT(frame->having && frame->where,query);
      Op[1] = #(Where { /* join condition in where & having */
                 (Run "nested_assotiative"  
                   (And {
                     (DOWN (C_code "frame->where "))
                     (DOWN (C_code "frame->having"))
                   })
                 )
               });
      free_node(frame->where);
      free_node(frame->having);
    }
   
# (Op:0 {
     (C_code "frame->from" )
     (C_code "frame->selection")
     (Op:1)
  }) ;
    
  return query;
}

static TXTREF 
make_groupped_tbl       (TXTREF query,query_level_t *frame) 
{
  TXTREF t,s, iq, cptr;
  i4_t    i,j;
  enum token q_code;
  
  q_code = CODE_TRN(query);
  TASSERT( q_code == QUERY ||
           q_code == SELECT ||
           q_code == SUBQUERY, query);
  TASSERT( DOWN_TRN(query) == frame->from, query);
  
  TASSERT(frame->one_row_groups!=1,query);/*we don't have fictive grouping*/
    
  if (ARITY_TRN(frame->group_by)==0)  /* select count(*) from ..... */
    {
      TXTREF scan = TABL_DESC(DOWN_TRN(frame->from));
      if (COR_COLUMNS(scan) == TNULL) /* if (select count(*) from table) */
        add_column(scan,COL_NAME(TBL_COLS(COR_TBL(scan))));
      DOWN_TRN(frame->group_by) = #(ColPtr (C_code "COR_COLUMNS(scan)"));
      ARITY_TRN(frame->group_by) =1;
    }
  CODE_TRN(frame->group_by) = SELECTION;
  FOR(group_by,i)
    {
      TASSERT(CODE_TRN(cptr)==COLPTR,cptr);
      ClrF_TRN(cptr,EXACT_F);
    }
  if(frame->having)
    CODE_TRN(frame->having) = WHERE;
  iq =  
#    (Query {  
       (C_code "frame->from" )
       (C_code "frame->group_by")
       (C_code "frame->where")   
     } )
    ;
  /* 'iq' points to inner query now */

  t = DOWN_TRN(frame->group_by);
  s = #(MakeGroup [] <make_scan(t,iq)> );
  {
    TXTREF t2,t1,v = gen_vect(ARITY_TRN(frame->scol_list));
    TASSERT(ARITY_TRN(frame->scol_list) == ARITY_TRN(frame->sort_list),
            frame->sort_list);
    for(j=0,t1 = DOWN_TRN(frame->sort_list);t1;t1=t2,j++)
      {
        t2 = RIGHT_TRN(t1);
        RIGHT_TRN(t1) = TNULL;
        VOP(v,j).txtp = t1;
      }
    XVEC_TRN(s,0) = v;
    free_node(frame->sort_list);
    frame->sort_list = TNULL;
  }
  
  t = gen_node(TMPTABLE);
  VIEW_QUERY(t) = s;
  add_info(t);
  
  {
    char str[100];
    s = #(Scan "" <-1> <t> );
    add_info(s);                    /* Scan     */
    sprintf(str,"gb_scan_%s_%s_%d",STRING(TBL_FNAME(t)),
            STRING(TBL_NAME(t)),COR_NO(s));
    COR_NAME(s) = ltr_rec(str);
  }

  for(i=0,t=DOWN_TRN(frame->scol_list);t;i++,t = RIGHT_TRN(t))
    {
      TXTREF t1;
      
      TASSERT(CODE_TRN(t) == SCOLUMN,t);
      
      COPY_NODE(COLUMN,t,t1);
      COL_TBL(t1) = COR_TBL(s);
      COL_TBL(t) = s;
      COL_DEFAULT(t1) = copy_tree(COL_DEFAULT(t1));
      
      TASSERT(CODE_TRN(t1) == COLUMN,t1);
      add_info(t1);  /* add tmptable column */
      TASSERT(CODE_TRN(t1) == COLUMN,t1);
    }
  COR_COLUMNS(s) = DOWN_TRN(frame->scol_list);
  t = COR_TBL(s);
  TASSERT(TBL_NCOLS(t) == i,t);
  TASSERT(ARITY_TRN(frame->scol_list) == i,frame->scol_list);
  free_node(frame->scol_list);
  frame->scol_list = TNULL;
  SetF_TRN(t,CHECKED_F);
  
  {
    TXTREF Op[1];
    Op[0] = query;
#   (Op:0 {
      (From { (TblPtr (C_code "s")  ) } )
      (C_code "frame->selection")
      (C_code "frame->having")
    } )
      ;
  }
  if(TstF_TRN(query,HAVING_F))
    {
      SetF_TRN(query,WHERE_F);
      ClrF_TRN(query,HAVING_F);
    }
  ClrF_TRN(query,GROUP_F);
    
  return query;
}

static void
fix_into_subtree(TXTREF select,query_level_t *frame)
{
  TXTREF p,s;
#define T(n) *node_type(n)
  
  TASSERT(frame->query_code==SELECT,select); 
  
  for ( p = DOWN_TRN(frame->into),s = DOWN_TRN(frame->selection);
        p && s;
        p = RIGHT_TRN(p), s = RIGHT_TRN(s))
    {
      TASSERT(CODE_TRN(p)==PARMPTR,frame->into);
      if ( (T(p)).code == SQLType_0 )
        T(p) = T(s);
      else if (!is_casted(T(s),T(p)))
        {
          file_pos = LOCATION_TRN(p);
          lperror("Parameter '%s' and selected expressions types mismatch",
                  STRING(PAR_NAME(OBJ_DESC(p))));
        }
      adjust_parameter(p,s);
          
    }
  if (p||s)
    yyerror("there are different number of elements in 'selection' "
            "and 'into' lists of SELECT statement");
  /*  into      selection from     SELECT  */
  p = RIGHT_TRN(s=RIGHT_TRN(DOWN_TRN(select)));
  if (p && CODE_TRN(p)==INTO)
    {
      RIGHT_TRN(s) = RIGHT_TRN(p);
      ARITY_TRN(select) --;
    }
  free_tree(frame->into);
#undef T
}

#(Rule:static "upd_query"
  (Query {
    (From { (TblPtr (Scan:0)) })
    --
  } )  
  "CODE_TRN(COR_TBL(Op[0]))==TABLE"
  {}
)


static TXTREF
skip_node(TXTREF n)
{
  free_tree(n);
  return gen_node(NOOP);
}

#define LD (flag==CYCLER_LD)
#define DR (flag==CYCLER_DR)
#define RL (flag==CYCLER_RL)
#define SUBTREE(branch)  			  \
if (gb_stack)				          \
{ 						  \
  cf=SPTR(gb,0);				  \
  cf->subtree_code = (LD) ? CODE_TRN(node) : UNKNOWN ;	  \
  if(LD) cf->branch = node;			  \
}

#define CLEAR(v) bzero(&v,sizeof(v))

TXTREF 
group_by_proc(TXTREF node,i4_t flag)
{
  
  query_level_t *cf = NULL;
  static i4_t            inside_function = 0;
  static query_level_t *func_frame      = NULL;

#if 0
  fprintf(STDERR,"group_by:%s:%s:\n:",NAME(CODE_TRN(node)),
          (LD?"LD":(DR?"DR":(RL?"RL":"**"))));
#endif
  
  switch(CODE_TRN(node))
    {
    case SELECT:
    case QUERY:
    case SUBQUERY:
      if(LD)
	{
	  query_level_t frame;
	  CLEAR(frame);
          frame.query_code = CODE_TRN(node);
	  PUSHS(gb,frame);
	}
      else if(DR)
	{
	  query_level_t frame;
          i4_t           distinct = TstF_TRN(node,DISTINCT_F);
	  POPS(gb,frame);
	  if(frame.scol_list == TNULL)
            {
              i4_t is_simple = 0;
              if (frame.one_row_groups==1) /* if we have a fictive grouping */
                node = extract_regular_query(node,&frame);
              rule_upd_query(&is_simple,node);
              if (!is_simple)
                SetF_TRN(node,RO_F);
            }
          else
            {
              /* here we need to construct 2-level query with 
               * makegroup operation
               */
              if (distinct)
                ClrF_TRN(node,DISTINCT_F);
              node = make_groupped_tbl (node,&frame);
              if (distinct)
                SetF_TRN(node,DISTINCT_F);
              SetF_TRN(node,RO_F);
            }
          if (CODE_TRN(node) == SELECT)
            fix_into_subtree(node,&frame);
	}
      break;
    case INTO:
      SUBTREE(into);
      if(gb_stack)
        {
          TASSERT(cf->query_code==SELECT,node);
        }
      break;
    case FROM:
      SUBTREE(from);
      break;
    case WHERE:
      SUBTREE(where);
      break;
    case GROUP:
      SUBTREE(group_by);
      if(DR)
        process_gb_clmn(node,cf);
      break;
    case HAVING:
      SUBTREE(having);
      break;
    case SELECTION:   /* these subtrees must be processed after */
    case RESULT:      /* group by and having                    */
      if (RL)
        {
          extern TXTREF transformer __P((TXTREF in, i4_t f));

          flag = CYCLER_LD;    /* left-2-down pass simulation */
          SUBTREE(selection);
          DOWN_TRN(node) = transformer(DOWN_TRN(node),0);
          flag = CYCLER_RL;    /* recover pass info */
          SUBTREE(selection);
        }
      else
        cycler_skip_subtree = 1;
      break;
    case TBLPTR:
      if (gb_stack && DR)
        {
          TXTREF tbl;
          cf = find_frame(node);
          tbl = COR_TBL(TABL_DESC(node));
          /* do something smart */
        }
      break;
    case COLPTR:
      if (DR && gb_stack)
	{
	  cf = find_frame(node);
	  if (inside_function)
	    {
	      if (func_frame == NULL)
		func_frame = cf;
	      else if (func_frame !=  cf)
		yyerror("Aggregate function applied to incorrect expression");
	    }
	  else
	    {
	      switch(cf->subtree_code)
		{
		case FROM:
                  file_pos = LOCATION_TRN(node);
		  yyfatal("using of columns isn't allowed in 'from' clause");
		case WHERE: /* do nothing - it's already ok */
		  break;
		case GROUP: /* we'll process them later */
		  break;
		case SELECTION:
		case RESULT:
		  if (cf->group_by==TNULL)
		    {
		      cf->is_col_in_selection = 1;
		      break;
		    }
		case HAVING:
		  node = replace_clmn(node,cf);
		  break;
		default:
		  yyfatal("Internal error: "
                          "Column found in unexpected clause of query ");
		}
	    }
	}
      break;
    default: 
      if (Is_Function(node))
	{
          if(!gb_stack)
            {
              yyerror("illegal usage of aggregate function not in SELECT frame");
              return skip_node(node);
            }
	  if (LD)
	    {
	      assert(inside_function==0);
	      inside_function = 1;
	      func_frame = NULL;
	    }
	  else if (DR)
	    {
	      inside_function = 0;
              if(!func_frame)
                func_frame = SPTR(gb,0);
	      switch(func_frame->subtree_code)
	        {
                case WHERE: 
                  yyerror("illegal usage of aggregate function in "
                          "WHERE clause");
                  return skip_node(node);
                  break;
                case SELECTION:
                  if (func_frame->is_col_in_selection)
                    {
                      if (func_frame->is_col_in_selection==1)
                        yyerror("Illegal usage both aggregate function"
                                " and columns in SELECT");
                      func_frame->is_col_in_selection++;
                      return skip_node(node);
                    }
                  break;
                default: break;
                }
	      node = replace_function(node,func_frame);
	      func_frame = NULL;
	    }
        }
    }
  return node;
}

