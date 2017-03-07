/*
 *  kitty.c -  tree recognizer and modification procedure generator
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Andrew Yahin.
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

/* $Id: kitty.c,v 1.253 1998/09/30 02:39:06 kimelman Exp $ */

#include <assert.h>
#include "kitty.h"
#include "cycler.h"

#define TEST_VAR(s)      STRING(XLTR_TRN(s,0))
#define BUFFER_VAR(s)     STRING(XLTR_TRN(s,3))

#define  RULENAME(fl,str,r) \
  fprintf(fl,"\n%sTXTREF\nrule_%s(i4_t *done,TXTREF in_tree)",str,RULE_NAME(r))

FILE *yout = NULL, *yhout = NULL;


static i4_t   line_no = 0;
static FILE *y1out; /* stream to emit out rule_checker functions */

static int
skip_c_code(FILE *infile)
{
  register i4_t c;
  if(yout)
    fprintf(yout,"\n#line %d \"%s.k\"\n", line_no+1,progname);
  while ((c = getc (infile)) != EOF)
    {
      if (c == '/')
	{
	  register i4_t prevc;
	  
	  putc(c,yout);
	  c = getc (infile);
	  if (c=='*') /* comment begins */
	    {
	      putc(c,yout);
              prevc = 0;
	      while ((c = getc (infile)) != EOF)
		{
		  if ( c=='/' && prevc=='*')
		    break;
		  if (c=='\n')
		    line_no ++;
		  putc(c,yout);
		  prevc = c;
		}
	    }
	}
      if (c == '#')
        {
          c = getc (infile);
          ungetc(c,infile);
          if ( c == '(')
            return c;
          c = '#';
        }
      if (c == '\n')
        {
#define BUF_SIZE 1024
          char buffer[BUF_SIZE], *buf_ptr=buffer;
          i4_t  passed_d = 0;
          
          line_no ++;
          putc(c,yout);
          while ((c = getc (infile)) != EOF)
            {
              *buf_ptr++ = c;
              if (buf_ptr - buffer >=BUF_SIZE/2)
                break;
              if (c == '#' && !passed_d)
                passed_d = 1;
              else if ( c == '(' && passed_d)
                {
                  ungetc(c,infile);
                  return c;
                }
              else if ( c != '\t' && c != ' ')
                break;
            }
#undef BUF_SIZE
          if (c != EOF)
            {
              *(--buf_ptr)=0;
              fputs(buffer,yout);
              ungetc(c,infile);
              continue;
            }
        }
      if (c == EOF)
	return c;
      putc(c,yout);
    }
  return EOF;
}

static TXTREF
get_next_tree(i4_t is_preprocessor, FILE *infile)
{
  if (is_preprocessor)
      if(skip_c_code(infile)==EOF)
	return TNULL;
  if(LOCAL_VCB_ROOT)
    free_line(LOCAL_VCB_ROOT);
  LOCAL_VCB_ROOT = TNULL;
  return read_trn(infile,&line_no,FLAG_VALUE(ACTION_F));
}

static TXTREF
cook_switch_list(TXTREF n, i4_t f)
{
  typedef struct s1 
  {
    LTRLREF    sel; 
    struct s1 *next;
  } check_list_t;
  static check_list_t  *switch_list;
  
  register check_list_t *cp;
  register LTRLREF v ; 
  
  if ( f==0)  /* init loop */
    {
      switch_list = NULL;
      cycler (n, cook_switch_list, CYCLER_LD); /* go walk */
      while(switch_list)
	{
	  cp = switch_list;
	  switch_list = switch_list->next;
	  xfree(cp);
	}
    }
  else if ( f==CYCLER_LD )  /* inside cycler loop */
    {
      if (CODE_TRN (n) != OPERAND)
	return n;
      
      v = XLTR_TRN (n, 0);
      if (!v)
	return n;
      
      for (cp = switch_list; cp; cp = cp->next)
	if (cp->sel == v)
	  return n;		/* everything has already done */
      fprintf (y1out, "    case %d : return %s(node);\n",
	       (i4_t) v, STRING (v));
      /* add new node in list */
      cp=(check_list_t *) xmalloc (sizeof (check_list_t));
      cp->next = switch_list;
      cp->sel = v;
      switch_list = cp;
    }
  else
    yyfatal("'switch_list_cook' MUST NOT be here");
  return n;
}

