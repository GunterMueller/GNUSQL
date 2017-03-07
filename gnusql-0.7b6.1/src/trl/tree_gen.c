/*
 *  tree_gen.c  - tree library for GNU SQL precompiler
 *                Generation tree ( part of parsing )
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Michael Kimelman
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
 *  Contact: gss@ispras.ru
 *
 */

/* $Id: tree_gen.c,v 1.248 1998/09/29 21:26:47 kimelman Exp $ */

#include "tree_gen.h"
#include "type_lib.h"
#include <string.h>
#include <assert.h>

void
set_location (TXTREF n, YYLTYPE p)
{
  LOCATION_TRN (n) = p.first_line;
}


TXTREF
join_list (TXTREF lst, TXTREF e)
{
  register TXTREF n = lst;
  if (n == TNULL)
    return e;
  while (RIGHT_TRN (n))
    n = RIGHT_TRN (n);
  RIGHT_TRN (n) = e;
  return lst;
}


TXTREF
get_param (LTRLREF l, i4_t disableadd, YYLTYPE p)
{
  register TXTREF n;
  register i4_t   msk=0;
  n = find_info (PARAMETER, l);
  if (n)
    return n;
  if (disableadd)
    {
      char xx[80];
      errno = 0;
      file_pos = p.first_line;
      strcpy (xx, "error :Unexpected parameter name \"");
      strcat (xx, STRING (l));
      strcat (xx, "\"");
      yyerror (xx);
      for (n = LOCAL_VCB_ROOT; n; n = RIGHT_TRN (n))
        {
          debug_trn (n);
          fprintf (STDERR, "\n");
        }
#if 0
      return TNULL;
#else
      SetF(msk,PSEUDO_F);
#endif
    }
  n = gen_node1 (PARAMETER,msk);
  PAR_NAME (n) = l;
  add_info (n);
  return n;
}

TXTREF
get_parm_node (LTRLREF l, YYLTYPE p, i4_t disableadd)
{
  register TXTREF n, prm = get_param (l, disableadd, p);
  n = gen_object (PARMPTR, p);
  OBJ_DESC (n) = prm;
  return n;
}

TXTREF
get_ind (TXTREF n, TXTREF n1, YYLTYPE p)
{
  if (TstF_TRN(n1,INDICATOR_F))
    {
      lperror("Warning: indicator host variable \"%s\" used more than once",
              STRING(PAR_NAME(n1)));
      errors--;
    }
  if (PAR_INDICATOR(n1))
    lperror("Error: host variable \"%s\" used more than once",
            STRING(PAR_NAME(n1)));
  if (TstF_TRN(n,INDICATOR_F))
    lperror("Error: host variable \"%s\" used more than once",
            STRING(PAR_NAME(n)));
  SetF_TRN(n1,INDICATOR_F);
  PAR_TTYPE(n1) = pack_type(SQLType_Int,10,0);
  PAR_INDICATOR(n) = n1;
  return get_parm_node(PAR_NAME(n),p,0);
}


TXTREF
gen_column (TN * scn, LTRLREF l, YYLTYPE p)
{
  register TXTREF i, v;
  i = gen_node (COLPTR);
  set_location (i, p);
  v = gen_node (COLUMN_HOLE);
  if (scn)
    {
      CHOLE_AUTHOR (v) = scn->a;
      CHOLE_TNAME (v) = scn->n;
    }
  CHOLE_CNAME (v) = l;
  OBJ_DESC (i) = v;
  return i;
}

TXTREF
gen_colptr (VCBREF col)
{
  register TXTREF i;
  i = gen_node (COLPTR);
  OBJ_DESC (i) = col;
  return i;
}

TXTREF
gen_func (enum token code, YYLTYPE p, TXTREF obj, i4_t set_dist)
{
  register TXTREF t;
  t = gen_node (code);
  set_location (t, p);
  FUNC_OBJ (t) = obj;
  ARITY_TRN (t) = (obj == TNULL) ? 0 : 1;
  if (set_dist)
    SetF_TRN (t, DISTINCT_F);
  if (code == COUNT)
    OPRN_TYPE (t) = pack_type (SQLType_Int, 10, 0);
  return t;
}


