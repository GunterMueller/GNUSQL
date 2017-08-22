/*
 *  trl.h  - interface of tree library of GNU SQL precompiler
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming, Russia
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
 *  Contact:  gss@ispras.ru
 *
 */

/* $Id: trl.h,v 1.245 1997/03/31 03:46:38 kml Exp $ */

#ifndef __TRL_H__
#define __TRL_H__

#ifndef CHECK_TRL
#  define CHECK_TRL
#endif

#include "setup_os.h"
#include "vmemory.h"
#include "liter.h"


typedef enum {          
  SQL_SUCCESS  = 0,                 
  SQL_ERROR    = 1,                 
  SQL_WARNING  =-1                  
} CODERT;                        /* general purpose return code */

/*******==================================================*******\
*******            Type descriptions                       *******
\*******==================================================*******/
#include "sql_type.h"
/*******==================================================*******\
*******            Position descriptions                   *******
\*******==================================================*******/

typedef i4_t POSITION;        /* line number in source SQL code */

/*******==================================================*******\
*******====================================================*******
*******               TREE  NODES                          *******
*******====================================================*******
\*******==================================================*******/

typedef union node *PNODE;

#define VCBREF  TXTREF
#define TXTREF  VADR
#define TNULL   (TXTREF)(VNULL)

PNODE Pnode __P((TXTREF n));

/*******==================================================*******\
*******             tree  node codes                       *******
\*******==================================================*******/
#define DEF_TOKEN(CODE,NAME,FORMAT,STRUCT,FLAGS)   CODE ,

enum token
{

#include "treecode.def"

  LAST_TOKEN};

#define NUM_TRN_CODE  ((i4_t)LAST_TOKEN+1)

struct tkn_info{
  char *name,
       *format,
        struct_cod,
       *flags;
  i4_t   length;
};

extern struct tkn_info token_info[NUM_TRN_CODE];
extern i4_t             Max_operands;

#define  FORMAT(CODE)   token_info[CODE].format
#define  STRUCT(CODE)   token_info[CODE].struct_cod
#define  NAME_l(CODE)   token_info[CODE].name
#define  NAME(CODE)     (NAME_l(CODE)?NAME_l(CODE):"NULL")
#define  FLAGS(CODE)    token_info[CODE].flags
#define  LENGTH(CODE)   token_info[CODE].length


/*******==================================================*******\
*******             tree  node flags                       *******
\*******==================================================*******/

typedef i4_t MASKTYPE;

#define DEF_FLAG_BIT(CODE,NAME,CLASS,BIT_NO)  CODE,
#define DEF_FLAG(CODE,NAME,CLASS)             CODE,
enum flags{
#include "treeflag.def"
  LAST_FLAG};

#define NUMBER_OF_FLAGS  ((i4_t)LAST_FLAG+1)


#define DEF_FLAG_BIT(CODE,NAME,CLASS,BIT_NO)  bt_##CODE = BIT_NO,
#define DEF_FLAG(CODE,NAME,CLASS)             bt_##CODE,

enum flags_bit{
#include "treeflag.def"
  LAST_AND_NONMEANABLE_BIT };


struct flag_info{
  char      *name,
             class_cod;
  MASKTYPE   value;
};

extern struct flag_info flg_info[NUMBER_OF_FLAGS];

#define NAME_FL(FCODE)        flg_info[FCODE].name
#define CLASS(FCODE)          flg_info[FCODE].class_cod
#define FLAG_VALUE(FCODE)     flg_info[FCODE].value

i4_t Compatible_Tok_Fl __P((enum token code,enum flags fcode));
/*
MASKTYPE SetF __P((MASKTYPE FLAG,enum flags FLAG_CODE));
MASKTYPE ClrF __P((MASKTYPE FLAG,enum flags FLAG_CODE));
MASKTYPE TstF __P((MASKTYPE FLAG,enum flags FLAG_CODE));
*/

#define SetF(FLAG,FCODE)  (FLAG SETVL FLAG_VALUE(FCODE))
#define ClrF(FLAG,FCODE)  (FLAG CLRVL FLAG_VALUE(FCODE))
#define TstF(FLAG,FCODE)  (FLAG TSTVL FLAG_VALUE(FCODE))


/*******==================================================*******\
*******             Common tree  nodes                     *******
\*******==================================================*******/


typedef union trunion_def{
     TXTREF            txtp;
     LTRLREF           ltrp;
     POSITION          pos;
     sql_type_t        type;
     i4_t              l;
     float             f;
     void             *ptr;
    }  tr_union;


typedef struct {
   enum token   code;               /* node type code             */
   MASKTYPE     mask;               /* node flags                 */
                                    /*                            */
   tr_union    operands[1];         /* array of node's fileds     */
 } tree_t, *trn;


