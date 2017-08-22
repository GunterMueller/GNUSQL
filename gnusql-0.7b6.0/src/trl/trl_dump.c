/*
 *  trl_dump.c  - support library for dumping and reading tree of 
 *                GNU SQL compiler
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
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
 *  Contacts: gss@ispras.ru
 *
 */

/* $Id: trl_dump.c,v 1.246 1997/03/31 11:01:48 kml Exp $ */

#include <assert.h>
#include "trl.h"
#include "trlinter.h"
#include "tree_gen.h"
#include "type_lib.h"

/*******==================================================*******\
*******      Printing trn for debugging dumps.             *******
\*******==================================================*******/
static i4_t vcb_dump_in_progress = 0;
static FILE *outfl = NULL;
char spaces[]=
"                                                                            "
"                                                                            "
"                                                                            "
"                                                                            "
 ;

static i4_t sawclose = 0;
static i4_t backlay = 0;
static i4_t just_for_info = 0;

#define PRINT_INDENT fprintf (outfl, "\n%s%s", (just_for_info?";;":""),\
                              spaces + sizeof spaces - indent*2)
#define INC_INDENT indent+=1
#define DEC_INDENT indent-=1

/* Print IN_TRN onto OUTFL.  This is the recursive part of printing.  */
static void
print_trn (trn in_trn, char type_mode,i4_t go_right)
{

  static i4_t indent = 1;
  static i4_t clr_pattern = 0;
  static i4_t check_integrity = 1;
  register enum token code;
  register i4_t i, j, sc, type_vcb_subtree = 1;
  register char *format_ptr;
  TXTREF   lvr = TNULL;
  TXTREF parm;
  i4_t printback, jfi = 0;
  trn in_trn1 = in_trn;
  printback = 0;
  if (type_mode == 'P')
    type_mode = 't';
  if (indent < 0)
    indent = 0;

  sc = 0;
  if (in_trn)
    code = in_trn->code;
  else
    code = NIL_CODE;
  if (code <= UNKNOWN || code >= LAST_TOKEN)
    {
      sc = code;
      code = UNKNOWN;
    }
  if (code == SKIP_CODE)
    {
      fputs (" - ", outfl);
      goto GO_RIGHT;
    }
  else if (code == SKIP_CODES)
    {
      fputs (" -- ", outfl);
      goto GO_RIGHT;
    }
  /* print name of expression code */

  if (code <= NIL_CODE)
    {
      fprintf (outfl, " (%s)", NAME (code));
      if (sc)
        {
          fprintf (outfl, "  ;; !!!!! code value ==%d", sc);
          sawclose = 1;
        }
      return;
    }
  else if (Tstf_TRN (in_trn, MARK_F))
    {
      fprintf (outfl, " (%s:mark_f)", NAME (code));
      return; 
      /* goto GO_RIGHT; */
    }
  
  if (sawclose)
    {
      PRINT_INDENT;
      sawclose = 0;
    }
  else
    PRINT_INDENT;

  if (Is_statement(in_trn))
    {
      PRINT_INDENT;
      fprintf (outfl, ";;-------------------------------");
      PRINT_INDENT;
    }
#if 0
  fprintf (outfl, "(/*%p*/%s",in_trn, NAME (code));
#else
  fprintf (outfl, "(%s",NAME (code));
#endif

  /* print flags */
  i = MASk_TRN (in_trn) & (FLAG_VALUE (VCB_F) - 1);
  if (i)
    fprintf (outfl, ":%d", i - 1);
  for (i = VCB_F; i < LAST_FLAG; i++)
    if (compatible_tok_fl (code, (enum flags) i))
      if (Tstf_TRN (in_trn, (enum flags) i))
	if ((i != PATTERN_F) || (clr_pattern == 0))
	  fprintf (outfl, ":%s", NAME_FL ((enum flags) i));
  fputs ("  ", outfl);

  if (Is_statement(in_trn) || (code==VIEW))
    {
      register TXTREF v;
      lvr = LOCAL_VCB_ROOT;
#if 0
      v = XVCb_TRN (in_trn, 7);
      if ( v || go_right)
        LOCAL_VCB_ROOT = v;
#else
      LOCAL_VCB_ROOT = (code==VIEW?VIEw_VCB(in_trn):XVCb_TRN (in_trn, 7));
#endif
      for (v = LOCAL_VCB_ROOT; v; v = RIGHT_TRN (v))
	{
	  if (TstF_TRN(v,MARK1_F))
            {
              ClrF_TRN (v, MARK1_F);
              yyerror("? ?");
              errors--;
            }
	}
    }
  
  if (Is_table (in_trn) || (code==SCAN))
    {
      if (!Tstf_TRN (in_trn, MARK1_F))
        Setf_TRN (in_trn, MARK1_F);
      else /* if (code!=SCAN) */
        type_vcb_subtree = 0;
    }
  if (Tstf_TRN(in_trn,VCB_F) && check_integrity && (Is_table (in_trn) || (code==SCAN)))
    {
      TXTREF v;
      if ((code == SCAN) || (code==TMPTABLE))
        v = LOCAL_VCB_ROOT;
      else
        v = VCB_ROOT;
      for (; v; v = RIGHT_TRN(v))
        if ( Ptree(v) == in_trn )
          break;
      if (!v) /* IF node not found in vocabulary */
        {
          check_integrity = 0;
          trl_wrn("vcb node not in vcb list",__FILE__,__LINE__,in_trn);
        }
    }
  Setf_TRN (in_trn, MARK_F);

  format_ptr = FORMAT (code);

  for (i = 0; i < LENGTH (code); i++, format_ptr++)
    {
      
#define TST_PAT_BIT(n) TST_BIT(XLNg_TRN(in_trn,PTRN_OP(code,n)),PTRN_BIT(n))
      
      if ( Tstf_TRN(in_trn,ACTION_F) && TST_PAT_BIT(i) )
        {
          fprintf(outfl," <%s> ",STRING(XLTr_TRN(in_trn,i)));
          continue;
        }

      if ((!type_vcb_subtree) && (*format_ptr != 's') &&
          ((code!=SCAN) || (*format_ptr!='t') ))
        continue;
      
      if ((type_vcb_subtree) && (*format_ptr == 'V') &&
          (code==SCAN) && XTXt_TRN(in_trn,i))
        {
          just_for_info++; jfi = 1; /* type scolumn list */
          goto case_n;
        }
      
      switch (*format_ptr)
        {
        case 's':
        case 'S':
          sawclose = 0;
          if (XLTr_TRN (in_trn, i) == 0)
            fprintf (outfl, " \"\"");
          else
            {
              fprintf (outfl, " \"%s\"", STRING (XLTr_TRN (in_trn, i)));
              sawclose = 1;
            }
          break;

          /* 0 indicates a field for internal use that should not be printed.  */
        case '0':
        case 'r':
        case 'a':
        case 'd':
          break;

        case 'V':	/* special vocabulary reference (backward link) */
          if (backlay == 0)
            break;
          printback++;
          backlay--;
        case 't':
        case 'P':		/* pattern tree */
        case 'v':
          if ((type_mode == 't') || (*format_ptr == 't') || (*format_ptr == 'P'))
            {
              i4_t old_clr_pat = clr_pattern;
              INC_INDENT;
              if (!sawclose)
                fprintf (outfl, " ");
              if (*format_ptr == 'P')
                clr_pattern = 1;
              if (type_vcb_subtree)
                print_trn (Ptree (XTXt_TRN (in_trn, i)), *format_ptr,0);
              else
                fprintf (outfl, " (nil /**/)");
              DEC_INDENT;
              clr_pattern = old_clr_pat;
            }
          if (printback)
            {
              backlay++;
              printback--;
            }
          break;

        case 'p':
          if (!Tstf_TRN (in_trn, PATTERN_F))
            {
              fprintf (outfl, " line:%d", (i4_t) LOCATIOn_TRN (in_trn));
              sawclose = 0;
            }
          break;

        case 'y':
          {
            char s[20];
            conv_type (s, &XOp_TRN (in_trn, i).type, 0);
            fprintf (outfl, " %s", s);
            sawclose = 0;
          }
          break;

        case 'i':
          fprintf (outfl, " %d", (i4_t) XLNg_TRN (in_trn, i));
          sawclose = 0;
          break;

        case 'x':
          fprintf (outfl, " %x", XLNg_TRN (in_trn, i));
          sawclose = 0;
          break;

        case 'l':
          fprintf (outfl, " %d", XLNg_TRN (in_trn, i));
          sawclose = 0;
          break;

        case 'R':
          fprintf (outfl, " R:%d", XLNg_TRN (in_trn, i));
          sawclose = 0;
          break;

        case 'f':
          fprintf (outfl, " %f", XFLt_TRN (in_trn, i));
          sawclose = 0;
          break;

        case 'T':
        case 'L':
          INC_INDENT;
          if (TNULL != XVEc_TRN (in_trn, i) && type_vcb_subtree)
            {
              TXTREF vec = XVEc_TRN (in_trn, i);
              sc = 0;
              if (*format_ptr == 'T')
                if (VLEN (vec))
                  sc = 1;
              if (sawclose || sc)
                {
                  PRINT_INDENT;
                  sawclose = sc;
                }
              fprintf (outfl, " [");
              INC_INDENT;
              if (VLEN (vec) == 0)
                {
                  fprintf (outfl, "\n;; vector lenght =%d ", VLEN (vec));
                  PRINT_INDENT;
                }
              for (j = 0; j < VLEN (vec); j++)
                {
                  switch (*format_ptr)
                    {
                    case 'T':
                      print_trn (Ptree ((VOP (vec, j).txtp)), 't',0);
                      break;
                    case 'L':
                      fprintf (outfl, " %d", VOP (vec, j).l);
                      if (j % 5 == 4)
                        {
                          PRINT_INDENT;
                          sawclose = 1;
                        }
                      break;
                    default:
                      fprintf (outfl, "\n;; unexpected format");
                      PRINT_INDENT;
                      /**/ ;
                    }
                }
              DEC_INDENT;
              sawclose += sc;
              if (sawclose)
                PRINT_INDENT;
              if (sawclose)
                fprintf (outfl, "]");
              else
                fprintf (outfl, " ]");
            }
          else
            fprintf (outfl, " [] ");
          sawclose = 1;
          DEC_INDENT;
          break;
        case 'N':
          if (XTXt_TRN(in_trn,i)==TNULL || !type_vcb_subtree)
            fprintf (outfl," >><< ");
          else
            {
            case_n:
              PRINT_INDENT;
              INC_INDENT;
              sawclose = 1;
              fprintf (outfl," >> ");
              print_trn (Ptree (XTXt_TRN(in_trn, i)), 't',1);
              DEC_INDENT;
              if (sawclose)
                PRINT_INDENT;
              if (sawclose)
                fprintf (outfl, "<<");
              else
                fprintf (outfl, " <<");
              PRINT_INDENT;
              sawclose = 0;
            }
          break;
        default:
          fprintf (STDERR,
                   "Internal error %s:%d:(print_trn) format '%c' wrong\n",
                   __FILE__, __LINE__, *format_ptr);
          yyfatal (" Abort");
        }
      if(jfi)
        {
          just_for_info-=jfi;
          jfi = 0;
          PRINT_INDENT;
        }
    }
  /* print comparision pattern */
  if (Tstf_TRN (in_trn, PATTERN_F))
    {
      i4_t k;
      k = PTRN_ELEM (code);
      i = LENGTH (code) + k - 1;
      while (k--)
	fprintf (outfl, " %X", (u4_t) XLNg_TRN (in_trn, i - k));
    }
  /* print parameters */
  fputs (" ", outfl);
  if (HAS_DOWn (in_trn))
    {
      INC_INDENT;
      PRINT_INDENT; sawclose = 0; 
      fprintf (outfl, "{ ");
      INC_INDENT;
      if (ARITy_TRN (in_trn))
	sawclose = 1;

      for (i = ARITy_TRN (in_trn), parm = DOWn_TRN (in_trn);
	   i && parm;
	   parm = RIGHT_TRN (parm), i--);
      if (i || parm)
	{
	  fprintf (outfl, "\n;; node parameters number mismath ");
	  sawclose = 1;

          /* ARITY correcting */
          for (; parm; parm = RIGHT_TRN (parm))
            (ARITy_TRN (in_trn))++;
          ARITy_TRN (in_trn) -= i;
	}
      print_trn (Ptree (DOWn_TRN(in_trn)), 't',1);

      DEC_INDENT;
      if (sawclose)
	PRINT_INDENT;

      fprintf (outfl, "} ");
      sawclose = 1;
      DEC_INDENT;
    }

  fprintf (outfl, ")");
  sawclose = 1;
  if ( Is_statement(in_trn) || (code==VIEW))
    {
      register TXTREF v;
      vcb_dump_in_progress = 1;
      /* backlay=1; */
      if (dump_vocabulary)
	fprintf (outfl, "\n;; ==========<< Local Vocabulary >>===============");
      for (v = LOCAL_VCB_ROOT; v; v = RIGHT_TRN (v))
	{
	  if (TstF_TRN(v,MARK1_F))
	    ClrF_TRN (v, MARK1_F);
	}
      if (dump_vocabulary)
	{
	  fprintf (outfl, "\n");
	  print_trn (Ptree (XVCb_TRN (in_trn, 7)), 't',1);
	}
      /* backlay=0; */
      vcb_dump_in_progress = 0;
      if(code==VIEW)
        { assert(VIEw_VCB(in_trn) == LOCAL_VCB_ROOT); }
      else
        { assert(XVCb_TRN (in_trn, 7) == LOCAL_VCB_ROOT); }
      LOCAL_VCB_ROOT = lvr;
    }
  assert (in_trn == in_trn1);
GO_RIGHT:
  if (go_right && RIGHt_TRN(in_trn))
    print_trn (Ptree(RIGHt_TRN(in_trn)),type_mode,go_right);
  Clrf_TRN (in_trn, MARK_F);
#undef PRINT_INDENT
#undef INC_INDENT
#undef DEC_INDENT
}