TXTREF
gen_oper (enum token code, TXTREF l, TXTREF r, YYLTYPE p)
{
  if (code == DIV || code == SUB)       /* non assotiative operation */
    {
      register TXTREF c = gen_object (code, p);
      ARITY_TRN (c) = 2;
      RIGHT_TRN (l) = r;
      DOWN_TRN (c) = l;
      return c;
    }
  else
    {
      if (CODE_TRN (l) == code && CODE_TRN (r) == code)
        {
          register TXTREF c = DOWN_TRN (l);
          while (RIGHT_TRN (c))
            c = RIGHT_TRN (c);
          RIGHT_TRN (c) = DOWN_TRN (r);
          ARITY_TRN (l) += ARITY_TRN (r);
          free_node (r);
          return l;
        }
      else if (CODE_TRN (l) == code)
        {
          register TXTREF c = DOWN_TRN (l);
          while (RIGHT_TRN (c))
            c = RIGHT_TRN (c);
          RIGHT_TRN (c) = r;
          ARITY_TRN (l)++;
          return l;
        }
      else if (CODE_TRN (r) == code)
        {
          RIGHT_TRN (l) = DOWN_TRN (r);
          DOWN_TRN (r) = l;
          ARITY_TRN (r)++;
          return r;
        }
      else
        {
          register TXTREF c = gen_object (code, p);
          ARITY_TRN (c) = 2;
          RIGHT_TRN (l) = r;
          DOWN_TRN (c) = l;
          return c;
        }
    }
}

TXTREF
gen_parent (enum token code, TXTREF list)
{
  register TXTREF p = gen_node (code), c;
  register i4_t ar;
  for (ar = 0, c = list; c; c = RIGHT_TRN (c))
    ar++;
  DOWN_TRN (p) = list;
  ARITY_TRN (p) = ar;
  return p;
}


TXTREF
gen_up_node (enum token code, TXTREF child, YYLTYPE p)
{
  register TXTREF u = gen_node (code), c;
  register i4_t ar;
  set_location (u, p);
  for (ar = 0, c = child; c; c = RIGHT_TRN (c))
    ar++;
  DOWN_TRN (u) = child;
  ARITY_TRN (u) = ar;
  return u;
}


TXTREF
gen_object (enum token code, YYLTYPE p)
{
  register TXTREF n = gen_node (code);
  set_location (n, p);
  return n;
}


TXTREF
gen_scanptr (TXTREF tbl, LTRLREF nm, YYLTYPE p)
{
  register TXTREF s = gen_node (SCAN);
  register TXTREF t = gen_object (TBLPTR, p);
  COR_TBL (s) = tbl;
  add_info (s);
  if (nm)
    COR_NAME (s) = nm;
  else
    {
      char str[100];
      sprintf(str,"scan_%s_%s_%d",STRING(TBL_FNAME(tbl)),
              STRING(TBL_NAME(tbl)),COR_NO(s));
      COR_NAME(s) = ltr_rec(str);
    }
  TABL_DESC (t) = s;
  return t;
}


TXTREF
coltar_column (TXTREF cc)
{
  YYLTYPE a;
  register TXTREF c = cc;
  if (CODE_TRN (c) == VALUEHOLE)
    {
      a.first_line = 0;
      c = gen_column (NULL, HOLE_NAME (c), a);
      LOCATION_TRN (c) = LOCATION_TRN (cc);
      free_node (cc);
    }
  if (CODE_TRN(c) != COLPTR)
    {
      YYLTYPE l;
      l.first_line = file_pos = LOCATION_TRN(c);
      yyerror("syntax error - must be column or parameter here");
      free_tree(c);
      c = get_parm_node(ltr_rec("Error_node_name"),l,0);
    }
  return c;
}

/*--------------------------------------------------*\
*                  common function                   *
\*--------------------------------------------------*/

TXTREF
last_parameter (TXTREF parent)
{
  register TXTREF c = DOWN_TRN (parent), c1;
  if (c == TNULL)
    return TNULL;
  while ((c1 = RIGHT_TRN (c)) != TNULL)
    c = c1;
  return c;
}

void
add_child (TXTREF parent, TXTREF child)
{
  register TXTREF l;
  if (child == 0)
    return;
  l = last_parameter (parent);
  if (l)
    RIGHT_TRN (l) = child;
  else
    DOWN_TRN (parent) = child;
  if (RIGHT_TRN (child) != TNULL)
    RIGHT_TRN (child) = TNULL;
  ARITY_TRN (parent)++;
}