trn  Ptree __P((TXTREF n));

#define SIZE_TRN(code)     (sizeof(tree_t)+(LENGTH(code)-1)*sizeof(tr_union))
#define ADD_TRN_SIZE(code) (sizeof(tr_union)*PTRN_ELEM(code))

/*******<< FICTIVE FUNCTIONS >>******************************************/
/*-----<< Actually, that's lvalue macros >>-------------------*/
enum token  CODe_TRN __P((trn node));
MASKTYPE    MASk_TRN __P((trn node));

tr_union    XOp_TRN __P(( trn node,i4_t n));  /* really lvalue reference */
TXTREF      XTXt_TRN __P((trn node,i4_t n));
VCBREF      XVCb_TRN __P((trn node,i4_t n));
LTRLREF     XLTr_TRN __P((trn node,i4_t n));
i4_t        XLNg_TRN __P((trn node,i4_t n));
float       XFLt_TRN __P((trn node,i4_t n));
/* Tabid       XREl_TRN(trn node,i4_t n); */

POSITION    LOCATIOn_TRN __P((trn node));
TXTREF      RIGHt_TRN    __P((trn node));
TXTREF      DOWn_TRN     __P((trn node));
i4_t        ARITy_TRN    __P((trn node));
i4_t         HAS_DOWn     __P((trn node));

enum token  CODE_TRN  __P((TXTREF node));
MASKTYPE    MASK_TRN  __P((TXTREF node));
tr_union    XOP_TRN   __P((TXTREF node,i4_t n));  

TXTREF      XTXT_TRN  __P((TXTREF node, i4_t n ));
VCBREF      XVCB_TRN  __P((TXTREF node, i4_t n ));
LTRLREF     XLTR_TRN  __P((TXTREF node, i4_t n ));
i4_t        XLNG_TRN  __P((TXTREF node, i4_t n ));
float       XFLT_TRN  __P((TXTREF node, i4_t n ));

POSITION  LOCATION_TRN __P((TXTREF node));
TXTREF    RIGHT_TRN    __P((TXTREF node));
TXTREF    DOWN_TRN     __P((TXTREF node));
i4_t      ARITY_TRN    __P((TXTREF node));
i4_t       HAS_DOWN     __P((TXTREF node));

/*******==================================================*******\
*******            Pattern Recognizer info                 *******
\*******==================================================*******/

#define LONGBITS         (8*sizeof(i4_t))  /*32*/
#define PTRN_ELEM(code) \
 ((i4_t)(LENGTH(code)/LONGBITS)+(LENGTH(code)%LONGBITS?1:0))

#define PTRN_OP(code,op_n)    (LENGTH(code)+op_n/LONGBITS)
#define PTRN_BIT(op_n)        (op_n%LONGBITS)

#define SET_BIT(lv,bit)   lv SETVL BITVL(bit)
#define CLR_BIT(lv,bit)   lv CLRVL BITVL(bit)
#define TST_BIT(lv,bit)   lv TSTVL BITVL(bit)

#define COPY_NODE(code, orig, copy)			\
{							\
  enum token local_code = (code);			\
  register TXTREF local_c, local_o = (orig);		\
  local_c = gen_node1 (local_code, MASK_TRN (local_o)); \
  bcopy ((void *) Ptree (local_o),			\
         (void *) Ptree (local_c),			\
         trn_size (local_code, MASK_TRN(local_o)));	\
  CODE_TRN(local_c) = local_code;			\
  (copy) = local_c;					\
}

/*==============================================================*\
*   TRL vector.  These appear inside TRN's when there is a need  *
*  for a variable number of somethings.                          *
\*==============================================================*/

typedef struct trvec_def{
  i4_t       num_elem;             /* number of elements      */
  tr_union  elem[1];
} *trvec;


trvec   Ptrvec __P((TXTREF n));

TXTREF   XVEc_TRN __P((trn node,i4_t n));
TXTREF   XVEC_TRN __P((TXTREF node,i4_t n));

i4_t      VLEn __P((trvec  vec));
i4_t      VLEN __P((TXTREF vec));
tr_union VOp __P((trvec   vec,i4_t n));  /* really lvalue reference */
tr_union VOP __P((TXTREF  vec,i4_t n));  /* really lvalue reference */

i4_t      XLEN_VEC __P((TXTREF  node,i4_t vecnumber));

/*   XOP_VEC(node,n,m)   node -- TXTREF pointer to trn           
 *                        n   -- number of param, what is vector    
 *                        m   -- number of vectors element          
 */
tr_union XOP_VEC __P((TXTREF node,i4_t n,i4_t m)); /* this is lvalue reference */

/*******==================================================*******\
*******             Special tree  nodes                    *******
\*******==================================================*******/

