%{
/* 
 *  parse.y   -   SQL grammar (ANSI SQL'89 + some extention) 
 *                of GNU SQL compiler
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Michael Kimelman.
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

/* $Id: parse.y,v 1.247 1998/09/28 05:00:06 kimelman Exp $ */

#include "trl.h"
#include "tree_gen.h"
#include "cycler.h"
#include "sql_decl.h"
#include "xmem.h"
#include "type_lib.h"
#include "svr_lib.h"
#include <assert.h>
#include "tassert.h"

char *dyn_sql_stmt_name  = NULL;
i4_t   dyn_sql_section_id = 0;

static TN      tblnm;
static i4_t     del_local_vcb    = 0;
static char   *label_nf         = NULL;
static char   *label_er         = NULL;
static i4_t     subquery         = 0;
static TXTREF  new_table        = TNULL;
static i4_t     new_table_col_no = 0;
static call_t  call;

static enum 
{
  Esql,
  Module,
  Schema,
  Dynamic
} parse_mode=Esql;

#ifdef stderr
#  undef stderr
#endif
#ifdef stdout
#  undef stdout
#endif

#define stderr STDERR
#define stdout STDOUT

#define FICTIVE_NODE  (TXTREF)(-1L)

#define FREE_VCB   free_line(LOCAL_VCB_ROOT)
#define YYERROR_VERBOSE
#define YYERROK { new_table = TNULL; yyerrok; }
i4_t yylex (void);

static void   check_not_null __P((TXTREF ind));
static void   free_tail __P((void));
static TXTREF replace_column_holes __P((TXTREF rb,VCBREF tbl));
static void   add_table_column(enum token tbl_code, LTRLREF ltrp,
                               sql_type_t type, TXTREF def,TXTREF constr);
static void   emit_module_proc __P((VCBREF parmlist,LTRLREF procname));
static void   emit_call __P((TXTREF vcb,i4_t object_id, i4_t method_id));

#define add_base_column(ltrp,type,def,constr) \
     add_table_column(TABLE,ltrp,type,def,constr)
#define add_view_column(ltrp,type) \
     add_table_column(VIEW,ltrp,type,TNULL,TNULL)

%}

%expect 1

%union {
  LTRLREF  ltrp;
  TXTREF   node;
  i4_t      code;
  sql_type_t     type;
  void     *ptr;
}

/*
  token declarations
*/
%token TOK_ALL
%token TOK_ALTER
%token TOK_ANY
%token TOK_AS
%token TOK_ASC
%token TOK_AUTHORIZATION
%token TOK_BEGIN  
%token TOK_BETWEEN
%token TOK_BY
%token TOK_CHECK
%token TOK_CLOSE
%token TOK_COBOL_LANG
%token TOK_C_LANG
%token TOK_COMMIT
%token TOK_CONTINUE
%token TOK_CREATE
%token TOK_CURRENT
%token TOK_CURSOR
%token TOK_DECLARE
%token TOK_DEFAULT       
%token TOK_DELETE
%token TOK_DESC
%token TOK_DISTINCT
%token TOK_DROP
%token TOK_END          
%token TOK_ESCAPE
%token TOK_EXISTS
%token TOK_FETCH
%token TOK_FOR
%token TOK_FOREIGN       
%token TOK_FORTRAN_LANG
%token TOK_FOUND      
%token TOK_FROM
%token TOK_GOTO 
%token TOK_GRANT
%token TOK_GROUP
%token TOK_HAVING
%token TOK_IN
%token TOK_INDICATOR
%token TOK_INSERT
%token TOK_INTO
%token TOK_IS
%token TOK_KEY     
%token TOK_LANGUAGE
%token TOK_LIKE
%token TOK_MODULE
%token TOK_NULL
%token TOK_OF
%token TOK_ON
%token TOK_OPEN
%token TOK_OPTION
%token TOK_ORDER
%token TOK_PASCAL_LANG
%token TOK_PL1_LANG
%token TOK_PRIMARY       
%token TOK_INDEX         
%token TOK_PRIVILEGES
%token TOK_PROCEDURE
%token TOK_PUBLIC
%token TOK_REFERENCES    
%token TOK_REVOKE
%token TOK_ROLLBACK
%token TOK_SCHEMA
%token TOK_SECTION       
%token TOK_SELECT
%token TOK_SET
%token TOK_SOME
%token TOK_SQLCODE
%token TOK_SQLERROR      
%token TOK_TABLE
%token TOK_TO
%token TOK_UNION
%token TOK_UNIQUE
%token TOK_UPDATE
%token TOK_USER
%token TOK_VALUES
%token TOK_VIEW
%token TOK_WHENEVER      
%token TOK_WHERE
%token TOK_WITH
%token TOK_WORK

%token TOK_AND
%token TOK_OR
%token TOK_NOT

%token TOK_NE
%token TOK_GE
%token TOK_LE

%token TOK_AVG
%token TOK_MAX
%token TOK_MIN
%token TOK_SUM
%token TOK_COUNT

%token <ltrp> TOK_IDENTIFIER
%token <ltrp> TOK_PARAM
%token <node> TOK_INTEGER_CONST
%token <node> TOK_NUM_CONST
%token <node> TOK_REAL_CONST
%token <node> TOK_STRING_CONST

%token TOK_T_CHAR
%token TOK_T_NUM
%token TOK_T_INT
%token TOK_T_SINT
%token TOK_T_FLT
%token TOK_T_DOUBLE
%token TOK_T_PRECISION
%token TOK_T_REAL

%token UNARY BRACETS 

%left ','


%left TOK_ALL
%left TOK_UNION

%left TOK_OR
%left TOK_AND
%left TOK_NOT

%left '+' '-'
%left '*' '/'

%left UNARY
%nonassoc BRACETS

%left '.'

%start sql_compiled

%%

sql_compiled
  : schema_title
    { 
      if (progname && *progname)
        yyerror ("Unexpected schema operation in static mode compilation ");
      else if (parse_mode==Module)
        yyerror ("Unexpected schema operation inside module");
      if($<code>1)
        parse_mode=Schema;
    }
  | schema_element /* MUST be in dynamic compilation mode */
    { 
      if (progname && *progname)
        yyerror ("Unexpected schema operation in static mode compilation ");
      else if (parse_mode==Module)
        yyerror ("Unexpected schema operation inside module");
    }
  | module_header
    { if ($<code>1) parse_mode=Module; }
  | cursor
    { ; }
  | procedure
    { 
      if(parse_mode!=Module)
        yyerror("Unexpected module operation outside module");
    }
  | embedded_spec
    { ; }
  | statement
    {
      if(parse_mode==Esql)
        {
          free_tail();
          $<code>$=0;
        }
    }
  | dynamic {}
  | error 
    {
      yyerror("Error: unrecognizable SQL construction in input file");
      free_tail();
      YYERROK; yyclearin;
      YYACCEPT;
    }
  ;

/*
 *  dynamic additions
 */

tbl_or_view
  : TOK_TABLE { $<code>$ = TABLE; }
  | TOK_VIEW  { $<code>$ = VIEW;  }
  ;

dynamic 
  : queryexpr
    {
      TXTREF c,d = gen_parent(DECL_CURS,$<node>1);
      STMT_VCB(d) = LOCAL_VCB_ROOT;
      SetF_TRN(d,HAS_VCB_F);
      LOCAL_VCB_ROOT = TNULL;
      c = gen_parent(CUR_AREA,d);
      add_statement(c);
      d = gen_node(CURSOR);
      if (dyn_sql_stmt_name)
        CUR_NAME(d) = ltr_rec(dyn_sql_stmt_name);
      else
        yyfatal("Attemp to compile unnamed statement");
      CUR_DECL(d) = c;
      STMT_VCB(c) = d;
      $<code>$ = 0;
    }
  | TOK_DROP tbl_or_view table_desc
     {
       TXTREF d = gen_object(DROP,@1);
       CREATE_OBJ(d) = $<node>3;
       CODE_TRN($<node>3) = $<code>2;
       add_statement(d);
     }
  ; 

