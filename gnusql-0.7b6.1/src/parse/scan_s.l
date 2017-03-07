%{
/*
 *  scan_s.l - SQL scanner / server side.
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

/* $Id: scan_s.l,v 1.246 1998/09/29 21:26:35 kimelman Exp $ */
  
#include "xmem.h"
#include "sql_decl.h"
#include "trl.h"
#include "parse.h"
#include "const.h"
  
  
#if defined(stderr)
# undef stderr
#endif
#if defined(stdout)
# undef stdout
#endif
#define stderr STDERR
#define stdout STDOUT

#define SQL_PROC(code)  if(code)		\
  {						\
    yylloc.first_line=line_num; file_pos=yylloc.last_line=line_num; \
    if (code==TOK_IDENTIFIER && !strcmp(STRING(yylval.ltrp),"DEFINITION_SCHEMA")) \
      yylval.ltrp = ltr_rec(SYSADM);		\
    return code;				\
  }

#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) serv_scan_buf_inp(buf,&result,max_size)

extern YYLTYPE yylloc;
static i4_t line_num=0;
static char *bufstring=NULL;

static void 
serv_scan_buf_inp(char *buf, i4_t *result, i4_t max_size)
{ 
  register i4_t l = strlen (bufstring );
  l= l > max_size ? max_size : l ;
  if(l)
    {
      bcopy (bufstring, buf, l);
      bufstring += l;
      *result = l;
    }
  else
    {
      *result =  YY_NULL ;
      *buf = 0 ;
    }
}

/* void    SAVESTR(char *s,LTRLREF d); */
#define SAVESTR(s,d)      d=ltr_rec(s)

static str_buf s_scanner_buffer={0,0,NULL};
#define sql_str(s,l)  buffer_string(&s_scanner_buffer,s,l)

%}

%x SQL_str SQL_comment Identifier


WS              [ \t\f]+
OPTWS           [ \t\f]*
NOT_WS          [^ \t\f\n]

DIG             [0-9]
LET             [A-Za-z]
NOT_LET         [^A-Za-z0-9]
NAME            {LET}({LET}|{DIG}|_)*
NNS             [^A-Za-z0-9_]

SIGN            ("+"|"-")?
NUM_opt         {DIG}*
NUM             {DIG}+
INT             {NUM}
EXACT           {NUM_opt}"."{NUM_opt}
FLT_ERR         {EXACT}(e|E){SIGN}
FLT             {EXACT}(e|E){SIGN}{INT}
%%
    yy_flex_debug=server_scanner_debug;
  /*********************************************
   *          embedded SQL lexics              *
   *********************************************/
WHENEVER        SQL_PROC(TOK_WHENEVER);
FOUND           SQL_PROC(TOK_FOUND   );
SQLERROR        SQL_PROC(TOK_SQLERROR);
CONTINUE        SQL_PROC(TOK_CONTINUE);
GO(TO)?/{NNS}   SQL_PROC(TOK_GOTO    );
BEGIN           SQL_PROC(TOK_BEGIN   );
END             SQL_PROC(TOK_END     );
DECLARE         SQL_PROC(TOK_DECLARE );
SECTION         SQL_PROC(TOK_SECTION );
":"{NAME}       {
                   SAVESTR(yytext+1,yylval.ltrp);
                   SQL_PROC(TOK_PARAM);
                }

  /*********************************************
   *          Dynamic  SQL lexics              *
   *********************************************/
\?             {
                 static i4_t placeholders_counter = 0;
                 char str[100];
                 sprintf(str,"__%d",placeholders_counter++);
                 SAVESTR(str,yylval.ltrp);
                 SQL_PROC(TOK_PARAM);
               }

  /*********************************************
   *          clean  SQL MODULE lexic          *
   *********************************************/
MODULE          SQL_PROC(TOK_MODULE      );
LANGUAGE        SQL_PROC(TOK_LANGUAGE    );
PROCEDURE       SQL_PROC(TOK_PROCEDURE   );
SQLCODE         SQL_PROC(TOK_SQLCODE     );
";"             SQL_PROC(';'             );

  /*********************************************
   *          clean  SQL SCHEMA lexic          *
   *********************************************/
