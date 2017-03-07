/*
 *  gsqltrn.x  -  GNU SQL transaction client/server protocol interface
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Dyshlevoi Konstantin
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

/* $Id: gsqltrn.x,v 1.248 1998/09/29 21:26:19 kimelman Exp $ */

#ifdef RPC_SVC
%#define main  run_server
%#define RPC_SVC_FG
#endif

typedef string    string_t<>;
typedef string_t  init_params_t<>;

struct stmt_info_t
{ 
  string_t            stmt;
  string_t            stmt_name;
  int                 bline;
  struct stmt_info_t *next;
};

struct prep_elem_t { /*--------------------------------------------------*/
  int      type    ; /* parameter type (codes defined  in sql_type.def)  */
  int      length  ; /*                                                  */
  int      scale   ; /* need for cobol type of parameters like dec(7,3)  */
  string_t name    ; /* parameter name                                   */
  string_t ind_name; /* name of parameter indicator                      */
  int      nullable; /*  1 - if parameter can be undefined               */
                     /* -1 - if returned parameter (in  interface list)  */
};/*---------------------------------------------------------------------*/

typedef struct prep_elem_t descr_t<>;

struct compiled_object_t { /*--------------------------------------------*/
  struct compiled_object_t    /*                                         */
             *next;           /*                                         */
  descr_t     descr_in;       /* input parameters description            */
  descr_t     descr_out;      /* output parameters descriptions          */
  int         object;         /* id(number) of the object in module      */
  string_t    cursor_name;    /* used for UPDATE or DELETE positioned    */
  string_t    table_name;     /* this & next fields identify the table   */
  string_t    table_owner;    /* if prepared statement - updatable query */
};/*---------------------------------------------------------------------*/

struct call_subst_t  {    /*---------------------------------------------*/
  string_t    proc_name;      /* procedure name (for module compilation) */
  descr_t     interface;      /* procedure parameters list               */
  descr_t     in_sql_parm;    /* actual parameters list (input)          */
  descr_t     out_sql_parm;   /* actual parameters list (output)         */
  int         object;         /* called object id in module              */
  int         method;         /* id of invoked object method             */
  string_t    jmp_on_error;   /* label to jump on error in embedded sql  */
  string_t    jmp_on_eofscan; /* label to jump at the end of scan        */
};/*---------------------------------------------------------------------*/

struct call_t  {   /*----------------------------------------------------*/
  struct call_t       *next;  /* next call replacement                   */
  struct call_subst_t *subst; /* substitution infos - if it need         */
};/*---------------------------------------------------------------------*/

struct file_buf_t  /*----------- transfer file buffers:  ----------------*/
{                             /*        tree dumps,error stream, etc     */
  string_t           ext;     /*   extension of file.                    */
  string_t           text;    /*   buffer                                */
  struct file_buf_t *next;    /*   next file buffer (if exist)           */
};/*---------------------------------------------------------------------*/

const COMP_STATIC         = 1;
const COMP_DYNAMIC_CURSOR = 2;
const COMP_DYNAMIC_SIMPLE = 3;

union comp_data_t switch(int comp_type)
{
case  COMP_STATIC         : string_t module;
case  COMP_DYNAMIC_CURSOR : int     seg_ptr;
case  COMP_DYNAMIC_SIMPLE : int     segm;
default : void; /* comp_type always inited to 0 */
};

struct compiled_t { /*---------------------------------------------------*/
  int                errors;  /* errors encountered in compiled module   */
  file_buf_t        *bufs;    /* buffers of tree dumps,error stream, etc */
  compiled_object_t *objects; /* compiled object infos                   */
  call_t            *calls;   /* SQL substitution data                   */
  comp_data_t        stored;  /* reference to resulted module            */ 
};/*---------------------------------------------------------------------*/

struct link_cursor_t /*-- dynamically create cursor on query ------------*/
{                      /*                                  given by name */
  string_t cursor_name;/* created cursor name                            */
  string_t stmt_name;  /* name of compiled statement                     */
  int     segment;    /* pointer to packed raw segment image            */
};/*---------------------------------------------------------------------*/

struct seg_del_t { /*---- delete segment instruction --------------------*/
  int     segment;    /* pointer to packed raw segment image            */
  int     seg_vadr<>; /* list of VADR's of copies of the segment linked */
                       /*  to system. This can happen if module is       */
                       /*  cursor query declaration and several cursors  */
                       /*  was opened on the query                       */
};/*---------------------------------------------------------------------*/

#ifdef RPC_XDR
%#include "sql_type.h"
#endif

union data switch(int type)
{
case SQLType_Char    : opaque   Str<>;
case SQLType_Short   : short    Shrt ;
case SQLType_Int     : int      Int  ;
case SQLType_Long    : int      Lng  ;
case SQLType_Real    : float    Flt  ;
case SQLType_Double  : double   Dbl  ;
default              : void;
};

struct parm_t
{
  data value;
  int  indicator;
};

typedef parm_t   parm_row_t<>;

const RET_COMP = 1;
const RET_ROW  = 2;
const RET_TBL  = 3;
const RET_SEG  = 4;
const RET_VOID = 0;

union return_data switch(int rett)
{
case  RET_COMP: compiled_t comp_ret;
case  RET_ROW : parm_row_t row;
case  RET_TBL : parm_row_t tbl<>;
case  RET_SEG : int       segid;
default : void;
};

struct result_t
{
  int          sqlcode;
  return_data  info;
};

struct insn_t { /*-------------------------------------------*/
  int           vadr_segm;   /* compiled module identifier  */ 
  int            sectnum;     /* object in module identifier */ 
  int            command;     /* method of object to call    */
  int            options;     /* repetitions & so on         */
  parm_row_t     parms;       /* parms list                  */ 
  struct insn_t *next;        /* next instruction            */ 
};/*---------------------------------------------------------*/
    
program GSQL_TRN {
  version BETA0 {
    result_t    INIT_COMP(init_params_t)       = 1;
    result_t    COMPILE(stmt_info_t)           = 2;
    
    result_t    DEL_SEGMENT(seg_del_t)         = 3;
    result_t    LINK_CURSOR(link_cursor_t)     = 4;
    
    result_t    LOAD_MODULE(string_t)          = 5;
    result_t    EXECUTE_STMT(insn_t)           = 6;
    
    result_t    DB_CREATE(void)                = 7;
    
    result_t    RETRY(void)                    = 8;  /* repeat last answer */
    int         IS_RPC_READY(void)             = 9;  /* just return 1      */
  } = 1 ;
} = 0x40000010;

#ifdef RPC_HDR
%extern result_t gsqltrn_rc;
#endif

#ifdef RPC_SVC
%#undef GSQL_TRN
%#define GSQL_TRN  rpc_program_id
%extern long  rpc_program_id;
#endif