/*
  schema definition
*/

schema_title
  : schema_action TOK_SCHEMA autoriz_clause
   { {
     add_statement(gen_object($<code>1,@2));
     $<code>$ = 0;
   } }
  ;


schema_element
  : basetable
  | view
  | privilege
  ;

schema_action
  : TOK_CREATE
    { $<code>$=CREATE; }
  | TOK_ALTER
    { $<code>$=ALTER; }
  ;
  
basetable
  : schema_action basetable_def
     {
       register TXTREF e=gen_object($<code>1,@1);
       CREATE_OBJ(e)=$<node>2;
       add_statement(e);
       $<code>$=STMT_UID(e);
     }
  ;

basetable_def
  :TOK_TABLE table_def '(' basetableelement_list ')'
    {
      register TXTREF t=$<node>2,e=$<node>4;
      if (!t)
        free_line (e);
      else if (new_table != t)
        yyfatal ("Internal error: unexpected 'new_table'");
      else
        {
          CODE_TRN (t) = TABLE;
	  TBL_NCOLS(t) = new_table_col_no;
          SetF_TRN (t, CHECKED_F);
          new_table = TNULL;
        }
      $<node>$=t;
    }
  ;


basetableelement_list
  : basetableelement
    { $<node>$=$<node>1;}
  | basetableelement_list ',' basetableelement
    { $<node>$=join_list($<node>1,$<node>3);}
  ;

basetableelement
  : column_descr   { $<node>$=$<node>1; }
  | table_constr   { $<node>$=$<node>1; }
  ;

column_descr                                     /*==*/
  : TOK_IDENTIFIER type default_clause column_constrts
    {
      if(new_table)
        add_base_column($<ltrp>1,$<type>2,$<node>3,$<node>4);
      $<node>$ = TNULL;
    }
  ;

default_clause
  :  { $<node>$ = TNULL;     }
  | TOK_DEFAULT def_clause  
     { $<node>$ = $<node>2; }
  ;

def_clause
  : TOK_USER
    {
      $<node>$ = gen_object (USERNAME, @1);
      USR_NAME ($<node>$) = $<ltrp>1;
    }
  | TOK_NULL
    { $<node>$ = gen_object (NULL_VL, @1); }
  | constant
    { $<node>$ = $<node>1; }
  ;

column_constrts
  :
    { $<node>$ = TNULL;        }
  | constr_list 
    { $<node>$ = $<node>1; }
  ;

constr_list
  : column_constr
    { $<node>$ = $<node>1; }
  | constr_list column_constr
    { $<node>$ = join_list($<node>1,$<node>2); }
  ;

column_constr
  : TOK_NOT TOK_NULL
    { $<node>$ = gen_object (NULL_VL, @2); }
  | TOK_NOT TOK_NULL unique_spec
    { $<node>$ = gen_object ($<code>3, @3); } 
  | refer_spec
    { $<node>$ = $<node>1; }
  | TOK_CHECK cond
    {
      $<node>$ = gen_up_node (CHECK, $<node>2, @1);
    }
  ;

table_constr
  : table_constr1
    {{
      if(!new_table)
        yyerror("Unexpected constraints");
      else
	switch(CODE_TRN($<node>1))
	  {
          case NULL_VL:
            yyfatal("Unexpected NULL_VL node");
            break;
	  case PRIMARY:
	  case UNIQUE:
            check_not_null($<node>1);
	  case INDEX:
	    IND_INFO(new_table)=
	      join_list(IND_INFO(new_table),$<node>1);
	    break;
	  default:
            RIGHT_TRN($<node>1) = TNULL;
	    TBL_CONSTR(new_table)=
	      join_list(TBL_CONSTR(new_table),$<node>1);
	  }
      $<node>$ = TNULL;
    }}
  ;

table_constr1
  : unique_spec  '(' column_name_list ')'
    {
      $<node>$=gen_up_node($<code>1,
                           replace_column_holes($<node>3,new_table),
                           @1);
    }
  | TOK_FOREIGN TOK_KEY '(' column_name_list ')' refer_spec
    {
      $<node>$=
        gen_up_node(FOREIGN,
                    join_list(gen_parent(LOCALLIST,
                                         replace_column_holes($<node>4,
                                                              new_table)),
                              $<node>6),
                    @1);
    }
  | TOK_CHECK cond
    {
      $<node>$=gen_up_node(CHECK,
                           replace_column_holes($<node>2,
                                                new_table),
                           @1);
    }
  ;

unique_spec
  : TOK_UNIQUE           { $<code>$=UNIQUE; }
  | TOK_PRIMARY TOK_KEY  { $<code>$=PRIMARY; }
  | TOK_INDEX            { $<code>$=INDEX; }
  ;

refer_spec 
  : TOK_REFERENCES table_desc opt_column_name_list
    { {
      TXTREF t = gen_object(OPTR,@1);
      OBJ_DESC(t)  = $<node>2;
      $<node>$ = gen_up_node(REFERENCE,
                             join_list(t,gen_parent(NOOP,$<node>3)),
                             @1);
    } }
  ;

opt_column_name_list
  : { $<node>$=0; }
  |'(' column_name_list ')'
    { $<node>$=$<node>2;}
  ;

column_name_list
  : TOK_IDENTIFIER
    {
      $<node>$=gen_column(NULL,$<ltrp>1,@1);
    }
  | column_name_list ',' TOK_IDENTIFIER
    {
      $<node>$=join_list($<node>1,gen_column(NULL,$<ltrp>3,@3));
    }
  ;

view
  : TOK_CREATE view_definition
    {
      register TXTREF e;
      e=gen_object(CREATE,@1);
      CREATE_OBJ(e)=$<node>2;
      add_statement(e);
      $<code>$=STMT_UID(e);
    }
  ;

view_definition
  : TOK_VIEW table_def opt_column_name_list
    TOK_AS queryspec opt_check_option
    {
      register TXTREF t = $<node>2, e= $<node>3;
      if(!t)
        free_line (e);
      else
        {
          TXTREF     c;
          sql_type_t type  = pack_type(SQLType_0,0,0);
          i4_t        e_cnt = count_list (e);
          
          TXTREF sel_node = RIGHT_TRN (DOWN_TRN ($<node>5));
          
          CODE_TRN (t) = VIEW;

          if (e)
            if (e_cnt != ARITY_TRN (sel_node))
              {
                yyerror("Incorrect number of columns in VIEW defenition");
                free_line(e);
                e = TNULL;
              }
          if (e)
            {
              for ( c = e ; c ; c = RIGHT_TRN(c))
                add_view_column(CHOLE_CNAME(OBJ_DESC(c)),type);
              free_line (e);
            }
          else
            {
              for ( c = DOWN_TRN(sel_node) ; c ; c = RIGHT_TRN(c))
                {
                  if (CODE_TRN(c) == COLPTR)
                    add_view_column(CHOLE_CNAME(OBJ_DESC(c)),type);
                  else
                    yyerror("Expressions aren't allowed in selection list"
                            " of anonimous view");
                }
            }
          /* TASSERT(new_table_col_no == ARITY_TRN(sel_node),sel_node); */
	  TBL_NCOLS(t) = ARITY_TRN(sel_node);
            
          VIEW_QUERY (t) = $<node>5;
          if($<code>6) SetF_TRN (t, CHECK_OPT_F);
          SetF_TRN (t, CHECKED_F);
          VIEW_VCB (t) = LOCAL_VCB_ROOT;
          LOCAL_VCB_ROOT = TNULL;
          new_table = TNULL;
        }
      $<node>$=t;
    }
  ;

opt_check_option
  :                                { $<code>$=0; }
  |TOK_WITH TOK_CHECK TOK_OPTION   { $<code>$=1; }
  ;

privilege_action
  : TOK_GRANT
    { $<code>$=GRANT; }
  | TOK_REVOKE
    { $<code>$=REVOKE; }
  ;