/* Call this function from the debugger to see what X looks like.  */

void
toggle_mark1f(i4_t i)
{
  TXTREF v;
  for (v = VCB_ROOT; v; v = RIGHT_TRN (v))
    if (i)
      SetF_TRN (v, MARK1_F);
    else
      ClrF_TRN (v, MARK1_F);
  for (v = LOCAL_VCB_ROOT; v; v = RIGHT_TRN (v))
    if (i)
      SetF_TRN (v, MARK1_F);
    else
      ClrF_TRN (v, MARK1_F);
}

void
debug_trn_d (trn x)
{
  FILE *of;
  static i4_t prevent_loop = 0;
  if (prevent_loop)
    {
      fprintf (STDERR, ";; debug loop encountered --> BREAK \n");
      return; 
    }
  prevent_loop++;    
  vcb_dump_in_progress = 1;
  of = outfl;
  outfl = STDERR;
  /* backlay=1; */
  sawclose = 1;
  if (x)
    {
      MASKTYPE m = MASk_TRN (x);
      if (Tstf_TRN (x, MARK_F))
	{
	  Clrf_TRN (x, MARK_F);
	  fprintf (STDERR, ";; node below is marked \n");
	}
      print_trn (x, 't',0);
      MASk_TRN (x) = m;
    }
  else
    fprintf (STDERR, "(nil_code)\n");
  fprintf (STDERR, "\n");
  vcb_dump_in_progress = 0;
  backlay = 0;
  toggle_mark1f(0);
  outfl = of;
  prevent_loop--;
}

