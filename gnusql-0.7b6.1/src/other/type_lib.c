/*
 *  type_lib.c  -  types library of GNU SQL server
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Konstantin Dyshlevoi.
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

/* $Id: type_lib.c,v 1.252 1998/09/29 22:23:44 kimelman Exp $ */

#include <assert.h>

#include "global.h"
#include "type_lib.h"
#include "inprtyp.h"
#include "sql.h"
#include "totdecl.h"
#include "fieldtp.h"

extern data_unit_t *ins_col_values;
data_unit_t *data_st = NULL;
i4_t   dt_ind, dt_max;

i4_t res_hole_cnt = 0;
parm_t *res_hole = NULL;

/* In all leafs field Ptr points to element of type data_unit_t      *
 * where the type of the value is filled by codegen.              */

/* Type of result of subtree must be put into all according nodes *
 * ( if it's the logic value =>  T_BOOL).                         *
 * in nodes for subquery :.Rh- reference to programm for          *
 * handling this subquery.                                        */

#define CHECK_FLAG(du) 				     	          \
  if ((du).dat.nl_fl == NOT_CONVERTED)		     	          \
  {						     	          \
    STR_PTR (&((du).dat)) = V_PTR (STR_VPTR (&((du).dat)), char); \
    (du).dat.nl_fl = REGULAR_VALUE;		     	          \
  }

short
sizeof_sqltype (sql_type_t type)
/* values' lenght of this type */
{
  i2_t res;
  switch (type.code)
    {
    case T_SRT:
      res = SZ_SHT;
      break;
    case T_INT:
      res = SZ_INT;
      break;
    case T_LNG:
      res = SZ_LNG;
      break;
    case T_FLT:
      res = SZ_FLT;
      break;
    case T_DBL:
      res = SZ_DBL;
      break;
    case T_STR:
      res = type.len;
      break;
    default:
      res = 0;
      lperror ("Unexpected expression type '%s'", type2str(type));
    }
  return res;
}

VADR
load_tree (TRNode *node, VADR cur_seg, VADR str_seg)
     /* recursive function for copying the tree      *
      * for CHECK constraint from temporary segment  *
      * to current working segment of virtual memory */
{
  VADR V_res_node, V_ptr = VNULL, V_Dn, V_Rh;
  TRNode *res_node;
  TRNode *Rh_old, *Dn_old;
  data_unit_t *du = NULL;
  TpSet *dat = NULL;
  char *str_dat = NULL;
  
  if (!node)
    return VNULL;
  
  switch_to_segment (str_seg);
  Dn_old = V_PTR (node->Dn.off, TRNode);
  Rh_old = V_PTR (node->Rh.off, TRNode);
  if (node->code == COLUMN)
    V_ptr = node->Ptr.off;
  else
    if (node->Ptr.off)
      {
	du = V_PTR (node->Ptr.off, data_unit_t);
	dat = &(du->dat);
	if (du->type.code == T_STR)
	  str_dat = V_PTR (STR_VPTR (dat), char);
      }
  
  switch_to_segment (cur_seg);
  V_Dn = load_tree (Dn_old, cur_seg, str_seg);
  V_Rh = load_tree (Rh_old, cur_seg, str_seg);
  if (du)
    {
      V_ptr = VMALLOC (1, data_unit_t);
      *V_PTR (V_ptr, data_unit_t) = *du;
      if (str_dat)
	{
	  i4_t len = strlen (str_dat);
	  char *str_ptr;
	  VADR V_str_ptr;
	  
	  P_VMALLOC (str_ptr, len + 1, char);
	  bcopy (str_dat, str_ptr, len + 1);
	  STR_VPTR (&(V_PTR (V_ptr, data_unit_t)->dat)) = V_str_ptr;
	}
    }
  
  P_VMALLOC (res_node, 1, TRNode);
  *res_node = *node;
  res_node->Dn.off = V_Dn;
  res_node->Rh.off = V_Rh;
  res_node->Ptr.off = V_ptr;
    
  return V_res_node;
} /* load_tree */

