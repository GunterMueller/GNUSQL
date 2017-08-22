/************************************************************************/
/* check_tp - type checking and correction                              */
/*
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
 *  Contacts:  gss@ispras.ru
 *
 */

/* $Id: check_tp.c,v 1.248 1998/09/15 05:08:43 kimelman Exp $ */

#include "global.h"
#include "cycler.h"
#include "tassert.h"

#define CORRECT_TYPE(t) correct_type (&(t))
#define DEFAULT_STR_LEN 256
#define OPER_HAS_TYPE(n) (Is_Operation (n) && Is_Typed_Operation (n))

static sql_type_t
correct_type (sql_type_t *t)
{
  return (*t = pack_type (t->code, t->len, t->prec));
}

static CODERT
comp_type (sql_type_t type1, sql_type_t type2, sql_type_t * p_type)
{
  if ( is_casted(type1,type2) )
    {
      *p_type = type1;
      return (SQL_SUCCESS);
    }
  if ( is_casted(type2,type1) )
    {
      *p_type = type2;
      return (SQL_SUCCESS);
    }
  return SQL_ERROR;
} /* comp_type */

void
put_type_to_tree (TXTREF arg_node, sql_type_t *need_type)
{
  TXTREF cur_node;
  sql_type_t *type_ptr;
  
  if (OPER_HAS_TYPE (arg_node))
    {
      type_ptr = node_type (arg_node);
      TASSERT (type_ptr->code == SQLType_0, arg_node);
      *type_ptr = *need_type;
    }
  else
    switch (CODE_TRN (arg_node))
      {
      case PARMPTR:
        {
          TXTREF param = OBJ_DESC (arg_node);
          sql_type_t st,tt;
          
          st = PAR_STYPE (param);
          tt = PAR_TTYPE (param);
          comp_type (st, tt, &tt);
          comp_type (*need_type, tt, &tt);
          CORRECT_TYPE (tt);
          PAR_STYPE (param) = PAR_TTYPE (param) = tt;
        }
        break;
      case NULL_VL:
        NULL_TYPE(arg_node) = *need_type;
        break;
      default: ; 
      }
        
  if (HAS_DOWN (arg_node))
    for (cur_node = DOWN_TRN (arg_node); cur_node;
         cur_node = RIGHT_TRN (cur_node))
      put_type_to_tree (cur_node, need_type);
} /* put_type_to_tree */

static void
check_type (TXTREF node, sql_type_t need_type, char *err_string)
/* type of 'node' must be convinient for type need_type */
{
  sql_type_t tp, *type_source = node_type (node);

  if (type_source->code == SQLType_0)
    put_type_to_tree (node, &need_type);
  else
    if (comp_type (*type_source, need_type, &tp) == SQL_ERROR)
      yyerror (err_string);
} /* check_type */

static void
check_arg_types (TXTREF node)
/* compatibility checking for arguments of operation 'node',  *
 * putting of result type to 'node' if it hasn't type yet,    *
 * & making of types for trees - arguments with unknown types *
 * if result type is known now.                               */
{
  i4_t put_type_fl = FALSE;
  sql_type_t res_type, *right_type, *nd_type_ptr = &(OPRN_TYPE (node));
  TXTREF dn_node, right_node, arg_node;

  TASSERT (OPER_HAS_TYPE (node), node);
  TASSERT (HAS_DOWN (node) && DOWN_TRN (node), node);

  /* res_type is used for finding of result type */
  res_type = pack_type(SQLType_0,0,0);
  dn_node = DOWN_TRN (node);
  for (right_node = dn_node; right_node;right_node = RIGHT_TRN (right_node))
    {
      right_type = node_type (right_node);
      if (right_type->code == SQLType_0)
        put_type_fl = TRUE;
      if (comp_type (res_type, *right_type, &res_type) == SQL_ERROR)
        {
          file_pos = LOCATION_TRN(node);
          lperror("Incompatible types of arguments of operation '%s' ",
                  NAME (CODE_TRN(node)));
        }
    }

  /* saving of res_type in argument node */
  if (nd_type_ptr->code == SQLType_0)
    *nd_type_ptr = res_type;
  if (res_type.code == SQLType_0)
    res_type = *nd_type_ptr;
  
  /* making of types for argument trees if it's needed and possible */
  if (nd_type_ptr->code != SQLType_0 && put_type_fl == TRUE)
    for (arg_node = dn_node; arg_node; arg_node = RIGHT_TRN (arg_node))
      if (node_type (arg_node)->code == SQLType_0)
        put_type_to_tree (arg_node, &res_type);
} /* check_arg_types */

