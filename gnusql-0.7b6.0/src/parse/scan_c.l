%{
/*
 *  scan_c.l - scanner-extractor of GNU SQL/C compiler
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

/* $Id: scan_c.l,v 1.248 1997/10/16 10:01:17 kml Exp $ */

#include "xmem.h"
#include "gsqltrn.h"
#include "sql_decl.h"
#include "procname.h"
#include <assert.h>

#define SWITCH_COND(cond)  { save_condition();BEGIN(cond);}
#define REST_COND            rest_condition();

#define SCANNER_NAME      yylex_extractor

#ifdef YY_DECL
#  undef YY_DECL
#endif
#define YY_DECL           int SCANNER_NAME(void)

YY_DECL;

#define YY_USER_INIT      { BEGIN(START_CONDITION); line_num=1; }
#define YY_USER_ACTION    if(yy_flex_debug){ \
  fprintf(stderr,"\nCurrent lex state=%d\n",yy_start);}

#define SQL_PROC(code)  if(code){file_pos=line_num; return 1<<code;}

static str_buf c_scanner_buffer={0,0,NULL};
#define sql_str(s,l)  buffer_string(&c_scanner_buffer,s,l)

/* defined in main routine */
int open_file(FILE **f,char *ext);

static void  save_condition __P((void));
static void  rest_condition __P((void));

#ifndef START_CONDITION
#  define START_CONDITION   scan_mode_state
#endif

#define BEGIN_MODE(v)    { BEGIN(v); scan_mode_state=v; }

static char *statement_scanner_buffer=NULL;
static int   line_num=1;
static int   io_diff=0;
static char *progname_out=NULL;
static int   bline=1; 

#define APPEND_BUFFER	{ sql_str(yytext,yyleng);} 
#define POP_BUFFER	pop_cl_buffer()

void
pop_cl_buffer(void)
{
  register char *b=sql_str(NULL,0);
  statement_scanner_buffer=b?savestring(b):NULL;
}

%}

%x C_lang c_comment C_str
%x ESQL
%x MOD SCH SQL
%x SQL_str SQL_ident SQL_comment

WS              [ \t\f]+
NNS             [^A-Za-z0-9_]
OPTWS           [ \t\f]*
NOT_WS          [^ \t\f\n]
LET             [A-Za-z]
SQL_STRT        ((EXEC[ \t]+SQL)|("\$"))

%%
    static int esql_caller = 0;
    static int rout_title  = 0;

    int comment_caller     = 0;
    int first_start        = yy_init;
    int with_grant_options = 0;
    
    #ifdef FLEX_DEBUG
      yy_flex_debug=client_scanner_debug;
    #endif
  /*********************************************
   *                C lexics                   *
   *********************************************/
<INITIAL,C_lang>^([ \t]*{SQL_STRT})[ \t\n\r\f] {
                          esql_caller=C_lang;
                          bline=line_num;
                          BEGIN_MODE(ESQL);
			}
<C_lang>"/\*"           {
                          ECHO;
			  comment_caller=C_lang;
			  BEGIN_MODE(c_comment);
			}
<C_lang>\"              {
                          ECHO;
			  BEGIN_MODE(C_str);
			}
<INITIAL,C_lang>^([ \t]*{SQL_STRT})[^ \t\n\r\f] |
<C_lang>^.              { ECHO; }
<INITIAL,C_lang>\n      { ECHO; line_num++; }
  /*********************************************
   *               C string recognizer         *
   *********************************************/
<C_str>\"                { ECHO; BEGIN_MODE(C_lang); }
<C_str>\\\"              { ECHO; }
<C_str>\\\n              { ECHO; ++line_num; }
<C_str>\\?.              { ECHO; }
<C_str>\n                { 
                           ECHO;
                           yyerror("Unterminated C string");
                           BEGIN_MODE(C_lang);
                         }
  /*********************************************
   *             C comment recornizer          *
   *********************************************/