void
debug_trn (TXTREF x)
{
  debug_trn_d (Ptree (x));
#if 0
  if (!x)
    return;
  if ( Is_Statement(x))
    {
      TXTREF v,v1;
      fprintf(STDERR,"Local vocabulary ...\n");
      for (v1 = STMT_VCB(x); v1; v1 = RIGHT_TRN (v1))
        {
          debug_trn_d (Ptree (v1));
          for (v = STMT_VCB(x); v; v = RIGHT_TRN (v))
            if (TstF_TRN(v,MARK1_F))
              ClrF_TRN (v, MARK1_F);
        }
      fprintf(STDERR,"======================\n");
    }
#endif
}

/* External entry point for printing a chain of statements
 * starting with ROOT onto file OUTF.                    
 * A blank line separates insns.                         
 */

void
print_trl (TXTREF trn_first, FILE * outf)
{
  register TXTREF cur_stmt;

  outfl = outf;

  if (outfl == NULL)
    outfl = STDOUT;
  sawclose = 1;
  if (trn_first)
    print_trn (Ptree (trn_first), 't',0);
  else
    {
      print_trn (Ptree (ROOT), 't',1);
      fprintf (outfl, "\n\n;;--------------------------------------------\n\n");
      
      if (dump_vocabulary)
        {
          vcb_dump_in_progress = 1;
          /* backlay=1; */
          fprintf (outfl, ";; ==========<< Global Vocabulary >>===============");
          print_trn (Ptree (VCB_ROOT), 't',1);
          backlay = 0;
          vcb_dump_in_progress = 0;
        }
    }
  for (cur_stmt = VCB_ROOT; cur_stmt; cur_stmt = RIGHT_TRN (cur_stmt))
    if (TstF_TRN(cur_stmt,MARK1_F))
      ClrF_TRN (cur_stmt, MARK1_F);
  outfl = NULL;
}