CREATE       SQL_PROC(TOK_CREATE    );
DROP         SQL_PROC(TOK_DROP      );
ALTER        SQL_PROC(TOK_ALTER     );
SCHEMA       SQL_PROC(TOK_SCHEMA    );
TABLE        SQL_PROC(TOK_TABLE     );
VIEW         SQL_PROC(TOK_VIEW      );
GRANT        SQL_PROC(TOK_GRANT     );
REVOKE       SQL_PROC(TOK_REVOKE    );
CHECK        SQL_PROC(TOK_CHECK     );
TO/{NNS}     SQL_PROC(TOK_TO        );
DEFAULT      SQL_PROC(TOK_DEFAULT   ); 
FOREIGN      SQL_PROC(TOK_FOREIGN   );
KEY          SQL_PROC(TOK_KEY       );
PRIMARY      SQL_PROC(TOK_PRIMARY   );    
INDEX        SQL_PROC(TOK_INDEX     );    
OPTION       SQL_PROC(TOK_OPTION    );
PRIVILEGES   SQL_PROC(TOK_PRIVILEGES);
PUBLIC       SQL_PROC(TOK_PUBLIC    );
REFERENCES   SQL_PROC(TOK_REFERENCES);    
UNIQUE       SQL_PROC(TOK_UNIQUE    );

  /*********************************************
   *            clean  SQL lexic               *
   *********************************************/
AUTHORIZATION    SQL_PROC(TOK_AUTHORIZATION);

  /*********************************************
   *          common SQL  keywords             *
   *********************************************/
ALL/{NNS}       SQL_PROC(TOK_ALL       );
ANY/{NNS}       SQL_PROC(TOK_ANY       );
AS/{NNS}        SQL_PROC(TOK_AS        );
ASC/{NNS}       SQL_PROC(TOK_ASC       );
BETWEEN         SQL_PROC(TOK_BETWEEN   );
BY/{NNS}        SQL_PROC(TOK_BY        );
CLOSE           SQL_PROC(TOK_CLOSE     );
COMMIT          SQL_PROC(TOK_COMMIT    );
CURRENT         SQL_PROC(TOK_CURRENT   );
CURSOR          SQL_PROC(TOK_CURSOR    );
DELETE          SQL_PROC(TOK_DELETE    );
DESC/{NNS}      SQL_PROC(TOK_DESC      );
DISTINCT        SQL_PROC(TOK_DISTINCT  );
ESCAPE          SQL_PROC(TOK_ESCAPE    );
EXISTS          SQL_PROC(TOK_EXISTS    );
FETCH           SQL_PROC(TOK_FETCH     );
FOR/{NNS}       SQL_PROC(TOK_FOR       );
FROM            SQL_PROC(TOK_FROM      );
GROUP           SQL_PROC(TOK_GROUP     );
HAVING          SQL_PROC(TOK_HAVING    );
IN/{NNS}        SQL_PROC(TOK_IN        );
INDICATOR       SQL_PROC(TOK_INDICATOR );
INSERT          SQL_PROC(TOK_INSERT    );
INTO            SQL_PROC(TOK_INTO      );
IS/{NNS}        SQL_PROC(TOK_IS        );
LIKE            SQL_PROC(TOK_LIKE      );
NULL            SQL_PROC(TOK_NULL      );
OF/{NNS}        SQL_PROC(TOK_OF        );
ON/{NNS}        SQL_PROC(TOK_ON        );
OPEN            SQL_PROC(TOK_OPEN      );
ORDER           SQL_PROC(TOK_ORDER     );
ROLLBACK        SQL_PROC(TOK_ROLLBACK  );
SELECT          SQL_PROC(TOK_SELECT    );
SET/{NNS}       SQL_PROC(TOK_SET       );
SOME            SQL_PROC(TOK_SOME      );
UNION           SQL_PROC(TOK_UNION     );
UPDATE          SQL_PROC(TOK_UPDATE    );
USER            SQL_PROC(TOK_USER      );
VALUES          SQL_PROC(TOK_VALUES    );
WHERE           SQL_PROC(TOK_WHERE     );
WITH            SQL_PROC(TOK_WITH      );
WORK            SQL_PROC(TOK_WORK      );

  /******************************************
   *  common SQL  keywords  for data type   *
   ******************************************/
CHAR(ACTER)?/{NNS}    SQL_PROC(TOK_T_CHAR);
NUMERIC/{NNS}         SQL_PROC(TOK_T_NUM);
DEC(IMAL)?/{NNS}      SQL_PROC(TOK_T_NUM);
INT(EGER)?/{NNS}      SQL_PROC(TOK_T_INT);
SMALLINT/{NNS}        SQL_PROC(TOK_T_SINT);
FLOAT/{NNS}           SQL_PROC(TOK_T_FLT);
REAL/{NNS}            SQL_PROC(TOK_T_REAL);
DOUBLE/{NNS}          SQL_PROC(TOK_T_DOUBLE);
PRECISION/{NNS}       SQL_PROC(TOK_T_PRECISION);

  /******************************************
   *  common SQL  keywords  for expression  *
   ******************************************/
