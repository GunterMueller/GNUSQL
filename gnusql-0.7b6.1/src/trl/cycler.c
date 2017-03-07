/*
 *  cycler.c  - cycle processors library of GNU SQL compiler
 *
 * This file is a part of GNU SQL Server
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
 *  Contacts: gss@ispras.ru
 *
 */

/* $Id: cycler.c,v 1.247 1998/09/29 21:26:45 kimelman Exp $ */

#include "cycler.h"

i4_t    cycler_skip_subtree=0 ;
static PROC real_processor=NULL;
static TXTREF line_processor __P((TXTREF node,i4_t flag));
static TXTREF cycle_processor __P((TXTREF root,i4_t flag));


#define CYC_TST(fl)  (flag & CYCLER_##fl)

static TXTREF
processor(TXTREF node,i4_t flag,i4_t rlc_flag)
{
  TXTREF rl;

  rl=RIGHT_TRN(node);
  if(HAS_LOCATION(node))
    file_pos = LOCATION_TRN(node);
  node=real_processor(node,flag);
  if(rlc_flag && node) 
    RIGHT_TRN(node)=rl;
  return node;
}

TXTREF
cycler(TXTREF root,PROC exec,i4_t flag)
{
  PROC proc = real_processor;
  real_processor=exec;
  if(!real_processor)
    yyfatal("incorrect processing routine was given to cycler");
  else
    if(CYC_TST(LN))
      root=line_processor(root,flag);
    else
      root=cycle_processor(root,flag);
  real_processor = proc;
  return root;
}

static TXTREF
cycle_processor(TXTREF root,i4_t flag)
{
  register char      *fmt;
  register i4_t        i;
  register TXTREF     lvcb = TNULL;
  
  if(root==0)
    return TNULL;
  if (Is_Statement(root))
    {
      lvcb = LOCAL_VCB_ROOT;
      LOCAL_VCB_ROOT = STMT_VCB(root);
    }
  if (CYC_TST(LD))
    root=processor(root,CYC_TST(LD),CYC_TST(RLC));
  if ( (root==0) || (cycler_skip_subtree))
    goto exit;
  if(CYC_TST(OPER))
    { 
      for(i=0,fmt=FORMAT(CODE_TRN(root));*fmt;fmt++,i++)
        switch(*fmt)
          {
          case 'v':
	    if(!CYC_TST(VCB))
	      XTXT_TRN(root,i)=cycle_processor(XTXT_TRN(root,i), 
					       flag | CYCLER_VCB);
	    break;
          case 't':
	    XTXT_TRN(root,i)=cycle_processor(XTXT_TRN(root,i), 
					     flag & ~CYCLER_VCB);
	    break;
          case 'N':
	    XTXT_TRN(root,i)=line_processor(XTXT_TRN(root,i), 
                                            flag & ~CYCLER_VCB);
	    break;
          case 'T':
	    {
	      register i4_t    j;
	      for(j=XLEN_VEC(root,i);j--;)
		XVECEXP(root,i,j)=cycle_processor(XVECEXP(root,i,j), 
						  flag & ~CYCLER_VCB);
	    }
	    break;
          default : break;
	    /* do nothing */;
          }
    }      
  if(cycler_skip_subtree)
    goto exit;
  if(HAS_DOWN(root))
    DOWN_TRN(root)=line_processor(DOWN_TRN(root),flag & ~CYCLER_VCB);
  if(CYC_TST(DR))
    root=processor(root,CYC_TST(DR),CYC_TST(RLC));
  
exit:
  if (root && Is_Statement(root))
    {
      STMT_VCB(root) = LOCAL_VCB_ROOT;
      LOCAL_VCB_ROOT = lvcb;
    }
  cycler_skip_subtree = 0;
  return root;
}

static TXTREF
line_processor(TXTREF node, i4_t flag)
{
  TXTREF rn;

  if(node==0)
    return TNULL;
  rn = RIGHT_TRN(node);
  node=cycle_processor(node,flag);
  if(node==0)
    return line_processor(rn,flag);
  RIGHT_TRN(node)=line_processor(RIGHT_TRN(node),flag);
  if(CYC_TST(RL))
    {
      rn = RIGHT_TRN(node);
      node=processor(node,CYC_TST(RL),CYC_TST(RLC));
      if (node ==0)
        node = rn;
    }
  if(cycler_skip_subtree)
      cycler_skip_subtree=0;
  return node;
}

/**************************************************************************/
/*      STACK Library                                                     */
/**************************************************************************/

#define St_p struct Stack_portion

void
st_push(St_p **hdr,void *ptr,i4_t size)
{
  register St_p *hd=*hdr;
  if( hd==NULL || (hd->used==STACK_PORTION))
    {
      register St_p *cr=hd;
      hd=(St_p*)xmalloc(sizeof(*hd));
      hd->inf=xmalloc(STACK_PORTION*size);
      hd->nxt=cr;
    }
  bcopy(ptr,(char*)hd->inf+size*hd->used,size);
  hd->used++;
  *hdr=hd;
}

void
st_pop(St_p **hdr,void *ptr,i4_t size,char *f,i4_t l)
{
  register St_p *hd=*hdr;
  if(hd==NULL)
    fprintf(stderr,"\nInternal error at %s.%d: POP used for dummy stackn",
	    f,l);
  else
    {
      hd->used--;
      bcopy((char*)hd->inf+size*hd->used,ptr,size);
      if(hd->used==0)
	{
	  register St_p *cr=hd->nxt;
	  xfree(hd->inf);
	  xfree(hd);
	  hd=cr;
	}
    }
  *hdr=hd;
}

i4_t
st_depth(St_p *hd)
{
  register St_p *cr;
  register i4_t   lv;
  lv=0;
  for(cr=hd;cr;cr=cr->nxt)
     lv+=cr->used;
  return lv;
}

void
*st_get(St_p *hd,i4_t size,i4_t shift,char *f,i4_t l)
{
  register St_p *cr;
  register i4_t dp;
  for(cr=hd,dp=shift;cr && dp>=cr->used;cr=cr->nxt)
    dp-=cr->used;
  if(cr)
    return (void*)((char*)cr->inf+size*(cr->used-dp-1));
  else
    dp=fprintf(stderr,"Get Stack call too deeply at %s.%d\n",f,l);
  return NULL;
}
