%{
/*
 *  monitor.l - DB line monitor
 *            
 *  $Id: gsqlmon.l,v 1.251 1998/09/30 02:39:48 kimelman Exp $
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming, Russia
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
  
#include "xmem.h"
#include "sql_decl.h"
#include "sql.h"
#include "dyn_funcs.h"
#include <assert.h>
  

#define SWITCH_COND(cond)  { save_condition();BEGIN(cond);}
#define REST_COND            rest_condition();

#define SCANNER_NAME      yylex_extractor

#ifdef YY_DECL
#  undef YY_DECL
#endif
#define YY_DECL           i4_t SCANNER_NAME(void)

YY_DECL;

#define SQL_PROC(code)  if(code){return code;}

static str_buf c_scanner_buffer={0,0,NULL};
#define sql_str(s,l)  buffer_string(&c_scanner_buffer,s,l)

static void  save_condition(YY_PROTO(void));
static void  rest_condition(YY_PROTO(void));

static char *statement_scanner_buffer=NULL;

#define APPEND_BUFFER	{ sql_str(yytext,yyleng);} 
#define POP_BUFFER	pop_cl_buffer()
#define NEW_LINE        {\
  if (smth_done) fprintf(stderr,"(:%d)>",++line_num);\
  else           fprintf(stderr,"FreeSQL(:%d)>",++line_num); }

void pop_cl_buffer(void)
{
  register char *b=sql_str(NULL,0);
  statement_scanner_buffer= (b?savestring(b):NULL);
}

%}

%x SQL_str SQL_comment
%s Start

WS              [ \t\f]+
NNS             [^A-Za-z0-9_]
OPTWS           [ \t\f]*
NOT_WS          [^ \t\f\n]
LET             [A-Za-z]
SQL_STRT        ((EXEC[ \t]+SQL)|("\$"))

%%
    i4_t sql_str_d_quote=0;
    static i4_t line_num = 0;
    i4_t smth_done = 0;

                         NEW_LINE;
  /*********************************************
   *        lexics of embedded SQL in C        *
   *********************************************/
\;                       {
			   POP_BUFFER;
			   SQL_PROC(1); 
			 }
{WS}	                 { if(smth_done) { APPEND_BUFFER; } }
\'	                 {
                           APPEND_BUFFER;
                           smth_done = 1;
			   SWITCH_COND(SQL_str);
			 }
\"	                 {
                           APPEND_BUFFER;
                           smth_done = 1;
			   sql_str_d_quote=1;
			   SWITCH_COND(SQL_str);
			 }
"-"{2}	                 {
                           SWITCH_COND(SQL_comment);  
			 }
\n	                 {
                           if (smth_done)
                             {
                               APPEND_BUFFER;
                             }
                           NEW_LINE;
			 }
.		         {
                           APPEND_BUFFER;
                           smth_done = 1;
			 }
  /*********************************************
   *             SQL string recognizer         *
   *********************************************/
<SQL_str>\'{2}           { APPEND_BUFFER; }
<SQL_str>\n              {
                           yyerror(" Unterminated SQL string ");
                           NEW_LINE;
			   REST_COND;
		         }
<SQL_str>[^\'\"\n]+      { APPEND_BUFFER; }
<SQL_str>\"{2}           { APPEND_BUFFER; }
<SQL_str>\"              {
			   APPEND_BUFFER ;
		           if(sql_str_d_quote) 
                             {
			       sql_str_d_quote=0;
			       REST_COND;
			     }
			 }
<SQL_str>\'              {
                           APPEND_BUFFER;
			   if(!sql_str_d_quote) 
			     {
			       REST_COND;
			     }
			 }
  /*********************************************
   *             SQL comment recognizer        *
   *********************************************/
<SQL_comment>.      /* do nothing */;
<SQL_comment>\n          {
                           NEW_LINE;
			   REST_COND;
			 }
%%

typedef struct save_yy_cond {
      i4_t yy_cond;
      struct save_yy_cond *nxt;
    } *sv_ys_ptr;

static sv_ys_ptr save_yy_start=NULL;

static void
save_condition(YY_PROTO(void))
{
  register sv_ys_ptr nptr=(sv_ys_ptr)xmalloc(sizeof(struct save_yy_cond));
  nptr->yy_cond=yy_start;
  nptr->nxt=save_yy_start;
  save_yy_start=nptr;
#ifdef FLEX_DEBUG
  if(yy_flex_debug)
    fprintf(stderr,"\n Lex: pushing condition :%d => (%d)\n",
            yy_start,nptr->yy_cond);
#endif
}

static void
rest_condition(YY_PROTO(void))
{
  register sv_ys_ptr nptr=save_yy_start;
  if(nptr==NULL)
    fatal("Lex: restore conditions : condition stack is empty",NULL);
  yy_start=nptr->yy_cond;
  save_yy_start=nptr->nxt;
#ifdef FLEX_DEBUG
  if(yy_flex_debug)
    fprintf(stderr,"\n Lex: poping condition :%d <=(%d)\n",
            yy_start,nptr->yy_cond);
#endif
  xfree(nptr);
}

int
print_width(dyn_sql_type_code_t t,int len)
{
  switch(t)
    {
    case SQL__Long  : 
    case SQL__Real  : return 10;
    case SQL__Short : return  5;
    case SQL__Double: return 15;
    default:                   ;
    }
  return len; 
}

void
error_rprt(char *st,int rc, char *stmt)
{
  fprintf(stderr,"\n#### Error occured in '%s'\n%s\nat \"%s\"\n",
          stmt,gsqlca.errmsg,st);
}

