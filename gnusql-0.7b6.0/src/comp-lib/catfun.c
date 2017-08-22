/*
 *  catfun.c  -  DB catalogs request library
 *               GNU SQL server
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
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

/* $Id: catfun.c,v 1.248 1998/05/20 05:43:19 kml Exp $ */

#include "global.h"
#include "type_lib.h"
#include "inprtyp.h"
#include "sql.h"
#include "const.h"
#include "engine/fieldtp.h"
#include "engine/expop.h"
#include "engine/pupsi.h"
#include "index.h"
#include "exti.h"
#include "tree_gen.h"
#include <assert.h>

extern i4_t transaction;
extern data_unit_t *data_st;
extern i4_t dt_ind, dt_max;

#define DU_ARR_LEN MAX_COLNO /* max of ..._COLNO */
#define MAX_STAT_LEN 10      /* maximum string length for statistic */
static data_unit_t *du;
static data_unit_t *arg_du[DU_ARR_LEN];
static data_unit_t *res_du[DU_ARR_LEN];
static i4_t num_rows[DU_ARR_LEN], ind_clm_nums[DU_ARR_LEN], tab_clm_nums[DU_ARR_LEN];

/********************************************************************/

#define CHECK_LEN(owner)  {if (strlen (owner) > MAX_USER_NAME_LNG) \
                             owner[MAX_USER_NAME_LNG] = 0;}

#define ARGDAT(num) (&((arg_du[num])->dat))
#define RESDAT(num) (&((res_du[num])->dat))
#define RESCOD(num) ((res_du[num])->type.code)

#define IND_CLM_STR(num, adr)                          \
{ (arg_du[num])->type.code = T_STR;                    \
  (arg_du[num])->type.len = strlen (adr);              \
  STR_PTR (ARGDAT(num)) = adr;                         \
  NL_FL   (ARGDAT(num)) = REGULAR_VALUE;               \
}

#define TAB_CLM_STR(num, adr, colno)                   \
{ IND_CLM_STR(num, adr);                               \
  tab_clm_nums[num] = colno;                           \
}

#define IND_CLM_NUM(num, typ, adr)                     \
{ (arg_du[num])->type.code = typ;                      \
  mem_to_DU (REGULAR_VALUE, typ, 0, adr, arg_du[num]); \
}

#define TAB_CLM_NUM(num, typ, adr, colno)              \
{ IND_CLM_NUM(num, typ, adr);                          \
  tab_clm_nums[num] = colno;                           \
}

#define RES_CLM(num, typ, colno)  \
{                                 \
  (res_du[num])->type.code = typ; \
  num_rows[num] = colno;          \
}

#define IND_READ(indid, an, rn)                           \
  ind_read (indid, an, arg_du, rn, num_rows, res_du, NULL, RSC)
#define IND_NEXT_READ(indid, an, rn)                      \
  ind_read (indid, an, arg_du, rn, num_rows, res_du, &scanid, RSC)
#define IND_DEL_READ(indid, an, rn)                      \
  ind_read (indid, an, arg_du, rn, num_rows, res_du, &scanid, DSC)
#define IND_UPD_READ(indid, an, rn)                      \
  ind_read (indid, an, arg_du, rn, num_rows, res_du, &scanid, MSC)
#define TAB_READ(tabid, an, nmax, rn) tab_read (tabid, an, nmax, tab_clm_nums,   \
                                                arg_du, rn, num_rows, res_du)

/* for strings "ptr" gets new address of the result string, *
 * for number types the result is copied to "ptr"           */

#define GET_RES_STR(num, ptr)               \
{                                           \
  if (NL_FL (RESDAT(num)) == REGULAR_VALUE) \
    ptr = STR_PTR (RESDAT(num));            \
  else                                      \
    ptr = NULL;                             \
}

#define GET_RES_NUM(num, ptr)                                \
{ put_dat (&VL (RESDAT(num)), 0, RESCOD (num),               \
	   REGULAR_VALUE, ptr, 0, RESCOD (num), NULL);       \
}

void
db_func_init (void)
{
  i4_t i;
  
  du = TYP_ALLOC (DU_ARR_LEN * 2, data_unit_t);
  for (i = 0; i < DU_ARR_LEN; i++)
    {
      arg_du[i] = du + i;
      res_du[i] = du + i + DU_ARR_LEN;
      ind_clm_nums[i] = i;
    }
} /* db_func_init */

#define PUT_BUF(num, dat) {cond[num/2] |= (num/2 * 2 == num) ? dat : (char) dat << 4;}
#define GET_BUF(num) (cond[num/2] & ((char) ((num/2 * 2 == num) ? 15 : 240)))

int
cond_form (i4_t clm_cnt, i4_t max_clm_num, i4_t *clm_nums,
	   data_unit_t **cond_du, char **res_cond)
/* forming of condition for open of scanning.                *
   (max_clm_num = (max column number in array clm_nums) + 1) *
 * Returns length of scale and pointer to it in res_cond     */
{
  i4_t i, err, sc_len, beg_len;
  static i4_t cond_lng = 0;
  static char *cond = NULL;
  char *cur_in_cond;
  
  /* length of scale for condition */
  beg_len = max_clm_num/2 + 1;
  
  /* length of condition */
  for (sc_len = beg_len + clm_cnt * SZ_SHT, i = 0; i < clm_cnt; i++)
    sc_len += sizeof_sqltype ((cond_du[i])->type);
  
  /* scale of condition making : */
  CHECK_ARR_SIZE (cond, cond_lng, sc_len, char);
  bzero (cond, sc_len);
  for (i = 0; i < clm_cnt; i++)
    PUT_BUF (clm_nums[i], EQ);
  for (i = 0; i < max_clm_num; i++)
    if (!GET_BUF (i))
      PUT_BUF(i, ANY);
  PUT_BUF (max_clm_num, ENDSC);
  
  /* the rest part of condition making : */
  for (cur_in_cond = cond + beg_len, i = 0; i < clm_cnt; i++)
    {
      err = DU_to_buf (cond_du[i], &cur_in_cond, &((cond_du[i])->type));
      if (err < 0)
	return err;
    }
  
  *res_cond = cond;
  return sc_len;
} /* cond_form */