int
res_row_save (char *nl_arr, i4_t nr, data_unit_t **colval)
{
  char *pointbuf, *ptr, nl_fl;
  i4_t i, err;
  data_unit_t *to;
  i2_t len;
  
  pointbuf = nl_arr + nr;
  for (i = 0; i < nr; i++)
    {
      to = colval[i];
      len = 0;
      ptr = NULL;
      nl_fl = (nl_arr[i]) ? REGULAR_VALUE : NULL_VALUE;
      
      if (nl_fl == REGULAR_VALUE)
	{
	  len = t2bunpack (pointbuf);
	  if (len < 0)
	    return -ER_BUF;
	  pointbuf += sizeof(u2_t);
	  ptr = pointbuf;
	  pointbuf += len;
	}
      
      err = mem_to_DU (nl_fl, to->type.code, len, ptr, to);
      if (err < 0)
	return err;
    }
  return 0;
} /* res_row_save */


int
DU_to_buf (data_unit_t *dt_from, char **pointbuf, sql_type_t *to_type)
     /* returns 0 if O'K and < 0 if error */
     /* if to_type == NULL => type from dt_from won't be changed */
{	
  TpSet Place;
  i4_t len, err;
  
  if (dt_from->type.code == T_STR)
    {
      t2bto2char (len = dt_from->type.len, *pointbuf);
      *pointbuf += SZ_SHT;
      memcpy (*pointbuf, STR_PTR (&(dt_from->dat)), len);
      *pointbuf += len;
    }
  else
    {
      len = sizeof_sqltype (*to_type);
      t2bto2char (len, *pointbuf);
      *pointbuf += SZ_SHT;
      err = put_dat (&(dt_from->dat), 0, dt_from->type.code,
		     REGULAR_VALUE, &Place, 0,
		     (to_type) ? to_type->code : dt_from->type.code, NULL);
      if (err < 0)
	return err;
      memcpy (*pointbuf, (char *) (&Place), len);
      *pointbuf += len;
    }
  return 0;
} /* DU_to_buf */

int
DU_to_rpc (data_unit_t *dt_from, parm_t *to, i4_t typ)
{
  char null_fl;
  i4_t err = 0;
  
  assert (dt_from && to);
  null_fl= dt_from->dat.nl_fl;
  
  to->value.type = typ;
  to->indicator = (null_fl == NULL_VALUE) ? -1 : 0;
  if (null_fl != REGULAR_VALUE)
    { /* just to make sure */
      to->value.data_u.Str.Str_val = NULL;
      to->value.data_u.Str.Str_len = 0;
    }
  else
    {
      if (typ == T_STR)
        {
          to->value.data_u.Str.Str_val = STR_PTR (&(dt_from->dat));
          to->value.data_u.Str.Str_len = dt_from->type.len;
        }
      else
        err = put_dat (&VL(&(dt_from->dat)), 0, dt_from->type.code, 
                       REGULAR_VALUE, &(to->value.data_u), 0, typ, NULL);
    }
  return err;
} /* DU_to_rpc */

void
conv_DU_to_str (data_unit_t *from, data_unit_t *to)
{
  sql_type_t *typ_to, *typ_from;
  TpSet *dat_to, *dat_from;
  
  *to = *from;
  dat_from = &(from->dat);
  typ_from = &(from->type);
  dat_to   = &(to->dat);
  typ_to   = &(to->type);
  if (NL_FL (dat_from) == REGULAR_VALUE && typ_from->code != T_STR)
    {
      typ_to->code = T_STR;
      typ_to->len = sizeof_sqltype (*typ_from);
      STR_PTR (dat_to) = (char *) (&VL (dat_from));
    }
  MAX_LEN (dat_to) = 0;
} /* conv_DU_to_str */


int
mem_to_DU (char nl_fl, byte typ_from_code,
	   i2_t from_len, void *from, data_unit_t *to)