AND/{NNS}             SQL_PROC(TOK_AND       );
OR/{NNS}              SQL_PROC(TOK_OR        );
NOT/{NNS}             SQL_PROC(TOK_NOT       );

AVG/{NNS}             SQL_PROC(TOK_AVG   );
MAX/{NNS}             SQL_PROC(TOK_MAX   );
MIN/{NNS}             SQL_PROC(TOK_MIN   );
SUM/{NNS}             SQL_PROC(TOK_SUM   );
COUNT/{NNS}           SQL_PROC(TOK_COUNT );

\+              SQL_PROC('+'   );
\-              SQL_PROC('-'   );
\*              SQL_PROC('*'   );
\/              SQL_PROC('/'   );

\=              SQL_PROC('='   );
"<>"            SQL_PROC(TOK_NE);
">"             SQL_PROC('>'   );
"<"             SQL_PROC('<'   );
">="            SQL_PROC(TOK_GE);
"<="            SQL_PROC(TOK_LE);

\(              SQL_PROC('('   );
\)              SQL_PROC(')'   );
\,              SQL_PROC(','   );
\.              SQL_PROC('.'   );

{NAME}          { SAVESTR(yytext,yylval.ltrp);
                  SQL_PROC(TOK_IDENTIFIER);}

{INT}           { /*  read decimal constant  */
                  yylval.node=gen_const_node(SQLType_Int,yytext);
                  SQL_PROC(TOK_INTEGER_CONST); }
{EXACT}         { /*  read float constant  */
                  yylval.node=gen_const_node(SQLType_Num,yytext);
                  SQL_PROC(TOK_NUM_CONST); }
{FLT}           {  /*  read float constant  */
                  yylval.node=gen_const_node(SQLType_Real,yytext);
                  SQL_PROC(TOK_REAL_CONST); }
{FLT_ERR}       {  /*  read float constant with error in exponent */
                  yyerror("Lex: error format for floating constant");
                  sql_str(yytext,yyleng);
                  sql_str("0",1);  /* recovery error as possible */
                  {
                    char *ptr=sql_str(NULL,0);
                    yylval.node=gen_const_node(SQLType_Real,ptr);
                  }
                  SQL_PROC(TOK_REAL_CONST); }
{WS}            /* do nothing */
\'              BEGIN(SQL_str);
\"              BEGIN(Identifier);
"-"{2}          BEGIN(SQL_comment);  /* ? ? obsolete */
\n              line_num++;
.               { 
                  char strng[100];
                  file_pos=line_num;
		  sprintf(strng,
			  "syntax error: unexpected char '%c'",
			  *yytext);
                  yyerror(strng);
		}
  /*********************************************
   *             SQL string recognizer         *
   *********************************************/
<SQL_str>\'{2}      sql_str("'",1);
<SQL_str>\n         {
                      yyerror(" Unterminated SQL string ");
                      line_num++;
		      BEGIN(INITIAL);
                    }
<SQL_str>[^\'\n]+   sql_str(yytext,yyleng);
<SQL_str>\'         {
                      char *ptr=sql_str(NULL,0);
                      BEGIN(INITIAL);
                      yylval.node=gen_const_node(SQLType_Char,ptr);
                      SQL_PROC(TOK_STRING_CONST); 
		    }
  /*********************************************
   *             SQL quoted identifiers        *
   *********************************************/
<Identifier>\"{2}      sql_str("\"",1);
<Identifier>\n         {
                      yyerror(" Unterminated quoted identifier ");
                      line_num++;
		      BEGIN(INITIAL);
                    }
<Identifier>[^\"\n]+   sql_str(yytext,yyleng);
<Identifier>\"         {
                      BEGIN(INITIAL);
		      SAVESTR(sql_str(NULL,0),yylval.ltrp);
		      SQL_PROC(TOK_IDENTIFIER);
		    }
  /*********************************************
   *             SQL comment recognizer        *
   *********************************************/
<SQL_comment>.      /* do nothing */;
<SQL_comment>\n     { line_num++; BEGIN(INITIAL); }

%%

void
restart_server_scanner(char *stmt,i4_t bline)
{
  yy_init=1;
  line_num=bline;
  bufstring=stmt;
}
