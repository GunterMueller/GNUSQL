/*
 *  sql_type.h - Sql types support for GNU SQL compiler
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
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
 *  Contacts: gss@ispras.ru
 *
 */

/* $Id: type_lib.h,v 1.247 1997/11/05 16:04:29 kml Exp $ */
                
#ifndef __TYPE_LIB_H__
#define __TYPE_LIB_H__

#include "vmemory.h"
#include "sql_type.h"
#include "trl.h"
#include "gsqltrn.h"
#include "sql.h"

#define T_STR    SQLType_Char /* C-like string : length without \0 */
#define T_SRT    SQLType_Short
#define T_INT    SQLType_Int
#define T_LNG    SQLType_Long
#define T_FLT    SQLType_Real
#define T_DBL    SQLType_Double

#define SZ_SHT   sizeof(i2_t)
#define SZ_INT   sizeof(i4_t)
#define SZ_LNG   sizeof(i4_t)
#define SZ_FLT   sizeof(float)
#define SZ_DBL   sizeof(double)
#define SZ_PTR   sizeof(char*)

#define T_BOOL   T_INT

#define CHECK_FLAG(du) 				     	          \
  if ((du).dat.nl_fl == NOT_CONVERTED)		     	          \
  {						     	          \
    STR_PTR (&((du).dat)) = V_PTR (STR_VPTR (&((du).dat)), char); \
    (du).dat.nl_fl = REGULAR_VALUE;		     	          \
  }


typedef union {
  VADR  off;
  char *adr;
} PSECT_PTR;

typedef u4_t MASK;


/* let constants and columns have the same types */

typedef struct  Node{
   i4_t code;
   MASK  Mask;            /*                                       */
   MASK  Depend;          /* mask of dependence of subtree on table*/
   MASK  Handle;          /* mask of changes of current table's    */
                          /* strings                               */
   i4_t  Arity;           /* the number of operands                */
   PSECT_PTR Rh;          /* reference to the next operand         */
   PSECT_PTR Dn;          /* reference to the first operand        */
   sql_type_t Ty;         /* value type (if list) or subtree type  */
                          /* (if operattion's node)                */
   PSECT_PTR Ptr;         /* reference to data_unit_t for data; it also*/
                          /* is used as reference for result of    */
                          /* subtree computing if it isn't list    */
} TRNode;

typedef struct TypeSet
{
  union
  {
    PSECT_PTR StrPtr;
    i2_t      Srt;
    i4_t      Iint;
    i4_t      Lng;
    float     Flt;
    double    Dbl;
  } val;
  i2_t max_len;
  char  nl_fl;
} TpSet;

typedef struct
{
  TpSet       dat;
  sql_type_t  type;
} data_unit_t;


void  conv_type __P((char *s,sql_type_t *l,i4_t direct));/* direct -- the direction */
                                                  /* of type transformation  */
                                                  /* 1 == s->l ; 0 = l->s    */
char *type2str __P((sql_type_t t));
sql_type_t read_type __P((char *s));

/* definitions for working with types in interpretator */
#define BUF_SIZE_VL(DU_vl, type_to) (sizeof(i2_t) + \
                                     (((DU_vl).type.code == T_STR) ? \
                                      (DU_vl).type.len : sizeof_sqltype (type_to)))

#define DU_PTR_VL(DU_vl) (((DU_vl).type.code == T_STR) ?  \
			  STR_PTR (&((DU_vl).dat)) : \
                          (char *) (&VL (&((DU_vl).dat))))
     
VADR  load_tree  __P((TRNode *node, VADR cur_seg, VADR str_seg));
int   res_row_save  __P((char *nl_arr, i4_t nr, data_unit_t **colval));
i2_t  sizeof_sqltype   __P((sql_type_t type));
int   DU_to_buf  __P((data_unit_t *dt_from, char **pointbuf, sql_type_t *to_type));
int   DU_to_rpc  __P((data_unit_t *dt_from, parm_t *to, i4_t typ));
void  conv_DU_to_str  __P((data_unit_t *from, data_unit_t *to));
int   mem_to_DU  __P((char nl_fl, byte typ_from_code,
                 i2_t from_len, void *from, data_unit_t *to));
int   oper  __P((byte code, i4_t arity));
float sel_const  __P((i4_t op, data_unit_t *cnst, data_unit_t *min, data_unit_t *max));
void  type_to_bd  __P((sql_type_t *tp_from, sql_type_t *tp_to));
void  prepare_CONST  __P((TXTREF ptr, VADR V_PTN));
void  prepare_USERNAME  __P((TRNode *PTN));
int   rpc_to_DU  __P((parm_t *from, data_unit_t *to));

i4_t  user_to_rpc  __P((gsql_parm *parm, parm_t *rpc_arg));
i4_t  rpc_to_user  __P((parm_t *rpc_res, gsql_parm *parm));
i4_t  put_dat  __P((void *from, i4_t from_size, byte from_mask, char from_nl_fl,
	      void *to,   i4_t to_size_ , byte to_mask_ , char *to_nl_fl ));

#endif