i4_t
ind_read (Indid * indid, i4_t ind_clm_cnt, data_unit_t **ind_cond_du, i4_t read_cnt,
	  i4_t *read_col_nums, data_unit_t **read_du, Scanid *ext_scanid, char mode)
     /* Reading read_cnt columns from table accordingly index indid and *
      * given values (ind_cond_du) for finding in index;                *
      * If ext_scanid == NULL => function tries to read only first row  *
      *                        accordingly this index;                  *
      * If ext_scanid != NULL  => function reads next row in ext_scanid *
      * Function returns :  0 if the result was found,                  *
      *                     <0 if the result of index scanning is empty */
{
  int rc = 0, spn = 0, sc_len;
  char *ind_cond;
  Scanid local_scanid = -1, *scanid;
  
  scanid = (ext_scanid) ? ext_scanid : &local_scanid;
  
  if (*scanid < 0)
    {
      /* forming of condition for open of index scanning */
      sc_len = cond_form (ind_clm_cnt, ind_clm_cnt, ind_clm_nums, ind_cond_du, &ind_cond);
      if (sc_len < 0)
	return -ER_1;
      
      /* index scanning : */
      rc = *scanid = openiscan (indid, &spn, mode, read_cnt, read_col_nums, 0,
				 NULL, sc_len, (cond_buf_t) ind_cond, 0, NULL);
      if (spn > 0)
        {
          rollback (spn);
          return -ER_SYER;
        }
    }
  
  if (rc >= 0)
    rc = findrow (*scanid, read_du);
  if (!ext_scanid || rc < 0)
    {
      if (*scanid >= 0)
	closescan (*scanid);
      *scanid = -1;
    }
  
  return (rc);
} /* ind_read */

i4_t
tab_read (Tabid * tabid, i4_t tab_clm_cnt, i4_t max_clm_num,
	  i4_t *tab_col_nums, data_unit_t **tab_cond_du,
	  i4_t read_cnt, i4_t *read_col_nums, data_unit_t **read_du)
     /* Reading read_cnt columns from table tabid accordingly   *
      * given values (tab_cond_du) for finding in table;        *
      * (max_clm_num = max column number in array tab_col_nums) *
      * Function returns :  0 if the result was found,          *
      *            <0 if the result of table scanning is empty  */
{
  i4_t rc = 0, spn = 0, sc_len,  def_num[1];
  char *tab_cond;
  Scanid scanid;
  
  def_num[0] = 0;
  /* forming of condition for open of table scanning */
  sc_len = cond_form (tab_clm_cnt, max_clm_num + 1, tab_col_nums, tab_cond_du, &tab_cond);
  if (sc_len < 0)
    return -ER_1;
  
  /* table scanning : */
  if ((scanid = opentscan (tabid, &spn, 'R', (read_cnt) ? read_cnt : 1,
			   (read_cnt) ? read_col_nums : def_num,
			   sc_len, (cond_buf_t) tab_cond, 0, NULL)) < 0)
    return (scanid);
  if (spn > 0)
    {
      rollback (spn);
      return -ER_SYER;
    }
  rc = findrow (scanid, read_du);
  closescan (scanid);
  return (rc);
} /* tab_read */

i4_t
existsc (char *autnm)
     /* checking the existing of scheme with name "autnm"
	the function result is no 0 if that scheme
	doesn't exist and 0 in another way
     */
	
{
  CHECK_LEN (autnm);
  
  TAB_CLM_STR (0, autnm, SYSTABOWNER);

  return TAB_READ (&systabtabid, 1, SYSTABOWNER, 0);
} /* existsc */

/*-------------------------------------------------*/
i4_t
existtb (char *owner, char *tabnm, Tabid * tabid, char *type)
     /* checking the existing in scheme the table with name
	tabnm; this name must be qualified (with authorization
	identifier); the result of function is 0
	if table doesn't exist and 1 in another case.
        In the case of VIEW in tabid only field untabid is being filled.
    
      char  *tabnm;   * string containing table name        *
                     
      char  *owner;   * string containing user name         *
      Tabid *tabid;   * answer parameter - table identifier *
                      * if tabid == NULL,                   *
                      * identifier isn't returned           *
      char  *type;    * answer parameter - table type:      *
                      * 'B' - base table , 'V' - view       */
{
  i4_t int_res;
  char *tabtype;

  CHECK_LEN (owner);
  
  IND_CLM_STR (0, tabnm);
  IND_CLM_STR (1, owner);
  RES_CLM (0, T_INT, SYSTABUNTABID);
  RES_CLM (1, T_SRT, SYSTABSEGID);
  RES_CLM (2, T_INT, SYSTABTABD);
  RES_CLM (3, T_STR, SYSTABTABTYPE);

  int_res = IND_READ (SYSTABLESIND1ID, 2, 4);
  if (int_res < 0)
    return 0;
  
  GET_RES_STR (3, tabtype);
  if (type)
    *type = tabtype[0];

  if (tabid)
    {
      GET_RES_NUM (0, &(tabid->untabid));
      if (tabtype[0] == 'B')    /* base table */
        {
          GET_RES_NUM (1, &(tabid->segid));
          GET_RES_NUM (2, &(tabid->tabd));
        }
      else
        {
          assert (tabtype[0] == 'V'); /* view */
          tabid->segid = tabid->tabd = 0;
        }
    }
  return 1;
} /* existtb */

/*--------------------------------------------------------*/
i4_t
existcl (Tabid * tabid, char *colnm, sql_type_t * type,
         i4_t *colnumb, i4_t *def_func_id)
