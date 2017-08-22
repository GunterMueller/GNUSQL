%{
/*
 *  a2k_p1.l - ansi 2 knr processing
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

#include <assert.h>

/* $Id: a2k_p1.l,v 1.245 1997/03/31 03:46:38 kml Exp $ */

%}

%x C_str C_str1 Skip

WS              [ \t\f]+
NNS             [^A-Za-z0-9_]
OWS             [ \t\f]*
NOT_WS          [^ \t\f\n]
LET             [A-Za-z]
NAME            [A-Za-z_][A-Za-z_0-9]*
COMMENT         "/*"([^/*]+|[*]+[^/*]|"/"+)*[*]+"/"
EWS             ({WS}|\n|{COMMENT})*

%%
    
  /*********************************************
   *                C lexics                   *
   *********************************************/
{COMMENT}       ECHO;
\"              {
                          ECHO;
			  BEGIN(C_str);
		}
.|\n            ECHO;
"##"            fputs("/**/",yyout);  /* ## --> comments in macro */
{OWS}","{OWS}   fputs(",",yyout);     /* condence spaces in calls */
"("{OWS}        fputs("(",yyout);    /*        ---- // ----      */
{OWS}")"        fputs(")",yyout);    /*          ---- // ----    */
                /* adjust cpp directives    */ 
^{OWS}"#if"       fputs("#if",yyout);     
^{OWS}"#ifdef"    fputs("#ifdef",yyout);  
^{OWS}"#else"     fputs("#else",yyout);   
^{OWS}"#endif"    fputs("#endif",yyout);  
^{OWS}"#define"   fputs("#define",yyout);  
^{OWS}"#undef"    fputs("#undef",yyout);  
^{OWS}"#include"  { fputs("#include",yyout); BEGIN(Skip); }
^{OWS}"#pragma"   { fputs("#pragma",yyout);  BEGIN(Skip); }
^{OWS}"#line"     { fputs("#line",yyout);    BEGIN(Skip); }
^{OWS}"#"         fputs("#",yyout);  
^{OWS}"#"{NAME}  { fprintf(yyout,"\"%s",yytext+1); BEGIN(C_str); }
<Skip>.*\n        { ECHO; BEGIN(INITIAL); }
  /*********************************************
   *               C string recognizer         *
   *********************************************/
<C_str>\\\"              { ECHO; }
<C_str>\\\n              { ECHO; }
<C_str>\"                { BEGIN(C_str1); }
<C_str>\\?[^\"\n]         { ECHO; }
<C_str1>{WS}             ; /* waiting */
<C_str1>\n               ; /* waiting */
<C_str1>{COMMENT}        ; /* waiting */
<C_str1>\"               { BEGIN(C_str);  } /* clip strings */
<C_str1>^{OWS}"#if"       { fputs("\"\n#if",yyout);       BEGIN(INITIAL);  }
<C_str1>^{OWS}"#ifdef"    { fputs("\"\n#ifdef",yyout);    BEGIN(INITIAL);  }
<C_str1>^{OWS}"#else"     { fputs("\"\n#else",yyout);     BEGIN(INITIAL);  }
<C_str1>^{OWS}"#endif"    { fputs("\"\n#endif",yyout);    BEGIN(INITIAL);  }
<C_str1>^{OWS}"#define"   { fputs("\"\n#define",yyout);    BEGIN(INITIAL);  }
<C_str1>^{OWS}"#undef"    { fputs("\"\n#undef",yyout);    BEGIN(INITIAL);  }
<C_str1>^{OWS}"#include"  { fputs("\"\n#include",yyout);  BEGIN(Skip);  }
<C_str1>^{OWS}"#pragma"   { fputs("\"\n#pragma",yyout);   BEGIN(Skip);  }
<C_str1>^{OWS}"#line"     { fputs("\"\n#line",yyout);     BEGIN(Skip);  }
<C_str1>^{OWS}"#"         { fputs("\"\n#",yyout);         BEGIN(INITIAL);  }
<C_str1>"#"{NAME}          fputs(yytext+1,yyout); /* threw away # and leave name inside string */
<C_str1>,{OWS}           { fputs("\",",yyout); BEGIN(INITIAL);  }
<C_str1>.                { fprintf(yyout,"\"%s",yytext); BEGIN(INITIAL);  }
%%


int
yywrap()
{
  return 1;
}

int
main (argc,argv)
     i4_t argc;
     char **argv;
{
  if ( argc != 3 )
    {
      fprintf("Usage: %s input_file output_file\n",argv[0]);
      return 1;
    }
  yy_flex_debug = 0;
  yyin = fopen(argv[1],"r");
  assert( yyin != NULL);
  yyout = fopen(argv[2],"w");
  assert(yyout != NULL);
  yylex();
  fclose(yyin);
  fclose(yyout);
  return 0;
}