#include "treecode.def"       /* usefull tree structures declaration */

typedef union node {              /*===================================*/
   tree_t              t;         /* general purpose node       ( all )*/
   STATEMENT           stmt;      /* operators                  (tree )*/
   OPERATION           oper;      /* operation, subtree         (tree )*/
   DB_TABL             db_tbl;    /* table reference            (tree )*/
   DB_OBJ              db_obj;    /* other objects        (tree leaves)*/
   struct column       colmn;     /* table columns              (vocab)*/
   struct table        tbl;       /* table and views            (vocab)*/
   struct cursor_name  nm;        /* exported cursor names      (vocab)*/
   struct corr_name    crlnm;     /* scan (the table instance)    (vcb)*/
   struct constant     cnst;      /*                        (tree leaf)*/
   struct parameter    param;     /*                            (vocab)*/
} NODE;                           /*===================================*/ 

/*******==================================================*******\
*******====================================================*******
*******         Tree processing functions interface        *******
*******====================================================*******
\*******==================================================*******/

TXTREF   gen_node __P((enum token code));           /* generate node         */
TXTREF   gen_node1 __P((enum token code,MASKTYPE msk)); /* generate node     */
TXTREF   gen_vect __P((i4_t len));                   /* generate vector       */
TXTREF   realloc_vect __P((TXTREF n,i4_t newlen));   /* change size of vector */
void     free_node __P((TXTREF node));              /* free node             */
void     free_vect __P((TXTREF vec));               /* free vector           */
TXTREF   gen_const_node __P((SQLType code,char *info));
void     free_line __P((TXTREF right));
void     free_tree __P((TXTREF root));

void     toggle_mark1f __P((i4_t i));
void     debug_trn_d __P((trn x));                  /* for debugs dump       */
                                                    /* to stderr             */
void     debug_trn __P((TXTREF x));                 /* for debugs dump       */
                                                    /* to stderr             */
void     print_trl __P((TXTREF trn_first,           /* if trn_first=NULL     */
                          FILE *outf));             /* prints all program    */
TXTREF   read_trn __P((FILE *infile,i4_t *line,      /* read tree node from   */
		  MASKTYPE def_msk));               /* infile                */
void     load_trl __P((FILE *inf));                 /* read all tree from    */
                                                    /* infile                */
void     load_trlfile __P((char *flname));          /* read all tree from    */
                                                    /* flname                */

TXTREF   copy_trn __P((TXTREF orig));               /*                       */
i4_t      trn_equal_p __P((TXTREF x,TXTREF  y));     /*                       */
i4_t trn_size __P((enum token code, MASKTYPE msk));

CODERT   add_statement __P((TXTREF p));
TXTREF   get_statement __P((i4_t number));
CODERT   del_statement __P((i4_t number));

#include "vcblib.h"
#include "trl_macro.h"

enum kind_of_root{
  Tree_Root,
  Vcb_Root,
  Hash_Tbl,
  G_Author,
  Current_Tree_Segment
};
  
TXTREF *Root_ptr __P((enum kind_of_root sel));

#if defined(ROOT)  || defined(GL_AUTHOR)
  #error either ROOT or GL_AUTHOR has been already defined somewhere!!!!
#else
  #define ROOT          *(Root_ptr(Tree_Root))
  #define GL_AUTHOR     *(Root_ptr(G_Author))
#endif

/*******==================================================*******\
*******====================================================*******
*******               GLOBAL  OPERATIONS                   *******
*******====================================================*******
\*******==================================================*******/

/*
 *  these 3 functions used just for error reporting from macro and
 * routines which checks if the tree is correct.
 */
i4_t   trl_err __P((char *msg,char *file,i4_t line,trn ptr));
i4_t   trl_wrn __P((char *msg,char *file,i4_t line,trn ptr));
i4_t   fmt_eq __P((char *fmt,char *tst));

/*
 * create new tree segment, mark them as a current one and return identifier of it.
 */
TXTREF create_tree_segment __P((void));

/* do the same but load tree from buffer. */
TXTREF load_tree_segment __P((void *tree_buffer,i4_t len)); 

void   dispose_tree_segment __P((TXTREF ptr_inside));
void  *extract_tree_segment __P((TXTREF ptr_inside,i4_t *len));

/*
 * load tree segment from file "tree_file_name" or just create new one if file
 * name is NULL
 */
CODERT install  __P((char *tree_file_name));

/*
 * extract tree segment and store it in the file or just dispose segment if
 * file name is NULL 
 */
CODERT finish   __P((char *tree_file_name));

TXTREF get_current_tree_segment __P((void));
TXTREF set_current_tree_segment __P((TXTREF ptr_inside));

#endif