<c_comment>[^\*\n]*         { if (comment_caller == C_lang) { ECHO; } }
<c_comment>"\*"+[^\*\/\n]*  { if (comment_caller == C_lang) { ECHO; } }
<c_comment>\n               { if (comment_caller == C_lang) { ECHO; }
                              line_num++;
			    }
<c_comment>"\*"+"\/"        { if (comment_caller == C_lang)
				{
				  ECHO; 
				}
			      BEGIN_MODE(comment_caller);
			    }

  /*********************************************
   *        lexics of embedded SQL in C        *
   *********************************************/
<ESQL>\/\*              { if(esql_caller==C_lang)
			    {
			      comment_caller=ESQL;
			      BEGIN_MODE(c_comment);
			    }
                          else
			    yyerror("Lex: Unrecognizable literal'/*'\n");
			}
<ESQL>;                 { 
                          BEGIN_MODE(esql_caller);
			  POP_BUFFER;
			  SQL_PROC(ESQL); 
			}

  /*********************************************
   *          embedded SQL lexics              *
   *********************************************/
<INITIAL,SQL,MOD>MODULE/{NNS}  {
                           BEGIN_MODE(MOD);
			   APPEND_BUFFER;
			 }
<MOD>DECLARE/{NNS}     	 { 
                           POP_BUFFER;
			   APPEND_BUFFER;
			   SQL_PROC(MOD);
			 }
<MOD>PROCEDURE/{NNS}   	 {
			   rout_title=1;
			   POP_BUFFER;
			   APPEND_BUFFER;
			   SQL_PROC(MOD);
			 }
<MOD>";"        	 {
                           APPEND_BUFFER;
			   if(rout_title)	
			     rout_title=0;
			   else
			     {
			       POP_BUFFER;
			       SQL_PROC(MOD);
			     } 
			 }

  /*********************************************
   *          clean  SQL SCH lexic             *
   *********************************************/
<INITIAL,SQL,SCH>((CREATE)|(ALTER)|(DROP))/{NNS} {
                            {
                              BEGIN_MODE(SCH);
                              POP_BUFFER;
                              APPEND_BUFFER;
                              if(first_start)
                                first_start=0;
                              else
                                {
                                  SQL_PROC(SCH);
                                }
                            }                    }
<SCH>WITH/{NNS}          {
			   APPEND_BUFFER;
                           with_grant_options=1;
			 }
<SCH>GRANT/{NNS}         {
                           if(with_grant_options)
                             {
			       APPEND_BUFFER;
			       with_grant_options=0;
			     }
                           else
                             {
			       POP_BUFFER;
			       APPEND_BUFFER;
			       SQL_PROC(SCH);
			     }
			 }
<SCH>REVOKE/{NNS}        {
                           POP_BUFFER;
			   APPEND_BUFFER;
			   SQL_PROC(SCH);
			 }
<SCH><<EOF>>	         {
                           POP_BUFFER;
                           BEGIN(INITIAL);
			   SQL_PROC(SCH);
			 }

  /*********************************************
   *          common SQL  keywords             *
   *********************************************/
{WS}            /* do nothing */;
<ESQL,MOD,SCH>{WS}	 { APPEND_BUFFER; }
<ESQL,MOD,SCH>\'	 {
                           APPEND_BUFFER;
			   SWITCH_COND(SQL_str);
			 }
<ESQL,MOD,SCH>\"	 {
                           APPEND_BUFFER;
			   SWITCH_COND(SQL_ident);
			 }
<INITIAL,ESQL,MOD,SCH>"-"{2}	{
                           SWITCH_COND(SQL_comment);  
			 }
<ESQL,MOD,SCH>\n	 {
                           APPEND_BUFFER;
			   line_num++; 
			 }
<ESQL,MOD,SCH>.		 { 
                           APPEND_BUFFER;
			 }
  /*********************************************
   *             SQL string recognizer         *
   *********************************************/