privilege
  : privilege_action privileges TOK_ON table_desc TOK_TO grantees
    opt_grant
    {
      register TXTREF p,g;
      register i4_t    err = errors;
      
      p=replace_column_holes($<node>2,$<node>4);
      p = gen_parent(PRIVILEGIES, p);

      if (errors != err) /* if there was some error in the tree */
        {
          g =  gen_parent(GRANTEES, $<node>6);
          debug_trn(p);
          free_tree(p);
          debug_trn(g);
          free_tree(g);
        }
      else
        {
          g=gen_node(TBLPTR);
          TABL_DESC(g) = $<node>4;
          g=gen_up_node($<code>1,
                        join_list(g,       
                                  join_list(p,
                                            gen_parent(GRANTEES,
                                                       $<node>6))),
                        @1);
          if($<code>7)
            SetF_TRN(g,GRANT_OPT_F);
          add_statement(g);
          $<code>$=STMT_UID(g);
        }
    }
  ;

opt_grant
  :                               { $<code>$=0; }
  | TOK_WITH TOK_GRANT TOK_OPTION { $<code>$=1; }
  ;

privileges
  : TOK_ALL TOK_PRIVILEGES    { $<node>$=0; }
  | operation_list            { $<node>$=$<node>1; }
  ;

operation_list
  : operation                     { $<node>$=$<node>1;}
  | operation_list ',' operation  { $<node>$=join_list($<node>1,$<node>3);}
  ;

operation
  : TOK_SELECT    
    { $<node>$ = gen_object(SELECT,@1); }   
  | TOK_INSERT
    { $<node>$ = gen_object(INSERT,@1); }   
  | TOK_DELETE
    { $<node>$ = gen_object(DELETE,@1); }   
  | TOK_UPDATE     opt_column_name_list
    { $<node>$ = gen_up_node(UPDATE,$<node>2,@1); }   
  | TOK_REFERENCES opt_column_name_list
    { $<node>$ = gen_up_node(REFERENCE,$<node>2,@1); }
  ;

grantees
  : grantee_list              { $<node>$ = $<node>1;                  }
  | TOK_PUBLIC                
    {
      register TXTREF u=gen_object(USERNAME,@1);
      USR_NAME(u)=ltr_rec("PUBLIC");
      $<node>$=u;
    }
  ;

grantee_list
  : grantee                   { $<node>$=$<node>1;}
  | grantee_list ',' grantee  { $<node>$=join_list($<node>1,$<node>3);}
  ;

grantee
  : TOK_IDENTIFIER  /* user */
    {
      register TXTREF u=gen_object(USERNAME,@1);
      USR_NAME(u)=$<ltrp>1;
      $<node>$=u;
    }
  ;

/*=======================================================================\
|                       embedded statements                              |
\=======================================================================*/

embedded_spec
  : declare_section  { ; }
  | exception        { ; }
  ;

declare_section
  : TOK_BEGIN TOK_DECLARE TOK_SECTION  { $<code>$=1; }
  | TOK_END TOK_DECLARE TOK_SECTION    { $<code>$=0; }
  ;

exception
  : TOK_WHENEVER exc_cond exc_do
   {
     *( $<code>2 ? &label_nf : &label_er ) =
       ( $<ltrp>3 ? (STRING($<ltrp>3)) : NULL );
   }
  ;

exc_cond
  : TOK_NOT TOK_FOUND                  { $<code>$=1; }
  | TOK_SQLERROR                       { $<code>$=0; }
  ;

exc_do
  : TOK_CONTINUE                       { $<ltrp>$=0; }
  | TOK_GOTO TOK_IDENTIFIER            { $<ltrp>$=$<ltrp>2; }
  ;


/*=======================================================================\
|                       modules                                          |
\=======================================================================*/

module_header
  : module_titl lang_clause autoriz_clause
    { $<code>$=1; }
  | module_titl error
    {
      YYERROK;
      yyerror("Error in module title");
      $<code>$=0;
    }
    ;

module_titl
  : TOK_MODULE TOK_IDENTIFIER  
    { $<code>$=0; }
  | TOK_MODULE
    { $<code>$=0; }
  ;

lang_clause
  : TOK_LANGUAGE TOK_IDENTIFIER
    {
      if (strncmp("C",STRING($<ltrp>2),1)==0)
        $<code>$=0;
      else
        {
          lperror("SQL module for %s program can't be processed yet",
                  STRING($<ltrp>2));
          YYABORT;
        }
    }
  ;

autoriz_clause
  : TOK_AUTHORIZATION TOK_IDENTIFIER
   { {
     extern char *current_user_login_name;
     char *s, *s1;

     s1 = current_user_login_name;
     current_user_login_name = NULL;
     s = get_user_name ();
     current_user_login_name = s1;
     

     if(GL_AUTHOR && (GL_AUTHOR!=$<ltrp>2) && s &&
        (strcmp(STRING(GL_AUTHOR),s)))
       yyerror("Incorrect user identifier");
     else
       GL_AUTHOR=$<ltrp>2;
   } }
  ;

cursor
  : TOK_DECLARE TOK_IDENTIFIER TOK_CURSOR TOK_FOR /* cursor - IDENTIFIER */
    queryexpr
    orderby
    {
      register TXTREF c,d;
      d=gen_up_node(DECL_CURS,join_list($<node>5,$<node>6),@1);
      STMT_VCB(d)=LOCAL_VCB_ROOT;
      SetF_TRN(d,HAS_VCB_F);
      LOCAL_VCB_ROOT = TNULL;
      d=gen_up_node(CUR_AREA,d,@1);
      add_statement(d);

      c=gen_node(CURSOR);
      CUR_DECL(c)=d;
      CUR_NAME(c)=$<ltrp>2;
      STMT_VCB(d)=c;

      if(find_info(CURSOR,CUR_NAME(c)))
        {
          char cn[512];
          lperror("Static cursor name '%s' redeclaration",STRING(CUR_NAME(c)));
          sprintf(cn,"%s_1",STRING(CUR_NAME(c)));
          CUR_NAME(c) = ltr_rec(cn);
        }
      add_info(c);
      $<code>$=0;
    }
    ;

procedure
  : TOK_PROCEDURE TOK_IDENTIFIER parameter_list ';' statement ';'
    {
      emit_module_proc($<node>3,$<ltrp>2);
      free_line($<node>3);
      $<code>$=0;
    }
  | TOK_PROCEDURE TOK_IDENTIFIER ';' statement ';'
    {
      emit_module_proc(LOCAL_VCB_ROOT,$<ltrp>2);
      $<code>$=0;
    }
  | TOK_PROCEDURE TOK_IDENTIFIER ';' error ';'
    {
      YYERROK;
      yyerror("Error in procedure's body");
    }
  | TOK_PROCEDURE TOK_IDENTIFIER parameter_list ';' error ';'
    {
      YYERROK;
      yyerror("Error in procedure's body");
    }
  | TOK_PROCEDURE error ';'
    {
      YYERROK;
      yyerror("Error in procedure's title");
    }
  | error ';'
    {
      YYERROK;
      yyerror("Error: unexpected \';\' found before procedure title");
    }
  ;

orderby
  : { $<node>$ = TNULL;}
  | TOK_ORDER TOK_BY orderspec_list
    { $<node>$ = gen_up_node (ORDER, $<node>3, @1); }
  ;

orderspec_list
  : orderspec                     { $<node>$=$<node>1;}
  | orderspec_list ',' orderspec  { $<node>$=join_list($<node>1,$<node>3);}
  ;

orderspec
  : sort_obj sort_order
    {
      if($<code>2)SetF_TRN($<node>1,DESC_F);
      $<node>$=$<node>1;
    }
  ;

sort_obj
  : TOK_INTEGER_CONST
    { register TXTREF n=gen_object(SORT_POS,@1);
      SORT_IND(n)= atol(STRING(CNST_NAME($<node>1))) - 1;
      free_node($<node>1);
      $<node>$=n;
    }
  | column
    {
      register TXTREF s=gen_node(SORT_COL);
      SORT_CLM(s)=$<node>1;
      $<node>$=s;
    }
  ;