static void 
make_rule_checker (TXTREF pattern,char *title)
/* this routine generate function rule_Rname_checker as a alias for
   Op:n "test_function_or_macro_name"). Actually this function
   must look like :
   static int
   rule_rname_checker(TXTREF node,i4_t sel)
   {
     switch(sel){
     case 1:  return test_function_name1(node);
     case 34: return test_function_name2(node);
     }
     -- default:
     fprintf(stderr,"horrible internal fatal error");
     return 0;
   }
 */
{
  assert(y1out!=NULL);
  fprintf (y1out, 
	   "\n"
	   "static int\n"
	   "rule_%s_checker(TXTREF node,i4_t sel)\n"
	   "{\n"                  
           "  switch(sel){\n"
           ,title);
  cook_switch_list(pattern,0);
  fprintf (y1out,
	   "  }\n"
	   "  fprintf(stderr,\n"
	   "          "
	"\"Internal error: unexpected selector value %%x in rule '%%s'\",\n"
	   "          sel,\"%s\");\n"
           "  yyfatal(\"Internal error\");\n"
           "  return 0;\n"
           "}\n"
           , title);
}

static TXTREF
emit_lcc(TXTREF n,i4_t f)
{
  static i4_t need_lcc;
  if(f==0)
    {
      need_lcc = 0;
      cycler(n,emit_lcc,CYCLER_LD);
      if (need_lcc)
	fputs("  i4_t kitty_lcc;\n"
              , yout);
    }
  else if (f==CYCLER_LD)
    {
      switch(CODE_TRN(n))
	{
	case TRLcmd_SWITCH:
	  cycler_skip_subtree = 1;
	  break;
	case RUN_RULE:
	  need_lcc = 1 ;
	default:
          break;
	}
      if(need_lcc)
	cycler_skip_subtree = 1;
    }
  else
    yyfatal("Emit_lcc should not be here");
  return n;
}

static TXTREF
emit_rule1(TXTREF n,i4_t f)
{
  static i4_t need_rule1;
  if(f==0)
    {
      need_rule1 = 0;
      cycler(RULE_PATTERN (n),emit_rule1,CYCLER_LD);
      return need_rule1;
    }
  else if (f==CYCLER_LD)
    {
      switch(CODE_TRN(n))
	{
	case OPERAND:
          if (XLTR_TRN(n,0))
            need_rule1 = 1;
	  break;
	default:
          break;
	}
      if(need_rule1)
	cycler_skip_subtree = 1;
    }
  else
    yyfatal("Emit_rule1 should not be here");
  return n;
}

static void
make_rule (TXTREF r)
{
  char *str;
  char rule1_name[100];
  TXTREF action;

  if ( emit_rule1(r,0) )
    {
      make_rule_checker(RULE_PATTERN (r), RULE_NAME(r));
      sprintf(rule1_name,"rule_%s_checker",RULE_NAME(r));
    }
  else
    strcpy(rule1_name,"NULL");
  
  RULENAME (yout, "", r);
  
  fputs ( "\n{\n"
	  "  static TXTREF pat=TNULL;\n"
	  "  TXTREF Op[15],Rop=in_tree;\n"
	  ,yout);
  emit_lcc(r, 0);
  fputs(  "  \n"
	  "  if(!pat){\n"
	  "    pat=\n"
	  "       ", yout);
  
  emit_trn_constructor (RULE_PATTERN (r), 0); /* emit pattern tree */
  fprintf (yout, ";\n"                        
	   "    register_pattern(&pat);\n"
	   "  }\n"
	   "  if (!t_compare(pat,in_tree,Op,%s))\n" /* check pattern */
	   "     FAIL;\n",rule1_name);
  str = STRING (RULE_STR (r));
  if (RULE_STR (r))
    fprintf (yout, "  if(!(%s))FAIL;\n", str);  /* additional check */

  /* ACTIONS */

  for (action = DOWN_TRN (r); action; action = RIGHT_TRN (action))
    {
      TXTREF t = action;
      i4_t n = OPs_IND (t);
      switch (CODE_TRN (t))
	{
	case TRLcmd_CCODE:
	case TRLcmd_REMOVE:
	  if (n >= 0)
	    yyerror ("Unexpected return specification in ccode or Remove node");
	  break;
	default:
	  if ((OPs_IND (t) < 0) || (CODE_TRN (t) == OPERAND))
	    fprintf (yout, "\n\tRop=");
	  break;
	}
      emit_trn_constructor (t, 0);
      if (CODE_TRN (t) != TRLcmd_CCODE)
	fputs (";\n", yout);
    }
  fputs (
	  "\n"
	  "  goto SUCCESS_EXIT;\n"
	  " FATAL_EXIT:\n"
	  "  *done=0;\n"
	  "  return in_tree;\n"
	  " SUCCESS_EXIT:\n"
	  "  *done=1;\n"
	  "  return Rop;\n"
	  "}\n", yout);
  fprintf (yout, "/* end of %s */\n", RULE_NAME (r));
}

