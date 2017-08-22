/*
 *  repack.c - routines for getting data out from syntax  tree
 *             and packing it to structures used in executable
 *             format of GNU SQL server interpretator
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Andrew Yahin
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

/* $Id: repack.c,v 1.245 1997/03/31 03:46:38 kml Exp $ */

#include "global.h"
#include "tassert.h"
#include "syndef.h"
#include "cycler.h"


#define FUNC_WITH_DIST(code_d,code_a) \
  return ((TstF_TRN(func,DISTINCT_F))? code_d : code_a )

static char
code_func (TXTREF func)
{
  enum token code = CODE_TRN (func);
  switch (code)
    {
    case COUNT:
      FUNC_WITH_DIST (FN_DT_COUNT, FN_COUNT);
    case AVG:
      FUNC_WITH_DIST (FN_DT_AVG, FN_AVG);
    case SUM:
      FUNC_WITH_DIST (FN_DT_SUMM, FN_SUMM);
    case MAX:
      return FN_MAX;
    case MIN:
      return FN_MIN;
    default:
      TASSERT (Is_Function (func), func);
    }
  return (char) 0;
}

#undef FUNC_WITH_DIST


#define COL_NUMB(node) ((node!=TNULL) ? XLNG_TRN(node,0) : 0)

void
prepare_MG (TXTREF ptr, i4_t *g_clm_nmb, VADR * g_clm_arr,
	    i4_t *func_nmb, VADR * func_arr, VADR * func_clm_arr)
{
/*
 *  g_clm_nmb      the address of quantity of group rows.
 * *g_clm_arr      the address of array of group row numbers.
 *  func_nmb     - the quantity of functions
 * *func_arr     - the array of functions' codes
 * *func_clm_arr - the address of row numbers where rows are functions'
 *                 arguments
 */
  TXTREF vec = XVEC_TRN (ptr, 0), cur_pos = VOP (vec, 0).txtp;
  i4_t i, j, len = VLEN (vec);
  i4_t *g_clm_arr_n, *func_clm_arr_n;
  char *func_arr_n;
  
  i=0;
  while(i<len)
    {
      cur_pos = VOP (vec, i++).txtp;
      if(CODE_TRN (cur_pos) != COL){ i--;break;}
    }
  *g_clm_arr    = VMALLOC (i, i4_t);
  *func_arr     = VMALLOC (len - i, char);
  *func_clm_arr = VMALLOC (len - i, i4_t);
  g_clm_arr_n    = V_PTR (*g_clm_arr, i4_t);
  func_arr_n     = V_PTR (*func_arr, char);
  func_clm_arr_n = V_PTR (*func_clm_arr, i4_t); 
    
  *g_clm_nmb = i;
  *func_nmb = len - i;

  for (j = 0;
       j < i;
       g_clm_arr_n[j] = COL_NUMB (VOP (vec, j).txtp),
       j++);

  for (j = i; j < len; j++)
    {
      func_arr_n[j - i] = code_func (VOP (vec, j).txtp);
      func_clm_arr_n[j - i] = COL_NUMB (DOWN_TRN (VOP (vec, j).txtp));
    }
}

void
prepare_SRT (TXTREF ptr, i4_t *clm_nmb, VADR * clm_arr)
{
/*
 * clm_nmb - number of sorted columns
 * clm_arr - array of columns to sort
 */
  i4_t *arr;
  i4_t i;
  VADR V_arr;

  TXTREF vec = XVEC_TRN (ptr, 0);
  *clm_nmb = VLEN (vec);
  P_VMALLOC (arr, *clm_nmb, i4_t);
  for (i = 0; i < *clm_nmb; i++)
    arr[i] = VOP (vec, i).l;
  *clm_arr = V_arr;
}

/* returns number of updated columns */
int
prepare_CURS (TXTREF ptr, VADR * curs_arr)
{
/*
 ptr      - tree node
 curs_arr - array of numbers of updated columns
*/
  i4_t num_of_updted = 0, new_col;
  i4_t i, j, count, n, n_max;
  TXTREF upd;
  i4_t *vect, *arr;

  VADR V_arr;

  TASSERT (CODE_TRN (ptr) == DECL_CURS, ptr);
  for (upd = RIGHT_TRN (ptr); upd; upd = RIGHT_TRN (upd))
    if (CODE_TRN (upd) == UPDATE)
      num_of_updted += VLEN (XVEC_TRN (upd, 5));

  n_max = num_of_updted;
  vect = (i4_t *) xmalloc (sizeof (i4_t) * num_of_updted);

  num_of_updted = 0;

  for (upd = RIGHT_TRN (ptr); upd; upd = RIGHT_TRN (upd))
    if (CODE_TRN (upd) == UPDATE)
      for (j = 0, new_col = 1; j < VLEN (XVEC_TRN (upd, 5)); new_col = 1, j++)
	{
	  for (i = 0; i < num_of_updted; i++)
	    if (vect[i] == VOP (XVEC_TRN (upd, 5), j).l)
	      new_col = 0;
	  if (new_col)
	    vect[num_of_updted++] = VOP (XVEC_TRN (upd, 5), j).l;
	}
  n = num_of_updted;
  P_VMALLOC (arr, n, i4_t);

  count = 0;
  for (j = 0, new_col = 1; j < n_max; new_col = 1, j++)
    {
      for (i = 0; i < count; i++)
	if (vect[j] == arr[i])
	  new_col = 0;
      if (new_col)
	arr[count++] = vect[j];
    }
  *curs_arr = V_arr;
  xfree (vect);
  return n;
}

void
prepare_UPD (TXTREF ptr, i4_t *clm_nmb, VADR * clm_arr)
{
/*
 * number of modified cloumns and array of their indexes
 * clm_nmb - number of sorted columns
 * clm_arr - array of columns to sort
 */
  i4_t *arr;
  i4_t i;
  VADR V_arr;

  TXTREF vec = UPD_CLMNS (ptr);
  *clm_nmb = VLEN (vec);
  P_VMALLOC (arr, *clm_nmb, i4_t);
  for (i = 0; i < *clm_nmb; i++)
    arr[i] = VOP (vec, i).l;
  *clm_arr = V_arr;
}