<SQL_str>\'{2}           { APPEND_BUFFER; }
<SQL_str>\n              {
                           yyerror(" Unterminated SQL string ");
		           line_num++; 
			   REST_COND;
		         }
<SQL_str>[^\'\n]+        { APPEND_BUFFER; }
<SQL_str>\'              {
                           APPEND_BUFFER;
                           REST_COND;
			 }
  /*********************************************
   *             SQL string recognizer         *
   *********************************************/
<SQL_ident>\n              {
                           yyerror(" Unterminated SQL identifier ");
		           line_num++; 
			   REST_COND;
		         }
<SQL_ident>[^\"\n]+        { APPEND_BUFFER; }
<SQL_ident>\"{2}           { APPEND_BUFFER; }
<SQL_ident>\"              {
			   APPEND_BUFFER ;
                           REST_COND;
			 }
  /*********************************************
   *             SQL comment recognizer        *
   *********************************************/
<SQL_comment>.      /* do nothing */;
<SQL_comment>\n          {
                           line_num++;
			   REST_COND;
			 }
.			 { 
                           ECHO;
			   BEGIN_MODE(C_lang);
			 }
%%

typedef struct save_yy_cond {
      int yy_cond;
      struct save_yy_cond *nxt;
    } *sv_ys_ptr;

static sv_ys_ptr save_yy_start=NULL;

static void
save_condition (void)
{
  register sv_ys_ptr nptr=(sv_ys_ptr)xmalloc(sizeof(struct save_yy_cond));
  nptr->yy_cond=yy_start;
  nptr->nxt=save_yy_start;
  save_yy_start=nptr;
#ifdef FLEX_DEBUG
  if(yy_flex_debug)
    fprintf(stderr,"\n Lex: push condition :%d => (%d)\n",
            yy_start,nptr->yy_cond);
#endif
}

static void
rest_condition(void)
{
  register sv_ys_ptr nptr=save_yy_start;
  if(nptr==NULL)
    fatal("Lex: pop conditions : stack is empty",NULL);
  yy_start=nptr->yy_cond;
  save_yy_start=nptr->nxt;
#ifdef FLEX_DEBUG
  if(yy_flex_debug)
    fprintf(stderr,"\n Lex: pop condiiton :%d <=(%d)\n",
            yy_start,nptr->yy_cond);
#endif
  xfree(nptr);
}

static struct
{
  char *mode;
  char *ext;
  int   state;
} scan_modes[] =
{ 
  {"0"     ,"? ?",INITIAL},
  {"SQL"   ,"sql",SQL},
  {"MODULE","sql",MOD},
  {"SCHEMA","sql",SCH},
  {"C"     ,"ec" ,C_lang},
  {"DMP"   ,"trl",-1}
};

void
client_scan_mode(int *p,char *s)
{ 
  register int k;
  if(!s)
    return;
  for(k=sizeof(scan_modes)/sizeof(*scan_modes);k--;)
    if(0==strcmp(s,scan_modes[k].mode))
       break;
  *p=scan_modes[k].state;
}

static void
outtype(char *text,int ins_directives)
{
  char *p;
  if (ins_directives & 1)
    fprintf (yyout,"#line %d \"%s\" \n", line_num + ( ++ io_diff),
	     progname_out);
  if (text)
    for (p = text; *p ; p++)
      if (*p != '\n')
        putc(*p,yyout);
      else
        {
          if (ins_directives & 4)
            {
#if __STDC__   /* we assume produced code will be compiled by the same compiler */
              fputs("\"\n\t\"",yyout);
              io_diff++;
#else
              while (*p == ' ' || *p == '\n' || *p == '\t') p++;
              putc(' ',yyout); p--;
#endif
            }
          else
            {
              putc('\n',yyout);
              io_diff++;
            }
        }
  if (ins_directives & 2)
    {
      fprintf(yyout,"#line %d \"%s\" \n",line_num,progname);
      io_diff++;
    }
}