/* This function returns value:                         *
 * 0   - if it's all right,                             *
 * < 0 - if error,                                      *
 * > 0 - if argument is string and this value = lenght  *
 *       of this string. It is greater than res_size    *
 *       (so string was shortened).                     *
 * NOTE : if from_len < 0 & argument is string =>       *
 *        its lenght is evaluated as strlen (from)      */
{
  i4_t err = 0, len;
  sql_type_t *typ_to;
  TpSet *dat_to;
  
  typ_to = &(to->type);
  dat_to = &(to->dat);
  
  NL_FL (dat_to) = nl_fl;
  if (nl_fl == REGULAR_VALUE)
    {
      if (typ_from_code == T_STR)
	{
	  if (from_len < 0)
	    from_len = strlen ((char *)from);
	  CHECK_ARR_SIZE (STR_PTR (dat_to), MAX_LEN (dat_to), from_len+1, char);
	  len = typ_to->len = from_len;
	  STR_PTR (dat_to)[from_len] = 0; /* null-terminating of the string for *
					   * functions from catfun.c            */
	}
      else
	{
	  len = sizeof_sqltype (*typ_to);
	  if (MAX_LEN (dat_to))
	    {
	      xfree (STR_PTR (dat_to));
	      MAX_LEN (dat_to) = 0;
	    }
	}
      if (typ_from_code == typ_to->code)
	bcopy ((char *) from, DU_PTR_VL (*to), len);
      else if (typ_from_code == T_STR)
        err = -ER_1;
      else
        {
          TpSet tmp;
          sql_type_t type_tmp = pack_type(typ_from_code, 0, 0);
          
          bcopy ((char *) from, (char *)(&tmp), sizeof_sqltype (type_tmp));
          err = put_dat (&tmp, from_len, typ_from_code, REGULAR_VALUE,
                         DU_PTR_VL (*to), typ_to->len, typ_to->code, NULL);
        }
    }
  return err;
} /* rpc_to_mem */
  
int
rpc_to_DU (parm_t *from, data_unit_t *to)
/* This function returns value:                         *
 * 0   - if it's all right,                             *
 * < 0 - if error,                                      *
 * > 0 - if argument is string and this value = lenght  *
 *       of this string. It is greater than res_size    *
 *       (so string was shortened).                     */
{
  byte typ_from_code;
  
  typ_from_code = from->value.type;
  return mem_to_DU ((from->indicator < 0) ? NULL_VALUE : REGULAR_VALUE,
		    typ_from_code,
		    (typ_from_code == T_STR) ?
		    from->value.data_u.Str.Str_len : 0,
		    (typ_from_code == T_STR) ?
		    from->value.data_u.Str.Str_val :
		           (void *)(&(from->value.data_u)), to);
} /* rpc_to_DU */

#define OPERATION(simb) switch (m1) {                          \
        case T_SRT : SrtPred = SRT_VL (pt1) simb SRT_VL (pt2); \
                     break;                                    \
        case T_INT : IntPred = INT_VL (pt1) simb INT_VL (pt2); \
                     break;                                    \
        case T_LNG : LngPred = LNG_VL (pt1) simb LNG_VL (pt2); \
                     break;                                    \
        case T_FLT : FltPred = FLT_VL (pt1) simb FLT_VL (pt2); \
                     break;                                    \
        case T_DBL : DblPred = DBL_VL (pt1) simb DBL_VL (pt2); \
                     break;                                    \
        default    : return(-ER_1);       /* ERROR */          \
      }	/* switch */                                           \
      break
      
#define RET(err) { DtPop;        \
                   return err; }