sort_order
  :          { $<code>$=0; }
  | TOK_ASC  { $<code>$=0; }
  | TOK_DESC { $<code>$=1; }
  ;


parameter_list
  : parameter                    { $<node>$=$<node>1;              }
  | parameter_list ',' parameter { $<node>$=join_list($<node>1,$<node>3);}
  ;

parameter
  : TOK_IDENTIFIER type
   {
     register TXTREF n;
     n=get_param($<ltrp>1,0,@1);
     if(!n)
       yyfatal("can`t add parameter");
     if(TstF_TRN(n,CHECKED_F))
       $<node>$ = TNULL;
     else
       {
         PAR_STYPE(n)=$<type>2;
         SetF_TRN(n,CHECKED_F);
         n=copy_trn(n);
         RIGHT_TRN(n)=0;
         ClrF_TRN(n,VCB_F);
         $<node>$=n;
       }
   }
  | TOK_SQLCODE
   {
     register TXTREF n;
     n=get_param(ltr_rec("SQLCODEP"),0,@1);
     if(!n)
       yyfatal("can`t add parameter");
     if(TstF_TRN(n,CHECKED_F))
       $<node>$ = TNULL;
     else
       {
         PAR_STYPE(n)=pack_type(SQLType_Int,0,0);
         SetF_TRN(n,CHECKED_F);
         SetF_TRN(n,OUT_F);
         n=copy_trn(n);
         RIGHT_TRN(n)=0;
         ClrF_TRN(n,VCB_F);
         $<node>$=n;
       }
   }
  ;


/*
  statements
*/

statement
  : statement_body
    {
      register TXTREF u=$<node>1;
      if(subquery)
        fprintf(stderr,
                "Internal warning parse.y: Subquery flag not processed\n");
      subquery=0;
      if (u != FICTIVE_NODE)
        {
          if (u)
            {
              STMT_VCB(u)=LOCAL_VCB_ROOT;
              add_statement(u);
              emit_call(LOCAL_VCB_ROOT,STMT_UID(u),0);
              del_local_vcb=0;
            }
          else
            {
              $<node>$ = TNULL;
              /* dummy operator */
            }
        }
      $<node>$=$<node>1;
    }
  ;

statement_body
  : close_stmt
  | commit_stmt
  | delete_stmt
  | fetch_stmt
  | insert_stmt
  | open_stmt
  | roll_stmt
  | select_stmt
  | update_stmt
  ;

close_stmt
  : TOK_CLOSE cursor_name
    {
      if(!$<node>2)
        $<node>$ = TNULL;
      else
        {
          emit_call (TNULL, STMT_UID(CUR_DECL($<node>2)), 2);
          $<node>$=FICTIVE_NODE;
          del_local_vcb=1;
        }
    }
  ;

commit_stmt
  : TOK_COMMIT TOK_WORK
    {
      dyn_sql_section_id = -2; /* !! */
      del_local_vcb=1;
      emit_call (TNULL,dyn_sql_section_id,2);
      $<node>$=FICTIVE_NODE;
    }
  ;

delete_stmt
  : TOK_DELETE TOK_FROM host_scan where_d
    {
      register TXTREF u=$<node>3;
      u=gen_up_node(DELETE,u,@1);
      add_child(u,$<node>4);
      $<node>$=u;
    }
  | TOK_DELETE                    where_cur
    {
      $<node>$ = TNULL;
      if($<node>2)
        {
          register TXTREF c,d;
          c = $<node>2;
          emit_call(LOCAL_VCB_ROOT,STMT_UID(CUR_DECL(c)),3);
          d = gen_up_node (DELETE, TNULL, @1);
          UPD_CURS(d) = c;
          STMT_VCB(d) = LOCAL_VCB_ROOT;
          LOCAL_VCB_ROOT = TNULL;
          add_statement(d);
          $<node>$ = FICTIVE_NODE;
        }
      else
        {
          free_tree($<node>2);
          FREE_VCB;
        }
    }
  | TOK_DELETE TOK_FROM host_scan where_cur
    {
      $<node>$ = TNULL;
      if($<node>4)
        {
          register TXTREF c,d;
          c = $<node>4;
          emit_call(LOCAL_VCB_ROOT,STMT_UID(CUR_DECL(c)),3);
          d = gen_up_node (DELETE, $<node>3, @1);
          UPD_CURS(d) = c;
          STMT_VCB(d) = LOCAL_VCB_ROOT;
          LOCAL_VCB_ROOT = TNULL;
          add_statement(d);
          $<node>$ = FICTIVE_NODE;
        }
      else
        {
          free_tree($<node>3);
          FREE_VCB;
        }
    }
  ;

fetch_stmt
  : TOK_FETCH cursor_name TOK_INTO target_list /* cursor */
    {
      $<node>$ = TNULL;
      if($<node>2)
        {
          register TXTREF u;
          $<node>$=FICTIVE_NODE;
          u=gen_object(FETCH,@1);
          STMT_VCB(u)=LOCAL_VCB_ROOT;
          SetF_TRN(u,HAS_VCB_F);
          STMT_UID(u) = ARITY_TRN(CUR_DECL($<node>2));
          add_child(CUR_DECL($<node>2),u);
          emit_call(LOCAL_VCB_ROOT,STMT_UID(CUR_DECL($<node>2)),1);
          LOCAL_VCB_ROOT = TNULL;
        }
      free_line($<node>4);
      if ($<node>$ == TNULL)
        {
          FREE_VCB;
        }
    }
   ;

insert_stmt
  : TOK_INSERT TOK_INTO host_scan opt_column_name_list insrt_tail
     {
       register TXTREF i;
       i=gen_object(INSERT,@1);
       add_child(i,$<node>3);
       if($<node>4)
         {
           add_child(i,gen_parent(INTO,$<node>4));
           SetF_TRN(i,COLUMNS_F);
         }
       add_child(i,$<node>5);
       if(CODE_TRN($<node>5)==QUERY)SetF_TRN(i,QUERY_F);
       $<node>$=i;
     }
  ;

insrt_tail
  : TOK_VALUES '(' insertvalue_list ')'
    { $<node>$=gen_up_node(IVALUES,$<node>3,@1); }
  | queryspec
    { $<node>$=$<node>1;}
  ;

insertvalue_list
  : ins_value                       { $<node>$=$<node>1;}
  | insertvalue_list ',' ins_value  { $<node>$=join_list($<node>1,$<node>3);}
  ;

ins_value
  : expr_or_null   { $<node>$=$<node>1;  }
  ;

open_stmt
  : TOK_OPEN cursor_name
    {
     if(!$<node>2)
       $<node>$ = TNULL;
     else
       {
         register TXTREF u=$<node>2,v;
         register TXTREF vcb=LOCAL_VCB_ROOT;
         u=STMT_VCB (DOWN_TRN (CUR_DECL(u)));
         LOCAL_VCB_ROOT=u;
         for(v=vcb;v;v=RIGHT_TRN(v))
           if(CODE_TRN(v)==PARAMETER)
             {
               u=find_info(PARAMETER,PAR_NAME(v));
               if(!u)
                 continue;
               if(PAR_STYPE(u).code==0)
                 PAR_STYPE(u)=PAR_STYPE(v);
               else if (PAR_STYPE(u).code!=PAR_STYPE(v).code)
                 yyerror("Error: Mismath operand types");
             }
         emit_call(LOCAL_VCB_ROOT,STMT_UID(CUR_DECL($<node>2)),0);
         LOCAL_VCB_ROOT=vcb;
         $<node>$=FICTIVE_NODE;
         del_local_vcb=1;
       }
   }
  ;

roll_stmt
  : TOK_ROLLBACK TOK_WORK
    {
     $<node>$=FICTIVE_NODE;
     dyn_sql_section_id = -1; /* !! */
     emit_call (TNULL,dyn_sql_section_id,0);
     del_local_vcb=1;
    }
  ;

