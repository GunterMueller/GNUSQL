/*
 * node.c  - this module contains functions those generate C-code for
 *           tree generating.
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Software
 *  This file is written by Andrew Yahin.
 *  Modified by Michael Kimelman.
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

/* $Id: node.c,v 1.246 1997/03/31 11:03:38 kml Exp $ */

#include <stdarg.h>
#include <string.h>
#include "kitty.h"

extern FILE *yout;
static void  emit_mknode_parms  __P(( TXTREF t,i4_t look_to_the_right));
static i4_t   indent = 2;
static char *spaces = "                                               "; 

#define TYPE_INDENT fprintf(yout,"\n\t%s",spaces+strlen(spaces)-indent)

#define is_tree(s) \
        ((CODE_TRN(s)<RULE)||(CODE_TRN(s)>SKIP_CODES)||(TstF_TRN(s,PATTERN_F)))
#define is_production(s) \
        ((CODE_TRN(s)!=TRLcmd_REMOVE)&&(CODE_TRN(s)!=TRLcmd_CCODE)\
	 &&(CODE_TRN(s)!=TRLcmd_SWITCH))

static void
strupr(char *s)
{
  for(;*s;s++)
    if( (*s>='a') && (*s<='z'))
      *s-='a'-'A';
  return;
}

static void 
type_header(TXTREF s)
{
  enum token tmp_code;
  MASKTYPE   msk ;
  char       cn[100];
  
  tmp_code=CODE_TRN(s);
  msk = MASK_TRN(s);
  if (tmp_code == UNKNOWN)
    yyfatal("Unknown trn read");
  if (tmp_code == NIL_CODE)
    return ;
  if(!TstF(msk,PATTERN_F))
    {
      i4_t n=OPs_IND(s);
      if(n>=0)
	{
	  fprintf(yout,"Op[%d]=",n);
	  msk CLRVL (n+1);
	}
    }    
  MASK_TRN(s) = msk;
  ClrF(msk,ACTION_F);
  fputs("mk_node(",yout);
  switch(tmp_code)
    {
    case OPERAND:
      strcpy(cn,"OPERAND");
      break;
    default:
      strcpy(cn,NAME(tmp_code));
      strupr(cn);
    }
  fprintf(yout,"%s,%d,",cn,(u4_t)msk);
} 