int
oper (byte code, i4_t arity)
/* Operation "code" making. Two highest stack elements are * 
 * arguments. It both are being deleted and result of this *
 * operation is being put into head of stack               */
{
  byte m1, m2;
  TpSet *pt1, *pt2, dop;
  i2_t size;
  i4_t err, m, cod1 = 0, cod2 = 0;
  char cod_res;
  
  /* next table is used for calculation of the result       *
   * of AND & OR operations : 0 - NULL, 1 - FALSE, 2 - TRUE */
					  
  char OR_AND_tbl[2][3][3] = { /* NULL   FALSE   TRUE */
    /* AND : */
	           /* NULL  */ { { 0,      1,      0 },
                   /* FALSE */   { 1,      1,      1 },
		   /* TRUE  */   { 0,      1,      2 } },
  
    /* OR  : */                         
                   /* NULL  */ { { 0,      0,      2 },
                   /* FALSE */   { 0,      1,      2 },
	           /* TRUE  */   { 2,      2,      2 } }
  };
  
#define U_MINUS(type, val) case type: val = - val; break;

  if (arity == 1) /* unary operation */
    {
      if (Is_SQPredicate_Code (code))
	{
	  i4_t res = FALSE;
      
          assert (code == EXISTS || code == SUBQUERY );
 	  if (code == SUBQUERY) /* in comparision predicate */
            {
              if (NlFlPred == UNKNOWN_VALUE)
                DtPredEl = DtCurEl;
              else /* second row in SubQuery result */
                return -ER_SQ;
            }
	  NlFlCur = REGULAR_VALUE;
	  LngCur = res;
	  CodCur = T_BOOL;
	  LenCur = 0;
	}
      else
	switch (code)
	  {
	  case UMINUS :
	    switch (CodCur)
	      {
		U_MINUS (T_SRT, SrtCur);
		U_MINUS (T_INT, IntCur);
		U_MINUS (T_LNG, LngCur);
		U_MINUS (T_FLT, FltCur);
		U_MINUS (T_DBL, DblCur);
	      default:
		return (-ER_1);
	      }		
	    break;
	    
	  case NOT :
	    if (NlFlCur == REGULAR_VALUE)
	      LngCur = ! LngCur ;
	    break;
	  
	  case ISNULL :
	    LngCur = (NlFlCur == NULL_VALUE);
	    CodCur = T_BOOL;
	    NlFlCur = REGULAR_VALUE;
	    break;
	  
	  case ISNOTNULL :
	    LngCur = (NlFlCur == REGULAR_VALUE);
	    CodCur = T_BOOL;
	    NlFlCur = REGULAR_VALUE;
	    break;
	  
	  case NOOP :
	    break;
	    
	  default:
	    return (-ER_3);		/* ERROR */
	  }
      return 0;
    }
  
#undef U_MINUS
  
  /* other operations are binary */
  
  if ((code == OR) || (code == AND))
    {
      if (NlFlPred == REGULAR_VALUE)
	cod1 = LngPred + 1; 
      if (NlFlCur == REGULAR_VALUE)
	cod2 = LngCur + 1; 
      cod_res = OR_AND_tbl[code == OR][cod1][cod2];
      
      if (cod_res)
	{
	  NlFlPred = REGULAR_VALUE;
	  LngPred = cod_res - 1;
	  CodPred = T_BOOL;
	}
      
      else
	NlFlPred = NULL_VALUE;
      
      RET (0);
    }

  if ((NlFlPred != REGULAR_VALUE) ||
      (NlFlCur != REGULAR_VALUE))
    {
      NlFlPred = NULL_VALUE;
      RET (0);
    }
  
  NlFlPred = REGULAR_VALUE;
  if (CodPred == T_STR)
    {				/* string */
      /* There are only binary comparison operations for strings. *
       * Strings aren't in any other operations.                  */

      size = (LenPred > LenCur) ? LenPred : LenCur;
      m = strncmp (StrPred, StrCur, size);
      LngPred = 0;
      switch (code)
	{
	case EQU:
	  if (m == 0)
	    LngPred = 1;
	  break;
	case NE:
	  if (m != 0)
	    LngPred = 1;
	  break;
	case GT:
	  if (m > 0)
	    LngPred = 1;
	  break;
	case LT:
	  if (m < 0)
	    LngPred = 1;
	  break;
	case GE:
	  if (m >= 0)
	    LngPred = 1;
	  break;
	case LE:
	  if (m <= 0)
	    LngPred = 1;
	  break;
	default:
	  RET (-ER_3);
	}			/* switch */

      CodPred = T_BOOL;
    }
  else
    {				/* operation between numbers */
      m1 = CodPred;
      pt1 = &DatPred;
      m2 = CodCur;
      pt2 = &DatCur;
      if (m1 > m2)
	{
	  pt2 = &dop;
	  err = put_dat (&DatCur, 0, m2, REGULAR_VALUE, pt2, 0, m1, NULL);
	  if (err)
	    RET (err);
	}
      if (m1 < m2)
	{
	  pt1 = &dop;
	  err = put_dat (&DatPred, 0, m1, REGULAR_VALUE, pt1, 0, m2, NULL);
	  if (err)
	    RET (err);
       	  m1 = m2;
	}
      CodPred = m1;
      /* type of the result is in ResCode now */
      switch (code)
	{
	case ADD:  OPERATION (+);
	case SUB:  OPERATION (-);
	case DIV:  OPERATION (/);
	case MULT: OPERATION (*);
	case NE:   OPERATION (!=);
	case EQU:  OPERATION (==);
	case GT:   OPERATION (>);
	case LT:   OPERATION (<);
	case GE:   OPERATION (>=);
	case LE:   OPERATION (<=);
	default:
	  RET (-ER_3);
	  /* . . . */

	}			/* switch */
    }				/* if else */
  RET (0);
}				/* "oper" */

