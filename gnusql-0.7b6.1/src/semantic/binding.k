/*
 *  binding.c - base binding (table and column binding)            
 *              GNU SQL server
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Eugene W. Woynov,1994
 *  Corrected by Michael Kimelman, 1995
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
 *  Contacts:  gss@ispras.ru
 *
 */

/* $Id: binding.k,v 1.252 1998/09/29 22:23:46 kimelman Exp $ */

#include "global.h"
#include "seman.h"
#include "funall.h"
#include "cycler.h"
#include "tassert.h"
#include <assert.h>

/********************************************************************/
/***    external variable				          ***/
/********************************************************************/

static TXTREF *S_stack = NULL;
static i4_t c_stack = 0, stack_size = 0;
i2_t s_e_error    = 0;
i2_t s_w_error    = 0;

static void base_binding  __P((TXTREF));
static void scan_binding (TXTREF);	/* scan binding               */
				        /* (bypass subquery)          */
static void connect_field (TXTREF);	/* column binding             */
static void find_subquery (TXTREF);	/* subquery retrieval         */

static TXTREF exist_column (LTRLREF n1,	/* check of scan existing     */
                            LTRLREF n2,	/* in stack on column name    */
                            LTRLREF n3);/* (scope by context.f_scope) */

static void table_denaming (TXTREF);	/* table denaming at list     */
static void sem_add_stack (VCBREF);	/* add stack                  */
static void stack_recovery ();         	/* stack recovery             */
static void is_scan_uniq (VCBREF);	/* check of scan unique       */
                                        /* in stack                   */
static TXTREF find_star  __P((TXTREF));
void          load_table(TXTREF ptbl);
static void   bind_schema_action  __P((enum token action, TXTREF ptbl));
/********************************************************************/

static int f_grant = 0 ;

static struct 
{
  TXTREF ref_order;
  TXTREF star;
} binding_seman =
{TNULL, TNULL};

/********************************************************************/

i4_t
binding (void)
{
  TXTREF t, rt;
  s_w_error = s_e_error = 0;
  for (t = ROOT; t; t  = rt)
    {
      rt = RIGHT_TRN(t);
      if ((ROOT == t) && (CODE_TRN(t) == CREATE || CODE_TRN(t) == ALTER))
        if ( CREATE_OBJ(t)==TNULL)
          ROOT = rt;
      base_binding (t);
      assert(c_stack == 0);
    }
  xfree(S_stack);
  S_stack = NULL;
  stack_size = 0;
  errors += s_e_error;
  s_e_error = 0;
  return 0;
}				/* end binding */

/**********************************************************************/

