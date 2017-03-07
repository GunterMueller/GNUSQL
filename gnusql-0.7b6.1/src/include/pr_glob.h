/* 
 *  pr_glob.h - file with some constants, data types definitions
 *              and functions prototypes  used by all components
 *              of GNU SQL compiler
 *                
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Konstantin Dyshlevoj 
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

/* $Id: pr_glob.h,v 1.248 1998/09/29 21:25:55 kimelman Exp $ */

#ifndef __PR_GLOB_H__
#define __PR_GLOB_H__

#include "setup_os.h"

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif


#include "sql.h"
#include "type.h"
#include "typecpi.h" 
#include "typepi.h" 
#ifdef __INTERPR__
  #include "typepi.h"
  #include "typeif.h" 
#endif

#define TY_EST      long

typedef i4_t        TYPE_VARI;
typedef char        TYPE_VW;

extern i4_t         PRCD_DEBUG;

typedef struct {
  void *SHPls;
  i2_t  TbNum;
  i4_t  Cnt;
  i4_t *Trees;
} SetPls;

#define UNKNOWN_VALUE 0
#define REGULAR_VALUE 1
#define NULL_VALUE    2
#define NOT_CONVERTED 3

/* determination of code of value                 *
 * (UNKNOWN_VALUE, REGULAR_VALUE or NULL_VALUE) : */
#define TRN_FINISH    exit(0)

#define IE    (1L<<4)    /*     there is some index           */
#define IC    (1L<<5)    /*                   clustered index */
#define IU    (1L<<6)    /*                   unique index    */
#define IJ    (1L<<7)    /*                   composite index */


#define GET_MEMC(name,n)   (name *)xmalloc(sizeof(name)*n);

#define SIZES(x)           sizeof(struct x)

#define CH_ASC     'A'
#define CH_DES     'D'

#define FN_COUNT    1
#define FN_AVG      2
#define FN_MAX      3
#define FN_MIN      4
#define FN_SUMM     5
#define FN_DT_COUNT 6
#define FN_DT_AVG   7
#define FN_DT_SUMM 10

#define BitOR(x,y)  ((x)|(y))
#define BitAND(x,y) ((x)&(y))
#define BitNOT(x)   (~(x))

#define SZ_SU   sizeof(StackUnit)
#define SZ_DU   sizeof(data_unit_t)

#define MK_UN_OPER(op, arg)         \
{                                   \
  DtPush; DtCurEl = (arg);          \
  error = oper (op, 1);             \
  DtPop;                            \
  if (error < 0)                    \
    return error;                   \
}

#define MK_BIN_OPER(op, arg1, arg2) \
{                                   \
  DtPush; DtCurEl = (arg1);         \
  DtPush; DtCurEl = (arg2);         \
  error = oper (op, 2);             \
  DtPop;                            \
  if (error < 0)                    \
    return error;                   \
}

#define COMP_RESULT LngNext
#define OPER_RESULT DtNextEl

/* curdate - put current date to string *
 * (8 simbols & \0) as "03:10:72"   :   *
 * day, month, year                     */


#define curdate(to)  { time_t _tt=time(NULL);       \
                       CFTIME(to, 9,"%d.%m.%y", &_tt); }

/* curtime - put current time to string *
 * (8 simbols & \0) as "07:45:00"   :   *
 * hours, minutes, seconds              */

#define curtime(to)  { time_t _tt=time(NULL);  \
                       CFTIME(to, 9, "%H:%M:%S", &_tt); }

struct S_IndInfo
{
  struct S_IndInfo *Next;
  Indid             Ind;
  MASK              Mask;
  i4_t               Cnt;     /* index columns number             */
  i2_t            *Clm;     /* array of columns numbers (in DB) */
};

typedef struct {
  MASK    Depend;    /* mask: what tables are considered in SP                */
  i4_t     SP_Num;    /* number of interpretations as simple predicate         */
  TXTREF  SP_Trees;  /* pointer to first interpretation                       *
		      * all interpretations are linked through RIGHT pointer  */
  MASK    SQ_deps;   /* dependency of subquery from predicate (=0 if SQ doesn't exist) */
  float   SQ_cost;   /* estimation of the cost for subquery (=0.0 if SQ doesn't exist) */
  char    flag;      /* flag = 1 if one of interpretations was used in OPSCAN */
} Simp_Pred_Info;

VADR codegen __P((void));

struct Id{
  i2_t Num,Off;
};

/* structures for work with constraints realisation : */

struct S_Constr_Info {
  i2_t constr_type; /* FOREIGN, REFERENCE, CHECK */
  char *mod_cols;    /* using flags for columns of the table in this constraint */
  VADR  constr;      /* pointer to structure S_Constraint for this constraint   */
  i4_t   conid;       /* identifier of CHECK constraint                          */
  i4_t   consize;     /* size in bytes of CHECK constraint                       */
  struct S_Constr_Info *next;
};

typedef struct S_Constr_Info Constr_Info;
  
typedef struct {
  i2_t constr_type;  /* FOREIGN, REFERENCE, CHECK    */
  VADR  object;       /* Tabid, Indid, tree           */
  i4_t   colnum;       /* columns number in constraint */
  VADR  col_arg_list; /* i4_t-array of columns numbers */
  VADR  col_ref_list; /* i4_t-array of columns numbers */
} S_Constraint;

typedef struct {
  VADR scan;        /* changing table's scan                */
  i4_t  constr_num;  /* number of elements in the next array */
  VADR constr_arr;  /* pointer to array of pointers         *
		     * (VADR) to structures S_Constraint    */
  i4_t  untabid;     /* untabid of changing table            */
  i4_t  tab_col_num; /* column number in the table           */
  i4_t  old_vl_cnt;  /* length of the next 2 arrays          */
  VADR old_vl_types;/* pointer to array of types (TYPE)     *
		     * of reading columns                   */
  VADR old_vl_nums; /* pointer to array of columns          *
		     * numbers to be read from scan         */
} S_ConstrHeader;

/* structures with statistic information about table & its columns : */

typedef struct {
  data_unit_t min, max; /* min & max values in column */
} col_stat_info;

typedef struct {
  data_unit_t ins_min, ins_max; /* min & max inserted values in   column */
  data_unit_t del_min, del_max; /* min & max deleted  values from column */
} col_change_info;

struct S_tab_stat_info {
  i4_t untabid;
  i2_t nrows_change, ncols;
  struct S_tab_stat_info *next;
  col_change_info *col_info;
};

typedef struct S_tab_stat_info tab_stat_info;

#ifdef __PRCD__

#include "typepf.h" 

#endif

#endif