select_stmt
  : TOK_SELECT select_spec selection TOK_INTO target_list tableexpr
    {
      register TXTREF S=gen_object(SELECT,@1),s=$<node>3,t;
      t=gen_up_node(INTO,$<node>5,@4);
      MASK_TRN(S)  = MASK_TRN($<node>6);
      DOWN_TRN(S)  = DOWN_TRN($<node>6);
      ARITY_TRN(S) = ARITY_TRN($<node>6);
      free_node($<node>6);
      if($<code>2)SetF_TRN(S,DISTINCT_F);
      RIGHT_TRN(t)=RIGHT_TRN(DOWN_TRN(S));      /* insert after from */
      RIGHT_TRN(s)=t;
      RIGHT_TRN(DOWN_TRN(S))=s;
      ARITY_TRN(S)+=2;
      $<node>$=S;
    }
  ;

select_spec
  : all              { $<code>$=0;}
  | TOK_DISTINCT     { $<code>$=1;}
  ;

update_stmt
  : TOK_UPDATE host_scan TOK_SET assignment_list where_d
    {
      register TXTREF u=
       join_list(join_list($<node>2,gen_parent(ASSLIST,$<node>4)),$<node>5);
      $<node>$=gen_up_node(UPDATE,u,@1);
    }
  | TOK_UPDATE host_scan TOK_SET assignment_list where_cur
    {
      register TXTREF u = TNULL;
      if ($<node>5)
        {
          u=gen_up_node(UPDATE,join_list($<node>2,
                                         gen_parent(ASSLIST,$<node>4)),@1);
          UPD_CURS(u)=$<node>5;
          SetF_TRN($<node>5,CURS_UPD_F);
        }
      $<node>$=u;
    }
  ;

assignment_list
  : assignment                      { $<node>$=$<node>1;}
  | assignment_list ',' assignment  { $<node>$=join_list($<node>1,$<node>3);}
  ;

assignment
  : TOK_IDENTIFIER '=' assign_tail /* column - IDENTIFIER */
    {
      $<node>$=gen_up_node(ASSIGN,
                  join_list(gen_column(NULL,$<ltrp>1,@1),$<node>3),
                           @2);
    }
  ;

assign_tail
  : expr_or_null   { $<node>$=$<node>1;  }
  ;

expr_or_null
  : expr       { $<node>$=$<node>1;  }
  | TOK_NULL   { $<node>$=gen_object(NULL_VL,@1);  }
  ;

/*
  query expression
*/

queryexpr
  : queryterm
    { $<node>$=$<node>1; }
  | queryexpr TOK_UNION all queryterm
    {
      register TXTREF n=$<node>1;
      if ( (CODE_TRN(n)==UNION) && (($<code>3==0) || TstF_TRN(n,ALL_F) ))
        {
          add_child(n,$<node>4);
          if($<code>3==0)
            ClrF_TRN(n,ALL_F);
        }
      else
        {
          n=gen_up_node(UNION,join_list($<node>1,$<node>4),@2);
          if($<code>3 > 0)
            SetF_TRN(n,ALL_F);
        }
      $<node>$=n;
    }
  ;

queryterm
  : queryspec           { $<node>$=$<node>1; }
  | '(' queryexpr ')'   { $<node>$=$<node>2; }
  ;

queryspec
  : '(' queryspec1 ')'  { $<node>$=$<node>2; }
  ;

queryspec1
  : TOK_SELECT select_spec selection tableexpr 
    {
      register TXTREF q=$<node>4,s=$<node>3;
      set_location(q,@1);
      CODE_TRN(q)=QUERY;
      if($<code>2)SetF_TRN(q,DISTINCT_F);
      RIGHT_TRN(s)=RIGHT_TRN(DOWN_TRN(q));     /* insert after from */
      RIGHT_TRN(DOWN_TRN(q))=s;
      ARITY_TRN(q)++;
      $<node>$=q;
    }
  ;

selection
  : expr_list
    {
      $<node>$=gen_parent(SELECTION,$<node>1);
    }
  | '*'
    {
      $<node>$=gen_parent(SELECTION,gen_object(STAR,@1));
    }
  ;

expr_list
  : expr                      { $<node>$=$<node>1;}
  | expr_list ',' expr        { $<node>$=join_list($<node>1,$<node>3);}
  ;

tableexpr
  : from where_d groupby having
    {
      register TXTREF tbl;
      tbl=gen_parent(TBLEXP,join_list(
                              join_list(
                                join_list($<node>1,$<node>2),
                                $<node>3),
                              $<node>4));
      if($<node>2)SetF_TRN(tbl,WHERE_F);
      if($<node>3)SetF_TRN(tbl,GROUP_F);
      if($<node>4)SetF_TRN(tbl,HAVING_F);
      if(subquery)SetF_TRN(tbl,SUBQUERY_F);
      subquery=0;
      $<node>$=tbl;
    }
  ;

from
  : TOK_FROM scan_list
    {
      $<node>$=gen_up_node(FROM,$<node>2,@1);
    }
  ;

scan_list
  : scan_desc                 { $<node>$=$<node>1;}
  | scan_list ',' scan_desc   { $<node>$=join_list($<node>1,$<node>3);}
  ;

scan_desc
  : table_desc TOK_IDENTIFIER /* IDENTIFIER -- scan name */
    {
      $<node>$ = gen_scanptr ($<node>1, $<ltrp>2, @2);
    }
  | table_desc
    {
      $<node>$ = gen_scanptr ($<node>1, (LTRLREF) NULL, tblnm.p);
    }
  ;

where_d
  :        { $<node>$ = TNULL;    }
  | where  { $<node>$ = $<node>1;}
  ;

where
  : TOK_WHERE cond
    {
      $<node>$=gen_up_node(WHERE,$<node>2,@1);
    }
  ;

where_cur        /* return pointer to cursor */
  : TOK_WHERE TOK_CURRENT TOK_OF cursor_name   { $<node>$=$<node>4; }
  ;

cursor_name
  : TOK_IDENTIFIER
    {
      register TXTREF cursor=find_info(CURSOR,$<ltrp>1);
      if(!cursor)
        lperror("Cursor name '%s' not  found ",STRING($<ltrp>1));
      $<node>$=cursor;
    }
  ;

groupby
  : { $<node>$ = TNULL;    }
  | TOK_GROUP TOK_BY column_list
    {
      $<node>$ = gen_up_node (GROUP, $<node>3, @1);
    }
  ;

having
  : { $<node>$ = TNULL;    }
  | TOK_HAVING cond
    {
      $<node>$ = gen_up_node (HAVING, $<node>2, @1);
    }
  ;

/*
  search condition
*/

cond
  : pred
    { $<node>$=$<node>1; }
  | '(' cond ')'  %prec BRACETS
    { $<node>$=$<node>2; }
  | cond TOK_OR cond  %prec TOK_OR
    { $<node>$=gen_oper(OR,$<node>1,$<node>3,@2); }
  | cond TOK_AND cond %prec TOK_AND
    { $<node>$=gen_oper(AND,$<node>1,$<node>3,@2); }
  | TOK_NOT cond  %prec TOK_NOT
    {
      TXTREF n=gen_node(NOT);
      set_location(n,@1);
      FUNC_OBJ(n)=$<node>2;
      ARITY_TRN(n)=1;
      $<node>$=n;
    }
  | error TOK_OR
    {
      $<node>$=TNULL;
      YYERROK;
      yyerror("error in condition");
    }
  | error TOK_AND
    {
      $<node>$=TNULL;
      YYERROK;
      yyerror("error in condition");
    }
  | '(' error ')' 
    {
      $<node>$=TNULL;
      YYERROK;
      yyerror("error in condition");
    }
  ;