static void
base_binding (TXTREF txtref)
{
  TXTREF curr_ref = TNULL;
  Tabid  tabid;
  char   type;

/**** scan find */

  switch (CODE_TRN (txtref))
    {
    case DECL_CURS:
      LOCAL_VCB_ROOT = STMT_VCB(txtref);
      curr_ref = DOWN_TRN (txtref);
     
      if (RIGHT_TRN (curr_ref) && CODE_TRN (RIGHT_TRN (curr_ref)) == ORDER)
        binding_seman.ref_order = RIGHT_TRN (curr_ref);
      
    case CUR_AREA:
      base_binding (DOWN_TRN (txtref));
      break;

    case UNION:
      for (curr_ref =DOWN_TRN (txtref); curr_ref;curr_ref =RIGHT_TRN (curr_ref))
        base_binding (curr_ref);
      break;
      
    case INSERT:
    case UPDATE:		
    case DELETE:		
      curr_ref = txtref;
    case SELECT:
      LOCAL_VCB_ROOT = STMT_VCB(txtref);
    case SUBQUERY:
    case QUERY:
      curr_ref = DOWN_TRN (txtref);

      /* new stack frame */
      sem_add_stack(TNULL);

      table_denaming (curr_ref);

      scan_binding (txtref);
      
      if(CODE_TRN(txtref)==SUBQUERY) 
        {           /* result    from     subquery */
          TXTREF res = RIGHT_TRN(DOWN_TRN(txtref));
          TASSERT(CODE_TRN(res) == RESULT,txtref);
          if (CODE_TRN(DOWN_TRN(res))==STAR)  
            {  /* (select * from ....)  -> (select 0 from ...) */
              TXTREF t;
              free_node(DOWN_TRN(res));
              for ( t = DOWN_TRN(DOWN_TRN(txtref)); t; t = RIGHT_TRN(t))
                if ( COR_COLUMNS(TABL_DESC(t)))
                  break;
              if (!t) /* if there is no scans with columns */
                {
                  TXTREF sc,tbl;
                  t = DOWN_TRN(DOWN_TRN(txtref));
                  sc = TABL_DESC(t);
                  tbl = COR_TBL(sc);
                  TASSERT(TstF_TRN(tbl,CHECKED_F),txtref);
                  add_column(sc, COL_NAME(TBL_COLS(tbl)));
                }
              /*
               * now t is a pointer to nonempty scan and we can replace
               * '*' by the first column of the scan
               */
              DOWN_TRN(res) = #(ColPtr <COR_COLUMNS(TABL_DESC(t))>);
            }
        }
      
      if (binding_seman.ref_order)
        {
          scan_binding (binding_seman.ref_order);
          binding_seman.ref_order = TNULL;
        }

      find_subquery (txtref);

      stack_recovery ();
      break;

    case DROP:
      load_table(CREATE_OBJ (txtref));
      break;  
    case CREATE:
      if (CREATE_OBJ(txtref)==TNULL)
	{
	  if (!existsc (STRING (GL_AUTHOR)))
            lperror("Error: schema %s has already exist",
                    STRING (GL_AUTHOR));
	}
      else if (existtb (STRING (TBL_FNAME (CREATE_OBJ (txtref))),
                        STRING (TBL_NAME (CREATE_OBJ (txtref))),
                        &tabid,
                        &type))
        {
          TXTREF tbl = CREATE_OBJ (txtref);
          file_pos = LOCATION_TRN(txtref);
          lperror("Error: %s %s.%s has already exist\n",
                  (CODE_TRN(tbl)==TABLE?"table":"view"),
                  STRING (TBL_FNAME (tbl)),
                  STRING (TBL_NAME (tbl)));
        }
    case ALTER:
      if (CREATE_OBJ(txtref)==TNULL)
        {
          free_node(txtref);
          return;
        }
      if (CODE_TRN(txtref)==ALTER)
        yyfatal("ALTER table or view has not realized yet");
      switch (CODE_TRN (CREATE_OBJ (txtref)))
        {
        case TABLE:
          bind_schema_action (CODE_TRN(txtref), CREATE_OBJ (txtref));
          break;
        case VIEW:
          base_binding (VIEW_QUERY (CREATE_OBJ (txtref)));
        default:
          break;
        }
      break;
    case GRANT:
      if (CODE_TRN (TABL_DESC (DOWN_TRN (txtref))) !=ANY_TBL &&
          TstF_TRN(TABL_DESC (DOWN_TRN (txtref)),CHECKED_F))
        break;
      else
	{
	  f_grant = 1;
	  table_denaming (DOWN_TRN (txtref));
	  f_grant = 0;
	}
      break;
    default:
      SFATAL_ERROR;
    }				/* end switch */
}				/* end base_binding */

/**********************************************************************/
/***    scan binding (bypass subquery)                              ***/
/**********************************************************************/

static void
scan_binding (TXTREF parent)
{
  TXTREF txtref;

  for (txtref = DOWN_TRN (parent);
       txtref;
       txtref = RIGHT_TRN (txtref))
    switch(CODE_TRN(txtref))
      {
      case COLPTR:
        connect_field(txtref);
        break;
      case SORT_COL:
        connect_field(OBJ_DESC(txtref));
        break;
      case STAR:
        if (binding_seman.star)
          {
            TXTREF scol, chain =TNULL, from = binding_seman.star, t;
            
            binding_seman.star =TNULL;
            TASSERT(DOWN_TRN(parent)==txtref && ARITY_TRN(parent)==1,parent);
            free_node (txtref);
            DOWN_TRN(parent) = TNULL;
            ARITY_TRN(parent) = 0;
            
            for (t = DOWN_TRN (from); t; t = RIGHT_TRN(t))
              {
                check_scan_cols(t);
                for (scol = COR_COLUMNS(TABL_DESC (t));
                     scol;
                     scol = COL_NEXT(scol))
                  {
                    if (chain == TNULL)
                      txtref = DOWN_TRN (parent) = chain = #(ColPtr <scol>);
                    else
                      chain  = RIGHT_TRN (chain) = #(ColPtr <scol>);
                    ARITY_TRN(parent)++;
                  }
              }
          }
        break;
      case QUERY:
      case SUBQUERY:
        continue;
      default:
        if (HAS_DOWN(txtref))
          scan_binding (txtref);
      }
}				/* end scan_binding */