/*
  checking the existing of column  with given name in the pointed
  table; column name has to be  simple; the result of function is
  0 if this column exists, >0  if table exists but column doesn't
  and result is <0 if given table doesn't exist
  
      Tabid      *tabid;    ;; table identifier                      
      char       *colnm;    ;; column name                           
      sql_type_t *type;     ;; answer parameter - column description 
      i4_t        *colnumb;  ;; answer parameter - column number

*/
{
  i4_t int_res, collen, colprec;
  i2_t numb;
  char *coltype;

  IND_CLM_STR (0, colnm);
  IND_CLM_NUM (1, T_INT, &(tabid->untabid));
  
  RES_CLM (0, T_SRT, SYSCOLCOLNO);
  RES_CLM (1, T_STR, SYSCOLCOLTYPE);
  RES_CLM (2, T_INT, SYSCOLCOLTYPE1);
  RES_CLM (3, T_INT, SYSCOLCOLTYPE2);
  
  int_res = IND_READ (SYSCOLUMNSIND1ID, 2, 4);
  if (int_res < 0)
    return int_res;
  
  GET_RES_NUM (0, &numb);
  GET_RES_STR (1, coltype);
  GET_RES_NUM (2, &collen);
  GET_RES_NUM (3, &colprec);
  
  *colnumb  = numb;
  *def_func_id = 0;
  if (*coltype == SQLType_Char && colprec > 0)
    {
      *def_func_id = colprec;
      colprec = 0;
    }
  *type = pack_type(*coltype,collen,colprec);
  
  return 0;
}

/*--------------------------------------------------------*/

#define   STR_SAVE(to)                           \
{                                                \
  if (string)                                    \
    {                                            \
      to = GET_MEMC (char, strlen (string) + 1); \
      strcpy (to, string);                       \
    }                                            \
  else                                           \
    to = NULL;                                   \
}

i4_t
tab_cl (Tabid * tabid, i2_t clnm, sql_type_t * type, 
	char **cl_name, char **defval, char **defnull,
        i4_t *def_func_id)
/*
  this  function gets information  about  column which number  is
  clnm.  The function  result is 0  in the case of correct finish
  of function and <0 if that table doesn't exist
	
      Tabid *tabid;      ;; table identifier                   
      i4_t    clnm;       ;; column number                      
      sql_type_t  *type; ;; answer parameter - column description 

*/
{
  i4_t int_res, collen, colprec;
  char *coltype, *string;
  
  IND_CLM_NUM (0, T_INT, &(tabid->untabid));
  IND_CLM_NUM (1, T_SRT, &clnm);
  
  RES_CLM (0, T_STR, SYSCOLCOLNAME);
  RES_CLM (1, T_STR, SYSCOLCOLTYPE);
  RES_CLM (2, T_INT, SYSCOLCOLTYPE1);
  RES_CLM (3, T_INT, SYSCOLCOLTYPE2);
  RES_CLM (4, T_STR, SYSCOLDEFVAL);
  RES_CLM (5, T_STR, SYSCOLDEFNULL);
  
  int_res = IND_READ (SYSCOLUMNSIND2ID, 2, 6);
  if (int_res < 0)
    return int_res;
  
  if (cl_name)
    {
      GET_RES_STR (0, string);
      STR_SAVE (*cl_name);
    }
  GET_RES_STR (1, coltype);
  GET_RES_NUM (2, &collen);
  GET_RES_NUM (3, &colprec);
  if (defval)
    {
      GET_RES_STR (4, string);
      STR_SAVE (*defval);
    }
  if (defnull)
    {
      GET_RES_STR (5, string);
      STR_SAVE (*defnull);
      if (**defnull == '0')
	**defnull = 0;
    }
  if (def_func_id)
    {
      *def_func_id = 0;
      if (*coltype == SQLType_Char && colprec > 0)
	{
	  *def_func_id = colprec;
	  colprec = 0;
	}
    }
  *type = pack_type(*coltype,collen,colprec);
  
  return 0;
} /* tab_cl */

/*--------------------------------------------------------*/
/*
 * get statistic information about column which number is
 * clnm. The function result is 0 in the case of correct
 * finish of function and < 0 if error
 */
i4_t
get_col_stat (i4_t untabid, i2_t clnm, col_stat_info *col_info)
{
  i4_t int_res, collen, colprec;
  char *coltype, *string;
  
  IND_CLM_NUM (0, T_INT, &untabid);
  IND_CLM_NUM (1, T_SRT, &clnm);
  
  RES_CLM (0, T_STR, SYSCOLCOLTYPE);
  RES_CLM (1, T_INT, SYSCOLCOLTYPE1);
  RES_CLM (2, T_INT, SYSCOLCOLTYPE2);
  RES_CLM (3, T_STR, SYSCOLMIN);
  RES_CLM (4, T_STR, SYSCOLMAX);
  
  int_res = IND_READ (SYSCOLUMNSIND2ID, 2, 5);
  if (int_res < 0)
    return int_res;
  
  GET_RES_STR (0, coltype);
  col_info->min.type.code = col_info->max.type.code = *coltype;
  
  GET_RES_NUM (1, &collen);
  col_info->min.type.len = col_info->max.type.len = collen;
  
  GET_RES_NUM (2, &colprec);
  col_info->min.type.prec = col_info->max.type.prec = colprec;
  
  GET_RES_STR (3, string);
  mem_to_DU ((string) ? REGULAR_VALUE : NULL_VALUE, col_info->min.type.code,
	     (string) ? strlen (string) : 0, string, &(col_info->min));
  
  GET_RES_STR (4, string);
  mem_to_DU ((string) ? REGULAR_VALUE : NULL_VALUE, col_info->max.type.code,
	     (string) ? strlen (string) : 0, string, &(col_info->max));
  
  return 0;
} /* get_col_stat */

/*--------------------------------------------------------*/
static int
DU_change_type (data_unit_t *dt, i4_t tp_code)
{
  int rc = 0;
  
  if (dt->type.code != tp_code && dt->dat.nl_fl == REGULAR_VALUE)
    {
      rc = put_dat (&VL(&(dt->dat)), 0, dt->type.code, 
                         REGULAR_VALUE, &VL(&(dt->dat)), 0, tp_code, NULL);
      dt->type.code = tp_code;
    }
  if (dt->type.len > MAX_STAT_LEN)
    dt->type.len = MAX_STAT_LEN;
    
  return rc;
} /* DU_change_type */