pred
  : TOK_EXISTS subquery                             /* existence */ 
    {
      $<node>$=gen_up_node(EXISTS,$<node>2,@1);
    }
  | column TOK_IS not TOK_NULL   /* column !!!*/   /* null       */ 
    {
      $<node>$=gen_up_node(($<code>3?ISNOTNULL:ISNULL),
                           coltar_column($<node>1),@2);
    }
  | expr not TOK_BETWEEN expr TOK_AND expr         /* between */ 
    {
      register TXTREF b;
      b=gen_up_node(BETWEEN,join_list(join_list($<node>1,$<node>4),
                                      $<node>6),@3);
      if ($<code>2)
        b=gen_parent(NOT,b);
      $<node>$=b;
    }
  | expr not TOK_LIKE atom esc                   /* like */ 
    {
      register TXTREF l;
      l=gen_up_node(LIKE,join_list(join_list(coltar_column($<node>1),
                                             $<node>4),$<node>5),@3);
      if ( $<code>2)
        l = gen_parent(NOT,l);
      $<node>$=l;
    }
  | expr not TOK_IN inin                           /* in  */ 
    {
      register TXTREF i;
      i = gen_up_node(IN,join_list($<node>1,$<node>4),@3);
      if ( $<code>2)
        i = gen_parent(NOT,i);
      $<node>$=i;
    }
  | expr comp_op compare_to                        /* compare   */
    {
      TXTREF c = $<node>3;
      if (CODE_TRN(c) == NE)  /* quantified subquery */
        {
          CODE_TRN(c) = $<code>2;
          DOWN_TRN(c) = join_list($<node>1,DOWN_TRN(c));
          ARITY_TRN(c) = 2;
          $<node>$ = c;
        }
      else
        $<node>$=gen_up_node($<code>2,join_list($<node>1,c),@2);
    }
  ;

compare_to
  : expr      { $<node>$=$<node>1; }
  | subquery  { $<node>$=$<node>1; }
  | allany subquery
    {
      register TXTREF c = $<node>2;
      c=gen_parent(NE,c);
      SetF_TRN(c,QUANTIF_F);
      if($<code>1==2) SetF_TRN(c,SOME_F);
      $<node>$=c;
    }
  ;

comp_op
  : '='         { $<code>$=EQU; }
  | TOK_NE      { $<code>$=NE;  }
  | '<'         { $<code>$=LT;  }
  | '>'         { $<code>$=GT;  }
  | TOK_LE      { $<code>$=LE;  }
  | TOK_GE      { $<code>$=GE;  }
  ;

allany
  : TOK_ALL       { $<code>$=1; }
  | TOK_ANY       { $<code>$=2; }
  | TOK_SOME      { $<code>$=2; }
  ;

not
  :             { $<code>$=0; }
  | TOK_NOT     { $<code>$=1; }
  ;

esc
  :                 { $<node>$ = TNULL; }
  | TOK_ESCAPE atom { $<node>$ = $<node>2; }
  ;

inin
  : subquery
    { $<node>$=$<node>1;}
  | '(' atom_list ')'
    { $<node>$=gen_parent(VALUES,$<node>2); }
  | '(' error ')'
    {
      $<node>$=TNULL;
      YYERROK;
      yyerror("error in IN predicate list" );
    }
  ;

atom_list
  : atom                      { $<node>$=$<node>1;}
  | atom_list ',' atom        { $<node>$=join_list($<node>1,$<node>3);}
  ;

subquery
  : '(' TOK_SELECT select_spec result_spec tableexpr ')'
    {
      register TXTREF sq=$<node>5,s=$<node>4;
      set_location(sq,@1);
      CODE_TRN(sq)=SUBQUERY;
      if($<code>3)SetF_TRN(sq,DISTINCT_F);
      RIGHT_TRN(s)=RIGHT_TRN(DOWN_TRN(sq));     /* insert after from */
      RIGHT_TRN(DOWN_TRN(sq))=s;
      ARITY_TRN(sq)++;
      subquery=1;
      $<node>$=sq;
    }
  | '(' TOK_SELECT error ')'
   { 
     yyerror("Error in subquery: skipped to \')\'");
     $<node>$ = TNULL; 
   }  
  ;

result_spec
  : expr
    {
      $<node>$=gen_parent(RESULT,$<node>1);
    }
  | '*'
    {
      $<node>$=gen_parent(RESULT,gen_object(STAR,@1));
    }
  ;

all
  :            { $<code>$=0; }
  |TOK_ALL     { $<code>$=1; }
  ;

/*
  expression
*/

expr
  : prim          { $<node>$=$<node>1; }
  | '(' expr ')'  { $<node>$=$<node>2; }
  | expr '+' expr { $<node>$=gen_oper(ADD,$<node>1,$<node>3,@2); }
  | expr '-' expr { $<node>$=gen_oper(SUB,$<node>1,$<node>3,@2); }
  | expr '*' expr { $<node>$=gen_oper(MULT,$<node>1,$<node>3,@2); }
  | expr '/' expr { $<node>$=gen_oper(DIV,$<node>1,$<node>3,@2); }
  | sign expr    %prec UNARY
    {
      TXTREF n=$<node>1;
      if(!n)
        $<node>$=$<node>2;
      else
        if(CODE_TRN($<node>2)==UMINUS)
          {
            $<node>$=FUNC_OBJ($<node>2);
            free_node($<node>2);     /* double minus elimination */
            free_node($<node>1);
          }
        else
          {
            FUNC_OBJ(n)=$<node>2;
            ARITY_TRN(n)=1;
            $<node>$=n;
          }
    }
  ;

sign
  : '+' {   $<node>$ = TNULL;           }
  | '-' {   $<node>$ = gen_object (UMINUS, @1); }
  ;

prim
  : coltar         { $<node>$=$<node>1; }
  | constant       { $<node>$=$<node>1; }
  | funcref        { $<node>$=$<node>1; }
  | TOK_USER
    {  $<node>$=gen_object(USERNAME,@1); }
  ;

atom
  : target1        { $<node>$=$<node>1; }
  | constant       { $<node>$=$<node>1; }
  | sign constant  %prec UNARY
    {
      TXTREF n = $<node>1;
      if ( ! n )
        $<node>$ = $<node>2;
      else
        {
          char *s;
          i4_t  l;
          free_node (n);
          $<node>$ = n = $<node>2;
          l = strlen (STRING (CNST_NAME (n))) + 2;
          s = xmalloc (l);
          strcpy (s, "-");
          strcat (s, STRING(CNST_NAME(n)));
          CNST_NAME (n) = ltr_rec (s);
          xfree (s);
        }
    }
  | TOK_USER
    {  $<node>$ = gen_object(USERNAME,@1); }
  ;

funcref
  : TOK_COUNT '(' '*' ')'
    {
      $<node>$=gen_func(COUNT,@1, TNULL,0);
    }
  | TOK_COUNT '(' TOK_DISTINCT column ')'
    {
      $<node>$=gen_func(COUNT,@1,$<node>4,1);
    }
  | fname '(' TOK_DISTINCT column ')'
    {
      $<node>$=gen_func($<code>1,@1,$<node>4,1);
    }
  | fname '(' all expr ')'
    {
      $<node>$=gen_func($<code>1,@1,$<node>4,0);
    }
  ;

fname
  : TOK_AVG { $<code>$=AVG; }
  | TOK_MAX { $<code>$=MAX; }
  | TOK_MIN { $<code>$=MIN; }
  | TOK_SUM { $<code>$=SUM; }
  ;

constant
  : TOK_INTEGER_CONST { $<node>$=$<node>1; }
  | TOK_REAL_CONST    { $<node>$=$<node>1; }
  | TOK_NUM_CONST     { $<node>$=$<node>1; }
  | TOK_STRING_CONST  { $<node>$=$<node>1; }
  ;