void
emit_trn_constructor(TXTREF s,i4_t look_to_the_right)
{
  static i4_t lrop=0;
  i4_t i;
  char *rl_cor_str;
  if(s==0)
    {
      fputs("TNULL",yout);
      return;
    }

  if(is_tree(s))
    {
      TYPE_INDENT;  indent+=4;
      type_header(s);
      emit_mknode_parms(s,look_to_the_right);
      return;
    }
  if(TstF_TRN(s,LIST_F))
    rl_cor_str="rl_corrector(1,";
  else
    rl_cor_str="rl_corrector(0,";
  i=OPs_IND(s);
  if(CODE_TRN(s)==OPERAND)
    {
      if(i>=0)
	{
	  register i4_t ind=0;
	  if(look_to_the_right)
	    {
	      TYPE_INDENT;  indent+=4;
	      ind++;
	      fputs(rl_cor_str,yout);
	    }
	  if(DOWN_TRN(s))
	    {
	      if(ind==0)
		{ TYPE_INDENT;  indent+=4; }
	      ind++;
	      if(TstF_TRN(s,LIST_F))
		fputs("dl_corrector(1,",yout);
	      else
		fputs("dl_corrector(0,",yout);
	    }
	  fprintf(yout,"Op[%d]",i);
	  if(DOWN_TRN(s))
	    {
	      fputs(",",yout);
	      emit_trn_constructor(DOWN_TRN(s),1);
	      fputs(")",yout);
	    }
	  if(ind)
	    indent-=4;
	  if(look_to_the_right)
	    {
	      fputs(",",yout);
	      emit_trn_constructor(RIGHT_TRN(s),1);
	      fputs(")",yout);
	    }
	}
      else
	trl_err("Unexpected operand expression",__FILE__,__LINE__,
		Ptree(s));
      return;
    }


  TYPE_INDENT;  indent+=4;
  if(lrop)
    {
      lrop=0;
      fputs("Rop=",yout);
    }
      
  if(i>=0)
    fprintf(yout,"Op[%d]=",i);
  if(look_to_the_right)
    fputs(rl_cor_str,yout);
  switch(CODE_TRN(s))
    {
    case TRLcmd_CCODE :
      fputs(STRING(XLTR_TRN(s,0)),yout);
      indent-=4;
      if(look_to_the_right)
	{
	  fputs(",",yout);
	  emit_trn_constructor(RIGHT_TRN(s),look_to_the_right);
	  fputs(")",yout);
	}
      return; 
    case TRLcmd_DELETE :
      fprintf(yout,"del_op(");
      break;
    case TRLcmd_DOWN   :
      fprintf(yout,"DOWN_TRN(");
      break;
    case TRLcmd_SET    :
      fprintf(yout,"(");
      break;
    case TRLcmd_RIGHT  :
      fprintf(yout,"RIGHT_TRN(");
      break;
    case TRLcmd_COPY   :
      fprintf(yout,"copy_tree(");
      break;
    case TRLcmd_IMAGE  :
      fprintf(yout,"image(");
      break;
    case TRLcmd_INSERT :
      fprintf(yout,"insert_node(%d,",(TstF_TRN(s,BEFORE_F))?1:0);
      break;
    case TRLcmd_REPLACE:
      fprintf(yout,"replace_node(");
      break;
    case TRLcmd_REMOVE :
      if(TstF_TRN(s,TREE_F))
	fputs("free_tree(",yout);
      else
	fputs("free_node(",yout);
      break;
    case TRLcmd_SWITCH :
      {
	TXTREF ss=DOWN_TRN(s);
	fputs("( ( { i4_t kitty_lcc=0;",yout);
	if(XTXT_TRN(s,0))
	  {
	    TYPE_INDENT;
	    fputs("Rop=",yout);
	    indent+=4;
	    emit_trn_constructor(XTXT_TRN(s,0),0);
	    fputs(";",yout);
	    indent-=4;
	  }
	TYPE_INDENT;
	fputs("while(1)",yout);
	indent+=2;
	TYPE_INDENT;
	fputs("{",yout);
	indent+=2;
	TYPE_INDENT;
	fputs("kitty_lcc=0;",yout);
	while (ss)
	  { 
	    if(is_production(ss))
	      lrop=1;
	    if(CODE_TRN(ss)==TRLcmd_CASE)
	      {
		TYPE_INDENT;
		fprintf(yout,"Rop=rule_%s(&kitty_lcc,Rop)",
			STRING(XLTR_TRN(ss,0)));
	      }
	    else
	      emit_trn_constructor(ss,0);
	    fprintf(yout,";");
	    TYPE_INDENT;
	    /* if(CODE_TRN(ss)==RUN_RULE) */
            if(TstF_TRN(s,CYCLE_F))
              fputs("if(kitty_lcc) continue;",yout);
            else
              fputs("if(kitty_lcc) break;",yout);
	    ss=RIGHT_TRN(ss);
	  }
        if( ! TstF_TRN(s,CYCLE_F) )
	  fputs(
             /* "yyfatal(\"Unrecognizable SWITCH selector\");" */
	     /* "yyerror(\"Unrecognizable SWITCH selector\");" */
	        "puts(\"Unrecognizable SWITCH selector\")   ;"
	      ,yout);
        fputs("break;",yout);
	indent-=2;
	TYPE_INDENT;
	fputs("}",yout);
	indent-=4;
	TYPE_INDENT;
	fputs("} ), Rop)",yout);
	if(look_to_the_right)
	  {
	    fputs(",",yout);
	    emit_trn_constructor(RIGHT_TRN(s),look_to_the_right);
	    fputs(")",yout);
	  }
	return;
      }
    case RUN_RULE:
      fprintf(yout,"rule_%s(&kitty_lcc,",STRING(XLTR_TRN(s,0)));
      emit_trn_constructor(XTXT_TRN(s,2),0);
      fputs(")",yout);
      indent-=4;
      if(look_to_the_right)
	{
	  fputs(",",yout);
	  emit_trn_constructor(RIGHT_TRN(s),look_to_the_right);
	  fputs(")",yout);
	}
      return;
    default:
      trl_wrn("Unexpected node here",__FILE__,__LINE__,Ptree(s));
      type_header(s);
    }
  emit_mknode_parms(s,look_to_the_right);
}

#define CHECK_COMMA {if (need_comma) fputs(",",yout); else need_comma=1;}
#define IS_SET(n)  TST_BIT(XLNG_TRN(t,PTRN_OP(tmp_code,(n))),PTRN_BIT((n)))