int
main(int argc, char **argv)
{
  int stmt = 0;
  int rc;
  int i,j;
  static char *sps =
    "                                                                       "
    "                                                                       "
    "                                                                       "
    "                                                                       "
    "                                                                       "
    ;
  yyin = stdin;
  yyout = stderr;
#if 0 
  if (argc > 1)
    {
      char *host , *user, *passwd;
      host   = argv[1];
      user   = (argc > 2 ? argv[2]: getlogin() );
      passwd = (argc > 3 ? argv[3]: NULL);
      if(SQL__session(host,user,passwd)!=0)       /* Not implemented yet */
        yyfatal("Can't connect to given host");
    }
#endif
  
#define CHECK(st) if((rc=SQL__##st)!=0){ error_rprt(#st,rc,statement_scanner_buffer); goto error; }
  while(SCANNER_NAME())
    {
      char *b    = NULL;
      int   l;
      int  *clms = NULL;
      
      SQL_DESCR in,out;

      if(statement_scanner_buffer==NULL)
        continue;
      if(strncmp(statement_scanner_buffer,"select",strlen("select")))
        {
          /* if not select statement */
          CHECK(execute_immediate(statement_scanner_buffer));
          stmt++;
          continue;
        }

      /* put select in parenthesys */
      l = strlen(statement_scanner_buffer);
      b = xmalloc(l+3);
      b[0] = '(';
      strcpy(b+1,statement_scanner_buffer);
      strcat(b+1+l,")");
      xfree(statement_scanner_buffer);
      statement_scanner_buffer = b;
          
      b = xmalloc(20);
      sprintf(b,"%dth Stmt",stmt++);

      CHECK(prepare(b,statement_scanner_buffer));
      CHECK(allocate_cursor(b,"CURSOR"));
      in  = SQL__allocate_descr("IN",0);
      out = SQL__allocate_descr("OUT",0);

      CHECK(describe(b,1,in));
      CHECK(describe(b,0,out));

      clms = (int*)xmalloc(out->count*sizeof(int));
      
      /* open cursor and write headers */
      j=1;
      fprintf(stderr,"\n|");
      for( i=0; i < out->count; i++)
        {
          int jj, k = 0;
          sql_descr_element_t *e = &(out->values[i]);
          if (e->unnamed)
            jj=fprintf(stderr," col_%03d ",i);
          else
            jj=fprintf(stderr," %s ",e->name);
          k = print_width(e->type,e->length) - jj;
          jj += fprintf(stderr,"%s|",sps+strlen(sps)- (k>0?k:0) );
          clms [i] = jj - 1;
          j += jj;
        }

      /* -------------------------------------------------*/
      fprintf(stderr,"\n");
      for(i=j;i--;)
        fprintf(stderr,"-");

      if(in->count)
        {
          CHECK(open_cursor("CURSOR",in));
        }
      else
        {
          CHECK(open_cursor("CURSOR",NULL));
        }
        
      while(1)
        {
          char oline[1024];
          for( i=0; i < out->count; i++)
            out->values[i].indicator = 0;

          CHECK(fetch("CURSOR",out));
          
          if(SQLCODE!=0)
            break;
          
          sprintf(oline,"\n|");
          for (i=0; i < out->count; i++)
            {
              int k ;
              int kk = 0 ;
              char buf[4096] ;
              sql_descr_element_t *e = &(out->values[i]) ;
              
              if (e->indicator < 0)
                {
                  if (!e->nullable)
                    if (e->name)
                      fprintf(stderr,"\nindicator < 0 for 'not null' field '%s'",e->name);
                    else
                      fprintf(stderr,"\nindicator < 0 for 'not null' field '%d'",i);
                  sprintf(buf,"null");
                }
              else switch(e->type)
                {
                case SQL__Char:
                case SQL__CharVar:
                  sprintf(buf,"%s",(char*)e->data);
                  break;
                case SQL__Double: /* DOUBLE */
                case SQL__Float: /* FLOAT  */
                  sprintf(buf,"%g",*((double*)(e->data)));
                  break;
                case SQL__Real: /* REAL   */
                  sprintf(buf,"%g",*((float*)(e->data)));
                  break;
                case SQL__Int: /* int */
                  sprintf(buf,"%X",*((int*)(e->data)));
                  break;
                case SQL__Short: /* small */
                  sprintf(buf,"%d",(int)*((short*)(e->data)));
                  break;
                default:
                  sprintf(buf," **%d** ",e->type);
                }
              assert(kk==0);
              k = (clms[i] - strlen(buf))/2; 
              if (k > 4*clms[i])
                {
                  strcat(oline,"!!");
                  k = 0;
                }
              k = sprintf(oline+strlen(oline),"%s%s",
                          sps + strlen(sps) - (k>0?k:0),buf);
              k = clms[i] - k;
              sprintf(oline+strlen(oline),"%s|",
                      sps + strlen(sps) - (k>0?k:0));
            }
          fprintf(stderr,"%s",oline);
        }
      
      /* -------------------------------------------------*/
      fprintf(stderr,"\n");
      for(i=j;i--;)
        fprintf(stderr,"-");
      fprintf(stderr,"\n\n");
      CHECK(close_cursor("CURSOR"));
      CHECK(deallocate_prepare(b));
    error:
      SQL__deallocate_descr(&in);
      SQL__deallocate_descr(&out);
      if (clms)
        xfree(clms);
      clms = NULL;
      if (b)
        xfree(b);
      b = NULL;
    }
      
  _SQL_commit();
  return 0;
}