/********************************************************************/
/***    subquery retrieval                                        ***/
/********************************************************************/

static void
find_subquery (TXTREF node)
{
  TXTREF t;
  enum token code;

  for (t = DOWN_TRN (node);t; t = RIGHT_TRN (t))
    {
      code = CODE_TRN (t);
      if ((code == QUERY) || (code == SUBQUERY))
        base_binding (t);
      else if (HAS_DOWN(t))
	find_subquery (t);
    }
}				/* end find_subquery */


/********************************************************************/
/***    column binding                                            ***/
/********************************************************************/

static void
connect_field (TXTREF colptr)	/* txtref to COLPTR */
{
  TXTREF scol;

  TASSERT (CODE_TRN (colptr) == COLPTR , colptr);
  
  scol = OBJ_DESC (colptr);
  if (CODE_TRN (scol) == SCOLUMN)
    return;

  TASSERT (CODE_TRN (scol) == COLUMN_HOLE,colptr);

  /* set posiiton for the case of following error report */
  file_pos = LOCATION_TRN(colptr);  
  
  scol = exist_column (CHOLE_AUTHOR (scol),
                       CHOLE_TNAME (scol),
                       CHOLE_CNAME (scol));

  free_node(OBJ_DESC(colptr));
  OBJ_DESC(colptr) = scol;
  return;
}				/* end connect_field */

/**********************************************************************/
/***  check of scan existing in stack on column name                ***/
/**********************************************************************/

static TXTREF
exist_column (LTRLREF an, /* authorization name */
	      LTRLREF tn, /* table  name */
	      LTRLREF cn  /* column name */
              )
{
  i4_t   stack;

  for (stack = c_stack -1 ; stack >=0 ; stack--)
    if (S_stack[stack])
      {
        TXTREF scn = S_stack[stack];
        TXTREF tbl = COR_TBL(scn);
        LTRLREF scan_name, tbl_an, tbl_tn;
        scan_name = COR_NAME (scn);
        tbl_an = TBL_FNAME (tbl);
        tbl_tn = TBL_NAME (tbl);

        if ( an )
          {
            if ( (an !=tbl_an) || (tn != tbl_tn) )
              continue;
          }
        assert ( an == NULL || tbl_an == an );
        if ( tn )
          if ((tn != scan_name) && (tn !=tbl_tn))
            continue;
        assert ( tn == 0 || tn == tbl_tn || tn == scan_name );
        {
          TXTREF c = find_column(tbl,cn);
          if(c)
            {
              c = find_column(scn,cn);
              if (!c)
                {
                  add_column(scn,cn);
                  c = find_column(scn,cn);
                }
              return c;
            }
        }
      }
  lperror("There is no table containing '%s.%s.%s'",
          STRING(an),STRING(tn),STRING(cn));
  return TNULL;
}				/* end exist_column */

void
correct_tbl_ind_info (TXTREF cur_node)
{
  if (!cur_node)
    return;
  
  if (CODE_TRN (cur_node) == TBLPTR)
    {
      TXTREF scan  = TABL_DESC (cur_node);
      TXTREF table = COR_TBL (scan);
      
      if (!((TBL_TABID (table)).untabid))
        {
          if (!existtb (STRING (TBL_FNAME (table)),
                        STRING (TBL_NAME (table)),
                        &(TBL_TABID (table)), NULL))
            yyfatal ("Incorrect system information about VIEW");

          if (IND_INFO (table))
            free_tree (IND_INFO (table));
            
          IND_INFO (table) = existind (&(TBL_TABID(table))); 
        }      
    }      
  correct_tbl_ind_info (RIGHT_TRN (cur_node));
  if (HAS_DOWN (cur_node))
    correct_tbl_ind_info (DOWN_TRN (cur_node));
} /* correct_tbl_ind_info */