/*******==================================================*******\
*******             Reading trn from file                  *******
\*******==================================================*******/

/* Subroutines of read_trn.  */
/* Dump code after printing a message.  Used when read_trn finds invalid
   data.  */

static FILE *infile = NULL;
static i4_t line_num = 0;

static void
ftail (void)
{
  i4_t c, i;

  fprintf (STDERR, "\nFollowing characters are:\n\t");
  for (i = 0; i < 200; i++)
    {
      c = getc (infile);
      if (EOF == c)
	break;
      putc (c, STDERR);
    }
  fprintf (STDERR, "Aborting.\n");
  abort ();
}

static void
dump_and_abort (i4_t expected_c, i4_t actual_c)
#define CHECK_CHAR(SYMBOL)  if(c!=SYMBOL) dump_and_abort(SYMBOL,c)
{
  fprintf (STDERR,
	   "Expected :\'%c\'. Read: \'%c\'. At file position:%ld",
	   expected_c, actual_c, ftell (infile));
  if (line_num)
    {
      fprintf (STDERR, " %s:%d:", progname ? progname : "", line_num);
      line_num = 0;
    }
  ftail ();
}

/* Read chars from INFILE until a non-whitespace char and return that.
   Comments, both Lisp style and C style, are treated as whitespace. Tools
   such as gen* use this function.  */