/*--------------------------------------------------------*/
#define INS_min change_info->ins_min
#define INS_max change_info->ins_max
#define DEL_min change_info->del_min
#define DEL_max change_info->del_max
i4_t
put_col_stat (i4_t untabid, i2_t clnm, col_change_info *change_info)
     /* modifies info in SYSCOLUMNS about MIN and/or MAX value in column. */
{
  static data_unit_t null_val;
  data_unit_t du_arr[COLUMNS_COLNO];
  data_unit_t *new_min = NULL, *new_max = NULL;
  i4_t mlist[2], nm = 0, rc;
  sql_type_t tp[2];
  Scanid scanid = -1;
  col_stat_info col_info;
  i4_t error, col_nl_fl, ins_nl_fl, del_nl_fl;

  bzero(&col_info,sizeof(col_info));

  null_val.dat.nl_fl = NULL_VALUE;
  
  if (!change_info ||
      (INS_min.dat.nl_fl == UNKNOWN_VALUE &&
       INS_max.dat.nl_fl == UNKNOWN_VALUE &&
       DEL_min.dat.nl_fl == UNKNOWN_VALUE &&
       DEL_max.dat.nl_fl == UNKNOWN_VALUE))
    return 0;
  
  if ((error = get_col_stat (untabid, clnm, &col_info)) < 0)
    return error;

  col_nl_fl = col_info.min.dat.nl_fl;
  ins_nl_fl = INS_min.dat.nl_fl;
  del_nl_fl = DEL_min.dat.nl_fl;
  assert (col_nl_fl == col_info.max.dat.nl_fl &&
          ins_nl_fl == INS_max.dat.nl_fl &&
          del_nl_fl == DEL_max.dat.nl_fl);
  /* making of new_min & new_max */
  if (col_nl_fl == NULL_VALUE) {
    if (ins_nl_fl == REGULAR_VALUE)
      /* insertion of not null value */
      {
        new_min = &(INS_min);
        new_max = &(INS_max);
      }
    else {}
  } else {
    if (del_nl_fl == NULL_VALUE || ins_nl_fl == NULL_VALUE)
      /* deletion or insertion of unknown value */
      new_min = new_max = &null_val;
    else
      {
        if (ins_nl_fl == REGULAR_VALUE)
          {
            MK_BIN_OPER (LT, INS_min, col_info.min);
            if (COMP_RESULT)
              {
                new_min = &(INS_min);
                col_info.min = INS_min;
              }
            MK_BIN_OPER (GT, INS_max, col_info.max);
            if (COMP_RESULT)
              {
                new_max = &(INS_max);
                col_info.max = INS_max;
              }
          }
        if (del_nl_fl == REGULAR_VALUE)
          {
            i4_t lo=0,hi=0;
            MK_BIN_OPER (LE, DEL_min, col_info.min);
            if (COMP_RESULT) lo = 1 ;
            MK_BIN_OPER (GE, DEL_max, col_info.max);
            if (COMP_RESULT) hi = 1;
            
#if 0
            if (lo) new_min = &null_val; /* conflict with assertation above */
            if (hi) new_max = &null_val;
#else
            if ( lo && hi )
              new_min = new_max = &null_val;
#endif
          }
      }
  }

  if (new_min || new_max)
    {
      bzero (du_arr, sizeof (data_unit_t) * COLUMNS_COLNO);
      IND_CLM_NUM (0, T_INT, &untabid);
      IND_CLM_NUM (1, T_SRT, &clnm);
  
      if ((rc = IND_UPD_READ (SYSCOLUMNSIND2ID, 2, 0)))
        return rc;
  
      if (new_min)
        {
          DU_change_type (new_min, col_info.min.type.code);
          mlist[nm] = SYSCOLMIN;
          conv_DU_to_str (new_min, du_arr + SYSCOLMIN);
          tp[nm++] = du_arr[SYSCOLMIN].type;
        }
      if (new_max)
        {
          DU_change_type (new_max, col_info.min.type.code);
          mlist[nm] = SYSCOLMAX;
          conv_DU_to_str (new_max, du_arr + SYSCOLMAX);
          tp[nm++] = du_arr[SYSCOLMAX].type;
        }
  
      if ((rc = mod_data (scanid, nm, du_arr, tp, mlist)))
        return rc;
      closescan (scanid);
    }
  return 0;
} /* put_col_stat */
#undef INS_min
#undef INS_max
#undef DEL_min
#undef DEL_max

/*--------------------------------------------------------*/
i4_t
tabclnm (Tabid * tabid, i4_t *nnulcolnum)
     /* this function gets the number of columns in pointed table;
	the result of function is this number (>0)
	or negative if table doesn't exist
  
      Tabid *tabid;        * table identifier          */
{
  i4_t rc;
  i2_t ncols;			/* the number of columns   */
  
  assert (nnulcolnum);
  IND_CLM_NUM (0, T_INT, &(tabid->untabid));
  RES_CLM (0, T_SRT, SYSTABNCOLS);
  RES_CLM (1, T_INT, SYSTABNNULCOLNUM);

  rc = IND_READ (SYSTABLESIND2ID, 1, 2);
  if (rc < 0)
    return rc;
  
  GET_RES_NUM (0, &ncols);
  GET_RES_NUM (1, nnulcolnum);
  
  return ncols;
} /* tabclnm */

/*--------------------------------------------------------*/
i4_t
get_nrows (i4_t untabid)
     /* function gets the number of rows in table; *
      * the result of function is this number (>0) *
      * or negative if table doesn't exist         */
{
  i4_t rc, nrows;
  
  IND_CLM_NUM (0, T_INT, &untabid);
  RES_CLM (0, T_INT, SYSTABNROWS);

  if ((rc = IND_READ (SYSTABLESIND2ID, 1, 1)))
    return rc;
  
  GET_RES_NUM (0, &nrows);
  
  return nrows;
} /* get_nrows */

/*--------------------------------------------------------*/
i4_t
put_nrows (i4_t untabid, i4_t nrows)
/* modifies info in SYSTABLES about rows number in table */
{
  i4_t mlist = SYSTABNROWS, rc;
  sql_type_t tp;
  data_unit_t du_arr[TABLES_COLNO];
  Scanid scanid = -1;
  
  tp = pack_type(T_INT,0,0);
  
  bzero (du_arr, sizeof (data_unit_t) * TABLES_COLNO);
  IND_CLM_NUM (0, T_INT, &untabid);
  
  if ((rc = IND_UPD_READ (SYSTABLESIND2ID, 1, 0)))
    return rc;
  
  du_arr[SYSTABNROWS].type = tp;
  mem_to_DU (REGULAR_VALUE, T_INT, 0, &nrows, du_arr + SYSTABNROWS);
  if ((rc = mod_data (scanid, 1, du_arr, &tp, &mlist)))
    return rc;
  closescan (scanid);
  return 0;
} /* put_nrows */