static void
adjust_name(char *which_name, LTRLREF *l)
{

  if (strlen(STRING(*l)) > MAX_USER_NAME_LNG)
    {
      char buf[MAX_USER_NAME_LNG + 1];
      bcopy (STRING(*l),buf,MAX_USER_NAME_LNG);
      buf[MAX_USER_NAME_LNG]=0;
      lperror("Warning: %s name '%s' is too long - adjusted to '%s'",
              which_name, STRING (*l), buf);
      errors--;
      *l = ltr_rec(buf);
    }
}

void 
load_table(TXTREF ptbl)
{
  Tabid      tabid;
  char       type;
  i4_t        i,ncols;
  sql_type_t column_type, *coltype = &column_type;
  VCBREF     cl_new;
  
  if ( TstF_TRN (ptbl, CHECKED_F))
    return;
  adjust_name("table", &TBL_NAME (ptbl));
  adjust_name("table owner", &TBL_FNAME (ptbl));
  if (!existtb (STRING (TBL_FNAME (ptbl)), STRING (TBL_NAME (ptbl)), &tabid, &type))
    { /* table was not found */
      char 
	*a = STRING (TBL_FNAME (ptbl)),
	*c = STRING (TBL_NAME (ptbl));
      i4_t is_fname = (a && *a), is_name = c && *c;
      lperror("Error: table '%s%s%s' was not found",
	      is_fname ? a : "", is_fname ? "." : "",
	      is_name ? c : "(dummy)");
      yyfatal("Abort");
    }
  /* table was found */
  /* assert tabid, val_type, tabclnm in global vocab */
  switch(CODE_TRN(ptbl))
    {
    case ANY_TBL: break;
    case TABLE:   type = (type=='B'?'B':'E'); break;
    case VIEW:    type = (type=='V'?'V':'E'); break;
    default:      type = 'E';
    }
  if (type=='E')
    {
      /* table was not found or mismatchs to our expectation */
      char 
	*a = STRING (TBL_FNAME (ptbl)),
	*c = STRING (TBL_NAME (ptbl));
      i4_t is_fname = (a && *a), is_name = c && *c;
      lperror("Error: Incorrect table '%s%s%s' type",
	      is_fname ? a : "", is_fname ? "." : "",
	      is_name ? c : "(dummy)");
      yyfatal("Abort");
    }
  
  switch(type)
    {
    case 'B': CODE_TRN(ptbl) = TABLE; TBL_TABID(ptbl) = tabid;         break;
    case 'V': CODE_TRN(ptbl) = VIEW ; VIEW_UNID(ptbl) = tabid.untabid; break;
    default:
      fprintf (STDERR, "\tInternal error at %s:%d:\n\t unexpected table"
	       " type received from existtb -'%c'(%d)\n",
	       __FILE__, __LINE__, type, (i4_t) type);
      yyfatal("Abort");
    }
  
  i = ncols = tabclnm (&tabid, (i4_t *) &(TBL_NNULCOL (ptbl)));
  while (i--)
    {
      char *cl_name =NULL, *defval =NULL, *defnull =NULL;
      i4_t   def_func_id;
      TXTREF defv = TNULL;
      i4_t rc;
      
      if (find_column_by_number (ptbl , i) ) 
	continue;
      /* insert column into global vocab */;
      rc = tab_cl (&tabid, i, coltype, &cl_name, &defval, &defnull,&def_func_id);
      if ( rc < 0)
	{ /* If  column of table was not found; */
	  char *a = STRING (TBL_FNAME (ptbl));
	  char *c = STRING (TBL_NAME  (ptbl));
	  i4_t   is_fname = a && *a;
          i4_t   is_name  = c && *c;
          errno = rc;
	  lperror("Error: inconsistent information in DB of %d column"
                  " of table '%s%s%s'",
		  i, is_fname ? a : "", is_fname ? "." : "",
		  is_name ? c : "(dummy)");
          continue;
	}
      cl_new = find_column (ptbl, ltr_rec (cl_name));
      
      if (defval)
	defv = #(Const <ltr_rec(defval)> <*coltype> <*coltype>) ;
      else if ( def_func_id)
	switch (def_func_id)
	  {
	  case USERNAME : defv = gen_node (USERNAME); break;
	  default :
	    fprintf (STDERR,
                     "\t%s:%d: Internal error: unrecognized function name '%s'\n\t",
		     __FILE__, __LINE__,NAME(def_func_id));
	  }
      else if ( *defnull==0 )
	defv = #(Null_vl <*coltype> ) ;
      if (!cl_new)
        {
          cl_new = #(Column <ltr_rec(cl_name)> <i> <*coltype> <ptbl> <defv> );
          cl_new = add_info(cl_new);
        }
      else /* if column has already in list */
        { /* check that it was just a stub */
          TASSERT(COL_NO(cl_new)==-1,ptbl);
          /* and update parameters */
          COL_NO(cl_new)      =  i;
          COL_TYPE(cl_new)    = *coltype;
          COL_DEFAULT(cl_new) =  defv;
        }
    }/* insert column into global vocab */
  while ( TBL_NCOLS (ptbl) != ncols )
    { /* if there are some unexpected columns */
      TXTREF col = TBL_COLS(ptbl);
      while (COL_NO(col) != -1)
        col = RIGHT_TRN(col);
      TASSERT(col,ptbl); /* we must find such column */
      lperror("Column '%s' not found in '%s.%s'",
              STRING(COL_NAME(col)),
              STRING(TBL_FNAME(ptbl)),STRING(TBL_NAME(ptbl)));
      del_info(col);
    }
  switch(CODE_TRN(ptbl))
    {
    case TABLE:
      /* load index information */
      IND_INFO(ptbl) = existind(&(TBL_TABID(ptbl))); 
      break;
    case VIEW:
      {
        /* load view query tree */
	TXTREF lvcb, sav_segm, tr_segm, vt;
	char  *beg_segm,*b;
	i4_t    len_segm;
        
	lvcb = LOCAL_VCB_ROOT;
	LOCAL_VCB_ROOT= TNULL;
	len_segm = getview (VIEW_UNID(ptbl), &beg_segm);
        b = xmalloc(len_segm);          /* vmemory will free memory later */
        bcopy(beg_segm,b,len_segm);
	sav_segm = get_current_tree_segment ();
        tr_segm = load_tree_segment (b,len_segm);
        vt = resolve_local_reference ("VIEW_TREE");
	set_current_tree_segment (sav_segm);
	LOCAL_VCB_ROOT = TNULL;         /* VIEW_VCB(ptbl)                 */
	VIEW_QUERY(ptbl) = copy_tree (vt);
	VIEW_VCB(ptbl) = LOCAL_VCB_ROOT;
        correct_tbl_ind_info (VIEW_QUERY(ptbl));
	LOCAL_VCB_ROOT = lvcb;
        dispose_tree_segment (tr_segm); /* implies freeing memory         */
        assert(sav_segm == get_current_tree_segment ());
      }
      break;
    default: break;
    }
  sort_list_by_col_no(&(TBL_COLS(ptbl)),NULL,NULL,1,NULL,NULL);
  SetF_TRN (ptbl, CHECKED_F);
}

