/*
 *  compare.c - contains function for tree recognition
 *
 * This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Software
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

#pragma "$Id: compare.c,v 1.247 1998/09/16 21:00:33 kimelman Exp $"

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include "kitty.h"

#undef PR

#define OPS_NUMBER 15

typedef struct cmp_frame {
  struct cmp_frame *up;
  TXTREF            pattern;
  TXTREF            example;
  TXTREF            e_right;
  TXTREF            e_down;
  i4_t               state;
} cmp_stack;

typedef enum {
  NO_DATA,              /* op does not contain any data             */
  STANDARD_COMPARISION, /* op - address of the once recognized node */
  MULTIPLE_COMPARISION  /* op - vector of recognized nodes          */
} cmp_type;

typedef struct  {
  Rule1_type        rule1;
  TXTREF            op[OPS_NUMBER];
  i4_t               op_desc[OPS_NUMBER];
} comp_data; 

static int
recognize_masks(MASKTYPE mx,MASKTYPE my)
{
  MASKTYPE msk,msk1,msk2;
  
  msk1=0;
  SetF(msk1,MARK_CYC_F);
  msk1=2*msk1-1;
  SetF(msk1,MARK_F);
  
  msk=mx;
  msk CLRVL msk1;   /* clear all bits less than MARK_CYC_F and also MARK_F */
  
  msk2=my;
  msk2 CLRVL msk1;

  if(TstF(mx,EXACT_F))
    {
      if(msk!=msk2)
	return 0;
    }
  else if((msk & msk2)!=msk)
      return 0;
  return 1;
}

/* 
 * loc_compare compares income subtree 'y' with a pattern 'x'. 'context'
 * contains information about the previous 'upper in the stack' comparision. 
 * NULL value of 'context' means the upper level of comparision or 
 * 'env' is a pointer to the data, which should be returned to the user.
 * this function compares node of pattern tree with given node or may be
 * following nodes of analized tree. Beeing recognized, node of income tree
 * marked by MARK_CMP_F flag to avoid duplicate comparision. The sequence of 
 * analyzing is: attributes - operands (underlied subtrees) - right brothers.
 */

#define RET1   goto SUCCESS_EXIT
#define FAIL   goto FATAL_EXIT
#define GO_UP  					\
{  						\
  if (!context ||				\
      loc_compare(RIGHT_TRN(context->pattern),  \
		  context->e_right,             \
		  context->up,env))		\
    RET1;					\
  else						\
    FAIL;					\
}