float
sel_const (i4_t op, data_unit_t *cnst, data_unit_t *min, data_unit_t *max)
     /* makes estimation of selectivity for simple predicate  *
      * of type : col_name 'op' cnst;                        *
      * min & max - statistic about column col_name           *
      * function assumes that cnst, min, max are all REGULAR */
{
  float sel = 0.0;
  int error;
#define pow_256(pow) (0x1000000 >> (8*pow))
  
  if (op == EQU)
    return sel;
  
  CHECK_FLAG (*cnst); 
  assert (cnst->dat.nl_fl == REGULAR_VALUE);
  MK_BIN_OPER (GT, *cnst, *min);
  if (COMP_RESULT)
    /* cnst > min */
    {
      MK_BIN_OPER (LT, *cnst, *max);
      if (COMP_RESULT)
	/* cnst < max */
	{
	  if (cnst->type.code == T_STR)
	    {
	      u4_t min_long = 0, max_long = 0, cnst_long = 0;
	      i4_t min_str_lng, i, j;
	      i4_t len_min = min->type.len;
	      i4_t len_max = max->type.len;
	      i4_t len_cnst = cnst->type.len;
	      char *str_min = STR_PTR (&(min->dat));
	      char *str_max = STR_PTR (&(max->dat));
	      char *str_cnst = STR_PTR (&(cnst->dat));
	    
	      min_str_lng = (len_min < len_max) ? len_min : len_max;
	      min_str_lng = (min_str_lng < len_cnst) ? min_str_lng : len_cnst;
	      for (i = 0; i < min_str_lng; i++)
		if (str_min[i] != str_cnst[i] || str_max[i] != str_cnst[i])
		  break;
	    
	      for (j = 0; j < SZ_LNG && i+j < len_min; j++)
		min_long += ((u4_t) (str_min[i+j])) * pow_256(j);
	      for (j = 0; j < SZ_LNG && i+j < len_max; j++)
		max_long += ((u4_t) (str_max[i+j])) * pow_256(j);
	      for (j = 0; j < SZ_LNG && i+j < len_cnst; j++)
		cnst_long += ((u4_t) (str_cnst[i+j])) * pow_256(j);
	      assert (min_long != max_long);
	    
	      sel = ((float)cnst_long - (float)min_long) /
		((float)max_long - (float)min_long);
	    }
	  else
	    {
	      data_unit_t  cnst_sub_min, max_sub_min;
	    
	      MK_BIN_OPER (SUB, *max, *min);
	      max_sub_min = OPER_RESULT;
	      MK_BIN_OPER (SUB, *cnst, *min);
	      cnst_sub_min = OPER_RESULT;
	    
	      /* changing both subtraction results to float */
	      error = put_dat (&(cnst_sub_min.dat), 0, cnst_sub_min.type.code,
			       REGULAR_VALUE, &(cnst_sub_min.dat), 0, T_FLT, NULL);
	      if (error < 0)
		return error;
	      cnst_sub_min.type.code = T_FLT;
	      error = put_dat (&(max_sub_min.dat), 0, max_sub_min.type.code,
			       REGULAR_VALUE, &(max_sub_min.dat), 0, T_FLT, NULL);
	      if (error < 0)
		return error;
	      max_sub_min.type.code = T_FLT;
	    
	      MK_BIN_OPER (DIV, cnst_sub_min, max_sub_min);
	      sel = FLT_VL (&(OPER_RESULT.dat));
	    }
	  if (op == GT || op == GE)
	    sel = 1.0 - sel;
	}
      else /* cnst >= max */
	if (op == LT || op == LE)
	  sel = 1.0;
    }
  else/* cnst <= min */
    if (op == GT || op == GE)
      sel = 1.0;
  
  return sel;
} /* sel_const */