/*--------------------------------------------------------*/
int
get_tabid (i4_t untabid, Tabid *tabid)
     /* this function gets all fields of tabid for table with this untabid *
      * The result is 0 if o'k or negative if table doesn't exist          */
{
  i4_t rc;
  
  IND_CLM_NUM (0, T_INT, &untabid);
  RES_CLM (0, T_SRT, SYSTABSEGID);
  RES_CLM (1, T_INT, SYSTABTABD);

  rc = IND_READ (SYSTABLESIND2ID, 1, 2);
  if (rc < 0)
    return rc;
  
  tabid->untabid = untabid;
  GET_RES_NUM (0, &(tabid->segid));
  GET_RES_NUM (1, &(tabid->tabd));
  
  return 0;
} /* tabclnm */

/*--------------------------------------------------------*/

#define SEE_PVLG(type, j)                         \
{                                                 \
  u_need = r_need = 0;                            \
  for (i = 0, j = 1; (acttype[i]) && (j!=2); i++) \
    if (!strchr (type, acttype[i]))               \
      switch (acttype[i])                         \
	{                                         \
	case 'U' :                                \
	  if (un)                                 \
	    if (strchr (type, 'u'))               \
	      { j = 0; u_need ++; break; }        \
	  j = 2;                                  \
	  break;                                  \
	case 'R' :                                \
	  if (rn)                                 \
	    if (strchr (type, 'r'))               \
	      { j = 0; r_need ++; break; }        \
	  j = 2;                                  \
	  break;                                  \
	default :                                 \
	  j = 2;                                  \
	}                                         \
}

i4_t
tabpvlg (i4_t untabid, char *user, char *acttype,
	 i4_t un, i4_t *ulist,i4_t rn, i4_t *rlist)
/*
* checking access privilegies
i4_t untabid   - table identifier
char *user    - user name
char *acttype - string with the following codes:
                      * 'S' - SELECT,                     *
                      * 'D' - DELETE,                     *
		      * 'U' - UPDATE,                     *
		      * 'I' - INSERT,                     *
		      * 'R' - REFERENCES                  *
		      * 'G' - GRANT OPTION                *
i4_t un       - the number of rows to which the act "update"
               is applied      
i4_t *ulist   - array of column number to which the act "update"
               is applied
i4_t rn,      - the number of rows for which the act REFERENCES
	       is applied
i4_t *rlist   - array of rows for which the act REFERENCES
	       is applied                                    */

{
  /* in SYSTABAUTH.tabauth : 'S','D','U','I','R','G' - rights *
   * on all columns of table. 'u' and 'r' - there are not     *
   * full rights but in table SYSCOLAUTH are described        *
   * rights on some (not all) columns.                        */
  
  i4_t i, j, pvlg, u_need, r_need, rc, rc1;
  char type[MAX_COLNO], *type1 = NULL, *type2 = NULL;
  i2_t colno;
  char * public = "PUBLIC";
  
  type[0] = 0;
  CHECK_LEN (user);
  
  IND_CLM_NUM (0, T_INT, &untabid);
  IND_CLM_STR (1, user);
  RES_CLM (0, T_STR, SYSTABAUTHTABAUTH);
  
  rc = IND_READ (SYSTABAUTHIND1ID, 2, 1);
  
  if (!rc)
    /* there are previlegies on the table for this user */
    {
      GET_RES_STR (0, type1);
      SEE_PVLG (type1, pvlg);
      /* pvlg == 0 => we should check information for columns    *
       * pvlg == 1 => all wanted user rights were found          *
       * pvlg == 2 => user have not all wanted rights            */
      if (pvlg == 1)
	return 0;
    }
  if (type1)
    strcpy (type, type1);
  
  IND_CLM_STR (1, public);
  
  rc1 = IND_READ (SYSTABAUTHIND1ID, 2, 1);
  if (rc && rc1)
    return rc;
  if (!rc1)
    {
      GET_RES_STR (0, type2);
      if (type2)
	strcpy (type + strlen (type), type2);
    }

  SEE_PVLG (type, pvlg);
  if (pvlg)
    return pvlg - 1;
  
/*    check  rights UPDATE  or | and  REFERENCES for columns */
  
  IND_CLM_NUM (0, T_INT, &untabid);
  IND_CLM_STR (2, user);
  RES_CLM (0, T_STR, SYSCOLAUTHCOLAUTH);

  if (u_need)
    for (j = 0; j < un; j++)
      {
	colno = ulist[j];
	IND_CLM_NUM (1, T_SRT, &colno);
	rc = IND_READ (SYSCOLAUTHIND1ID, 3, 1);
	GET_RES_STR (0, type1);
      
	if (rc || !strchr (type1, 'U'))
	  return 1;
      }
  
  if (r_need)
    for (j = 0; j < rn; j++)
      {
	colno = rlist[j];
	IND_CLM_NUM (1, T_SRT, &colno);
	rc = IND_READ (SYSCOLAUTHIND1ID, 3, 1);
	GET_RES_STR (0, type1);
      
	if (rc || !strchr (type1, 'R'))
	  return 1;
      }
  
  return 0;
} /* tabpvlg */

static TXTREF
get_table(Tabid *tabid)
{
  extern void load_table __P((TXTREF ptbl));
  TXTREF t;
  for ( t = *(Root_ptr(Vcb_Root)); t ; t = RIGHT_TRN(t))
    {
      if (CODE_TRN(t) == ANY_TBL)
	load_table(t);
      if ( (CODE_TRN(t) == TABLE) && ((TBL_TABID(t)).untabid == tabid->untabid))
	return t;
    }
  /* get_table_name for given tabid, create ANY_TBL and load_table */
  return TNULL;
}

#define      INDFM  pind->Ind