int
read_skip_spaces (void)
{
  register i4_t c;
  while ((c = getc (infile)) != EOF)
    {
      if (c == ' ' || c == '\t' || c == '\f')
	continue;
      else if (c == '\n')
	line_num++;
      else if (c == ';')
	{
	  while ((c = getc (infile)) != EOF && c != '\n');
	  if (c == '\n')
	    line_num++;
	}
      else if (c == '/')
	{
	  register i4_t prevc;
	  c = getc (infile);
	  CHECK_CHAR ('*');

	  prevc = 0;
	  while ((c = getc (infile)) != EOF)
	    {
	      if (prevc == '*' && c == '/')
		break;
	      prevc = c;
	    }
	}
      else
	break;
    }
  return c;
}



/* Read an trn code name into the buffer STR[]. It is terminated by any of
   the punctuation chars of trn printed syntax.  */

static void
read_name (char *str)
{
  register char *p;
  register i4_t c;

  c = read_skip_spaces ();

  p = str;
  while (1)
    {
      if (c == ' ' || c == '\n' || c == '\t' || c == '\f')
	break;
      if (c == ':' || c == '"' || c == '/' || c == '[' || c == ']'
	  || c == '(' || c == ')' || c == '{' || c == '}'
	)
	{
	  ungetc (c, infile);
	  break;
	}
      *p++ = c;
      c = getc (infile);
    }
  if (c == '\n')
    line_num++;
  *p = 0;
}

static LTRLREF
get_literal(char *terminators,i4_t *cp)
{
  i4_t saw_paren = 0;
  register i4_t j,c; 
  register char *buf;
  i4_t            bufsize;
  
#define EXIT(val)  { LTRLREF v=val; *cp = c; if(buf) xfree(buf); return v;}
  
  bufsize = 10;
  buf = (char *) xmalloc (bufsize + 1);
  c = getc (infile);
  if (c == '(' ) 
    {
      saw_paren = 1;
      c = read_skip_spaces ();
    }
  else if (c == '-')
    EXIT(TNULL);
  CHECK_CHAR (terminators[0]);
  j = 0;
  while (1)
    {
      if (j >= bufsize - 4)
	{
	  bufsize *= 2;
	  buf = (char *) xrealloc (buf, bufsize + 1);
	}
      buf[j] = getc (infile);	/* Read the string  */
      if (buf[j] == '\\')
	{
	  buf[j] = getc (infile);	/* Read the string  */
	  switch(buf[j])
	    {
	    case 'n': buf[j] = '\n'; break;
	    case 't': buf[j] = '\t'; break;
	    case 'b': buf[j] = '\b'; break;
	    case 'f': buf[j] = '\f'; break;
	    case 'r': buf[j] = '\r'; break;
	    default:               ; break;
	    }
	}
      else if (buf[j] == terminators[1])
	break;
      j++;
    }
  buf[j] = 0;	/* NUL terminate the string  */
  if (saw_paren)
    {
      c = read_skip_spaces ();
      CHECK_CHAR (')');
    }
  
  if (*buf)
    EXIT(ltr_rec (buf));
  EXIT(TNULL);
#undef EXIT
}

/*
 * Read an trn in printed representation from INFILE and return an actual trn
 * in core constructed accordingly. read_trn is not used in the compiler
 * proper, but rather in tools (kitty) and for debugging purpose
 */

static MASKTYPE default_mask = 0;