static int
loc_compare (TXTREF x, TXTREF y, cmp_stack *context, comp_data *env)
{
  register enum token  code;
  register char       *fmt ;
  register i4_t         i;
  auto     TXTREF      y_right; 

#ifdef PR
  i4_t chk_node=1;
  printf("........%ld.........\n",(i4_t)x);
#endif
  
  if (x == y)
    GO_UP;

  /* If the end of pattern line arrived but there are some more nodes in 
   * analized tree we need to check status flag. status==2 means that we've
   * just begined to find 'exist_op'. (Actually it doesn't seem realistic to 
   * catch x==0 in this case. */
  assert(x!=0 || !context || context->state != 2);
  /* state ==1 means that we are in progress of finding 'exist_op' subtree 
   * state ==0 will be found if we suddenly go in the recursive search of
   * this node and in the case or ordinal comparision. 
   */
  if( x == 0 && context && context->state == 1 ) 
    GO_UP;
  
  if ( context && context->state > 0 )
    context->state --;
  
  if(x == 0) 
    FAIL;

  assert(TstF_TRN(x,PATTERN_F));
  
  code = CODE_TRN (x);
  
  /* first step of special cases processing */
  if(code == SKIP_CODES)
    {
      if ( RIGHT_TRN(x) == TNULL) /* { (...) (...) -- } */
	/* pattern satisfied by this link of subtrees ; let's check the 
	   next subtree in the upper level */
	{GO_UP;}
      else /* if there are some additional test for current tree link */
	{
	  for (; y ; y = RIGHT_TRN(y))
	    if (loc_compare(RIGHT_TRN(x),y,context,env))
	      RET1;
	  FAIL;
	}
    }
  
  if ( y == 0 )
    FAIL;
  
#ifdef PR
  printf("%s   %s  %ld\n",NAME(code),NAME(CODE_TRN(y)),OPs_IND(x));
  printf("=============================\n");
#endif

  if(code == SKIP_CODE)
    return loc_compare(RIGHT_TRN(x),RIGHT_TRN(y),context,env);
  
  if(TstF_TRN(y, MARK_CMP_F))
    /* this node has been already associated with another pattern node */
    FAIL;    
  
  if(TstF_TRN(x,EXIST_OP_F))
    {
      TXTREF p;
      i4_t    state;
      if (!context)
	{
	  debug_trn(x);
	  yyfatal(" 8-0 : quite strange request to find existence of the\n"
		  "pattern above in dummy context ");
	}
      ClrF_TRN(x,EXIST_OP_F);
      state = context->state;
      for (p = context->e_down ; p ; p = RIGHT_TRN(p) )
	{
	  context->state = 2;
	  if(loc_compare(x,p,context,env))
	    break;
	}
      SetF_TRN(x,EXIST_OP_F);
      context->state = state;
      if (!p) /* x isn't comparable to any node */
	FAIL;
      RET1;
    }
  /* standart compare process begins here*/
  y_right = RIGHT_TRN(y);
  while(CODE_TRN(y)==NOOP)
    y = DOWN_TRN(y);

  if (code == OPERAND)
    {
      if (env->rule1!=NULL)
        if(XLNG_TRN(x,0))
          if (!env->rule1(y,XLNG_TRN(x,0)))
            FAIL;
      /* (env->rule1==NULL || env->rule1(y,XLNG_TRN(x,0))); */
      if (!recognize_masks(MASK_TRN(x),MASK_TRN(y)))
        FAIL;
      goto CHECK_OPERANDS;
    }
  
  if (code != CODE_TRN (y))
    FAIL;
  
  if ( ! recognize_masks(MASK_TRN(x),MASK_TRN(y)))
    FAIL;
  
  fmt = FORMAT (code);
  
  for (i = LENGTH (code) - 1; i >= 0; i--)
    {
      if(
	! ( TST_BIT(XLNG_TRN(x,PTRN_OP(code,i)),PTRN_BIT(i)) )
	) continue;
      
      switch (fmt[i])
        {
	case 's':  
	  if (XLNG_TRN (x, i) == XLNG_TRN(y, i)) 
	    break; /* because identical literals are saved only once */
	  if (0==strcmp(STRING(XLTR_TRN (x, i)),STRING(XLTR_TRN(y, i))))
	    break;
	  FAIL; 
	case 'i':
        case 'f':
        case 'l':
	case 'y':
	case 'x':
	case 'R':
	  if (XLNG_TRN (x, i) != XLNG_TRN(y, i))
	    FAIL;
	  break;
        case 't':
        case 'v':
        case 'N':
	  if(!loc_compare(XTXT_TRN(x,i),XTXT_TRN(y,i), NULL,env))
	    FAIL;
	  break;
	case '0':
        case 'a': 
        case 'V':
 	case 'p':
        case 'd': /* parameters will be tested later                     */
        case 'r': /* These are just pointers, so they don't matter.      */
          break;
	case 'L': /* array of longs */
	  { 
	    TXTREF patvect=XVEC_TRN(x,i);
	    TXTREF invect=XVEC_TRN(y,i);
	    i4_t j; 
	    
	    if(patvect && (!invect))
	      FAIL;
	    if(patvect && invect)
	      {
		j=VLEN(patvect);
		if(j!=VLEN(invect))
		  FAIL;
		for(;j;j--)
		  if(VOP(patvect,j).l != VOP(invect,j).l)
		    FAIL;
	      }
	  }
	  break;
	case 'T': /* array of expressions */
	  { 
	    TXTREF patvect=XVEC_TRN(x,i);
	    TXTREF invect=XVEC_TRN(y,i);
	    i4_t j;
	    
	    if(patvect && (!invect))
	      FAIL;
	    if(patvect && invect)
	      {
		j=VLEN(patvect);
		if(j!=VLEN(invect))
		  FAIL;
		for(;j;j--)
		  if(!loc_compare(VOP(patvect,j).txtp,VOP(invect,j).txtp,
				  NULL,env) 
		    )
		    FAIL;
	      }
	  }
	  break;
        default:
	  yyfatal("TRL.compare: unexpected format character");
        }
    }
CHECK_OPERANDS: 
  i=OPs_IND(x);
  if(i>=0)
    {
      switch(env->op_desc[i])
	{
	case NO_DATA:
	  env->op[i]=y;
	  env->op_desc[i]=STANDARD_COMPARISION;
	  break;
	case STANDARD_COMPARISION:
	  if (env->op[i] == TNULL)
	    env->op[i]=y;
	  else if(!trn_equal_p(env->op[i],y))
	    FAIL;
	  break;
	default:
	  yyerror("Unexpected comparision mode required");
	}
    }
  if(HAS_DOWN(x) && DOWN_TRN(x) && (!HAS_DOWN(y) || !DOWN_TRN(y)))
    FAIL;
  SetF_TRN(y, MARK_CMP_F);
  if(HAS_DOWN(x) && DOWN_TRN(x))
    {
      cmp_stack cur_context;
      cur_context.up      = context;
      cur_context.pattern = x;
      cur_context.example = y;
      cur_context.e_right = y_right;
      cur_context.e_down  = DOWN_TRN(y);
      cur_context.state   = 0;

      if(!loc_compare(DOWN_TRN(x), DOWN_TRN(y), &cur_context,env))
	FAIL;
    }
  else if (!TstF_TRN(y,VCB_F) || code == COLUMN || code==SCOLUMN )
    /*
     * here we use y_right instead of RIGHT_TRN(y) because of specific
     * of NOOP processing :
     * { ... (Noop  {(y)}) ...} means the same as { ... (y) ...}
     */ 
    if(!loc_compare(RIGHT_TRN(x),y_right, context,env))
      FAIL;
  
SUCCESS_EXIT:
  i=1;
#ifdef  PR
  if(chk_node)
    fprintf(stderr,"trees are equal:\n");
#endif
  goto JUST_EXIT;
FATAL_EXIT:
  if (x)
    {
      i=OPs_IND(x);
      /* clearing previously memorized pointer */
	if(i>=0)
	  switch(env->op_desc[i])
	    {
	    case NO_DATA:
	      break;
	    case STANDARD_COMPARISION:
	      if (env->op[i] == y)
		env->op[i]=TNULL;
	      break;
	    default:
	      yyerror("Unexpected comparision mode required");
	    }
    }
  i=0;
#ifdef  PR
  if(chk_node)
    fprintf(stderr,"trees aren't equal:\n");
#endif
  
JUST_EXIT:
  if(y)
    ClrF_TRN(y,MARK_CMP_F);   
#ifdef PR
  if(chk_node)
    {
      debug_trn(x);
      debug_trn(y);
      fprintf(stderr,"================\n");
    }
#endif
  return i;
}