/* Checking of index existence for pointed table.
   Function returns NULL when there are no indexes for
   pointed table or pointer for list of index descriptions */

TXTREF
existind (Tabid * tabid)
{
  i2_t ncol;
  i4_t i, rc;
  Indid  indid;
  char *inxtype;
  Scanid scanid = -1;
  TXTREF ind_list = TNULL,tbl, cptrs;
  
  indid.tabid = *tabid;
  
  tbl = get_table(tabid);
  if (!tbl)
    return TNULL;
  
  IND_CLM_NUM (0, T_INT, &(tabid->untabid));
  
  RES_CLM (0,  T_STR, SYSINDEXTYPE);
  RES_CLM (1,  T_SRT, SYSINDEXNCOL);
  RES_CLM (2,  T_INT, SYSINDEXUNINDID);
  RES_CLM (3,  T_SRT, SYSINDEXCOLNO1);
  RES_CLM (4,  T_SRT, SYSINDEXCOLNO2);
  RES_CLM (5,  T_SRT, SYSINDEXCOLNO3);
  RES_CLM (6,  T_SRT, SYSINDEXCOLNO4);
  RES_CLM (7,  T_SRT, SYSINDEXCOLNO5);
  RES_CLM (8,  T_SRT, SYSINDEXCOLNO6);
  RES_CLM (9,  T_SRT, SYSINDEXCOLNO7);
  RES_CLM (10, T_SRT, SYSINDEXCOLNO8);
  
  for (;;)
    {
      cptrs = TNULL;
      rc = IND_NEXT_READ (SYSINDEXIND, 1, 11);
      if (rc < 0)
        break;
      
      GET_RES_STR (0, inxtype);
      GET_RES_NUM (1, &ncol);
      GET_RES_NUM (2, &indid.unindid);
      
      for (i = ncol; i--;)
	{
	  i2_t col_no;
	  TXTREF cptr;
	  
	  GET_RES_NUM (3 + i, &col_no);
	  cptr = gen_colptr(find_column_by_number(tbl,col_no));
	  RIGHT_TRN(cptr) = cptrs;
	  cptrs = cptr;
	}
      
      {
	TXTREF ind;
	enum token code = INDEX;
	
	if (inxtype)
          switch (inxtype[0])
            {
            case 'U' :
              code = UNIQUE;
              break;
            case 'P' :
              code = PRIMARY;
              break;
            default  :
              break;
            }
	ind = gen_parent(code,cptrs);
	/* assert(ARITY_TRN(ind) == ncol); */
	IND_INDID(ind) = indid;
	ind_list = join_list(ind_list,ind);
      }
    }/* for */
  return ind_list;
} /* existind */

#define SAVE_CONSTR                           \
{                                             \
  if (pred_constr != NULL)                    \
    pred_constr->next = cur_constr;           \
  else                                        \
    constr_list = cur_constr;                 \
  pred_constr = cur_constr;                   \
}

int
new_ref_to_ind (Tabid *tabid, i2_t ncol, char *columns_fl, Scanid scanid)
     /* searching for index to scan for costraint checking *
      * and putting its identifier to SYSREFCONSTR         */
{
  TXTREF cur_ind, CurIndCol;
  i4_t unindid, mlist = SYSREFCONSTRINDTO;
  sql_type_t tp;
  data_unit_t du_arr[SYSREFCONSTRINDTO+1];
  
  tp = pack_type(T_INT, 0, 0);
  bzero (du_arr, sizeof (data_unit_t) * (SYSREFCONSTRINDTO+1));
  for (cur_ind = existind (tabid); cur_ind; cur_ind = RIGHT_TRN (cur_ind))
    if (ARITY_TRN (cur_ind) == ncol)
      {
        i4_t eq_num = ncol;
	for (CurIndCol = DOWN_TRN (cur_ind);
	     CurIndCol; CurIndCol = RIGHT_TRN (CurIndCol))
	  if (columns_fl[COL_NO (OBJ_DESC (CurIndCol))])
	    eq_num--;
	if (!eq_num)
	  break;
      }
  
  if (!cur_ind)
    yyfatal ("Incorrect state of database: there is no index for columns from constraint");
  
  unindid = (IND_INDID (cur_ind)).unindid;
  
  du_arr[SYSREFCONSTRINDTO].type = tp;
  mem_to_DU (REGULAR_VALUE, T_INT, 0, &unindid, du_arr + SYSREFCONSTRINDTO);
  mod_data (scanid, 1, du_arr, &tp, &mlist);
  return unindid;
}