/********************************************************************/
/*  table denaming at list                                          */
/********************************************************************/

static void
table_denaming (TXTREF t)
{
  VCBREF scan;
  TXTREF ptbl;
  
    switch (CODE_TRN (t))
      {
      case TBLPTR:
	scan = TABL_DESC (t);
	if (f_grant)
          ptbl = scan; 
	else
          ptbl = COR_TBL (scan);
	file_pos = LOCATION_TRN(t);
	load_table(ptbl);
	if (!f_grant)
          {
            is_scan_uniq (scan);
            sem_add_stack (scan);
          }
	break;
      case FROM:
        {
          TXTREF op;
          binding_seman.star = find_star (t);
          for (op = DOWN_TRN (t); op ; op = RIGHT_TRN (op))
            table_denaming (op);
        }
        break;
      default:
        {
          debug_trn(t);
          yyfatal("Unexpected node in tblptr's list");
        }
      }				/* end switch */

}				/* end table_denaming */

/********************************************************************/
/***   add stack                                                  ***/
/********************************************************************/

static void
sem_add_stack (VCBREF txtref)
{
  if (c_stack >= stack_size)
    {
      stack_size +=100*sizeof(TXTREF);
      if (S_stack)
        S_stack = (TXTREF*)xrealloc(S_stack,stack_size);
      else
        S_stack = (TXTREF*)xmalloc(stack_size);
    }
  S_stack[c_stack++] = txtref;
}				/* end sem_add_stack */