int 
main (int argc, char **argv)
{
  TXTREF r;
  char  *result = NULL;
  char   wf_name[100];
  FILE  *infile=NULL;
  i4_t    preprocessor_mode = 0;
  
#if 0
  extern i4_t debug_vmemory;
  
  debug_vmemory = 1; 
#endif
  
  yout = yhout = stdout;
  
  if (argc < 2)
    yyfatal("Usage: kitty pattern_file_name");
  
  if ((infile = fopen (argv[1], "r")) == NULL)
    {
      fprintf (STDERR, "Can't open '%s'\n", argv[1]);
      exit (FATAL_EXIT_CODE);
    }
  
  result = argv[1] + strlen (argv[1]);
  while ((result > argv[1]) && (*result != '.') && (*result != '/'))
    result--;
  preprocessor_mode = strcmp(result,".k")==0;
  if (*result == '.')
    *result = 0;
  while ((result > argv[1]) && (*result != '/'))
    result--;
  if (*result == '/')
    result++;
  strcpy (wf_name, result);
  strcat (wf_name, ".c");
  if ((yout = fopen (wf_name, "w")) == NULL)
    yyfatal ("Can't open output file");
  strcpy (wf_name, result);
  strcat (wf_name, ".h");
  if ((yhout = fopen (wf_name, "w")) == NULL)
    yyfatal ("Can't open output file");
  y1out = preprocessor_mode? yhout : yout;
 
  progname = result;
  fprintf (yout,
	   "/* \n"
           " *  %s.c - generated by kitty \n"
           " *  \n"
	   " *  DON'T EDIT THIS FILE      \n"
	   " */\n"
           "\n"
	   "#define  __%s_C__\n"
	   "#include  \"%s.h\"\n"
	   "#undef   __%s_C__\n"
           "\n"
	   "#define FAIL    goto FATAL_EXIT \n"
	   "#define RETURN  goto SUCCESS_EXIT \n"
	   "#define RET(v)  { Rop=v;RETURN;} \n"
           "\n",
	   result, result,result,result);
  fprintf (yhout,
	   "/*\n"
           " *  %s.h - generated by kitty \n"
	   " *  DON'T EDIT THIS FILE \n"
	   " */\n"
           "\n"
	   "#ifndef __%s_H__\n"
	   "#define __%s_H__\n"
	   "#include \"kitty.h\"\n"
	   ,result,result,result);

  while ((r = get_next_tree(preprocessor_mode,infile)) != TNULL)
    {
      switch (CODE_TRN (r))
	{
	case RULE:
	  {
	    i4_t static_rule = TstF_TRN (r, STATIC_F);
	    
	    if (static_rule)
	      fprintf(yhout,"\n#ifdef __%s_C__\n",result);
	    RULENAME (yhout, static_rule? "static ":"", r);
	    fputs (";\n", yhout);
	    if (static_rule)
	      fputs("#endif\n",yhout);
	  }
	  
	  fprintf (yout, "/* ---------- Rule '%s' ------------ */\n",
		   RULE_NAME (r));
	  y1out = yout;
	  make_rule (r);
	  break;
	  
	case TRLcmd_CCODE:
	  fputs (STRING (XLTR_TRN (r, 0)), yout);
	  break;
          
	case IF:
	  {
	    static i4_t uid = 0;
	    char       nm[100],rule1_nm[100];
	    sprintf(nm,"%s_if_%d",result,uid++);
            
	    y1out = yhout;

            if (emit_rule1(RULE_PATTERN(r),0))
              {
                fprintf (yhout,"#ifdef __%s_C__\n",result);
                make_rule_checker(RULE_PATTERN(r),nm);
                fprintf (yhout,"#endif\n");
                sprintf(rule1_nm,"rule_%s_checker,",nm);
              }
            else
              strcpy(rule1_nm,"NULL");
            
	    fprintf ( yout, "  if (!t_compare(ANONYM_P(\"%s\",",nm);
	    emit_trn_constructor(RULE_PATTERN(r),0);
	    fprintf ( yout, "),%s,%s,%s);\n"
		      "          break;\n",
		      TEST_VAR(r),BUFFER_VAR(r),rule1_nm);
	    break;
	  }
          
	default:
	  if(preprocessor_mode)
	    emit_trn_constructor(r,0);
	  else
	    fprintf (stderr,
		     "Unexpected statement node '%s' in rule program '%s'\n",
		     NAME (CODE_TRN (r)), result);
	}
      
      free_tree(r);
      if(LOCAL_VCB_ROOT)
	free_line(LOCAL_VCB_ROOT);
      LOCAL_VCB_ROOT = TNULL;
    }
  
  fprintf (yhout, 
	   "\n\n"
	   "#endif /*  __%s_H__ */\n"
	   ,result);
  fclose(infile);
  fclose (yout);
  fclose (yhout);
  finish (NULL);
  return (errors ? FATAL_EXIT_CODE : SUCCESS_EXIT_CODE);
}