Constr_Info *
get_constraints (i4_t untabid, i4_t tab_clm_num)
/* Gets information about constraints to table. *
 * Returns the list constraints descriptions    *
 * (it can be = NULL if the list is empty).     */
{
  i2_t ncol, cur_col;
  i4_t *columns_from, *columns_to;
  i4_t i, rc, ref_untabid, ref_ind;
  Constr_Info *constr_list = NULL, *cur_constr, *pred_constr = NULL;
  VADR V_constr_ptr, V_ref_tabid, V_columns_from, V_columns_to, V_ref_indid;
  S_Constraint *constr_ptr;
  Tabid *ref_tabid;
  Indid *ref_indid;
  Scanid scanid = -1;
  
  /* checking when this table is referencing : */
  
  for (;;)
    {
      IND_CLM_NUM (0,  T_INT, &(untabid));
  
      RES_CLM (0,  T_INT, SYSREFCONSTRTABTO);
      RES_CLM (1,  T_SRT, SYSREFCONSTRNCOLS);
      RES_CLM (2,  T_SRT, SYSREFCONSTRCOLNOFR1);
      RES_CLM (3,  T_SRT, SYSREFCONSTRCOLNOFR2);
      RES_CLM (4,  T_SRT, SYSREFCONSTRCOLNOFR3);
      RES_CLM (5,  T_SRT, SYSREFCONSTRCOLNOFR4);
      RES_CLM (6,  T_SRT, SYSREFCONSTRCOLNOFR5);
      RES_CLM (7,  T_SRT, SYSREFCONSTRCOLNOFR6);
      RES_CLM (8,  T_SRT, SYSREFCONSTRCOLNOFR7);
      RES_CLM (9,  T_SRT, SYSREFCONSTRCOLNOFR8);
      RES_CLM (10, T_SRT, SYSREFCONSTRCOLNOTO1);
      RES_CLM (11, T_SRT, SYSREFCONSTRCOLNOTO2);
      RES_CLM (12, T_SRT, SYSREFCONSTRCOLNOTO3);
      RES_CLM (13, T_SRT, SYSREFCONSTRCOLNOTO4);
      RES_CLM (14, T_SRT, SYSREFCONSTRCOLNOTO5);
      RES_CLM (15, T_SRT, SYSREFCONSTRCOLNOTO6);
      RES_CLM (16, T_SRT, SYSREFCONSTRCOLNOTO7);
      RES_CLM (17, T_SRT, SYSREFCONSTRCOLNOTO8);
      RES_CLM (18, T_INT, SYSREFCONSTRINDTO);
  
      rc = IND_UPD_READ (SYSREFCONSTRINDEX, 1, 19);
      if (rc < 0)
	break;
  
      cur_constr = GET_MEMC (Constr_Info, 1);
      cur_constr->next = NULL;
      cur_constr->mod_cols = GET_MEMC (char, tab_clm_num);
      P_VMALLOC (constr_ptr, 1, S_Constraint);
      cur_constr->constr = V_constr_ptr;
      constr_ptr->constr_type = cur_constr->constr_type = FOREIGN;
      
      GET_RES_NUM (0,  &ref_untabid);
      GET_RES_NUM (1,  &ncol);
      GET_RES_NUM (18, &ref_ind);
      
      P_VMALLOC (columns_from, ncol, i4_t);
      P_VMALLOC (columns_to,   ncol, i4_t);
      constr_ptr->col_arg_list = V_columns_from;
      constr_ptr->col_ref_list = V_columns_to;
      constr_ptr->colnum = ncol;
      assert(ncol<=8 && ncol<=tab_clm_num);
	
      for (i = 0; i < ncol; i++)
	{
	  GET_RES_NUM (2 + i, &cur_col);
          assert(cur_col<tab_clm_num);
	  columns_from[i] = cur_col;
	  GET_RES_NUM (10 + i, &cur_col);
	  columns_to[i] = cur_col;
	  (cur_constr->mod_cols)[columns_to[i]] = TRUE;
	}
      
      P_VMALLOC (ref_indid, 1, Indid);
      constr_ptr->object = V_ref_indid;
      get_tabid (ref_untabid, &(ref_indid->tabid));
      ref_indid->unindid = (ref_ind) ? ref_ind :
	new_ref_to_ind (&(ref_indid->tabid), ncol, cur_constr->mod_cols, scanid);
      
      SAVE_CONSTR;
    }				/* for */
  
  /* checking when this table is referenced : */
  
  for (;;)
    {
      IND_CLM_NUM (0,  T_INT, &(untabid));
      
      RES_CLM (0,  T_INT, SYSREFCONSTRTABFROM);
      RES_CLM (1,  T_SRT, SYSREFCONSTRNCOLS);
  
      rc = IND_NEXT_READ (SYSREFCONSTRIND1, 1, 18);
      if (rc < 0)
	break;
  
      cur_constr = GET_MEMC (Constr_Info, 1);
      cur_constr->next = NULL;
      cur_constr->mod_cols = GET_MEMC (char, tab_clm_num);
      P_VMALLOC (constr_ptr, 1, S_Constraint);
      cur_constr->constr = V_constr_ptr;
      constr_ptr->constr_type = cur_constr->constr_type = REFERENCE;
      
      GET_RES_NUM (0,  &ref_untabid);
      GET_RES_NUM (1,  &ncol);
      assert(ncol<=8 && ncol<=tab_clm_num);
      
      P_VMALLOC (columns_from, ncol, i4_t);
      P_VMALLOC (columns_to,   ncol, i4_t);
      constr_ptr->col_arg_list = V_columns_from;
      constr_ptr->col_ref_list = V_columns_to;
      constr_ptr->colnum = ncol;
	
      for (i = 0; i < ncol; i++)
	{
	  GET_RES_NUM (10 + i, &cur_col);
          assert(cur_col<tab_clm_num);
	  columns_from[i] = cur_col;
	  GET_RES_NUM (2 + i, &cur_col);
	  columns_to[i] = cur_col;
	  (cur_constr->mod_cols)[columns_from[i]] = TRUE;
	}
      
      P_VMALLOC (ref_tabid, 1, Tabid);
      constr_ptr->object = V_ref_tabid;
      get_tabid (ref_untabid, ref_tabid);
      
      SAVE_CONSTR;
    }				/* for */
      
  IND_CLM_NUM (0,  T_INT, &(untabid));
  
  RES_CLM (0,  T_INT, SYSCHCONSTRCHCONID);
  RES_CLM (1,  T_INT, SYSCHCONSTRCONSIZE);
  RES_CLM (2,  T_SRT, SYSCHCONSTRNCOLS);
  RES_CLM (3,  T_SRT, SYSCHCONSTRCOLNO1);
  RES_CLM (4,  T_SRT, SYSCHCONSTRCOLNO2);
  RES_CLM (5,  T_SRT, SYSCHCONSTRCOLNO3);
  RES_CLM (6,  T_SRT, SYSCHCONSTRCOLNO4);
  RES_CLM (7,  T_SRT, SYSCHCONSTRCOLNO5);
  RES_CLM (8,  T_SRT, SYSCHCONSTRCOLNO6);
  RES_CLM (9,  T_SRT, SYSCHCONSTRCOLNO7);
  RES_CLM (10, T_SRT, SYSCHCONSTRCOLNO8);
  
  /* finding of all 'CHECK' constraints for this table : */
  
  for (;;)
    {
      rc = IND_NEXT_READ (SYSCHCONSTRINDEX, 1, 11);
      if (rc < 0)
	break;
  
      cur_constr = GET_MEMC (Constr_Info, 1);
      cur_constr->next = NULL;
      cur_constr->mod_cols = GET_MEMC (char, tab_clm_num);
      cur_constr->constr = VNULL;
      cur_constr->constr_type = CHECK;
      
      GET_RES_NUM (0,  &(cur_constr->conid));
      GET_RES_NUM (1,  &(cur_constr->consize));
      GET_RES_NUM (2,  &ncol);
      assert(ncol<=8 && ncol<=tab_clm_num);
      
      for (i = 0; i < ncol; i++)
	{
	  GET_RES_NUM (3 + i, &cur_col);
          assert(cur_col<tab_clm_num);
	  (cur_constr->mod_cols)[cur_col] = TRUE;
	}
      
      SAVE_CONSTR;
    }				/* for */

  return constr_list;
} /* get_constraints */