type
  : TOK_T_CHAR
     { $<type>$=pack_type(SQLType_Char,1,0);                        }
  | TOK_T_SINT
     { $<type>$=pack_type(SQLType_Short,5,0);                       }
  | TOK_T_INT
     { $<type>$=pack_type(SQLType_Int,10,0);                        }
  | TOK_T_REAL
     { $<type>$=pack_type(SQLType_Real,0,0);                        }
  | TOK_T_DOUBLE
     { $<type>$=pack_type(SQLType_Double,0,0);                      }
  | TOK_T_DOUBLE TOK_T_PRECISION
     { $<type>$=pack_type(SQLType_Double,0,0);                      }
  | TOK_T_FLT  '(' TOK_INTEGER_CONST ')'
     { $<type>$=pack_type(SQLType_Float,atol(STRING(CNST_NAME($<node>3))),0);  }
  | TOK_T_FLT  
     { $<type>$=pack_type(SQLType_Real,0,0);  }
  | TOK_T_CHAR '(' TOK_INTEGER_CONST ')'
     { $<type>$=pack_type(SQLType_Char,atol(STRING(CNST_NAME($<node>3))),0);   }
  | TOK_T_NUM 
     { $<type>$=pack_type(SQLType_Int,0,0);    }
  | TOK_T_NUM '(' TOK_INTEGER_CONST ')'
     { $<type>$=pack_type(SQLType_Num,atol(STRING(CNST_NAME($<node>3))),0);    }
  | TOK_T_NUM '(' TOK_INTEGER_CONST ',' TOK_INTEGER_CONST ')'
     { $<type>$=pack_type(SQLType_Num,atol(STRING(CNST_NAME($<node>3))),
                           atol(STRING(CNST_NAME($<node>5))));
     }
  ;

table_def:  tablename
     {
       TXTREF n;
       if(!tblnm.a)
         tblnm.a=GL_AUTHOR;
       n = find_table (ANY_TBL, tblnm.a, tblnm.n);
       if (n)
         {
           yyerror ("Error: Duplicate table name definition\n");
           $<node>$ = TNULL;
         }
       else
         {
           n = gen_node (ANY_TBL);
           /* SetF_TRN(n,CHECKED_F); */
           /* table hasn't  not already defined yet */
           TBL_FNAME (n) = tblnm.a;
           TBL_NAME (n) = tblnm.n;
           add_info (n);
           $<node>$ = new_table = n;
	   new_table_col_no = 0;
         }
     }
  ;

table_desc: tablename
     {
       TXTREF n;
       if (!tblnm.a)
         tblnm.a=GL_AUTHOR;
       n = find_table (ANY_TBL, tblnm.a, tblnm.n);
       if (!n)
         {
           n = gen_node (ANY_TBL);
           TBL_FNAME (n) = tblnm.a;
           TBL_NAME (n) = tblnm.n;
           add_info (n);
         }
       $<node>$ = n;
     }
  ;

host_scan:  table_desc
     {
       $<node>$ = gen_scanptr ($<node>1, (LTRLREF) NULL, tblnm.p);
     }
  ;

tablename      /*++*/
  : TOK_IDENTIFIER
    { /*  user_identifier.tablename  or correlation name */
      tblnm.a  = (LTRLREF) NULL;
      tblnm.n  = $<ltrp>1;
      tblnm.p  = @1;
      $<code>$ = 0;
    }
  | TOK_IDENTIFIER '.' TOK_IDENTIFIER
    { /* authorization.tablename */
      tblnm.a  = $<ltrp>1;
      tblnm.n  = $<ltrp>3;
      tblnm.p  = @1;
      $<code>$ = 0;
    }
  ;

column_list
  : column                    { $<node>$ = $<node>1;                     }
  | column_list ',' column    { $<node>$ = join_list($<node>1,$<node>3); }
  ;

column
  : TOK_IDENTIFIER
    {  $<node>$=gen_column(NULL,$<ltrp>1,@1);  }
  | TOK_IDENTIFIER  '.' TOK_IDENTIFIER
    { /*  user_identifier.tablename  or correlation name */
      tblnm.a  = (LTRLREF) NULL;
      tblnm.n  = $<ltrp>1;
      tblnm.p  = @1;
      $<node>$ = gen_column (&tblnm, $<ltrp>3, @3);
    }
  | TOK_IDENTIFIER '.' TOK_IDENTIFIER  '.' TOK_IDENTIFIER
    { /* authorization.tablename */
      tblnm.a  = $<ltrp>1;
      tblnm.n  = $<ltrp>3;
      tblnm.p  = @1;
      $<node>$ = gen_column (&tblnm, $<ltrp>5, @5);
    }
  ;

target_list
  : target                  { $<node>$=$<node>1;                     }
  | target_list ',' target  { $<node>$=join_list($<node>1,$<node>3); }
  ;

target
  : target1 
    {
      register TXTREF t=OBJ_DESC($<node>1);
      SetF_TRN(t,OUT_F);
	  if( !TstF_TRN(t,INDICATOR_F) && PAR_INDICATOR(t) != TNULL )
	    SetF_TRN(PAR_INDICATOR(t),OUT_F);
      $<node>$=$<node>1;
    }
  ;
  
target1
  : TOK_IDENTIFIER
    {  $<node>$=get_parm_node($<ltrp>1,@1,1);  }
  | TOK_PARAM
    {  $<node>$=get_parm_node($<ltrp>1,@1,0);  }
  | TOK_IDENTIFIER TOK_INDICATOR TOK_IDENTIFIER
    {
      $<node>$=get_ind(get_param($<ltrp>1,1,@1),get_param($<ltrp>3,1,@3),@2);
    }
  | TOK_IDENTIFIER TOK_IDENTIFIER
    {
      $<node>$=get_ind(get_param($<ltrp>1,1,@1),get_param($<ltrp>2,1,@2),@2);
    }
  | TOK_PARAM TOK_INDICATOR TOK_PARAM
    {
      $<node>$=get_ind(get_param($<ltrp>1,0,@1),get_param($<ltrp>3,0,@3),@2);
    }
  | TOK_PARAM TOK_PARAM
    {
      $<node>$=get_ind(get_param($<ltrp>1,0,@1),get_param($<ltrp>2,0,@2),@2);
    }
  ;

coltar
  : TOK_IDENTIFIER
     {
       TXTREF n;
       n=find_info(PARAMETER,$<ltrp>1);
       if(n)
         $<node>$=get_parm_node($<ltrp>1,@1,1);
       else
         $<node>$=gen_column(NULL,$<ltrp>1,@1);
     }
  | TOK_IDENTIFIER  '.' TOK_IDENTIFIER
    { /*  user_identifier.tablename  or correlation name */
      tblnm.a  = (LTRLREF) NULL;
      tblnm.n  = $<ltrp>1;
      tblnm.p  = @1;
      $<node>$ = gen_column (&tblnm, $<ltrp>3, @3);
    }
  | TOK_IDENTIFIER '.' TOK_IDENTIFIER  '.' TOK_IDENTIFIER
    { /* authorization.tablename */
      tblnm.a  = $<ltrp>1;
      tblnm.n  = $<ltrp>3;
      tblnm.p  = @1;
      $<node>$ = gen_column (&tblnm, $<ltrp>5, @5);
    }
  | TOK_PARAM
     { $<node>$=get_parm_node($<ltrp>1,@1,0);     }
  | TOK_IDENTIFIER TOK_INDICATOR TOK_IDENTIFIER
     {
       $<node>$=get_ind(get_param($<ltrp>1,1,@1),get_param($<ltrp>3,1,@3),@2);
     }
  | TOK_IDENTIFIER TOK_IDENTIFIER
     {
       $<node>$=get_ind(get_param($<ltrp>1,1,@1),get_param($<ltrp>2,1,@2),@2);
     }
  | TOK_PARAM TOK_INDICATOR TOK_PARAM
     {
       $<node>$=get_ind(get_param($<ltrp>1,0,@1),get_param($<ltrp>3,0,@3),@2);
     }
  | TOK_PARAM TOK_PARAM
     {
       $<node>$=get_ind(get_param($<ltrp>1,0,@1),get_param($<ltrp>2,0,@2),@2);
     }
  ;


/* end of rules */
%%

static void
free_tail(void)
{
  if (del_local_vcb)
    FREE_VCB;
  LOCAL_VCB_ROOT = TNULL;
  del_local_vcb = 0;
}


static TXTREF holes_tbl = TNULL;