static TXTREF
read_trn_l (void)
{
  register i4_t i, j, list_counter;
  enum token tmp_code;
  register char *format_ptr;
  /* tmp_char is a buffer used for reading decimal integers and names of trn
     types and machine modes. Therefore, 256 must be enough.  */
  char tmp_char[256];
  TXTREF return_trn;
  MASKTYPE msk;
  i4_t c;
  i4_t minus = 0;

  c = read_skip_spaces ();	/* Should be open paren.  */
  if (c == EOF)
    return TNULL;
  return_trn = TNULL;
  if (c == '-')			/* special construction for omitted nodes '-'
				   and '--' */
    {
      while (1)
	{
	  c = getc (infile);
	  switch (c)
	    {
	    case '}':
	      ungetc (c, infile);
	    case ' ':
	    case '\n':
	    case '\t':
	    case '\f':
	    case '\r':
	      if (return_trn == TNULL)
		return gen_node1 (SKIP_CODE, default_mask);
	      return return_trn;
	    case '-':
	      if (return_trn)
		dump_and_abort (' ', '-');
	      else
		return_trn = gen_node1 (SKIP_CODES, default_mask);
	      break;
	    default:
	      dump_and_abort (' ', c);
	    }
	}
      /* unreachable code */
    }
  if (c == 'O')			/* looks like Op:n construction */
    {
      minus = 1;
      ungetc (c, infile);
    }
  else
    CHECK_CHAR ('(');

  read_name (tmp_char);

  /* fprintf(STDERR,"Name '%s' is read\n",tmp_char); */

  tmp_code = UNKNOWN;

  for (i = UNKNOWN; i < NUM_TRN_CODE; i++)	/* @@ might speed this search
						   up */
    {
      if (!strcmp (tmp_char, NAME (i)))
	{
	  tmp_code = (enum token) i;	/* get value for name */
	  break;
	}
    }
  if (tmp_char[0] == '-')
    tmp_code = NIL_CODE;

  if (tmp_code == UNKNOWN)
    {
      fprintf (STDERR,
	       "Unknown trn read in trl.read_trn_l(). Code name was '%s' .\n",
	       tmp_char);
      ftail ();
    }
  /* (NIL_CODE) stands for an expression that isn't there.  */
  if (tmp_code == NIL_CODE)
    {
      /* Discard the closeparen.  */
      while ((c = getc (infile)) != EOF && c != ')');
      return TNULL;
    }

  /* If what follows is `: flag ', read it and store the flags in the trn.  */

  c = read_skip_spaces ();
  msk = 0;
  while (c == ':')
    {
      read_name (tmp_char);
      for (i = LAST_FLAG - 1; i >= VCB_F; i--)
	if (compatible_tok_fl (tmp_code, (enum flags) i))
	  if (!strcmp (tmp_char, NAME_FL (i)))
	    {
	      SetF (msk, (enum flags) i);
	      break;
	    }
      if (i < VCB_F)
	if ((*tmp_char >= '0') && (*tmp_char <= '9'))
	  {
	    i4_t v;
	    if (1 == sscanf (tmp_char, "%d", &v))
	      msk |= (v > 14 ? 0 : v + 1);
	    else
	      yyerror ("Unexpected 'op' flag\n");
	  }
	else
	  {
	    fprintf (STDERR, "trl_read: unrecognized flag '%s' in line %d\n",
		     tmp_char, line_num);
	    errors++;
	  }
      c = read_skip_spaces ();
    }
  ungetc (c, infile);

  msk |= default_mask;
  return_trn = gen_node1 (tmp_code, msk);

  if ((tmp_code == OPERAND) && minus)
    return return_trn;

#define SET_PAT_BIT(n) \
  SET_BIT(XLNG_TRN(return_trn,PTRN_OP(tmp_code,n)),PTRN_BIT(n))
#define ATR_IS_READ if(msk) SET_PAT_BIT(i);
  
  msk = TstF (msk, PATTERN_F);
  format_ptr = FORMAT (tmp_code);
  for (i = 0, format_ptr = FORMAT (tmp_code);
       i < (LENGTH (tmp_code) + (msk ? PTRN_ELEM (tmp_code) : 0));
       i++, format_ptr++)
    {
      c = read_skip_spaces ();
      ungetc (c, infile);
      if (c == ')' || c == '{')
	break;
      if (c=='<')
	{
	  if(!TstF_TRN(return_trn,ACTION_F))
	    yyfatal("Unexpected external reference in dump");
	  switch(*format_ptr)
	    {
	    case '0': case 'r': case 'd': case 'a': case 'V':  case 'p':
	      break;
	    default:
	      SET_PAT_BIT (i);
	      if (msk)
		SET_PAT_BIT (i+LENGTH(tmp_code));
	      XLTR_TRN(return_trn,i) = get_literal("<>",&c);
	    }
	  continue;
	}
      if (i >= LENGTH (tmp_code))
	format_ptr = "x";
      switch (*format_ptr)
	{
	  /* 0 means a field For internal use only. Don't expect it to be
	     present in the input.  */
	case '0':
	case 'r':		/* right, down and arity field must be filled */
				/* later */
	case 'a':
	case 'd':
	  break;

	case 'V':		/* special vocabulary reference (backward */
				/* link) */
	  break;
	case 't':
	case 'v':
	  {
	    register TXTREF x = read_trn_l ();
	    if (msk)
	      if (CODE_TRN (x) == SKIP_CODE)
		{
		  free_node (x);
		  x = TNULL;
		}
	      else
		SET_PAT_BIT (i);
	    XTXT_TRN (return_trn, i) = x;
	  }
	  break;
	case 'P':		/* pattern tree */
	  {
	    MASKTYPE x = default_mask;
	    SetF (default_mask, PATTERN_F);
	    XTXT_TRN (return_trn, i) = read_trn_l ();
	    ATR_IS_READ;
	    default_mask = x;
	    break;
	  }
	case 'N':  /* '>> () ... () <<' as a simple reference */
	  {
	    register TXTREF cn = TNULL;
	    CHECK_CHAR('>');
	    c = getc(infile);
	    CHECK_CHAR('>');
	    c = getc(infile);
	    CHECK_CHAR('>');
	    for(;;)
	      {
		c = read_skip_spaces ();
		if (c == '<')
		  break;
		ungetc (c, infile);
		if (cn)
		  cn = RIGHT_TRN(cn) = read_trn_l();
		else
		  cn = XTXT_TRN(return_trn,i) = read_trn_l();
	      }
	    CHECK_CHAR('<');
	    c = getc(infile);
	    CHECK_CHAR('<');
	    break;
	  }		   
	case 'S':
	  /* 'S' is an optional string: If a closeparen follows, just store
	     NULL For this element.  */
	  if ((c != '\"') && (c != '(') && (c != '-')) /* " */
	    {
	      XLTR_TRN (return_trn, i) = 0;
	      break;
	    }

	case 's': 
	  {
	    XLTR_TRN(return_trn,i) = get_literal("\"\"",&c) ;
	    if (c == '-')
	      break;
	    ATR_IS_READ;
	    break;
	  }
	case 'p':
	  {
	    if (c == 'l')
	      {
		i4_t p = 0;
		CHECK_CHAR ('l');
		fscanf (infile, "line:%d", &p);
		LOCATION_TRN (return_trn) = p;
	      }
	    else
	      LOCATION_TRN (return_trn) = line_num;
	    break;
	  }		   
	case 'y':
	  {
	    if (c == '-')
	      {
		if (msk)
		  c = getc (infile);
		else
		  yyerror ("Warning: unexpected - in dump");
	      }
	    else
	      {
		sql_type_t t;
		char       s[50];
		fscanf (infile, "%[^ \t\n\f\r]", s);
		conv_type (s, &t, 1);
		XOP_TRN (return_trn, i).type = t;
		ATR_IS_READ;
	    }
	    break;
	  }
	case 'i':
	  {
	    if (msk && (c == '-'))
	      {
		c = getc (infile);
		break;
	      }
	    read_name (tmp_char);
	    XLNG_TRN (return_trn, i) = atoi (tmp_char);
	    ATR_IS_READ;
	    break;
	  }
	case 'x':
	  {
	    if (msk && (c == '-'))
	      {
		c = getc (infile);
		break;
	      }
	    fscanf (infile, "%X", (u4_t *) &(XLNG_TRN (return_trn, i)));
	    if (i < LENGTH (tmp_code))
	      ATR_IS_READ;
	    break;
	  }
	case 'l':
	  {
	    if (msk && (c == '-'))
	      {
		c = getc (infile);
		break;
	      }
	    read_name (tmp_char);
	    XLNG_TRN (return_trn, i) = atol (tmp_char);
	    ATR_IS_READ;
	    break;
	  }
	case 'R':
	  {
	    i4_t l;
	    if (msk && (c == '-'))
	      {
		c = getc (infile);
		break;
	      }
	    fscanf (infile, "R:%d", &l);
	    XLNG_TRN (return_trn, i) = l;
	    ATR_IS_READ;
	    break;
	  }
	case 'f':
	  {
	    if (msk && (c == '-'))
	      {
		c = getc (infile);
		break;
	      }
	    read_name (tmp_char);
	    XFLT_TRN (return_trn, i) = atof (tmp_char);;
	    ATR_IS_READ;
	    break;
	  }
	case 'T':
	case 'L':
	  {
	    struct trn_list
	    {
	      struct trn_list *next;
	      tr_union value;
	    };
	    register struct trn_list *next_trn, *trn_list_link;
	    struct trn_list *list_trn = NULL;

	    if (msk && (c == '-'))
	      {
		c = getc (infile);
		break;
	      }

	    c = read_skip_spaces ();
	    CHECK_CHAR ('[');

	    /* add expressions to a list, while keeping a count */
	    next_trn = NULL;
	    list_counter = 0;
	    while ((c = read_skip_spaces ())!= EOF && c != ']')
	      {
		ungetc (c, infile);
		list_counter++;
		trn_list_link = 
		  (struct trn_list *)xmalloc (sizeof (struct trn_list));
		switch (*format_ptr)
		  {
		  case 'T':
		    trn_list_link->value.txtp = read_trn_l ();
		    break;
		  case 'L':
		    read_name (tmp_char);
		    trn_list_link->value.l = atol (tmp_char);
		    break;
		  }
		if (next_trn == 0)
		  list_trn = trn_list_link;
		else
		  next_trn->next = trn_list_link;
		next_trn = trn_list_link;
		trn_list_link->next = 0;
	      }
	    /* get vector length and allocate it */
	    if (list_counter > 0)
	      {
		XVEC_TRN (return_trn, i) = gen_vect (list_counter);
		next_trn = list_trn;
		for (j = 0; j < list_counter; j++, next_trn = next_trn->next)
		  XOP_VEC (return_trn, i, j) = next_trn->value;
		while (list_trn)
		  {
		    next_trn = list_trn;
		    list_trn = list_trn->next;
		    xfree (next_trn);
		  }
	      }
	    /* close bracket gotten */
	    ATR_IS_READ;
	    break;
	  }

	default:
	  {
	    fprintf (STDERR,
		     "%ld: Internal error:(trn_read) format '%c' wrong\n",
		     ftell (infile), format_ptr[-1]);
	    yyfatal ("Abort");
	  }
	}
    }
  c = read_skip_spaces ();
  ungetc (c, infile);
  if (HAS_DOWN (return_trn) && ((c == '{') /* || (msk==0) */ ))
    {
      register TXTREF next_trn;
      TXTREF list_trn = TNULL;
      
      c = read_skip_spaces ();
      CHECK_CHAR ('{');
      
      /* add expressions to a list, while keeping a count */
      next_trn = TNULL;
      list_counter = 0;
      while ((c = read_skip_spaces ())!= EOF && c != '}')
	{
	  ungetc (c, infile);
	  list_counter++;
	  
	  list_trn = read_trn_l ();
	  if (next_trn == 0)
	    DOWN_TRN (return_trn) = list_trn;
	  else
	    RIGHT_TRN (next_trn) = list_trn;
	  next_trn = list_trn;
	}
      ARITY_TRN (return_trn) = list_counter;
      /* close bracket gotten */
    }
  
  c = read_skip_spaces ();
  CHECK_CHAR (')');
  /* add vocabulary information */
  if (TstF_TRN (return_trn, VCB_F) && !TstF_TRN(return_trn,ACTION_F) )
    {
      TXTREF tmp_trn = return_trn;
      switch (tmp_code)
	{
	case PARAMETER:
	case CURSOR:
	  return_trn = add_info_l (return_trn);
	  if (return_trn != tmp_trn)
	    free_tree (tmp_trn);
	  break;
	case SCAN:
	  return_trn = find_entry (return_trn);
	  if (return_trn)
	    free_tree (tmp_trn);
	  else
	    {
	      return_trn = tmp_trn;
	      RIGHT_TRN (tmp_trn) = LOCAL_VCB_ROOT;
	      LOCAL_VCB_ROOT = tmp_trn;
	    }
	  break;
	case SCOLUMN:		/* link all backward reference */
	case COLUMN:
	  return_trn = add_info_l (return_trn);
	  if (return_trn != tmp_trn)
	    free_node (tmp_trn);
	  break;
	default:
	  if (Is_Table (return_trn))
	    {
	      return_trn = add_info_l (return_trn);
	      if (return_trn != tmp_trn)
		free_tree (tmp_trn);
	      else
		{
		  TXTREF clmn;
		  for (clmn = TBL_COLS (return_trn); clmn; clmn = RIGHT_TRN (clmn))
		    COL_TBL (clmn) = return_trn;
		}
	    }			/* Is_Table */
	}
    }
  if (Is_Statement (return_trn))
    {
      STMT_VCB (return_trn) = LOCAL_VCB_ROOT;
      LOCAL_VCB_ROOT = TNULL;
    }
  /*----------------------------*/
  if (TstF_TRN (return_trn, MARK_F))
    {
      free_node (return_trn);
      return_trn = TNULL;
    }
  return return_trn;
#undef ATR_IS_READ
#undef SET_PAT_BIT
}

#undef CHECK_CHAR

TXTREF
read_trn(FILE * infl,i4_t *line,MASKTYPE def_msk)
{
  MASKTYPE x;
  TXTREF   t;
  infile = infl;
  line_num = line ? *line: 0;
  x = default_mask;
  default_mask = def_msk;
  t = read_trn_l ();
  default_mask = x;
  if (line)
    *line = line_num;
  return t;
}

void
load_trl (FILE * infl)
{
  MASKTYPE x;
  install (NULL);
  infile = infl;
  line_num = 0;
  x = default_mask;
  default_mask = 0;
  LOCAL_VCB_ROOT = TNULL;
  for(;;)
    {
      register TXTREF st = read_trn_l ();
      if (!st)
	break;
      add_statement (st);
    }
  default_mask = x;
  return;
}

void
load_trlfile (char *flname)
{
  FILE *fl = fopen (flname, "r");
  char *oldpn;
  if (!fl)
    {
      fprintf (STDERR, "Can't open '%s'\n", flname);
      exit (FATAL_EXIT_CODE);
    }
  oldpn = progname;
  progname = flname;
  load_trl (fl);
  progname = oldpn;
  fclose (fl);
}