char *
get_chconstr (i4_t chconid, i4_t consize)
     /* gets the string for 'CHECK' constraint with identifier 'chconid' */
{
  char *string = NULL, *frag, *cur_in_string;
  i2_t fragno;
  i4_t rc, fragsize;
  
  IND_CLM_NUM (0, T_INT, &chconid);
  RES_CLM (0, T_INT, SYSCHCONSTR2FRAGSIZE);
  RES_CLM (1, T_STR, SYSCHCONSTR2FRAG);
  
  cur_in_string = string = GET_MEMC (char, consize);
  
  for (fragno = 0; ; fragno++)
    {
      IND_CLM_NUM (1, T_SRT, &fragno);
      
      rc = IND_READ (SYSCHCONSTRTWOIND, 2, 2);
      if (rc < 0)
	break;
      
      GET_RES_NUM (0, &fragsize);
      GET_RES_STR (1, frag);
      bcopy (frag, cur_in_string, fragsize);
      cur_in_string += fragsize;
    }
  assert (cur_in_string - string == consize);
  return string;
} /* get_chconstr */

/*--------------------------------------------------------*/
i4_t
getview (i4_t untabid, char **res_segm)
/*
  receiving the  syntax tree  of view. Function returns
  the summary  size of memory  (in byte)>=0 in the case of
  success or negative value when the view doesn't exist.
   
      i4_t    untabid;   * unique view identifier              *
      char  **res_segm; * array of bytes with lenght "size"   *
                        *  which is used as answer parameter  *
                        *  of function.                       *
*/
{
  static char  *buf = NULL;
  static i4_t    buf_size = 0;
  i4_t   size = 0, rc, fragsize;
  i2_t fragno;
  char *frag;
  
  IND_CLM_NUM(0, T_INT, &untabid);

  RES_CLM ( 0, T_INT, SYSVIEWSFRAGSIZE);
  RES_CLM ( 1, T_STR, SYSVIEWSFRAG);

  for (fragno = 0; ; fragno++)
    {
      IND_CLM_NUM (1, T_SRT, &fragno);
      
      rc = IND_READ (SYSVIEWSIND, 2, 2);
      if (rc < 0)
	break;
      
      GET_RES_NUM (0, &fragsize);
      size += fragsize;
      CHECK_ARR_ELEM (buf, buf_size, size, size, char);
      GET_RES_STR (1, frag);
      bcopy (frag, buf + size - fragsize, fragsize);
    }

  *res_segm = buf;
  return size;
}

#define DO(x) {if ((rc = x)) return rc;}

#define IND_DELROWS(ind, tab) 	  	        \
{						\
  while (IND_DEL_READ (ind, 1, 0) >= 0)	        \
    { 					        \
      handle_statistic (SYS##tab.untabid, tab##_COLNO, 0, NULL, 0); \
      DO (delrow (scanid));      		\
    } 					        \
  if (scanid >= 0)				\
    closescan (scanid);				\
  scanid = -1;					\
}

i4_t
drop_table (i4_t untabid)
{
  Scanid scanid = -1, sv_scanid;
  char *tabtype;
  i4_t rc, conid;
  Tabid tabid;

  if (get_tabid (untabid, &tabid))
    return -ER_NDR;

  /* references checking */
  IND_CLM_NUM (0, T_INT, &untabid);
  if (IND_READ (SYSREFCONSTRIND1, 1, 0) >= 0)
    /* there are still references to this table */
    return -ER_DRTAB;
  
  /* SYSTABLES handling */
  RES_CLM (0, T_STR, SYSTABTABTYPE);
  
  if (IND_DEL_READ (SYSTABLESIND2ID, 1, 1))
    return -ER_NDR;
  GET_RES_STR (0, tabtype);
  if (tabtype[0] == 'B')    /* base table */
    DO (dropptab (&tabid))
  else
    {
      sv_scanid = scanid;
      scanid = -1;
      IND_DELROWS (SYSVIEWSIND, VIEWS);
      scanid = sv_scanid;
    }
  DO (delrow (scanid));
  handle_statistic (SYSTABLES.untabid, TABLES_COLNO, 0, NULL, 0);
  closescan (scanid);
  scanid = -1;
  
  IND_CLM_NUM (0, T_INT, &untabid);
  
  IND_DELROWS (SYSCOLUMNSIND2ID,  COLUMNS);
  IND_DELROWS (SYSCOLAUTHIND1ID,  COLAUTH);
  IND_DELROWS (SYSTABAUTHIND1ID,  TABAUTH);
  IND_DELROWS (SYSINDEXIND,       INDEXES);
  IND_DELROWS (SYSREFCONSTRINDEX, REFCONSTR);

  /* handling of tables with info about CHECK constraints */
  RES_CLM (0, T_INT, SYSCHCONSTRCHCONID);
  while (IND_DEL_READ (SYSCHCONSTRINDEX, 1, 1) >= 0)
    {
      GET_RES_NUM (0,  &conid);
      IND_CLM_NUM (0, T_INT, &conid);
      sv_scanid = scanid;
      scanid = -1;
      IND_DELROWS (SYSTABAUTHIND1ID, TABAUTH);
      scanid = sv_scanid;
      
      DO (delrow (scanid));
      handle_statistic (SYSCHCONSTR.untabid,
                        CHCONSTR_COLNO, 0, NULL, 0);
      IND_CLM_NUM (0, T_INT, &untabid);
    }
  if (scanid >= 0)
    closescan (scanid);
  return 0;
} /* drop_table */

#undef DO
#undef IND_DELROWS

/**************** THE E N D ****************/