static void 
emit_mknode_parms( TXTREF t,i4_t look_to_the_right )
{
  enum token tmp_code;
  char      *format_ptr;
  i4_t        right_ptr=0;
  i4_t        down_ptr=0;
  i4_t        need_comma=0;
  i4_t        i,act,shift;
  
  tmp_code   = CODE_TRN(t);
  format_ptr = FORMAT (tmp_code);
  act = TstF_TRN(t,ACTION_F);
  shift = (TstF_TRN(t,PATTERN_F) ? LENGTH(tmp_code) : 0);
  
  for (i = 0; i < LENGTH (tmp_code); i++)
    {
      i4_t subst = 0;
      char *transform = "SIMPLE2L";
      switch(format_ptr[i]) /* step for substitution C expr for atributes */
	{
	case '0': case 'r': case 'a': case 'd':	case 'V': 
	  break;
	case 'y':
	  transform = "type2long";
	default:
	  if (act && IS_SET (i + shift) )
	    {
	      CHECK_COMMA;
	      fprintf(yout,"%s(%s)",transform,STRING(XLTR_TRN(t, i)));
	      subst = 1;
	    }
	}
      if (subst)
	continue;
      switch (format_ptr[i])
	{
	case '0':break;
	case 'r':
	  right_ptr=1;
	  break;
	case 'a':
	  break;
	case 'd':
	  down_ptr=1;
	  break;
	
	case 'V': /* special vocabulary reference (backward link) */
	  break;
	case 't':
	  CHECK_COMMA;
	  emit_trn_constructor(XTXT_TRN(t,i),0);
	  break;
	case 'N':
	  CHECK_COMMA;
	  emit_trn_constructor(XTXT_TRN(t,i),1);
	  break;
	case 'v':
	  /*	  emit_trn_constructor(XTXT_TRN(t, i),0);*/
	  CHECK_COMMA;
	  fputs("0",yout);
	  break;
	
	case 'S':
	  /* 'S' is an optional string: if a closeparen follows,
	     just store NULL for this element.  */
	
	case 's':
	  {
	    CHECK_COMMA;
	    if((tmp_code==OPERAND) && (TstF_TRN(t,PATTERN_F)))
	      fprintf(yout,"%d",XLNG_TRN(t,i));
	    else  if(is_tree(t))
	      {
		char *ptr=STRING(XLTR_TRN(t, i));
		if(ptr && *ptr)
		  fprintf(yout,"ltr_rec(\"%s\")",ptr);
		else
		  fputs("TNULL",yout);
	      }
	  }
	  break;
	case 'p':
	case 'y':
	case 'i':
	case 'x':
	case 'l':
	case 'R':
	  CHECK_COMMA;
	  fprintf(yout,"%d",XLNG_TRN(t,i));
	  break;
	case 'f':
	  CHECK_COMMA;
	  fprintf(yout,"%g",XFLT_TRN(t, i));
	  break;
	case 'T':
	case 'L':
	  CHECK_COMMA;
	  if(XTXT_TRN(t,i))
	    {
	      TXTREF v = XVEC_TRN(t,i);
	      i4_t    l = VLEN(v),n;
	    
	      fprintf(yout,"mk_vect(%d,%d",(format_ptr[i]=='T'?1:0),l);
	      for(n=0;n<l;n++)
		{
		  fputs(",",yout); 
		  if(format_ptr[i]=='T')
		    emit_trn_constructor(VOP(v,n).txtp,0);
		  else
		    fprintf(yout,"%d",VOP(v,n).l);
		}
	      fputs(")",yout);
	    }
	  else
	    fputs("TNULL",yout);
	  break;
	
	default:
	  fprintf (stderr,
		   "switch format wrong in emit_mknode_parm(). format was: '%c'.\n",
		   format_ptr[i]);
	  yyfatal("Abort");
	}
    }  
  /* print null attribute information */
  if (shift)
    for(i = LENGTH(CODE_TRN(t));
	i < LENGTH(CODE_TRN(t)) + PTRN_ELEM(CODE_TRN(t));
	i++)
      {
	CHECK_COMMA;
	fprintf(yout,"%d",XLNG_TRN(t,i));
      }
  
  indent-=2;
  if(down_ptr)
    {
      CHECK_COMMA;
      emit_trn_constructor(DOWN_TRN(t),1);
    }
  indent-=2;
  if(right_ptr)
    if(look_to_the_right)
      {
	if(!is_tree(t))
	  fputs(")",yout);
	CHECK_COMMA;
	emit_trn_constructor(RIGHT_TRN(t),1);
      }
    else if(is_tree(t))
      fputs(",TNULL",yout);
  fprintf(yout,")");
  return;
}