/* translate data type in compiler to data type in BD */
void
type_to_bd (sql_type_t *tp_from, sql_type_t *tp_to)
{
  tp_to->len = sizeof_sqltype (*tp_from);
  
  switch (tp_from->code)
    {
    case T_STR:
      tp_to->code = TCH;
      break;
    case T_SRT:
      tp_to->code = T2B;
      break;
    case T_INT:
      tp_to->code = T4B;
      break;
    case T_LNG:
      tp_to->code = T4B;
      break;
    case T_FLT:
    case T_DBL:
      tp_to->code = TFLOAT;
      break;
    }
}

#define PR_CONST(typ_field,trn_func)                        \
  typ_field (dat_to) = trn_func (STRING (CNST_NAME (ptr))); \
  break

void
prepare_CONST (TXTREF ptr, VADR V_PTN)
/* this function prepares constant value for plan generator; it   *
 * means that in the place befor constant value function locates  *
 * flag(char) defining whether it's undefined or undeclared or    *
 * regular value; returns pointer to value itself                 */
{
  VADR V_str;
  char *str;
  data_unit_t *hole;
  VADR V_hole;
  TpSet *dat_to;
  TRNode *PTN;
  
  P_VMALLOC (hole, 1, data_unit_t);
  dat_to = &(hole->dat);
  PTN = V_PTR (V_PTN, TRNode);
  hole->type = PTN->Ty = CNST_STYPE (ptr);
  
  PTN->code = CONST;
  PTN->Ptr.off = V_hole;
  
  if (CODE_TRN (ptr) == NULL_VL)
    {
      NL_FL (dat_to) = NULL_VALUE;
      return;
    }
  
  assert (CODE_TRN (ptr) == CONST);
  NL_FL (dat_to) = REGULAR_VALUE;
  
  switch (CNST_STYPE (ptr).code)
    {
    case SQLType_Short:
      PR_CONST (SRT_VL, atol);
    case SQLType_Int:
      PR_CONST (INT_VL, atol);
    case SQLType_Long:
      PR_CONST (LNG_VL, atol);
    case SQLType_Float:
    case SQLType_Real:
      PR_CONST (FLT_VL, atof);
    case SQLType_Double:
      PR_CONST (DBL_VL, atof);
    case SQLType_Char:
      {
	i4_t len = strlen (STRING (CNST_NAME (ptr)));
	
	hole->type.len = PTN->Ty.len = len;
	P_VMALLOC (str, len+1, char);
	bcopy (STRING (CNST_NAME (ptr)), str, len);
        str[len]=0;
	STR_VPTR (dat_to) = V_str;
	NL_FL (dat_to) = NOT_CONVERTED;
      }
      break;
    default:
      yyfatal (" Unknown type in constant preparation \n");
    }
}

#undef PR_CONST

void
prepare_USERNAME (TRNode *PTN)
{
  PTN->code = USERNAME;
  PTN->Ty.code = T_STR;
  PTN->Ty.len = US_NAME_LNG;
}

VADR
prepare_HOLE (sql_type_t typ)
/* preparation of hole in module for data of this type */
{
  data_unit_t *hole;
  VADR V_hole;
  
  P_VMALLOC (hole, 1, data_unit_t);
  hole->type = typ;
  return V_hole;
} /* prepare_HOLE */