/********************************************************************/
/***    check of scan unique in stack                             ***/
/********************************************************************/

static void
is_scan_uniq (TXTREF scan)
{
  i4_t i;
  LTRLREF name_scan, autor, name_table;

#define IS_EQ(s,s1) (!strcmp (STRING (s), STRING (s1)))
#define EXIT(msg)   {lperror(msg,STRING (name_scan));return;}
                                
  name_scan = COR_NAME (scan);
  autor = TBL_FNAME (COR_TBL (scan));
  name_table = TBL_NAME (COR_TBL (scan));

  for (i = c_stack; S_stack[i - 1] != TNULL; i--)
    {
      TXTREF s = S_stack[i - 1], t = COR_TBL(s);
      LTRLREF sn = COR_NAME(s);
      
      if (name_scan)
        {
          if (sn)
            {
              if (IS_EQ(name_scan,sn))
                EXIT("Error : Umbigous correlation name '%s'");
            }
          else
            {
              if (IS_EQ(GL_AUTHOR,TBL_FNAME (t)) &&
                  IS_EQ(name_scan,TBL_NAME  (t)))
                EXIT("Error : Umbigous correlation name '%s' -"
                     "conflict with table name");
            }
        }
      else
	{
	  if (sn)
	    {
	      if (IS_EQ(sn   ,name_table)  &&
		  IS_EQ(autor,GL_AUTHOR ))
		EXIT("Error : Umbigous correlation name '%s' -"
                     "conflict with table name");
	    }
	  else
	    {
	      if (IS_EQ(TBL_NAME (t) ,name_table) &&
		  IS_EQ(TBL_FNAME (t),autor     ))
		{
                  lperror("Error : Umbigous table name '%s.%s'",
                          STRING (autor),
                          STRING (name_table));
		  return;
		}
	    }
	}
    }
#undef IS_EQ
#undef EXIT
}				/* end is_scan_uniq */

/********************************************************************/
/***   stack recovery                                             ***/
/********************************************************************/

static void
stack_recovery ()
{
  if (!c_stack)
    SINT_ERROR;

  while (S_stack[c_stack - 1])
    {
      TXTREF sc = S_stack[c_stack - 1];
      if ((CODE_TRN(sc) == SCAN) && (COR_COLUMNS(sc) == TNULL))
        {
          TXTREF tbl = COR_TBL(sc);
          TASSERT(TstF_TRN(tbl,CHECKED_F),sc);
          add_column(sc, COL_NAME(TBL_COLS(tbl)));
        }
      c_stack--;
    }
  c_stack--;

}				/* end stack_recovery */

/********************************************************************/
/*** find star                                                    ***/
/********************************************************************/

static TXTREF
find_star (TXTREF txtref)
{
  TXTREF curr_ref;
  
  if (!RIGHT_TRN (txtref)) return (TNULL);
  if (CODE_TRN (curr_ref =RIGHT_TRN (txtref)) == SELECTION)
    if (CODE_TRN (DOWN_TRN (curr_ref)) ==STAR)
      return (txtref);
    
  return (TNULL);
}				/* end find_star */

/********************************************************************/
/***                                                              ***/
/********************************************************************/