int
scanner (void)
{
  extern int open_file  __P((FILE ** f, char *ext));
  int sm, oldsm;
  sm = 0;
  oldsm = scan_mode_state;
  
  if ( ( yyin = fopen (progname, "r") ) == NULL )
    {
      yyerror ("can\'t open input file");
      return 0;
    }
  if (open_file (&yyout, "c") )
    {
      char b[120];
      strcpy (b, sql_prg);
      strcat (b, ".c");
      progname_out = savestring (b);
    }
  else
    yyfatal ("Can't create .c output file");
  if (scan_mode_state == INITIAL)
    {
      register i4_t k;
      register char *pn = progname+strlen(progname);
      for (k = sizeof(scan_modes)/sizeof(*scan_modes); k--; )
        if (0 == strcmp(pn - strlen(scan_modes[k].ext),scan_modes[k].ext))
	  break;
      scan_mode_state = scan_modes[k].state;
    }
#if 0  
  /*
   * analyzing for dump extension - 
   * may be load tree dump and continue processing from dumping point
   */
  if (scan_mode_state == INITIAL)
    {
      register int k;
      register char *pn = progname+strlen(progname);
      for (k = 0; compiler_passes[k].sname; k++)
        if (0 == strcmp(pn - strlen(compiler_passes[k].ext),compiler_passes[k].ext))
	  { 
	    scan_mode_state = -1;
	    break;
	  }
    }
#endif  
  yy_init = 1;
  io_diff = 0;
  line_num = 1;
  outtype ("/* This file is created automatically by gsql compiler */\n"
	   "#include <gnusql/sql.h>\n"
	   "#include \"",0);
  outtype (sql_prg,0);
  outtype (".Sc\"\n\n",2);
  while (SCANNER_NAME () )
    {
      extern char *prepare_statement(char *a,i4_t b,char **repl);
      char *repl = NULL;
      if (sm == 0)
	{
	  if(scan_mode_state == SCH)
            {
              int ln = line_num;
              line_num = 1;
              outtype ("\n"
                       "#include \"dyn_funcs.h\"\n"
                       "\n"
                       "int main(int argc, char **argv) {\n", 1);
              line_num = ln ;
            }
	  sm = scan_mode_state;
	}
      else
	assert ((sm == scan_mode_state) || (sm == INITIAL));
      if (statement_scanner_buffer == NULL)
	yyfatal ( "Unexpected null input line");
      if (sm == SCH) /* if we scan schema - there is no need to compile */
        {            /* anything now - let's do it all at run time      */
          outtype ("  SQL__execute_immediate (\"", 0);
          outtype (statement_scanner_buffer, 4);
          outtype ("  \");\n"
                   "  if(SQLCODE>0)\n"
                   "    fprintf(stderr,\"SQL warning:%s\\n\",gsqlca.errmsg);\n"
                   "  if(SQLCODE<0){\n"
                   "    fprintf(stderr,\"SQL error:%s\\n\",gsqlca.errmsg);\n"
                   "    goto errexit;\n"
                   "  }\n"
                   , 0);
        }
      else if ( ! prepare_statement (statement_scanner_buffer, bline,
                                     (sm == MOD? NULL:&repl)) )
	return 0;
      io_diff -= line_num - bline;
      if (sm != MOD)
        outtype (repl, (sm == SCH) ? 0 : 3 );
      bline = line_num;
    }
  if (sm == SCH)
    outtype ("\n"
             "  " CALL_COMMIT_str "\n"
             "  return 0;\n"
             " errexit:\n"
             "  " CALL_ROLLBACK_str "\n"
             "  return SQLCODE;\n"
             "}\n"
             "/* end of create schema routine */\n"
             "\n", 0);
  scan_mode_state = oldsm;
  if (yyin != stdin )
    fclose (yyin);
  fclose (yyout);
  return 1;
}