/*
 * tree_compare routine
 */

i4_t
t_compare (TXTREF x,TXTREF y, TXTREF *Ops, Rule1_type Rule1)
{
  register i4_t rc;
  comp_data    env;
  
  bzero(&env,sizeof(env));
  env.rule1=Rule1;
  rc=loc_compare(x,y, NULL, &env);
  if (rc)
    {
      i4_t i;
      for ( i=0 ; i < OPS_NUMBER; i++)
        if(env.op_desc[i] != NO_DATA)
          {
            assert(Ops);
            Ops[i] = env.op[i];
          }
    }
  return rc;
}

/*--------------------------------------------------------------*/
/* the following function are made for freeing patterns' memory */
/*--------------------------------------------------------------*/

#define DECL(typ,t2) \
typedef struct s_##typ { typ *v; t2 v1; struct s_##typ *n; } typ##_list_t;  \
static typ##_list_t *list; register typ##_list_t *cp;

#define IF_FIND(val) for(cp = list; cp; cp = cp->n ) if (cp->v==(val))

#define ADDLIST(typ,val) \
{ cp = (typ##_list_t *)xmalloc (sizeof (typ##_list_t)); \
  cp->n = list;					\
  cp->v = val;					\
  list = cp;					\
}

#define CLEAR_LIST(act) while(list) { cp = list; act; list = list->n; xfree(cp); }

void 
register_pattern(TXTREF *pattern)
{
  DECL(TXTREF,char);
  
  if(pattern)
    {
      IF_FIND(pattern)
	return;
      ADDLIST(TXTREF,pattern);
    }
  else
    {
      CLEAR_LIST(*(cp->v) = TNULL);
    }
}

TXTREF *
pattern_name(char *pn)
{
  DECL(char,TXTREF);
  
  if(pn)
    {
      IF_FIND(pn)
	return &(cp->v1);
      ADDLIST(char,pn);
      return &(cp->v1);
    }
  else
    {
      free_patterns();
      CLEAR_LIST(*(cp->v) = TNULL);
    }
  return NULL;
}