TXTREF 
handle_types (TXTREF cur_node, i4_t f)
{
  static sql_type_t type_bool  = {T_BOOL, 0, 0};
  static sql_type_t type_0     = {SQLType_0, 0, 0};

  if (!f)
    return cycler (cur_node, handle_types, CYCLER_LD + CYCLER_RL +
                   CYCLER_DR + CYCLER_LN + CYCLER_RLC);

  if (f == CYCLER_LD)
    {
      if ( OPER_HAS_TYPE (cur_node) &&
           node_type (cur_node)->code != SQLType_0)
        cycler_skip_subtree = 1;
      if ( TstF_TRN(cur_node,PATTERN_F))
        cycler_skip_subtree = 1;
    }
  else if (f == CYCLER_RL)
    {
      enum token code = CODE_TRN (cur_node);
        
      if ( TstF_TRN(cur_node,PATTERN_F))
        return cur_node;

      if (OPER_HAS_TYPE (cur_node))
        {
          if ( Is_Log_Oper_Code (code) ||   /* NOT, OR, AND   */
               Is_Comp_Code     (code) ||   /* >,<,==,>=,<=,!=*/
               Is_Predic_Code   (code))     /* predicates     */
            {
              OPRN_TYPE (cur_node) = type_bool;
              
              if (Is_Log_Oper_Code(code))
                {
                  sql_type_t *arg_typ;
                  TXTREF arg = DOWN_TRN (cur_node);
                  
                  for (; arg; arg = RIGHT_TRN (arg))
                    {
                      arg_typ = node_type (arg);
                      if (arg_typ->code != T_BOOL)
                        yyerror ("Argument of logic operation isn't of boolean type");
                    }
                }
              else
                check_arg_types (cur_node); 
            }
          else if (code != SUBQUERY && code != COUNT)
            {
              OPRN_TYPE (cur_node) = type_0;
              check_arg_types (cur_node);
            }
          CORRECT_TYPE (OPRN_TYPE (cur_node));
        }

      switch (code)
        {
        case COLPTR:
          CORRECT_TYPE (COL_TYPE (OBJ_DESC (cur_node)));
          break;
          
        case CONST:
          if (CNST_TTYPE (cur_node).code == SQLType_0)
            CNST_TTYPE (cur_node) = CORRECT_TYPE (CNST_STYPE (cur_node));
          else
            CNST_STYPE (cur_node) = CORRECT_TYPE (CNST_TTYPE (cur_node));
          break;

        case CREATE:
          if (CODE_TRN (CREATE_OBJ (cur_node)) == VIEW)
            {
              TXTREF view = CREATE_OBJ (cur_node);
              TXTREF col, sel, query = VIEW_QUERY (view);
              
              handle_types (query, 0);
              /*        sel_list(selection(from    (query */
              for(sel = DOWN_TRN(RIGHT_TRN(DOWN_TRN(query))),
                    col = TBL_COLS(view); col && sel;
                  col = COL_NEXT(col), sel = RIGHT_TRN(sel))
                COL_TYPE(col) = *node_type(sel);
              TASSERT( !col && !sel, view);
            }
          break;
          
        case AVG:
          OPRN_TYPE (cur_node) = pack_type (SQLType_Real, 0, 0);
        case SUM:
          if (node_type (DOWN_TRN (cur_node))->code == SQLType_Char)
            lperror("String can't be an argument of aggregate function %s",
                    NAME (cur_node));
          break;
          
        case LIKE:
          {
            TXTREF like_arg = DOWN_TRN (cur_node);
            i4_t i;
            
            for (i = 0; like_arg; like_arg = RIGHT_TRN (like_arg), i++)
              check_type (like_arg, pack_type (SQLType_Char, (i < 2) ?
                                               DEFAULT_STR_LEN : 1, 0),
                          "Incorrect type of LIKE argument");
          }
          break;
          
	case SUBQUERY:
	  OPRN_TYPE (cur_node) =
            *node_type (DOWN_TRN (RIGHT_TRN (DOWN_TRN (cur_node))));
	  break;
          
 	case CALLPR:
          yyerror ("node CALLPR can't exist before code generation");
	  break;
          
 	case COUNT:
          OPRN_TYPE (cur_node) = pack_type (SQLType_Int, 0, 0);
	  break;
          
        default:
          break;
        }
    }
  
  return cur_node;
} /* handle_types */