static void
bind_schema_action (enum token action, TXTREF ptbl)
{
  TXTREF checks,tcol;
  assert(action == CREATE);             /* ALTER is much more unclear */
  TASSERT(CODE_TRN(ptbl)==TABLE,ptbl);
  adjust_name("table", &TBL_NAME (ptbl));
  adjust_name("table owner", &TBL_FNAME (ptbl));
  for (tcol = TBL_COLS(ptbl); tcol; tcol = COL_NEXT(tcol))
    adjust_name("column", &COL_NAME (tcol));
  for (checks = TBL_CONSTR(ptbl); checks; checks=RIGHT_TRN(checks))
    if (CODE_TRN(checks) == FOREIGN)
      {
        /*
         * (Foreign {
         *   (Locallist { (ColPtr (Column))...(ColPtr..) })
         *   (Reference { (OPtr (Table))
         *                (Noop { (Colptr (Column_hole|Column)) ... }) })
         * } ) 
         */
        TXTREF local    = DOWN_TRN  (checks);         /* referenced column list     */
        TXTREF refer    = RIGHT_TRN (local);          /* reference node             */
        TXTREF reftbl   = OBJ_DESC(DOWN_TRN(refer));  /* referenced table           */
        TXTREF ind_cols = RIGHT_TRN(DOWN_TRN(refer)); /* referenced index cols list */
        TXTREF index;
        i4_t    ind_col_n = ARITY_TRN(ind_cols);
        load_table(reftbl);
        file_pos = LOCATION_TRN(refer);
        for (index = IND_INFO(reftbl); index; index = RIGHT_TRN(index))
          if (CODE_TRN(index)!=INDEX)
            {
              if  (ind_col_n==ARITY_TRN(index))
                { /* compare col list */
                  TXTREF icol,rcol;
                  for (icol = DOWN_TRN(index),rcol = DOWN_TRN(ind_cols);
                       icol && rcol;
                       icol = RIGHT_TRN(icol),rcol = RIGHT_TRN(rcol))
                    {
                      TXTREF r = OBJ_DESC(rcol);
                      if ( ((CODE_TRN(r) == COLUMN)?COL_NAME(r):CHOLE_CNAME(r)) !=
                           COL_NAME(OBJ_DESC(icol)))
                        break;
                        
                    }
                  if (!icol && !rcol) /* if lists are equal */
                    break;;
                }
              else if (ind_col_n==0 && CODE_TRN(index)==PRIMARY )
                break;
            }
        if (!index) /* if index not found */
          {
            lperror("There is illegal reference on table '%s.%s' ",
                    STRING(TBL_FNAME(reftbl)),STRING(TBL_NAME(reftbl)));
            break;
          }
        /* we found appropriate index - now we can check reference correctness */
        if (ARITY_TRN(local) != ARITY_TRN(index))
          {
            lperror("number of referenced columns mismath to number of "
                    "columns in index");
            break;
          }
        {
          TXTREF icol,rcol;
          for (icol = DOWN_TRN(index),rcol = DOWN_TRN(local);
               icol && rcol;
               icol = RIGHT_TRN(icol),rcol = RIGHT_TRN(rcol))
            if ( type2long(COL_TYPE(OBJ_DESC(rcol))) !=
                 type2long(COL_TYPE(OBJ_DESC(icol))))
              lperror("Column '%s.%s.%s' type (%s) mismaths to type of "
                      "referenced  column '%s.%s.%s'",
                      STRING(TBL_FNAME(ptbl)),STRING(TBL_NAME(ptbl)),
                      STRING(COL_NAME(OBJ_DESC(rcol))),
                      type2str(COL_TYPE(OBJ_DESC(rcol))),
                      STRING(TBL_FNAME(reftbl)),STRING(TBL_NAME(reftbl)),
                      STRING(COL_NAME(OBJ_DESC(icol))));
          assert (!icol && !rcol);
        }
        if (errors)
          break;
        /* if everything ok */
        /* restruct refernce structure */
        RIGHT_TRN(local) = #(OPtr <index>);
        free_tree(refer);
        /* add new index creation */
        index = copy_tree(local);  /* copy column reference list */
        CODE_TRN(index) = INDEX;   /* rename parent node         */
        add_info(index);           /* add it to table list       */
      }
}