static TXTREF 
replace_clm_hole(TXTREF clmptr,i4_t f)
{
  register TXTREF ptr,clm;
  register enum token code=CODE_TRN(clmptr);
  assert(f==CYCLER_LD);
  if(code!=COLPTR)return clmptr;
  ptr=OBJ_DESC(clmptr);
  if(CODE_TRN(ptr)!=COLUMN_HOLE)return clmptr;
  clm=find_column(holes_tbl,CHOLE_CNAME(ptr));
  if(!clm)
    {
      if (TstF_TRN(holes_tbl,CHECKED_F))
        lperror("Incorrect column name '%s' used",STRING(CHOLE_CNAME(ptr)));
      else
        {
          add_column(holes_tbl,CHOLE_CNAME(ptr));
          clm=find_column(holes_tbl,CHOLE_CNAME(ptr));
        }
    }
  free_node(ptr);
  OBJ_DESC(clmptr)=clm;
  return clmptr;
}

static TXTREF 
replace_column_holes(TXTREF rb,VCBREF tbl)
{
  holes_tbl = tbl;
  rb=cycler(rb,replace_clm_hole,CYCLER_LD+CYCLER_LN);
  holes_tbl = TNULL;
  return rb;
}

static void 
add_table_column(enum token tbl_code,LTRLREF ltrp,sql_type_t type,TXTREF def,TXTREF constr)
{
  register TXTREF col;
  register i4_t    can_be_null = 1;
  
  if(!new_table) return;
  if(CODE_TRN(new_table)==ANY_TBL)
    CODE_TRN(new_table)=tbl_code;
  TASSERT(tbl_code == CODE_TRN(new_table),new_table);
  col = find_column(new_table,ltrp);
  if (col)
    {
      lperror("Duplicate column '%s' definition in table '%s.%s'",STRING(ltrp),
              STRING(TBL_FNAME(new_table)),STRING(TBL_NAME(new_table)));
      if(def)
        free_tree(def);
      if(constr)
        free_tree(constr);
    }
  else
    {
      add_column(new_table,ltrp);
      col=find_column(new_table,ltrp);
      COL_TYPE(col)   = type;
      COL_DEFAULT(col)= def;
      COL_NO(col) = new_table_col_no++;
    }

  while(constr)
    {
      register TXTREF c = constr;
      i4_t             null_vl = 0;
      constr = RIGHT_TRN(c);
      RIGHT_TRN(c) = TNULL;
      switch(CODE_TRN(c))
	{
	case NULL_VL:
          free_node(c);
          null_vl=1;
	case PRIMARY:
	case UNIQUE:
	  can_be_null = 0;
#define P_(code,subtree) gen_parent(code,subtree)
          TBL_CONSTR(new_table) = 
	    join_list(P_(CHECK,P_(ISNOTNULL,gen_colptr(col))),
		      TBL_CONSTR(new_table));
#undef P_
          if(null_vl)
            break;
	case INDEX:
	  ARITY_TRN(c) = 1;
	  DOWN_TRN(c)  = gen_colptr(col);
	  IND_INFO(new_table)=join_list(c,IND_INFO(new_table));
	  break;
	case CHECK:
	  TBL_CONSTR(new_table) = 
	    join_list (replace_column_holes(c,new_table),
		       TBL_CONSTR(new_table));
	  break;
	case REFERENCE:
          TBL_CONSTR(new_table)=
            join_list(gen_parent(FOREIGN,
                                 join_list(gen_parent(LOCALLIST,gen_colptr(col)),c)),
                      TBL_CONSTR(new_table));
          break;
	default:
	  debug_trn(c);
	  yyfatal("Unexpected column constraints found");
	  break;
	}
    }
  
  if (!def && can_be_null)
    COL_DEFAULT (col) = gen_node (NULL_VL);
  
  if(COL_DEFAULT(col) && CODE_TRN(COL_DEFAULT(col))==NULL_VL)
    {
      if (can_be_null)
        NULL_TYPE(COL_DEFAULT (col)) = COL_TYPE(col);
      else
        yyerror("Error: ambiguous column description");
    }
#if 0
  {
    MASKTYPE x = MASK_TRN(new_table);
    debug_trn(new_table);
    MASK_TRN(new_table) = x;
  }
#endif
}

/*
 * 'check_not_null' check columns in UNIQUE or PRIMARY key definition
 * for constraints 'not null'.
 */

static void
check_not_null(TXTREF ind)
{
  TXTREF tbl = COL_TBL(OBJ_DESC(DOWN_TRN(ind)));
  TXTREF constr,cp;
  for ( cp = DOWN_TRN(ind);cp;cp = RIGHT_TRN(cp)) /* check every column */
    {
      TXTREF col = OBJ_DESC(cp);
      for(constr = TBL_CONSTR(tbl);constr;constr = RIGHT_TRN(constr))
        {
          /*
           * if ( constr = (Check (IsNotNull (ColPtr <col>))))
           */
          TXTREF pred = DOWN_TRN(constr);
          if(CODE_TRN(pred) == NOT)
            {
              pred = DOWN_TRN(pred);
              switch(CODE_TRN(pred))
                {
                case ISNULL:
                  CODE_TRN(pred)  = ISNOTNULL;
                  free_node(DOWN_TRN(constr));
                  DOWN_TRN(constr) = pred;
                  break;
                default: break; 
                }
            }
          if(CODE_TRN(pred) != ISNOTNULL)
            continue;
          pred = DOWN_TRN(pred);
          if(CODE_TRN(pred) != COLPTR) /* it quite strange */
            continue;
          if(OBJ_DESC(pred) == col) /* if there is a constraint on given column */
            break;
        }
      if (constr) /* something was found */
        continue;
      /* if we have not found 'not null' constraints on given column */
      file_pos = LOCATION_TRN(cp);
      lperror("Warning: primary key column '%s.%s.%s' not restricted\n"
              "\t\tby 'not null' qualifier",
              STRING(TBL_FNAME(tbl)),STRING(TBL_NAME(tbl)),
              STRING(COL_NAME(col)));
      errors--; /* clear error flag - translate message as a warning */
      if (CODE_TRN(COL_DEFAULT(col))==NULL_VL)
        {
          free_node(COL_DEFAULT(col));
          COL_DEFAULT(col) = TNULL;
        }
      col = gen_node(COLPTR);
      OBJ_DESC(col) = OBJ_DESC(cp);
      TBL_CONSTR(tbl) = 
        join_list (gen_parent(CHECK,gen_parent(ISNOTNULL,col)),
                   TBL_CONSTR(new_table));
    }
}

static void
emit_module_proc(VCBREF parmlist,LTRLREF procname)
{
  if (call.subst==NULL)
    call.subst = (call_subst_t*)xmalloc (sizeof(call_subst_t));
  call.subst->proc_name = savestring(STRING(procname));
  describe_stmt(&(call.subst->interface),parmlist,'I');
  free_tail();
}

static void
emit_call(TXTREF vcb,i4_t object_id, i4_t method_id)
{
  if (call.subst==NULL)
    call.subst = (call_subst_t*)xmalloc ( sizeof(call_subst_t));
  call.subst->object = object_id;
  call.subst->method = method_id;
  describe_stmt(&(call.subst->in_sql_parm),vcb,'>');
  describe_stmt(&(call.subst->out_sql_parm),vcb,'<');
  call.subst->jmp_on_eofscan = savestring(label_nf?label_nf:"");
  call.subst->jmp_on_error   = savestring(label_er?label_er:"");
}

call_t *
prepare_replacement(void)
{
  call_t *c;
  c = (call_t*)xmalloc ( sizeof(call_t));
  c->subst = call.subst;
  if(c->subst)  /* preserve NULL string reference */
    {
      if (!call.subst->proc_name)
        call.subst->proc_name      =  savestring("");
      if (!call.subst->jmp_on_eofscan)
        call.subst->jmp_on_eofscan =  savestring("");
      if (!call.subst->jmp_on_error)
        call.subst->jmp_on_error   =  savestring("");
    }
  return c;
}

i4_t 
sql_parse(void)
{
  i4_t rc;
  yydebug=parse_debug;
  parse_mode=Esql;
  dyn_sql_section_id = 0;
  bzero(&call,sizeof(call));
  rc = yyparse();
  dyn_sql_stmt_name = NULL;
  return rc;
}
