/*
 *  liter.c - library for literal processing of GNU SQL compiler
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

/* $Id: liter.c,v 1.246 1998/09/29 21:26:45 kimelman Exp $ */

#include "liter.h"
#include "trlinter.h"
#include "xmem.h"

#define HASH_SECTION_SIZE  64

typedef struct hash_elem{
  LTRLREF ptr;
  word    code;
  i2_t   next;
  word    leng;
} HE;

typedef struct hash_sect {
  TXTREF            next_sect;
  i4_t               sect_num;
  HE                sect[HASH_SECTION_SIZE];
} *HASH;

#define HASHTBL *Root_ptr(Hash_Tbl)
static  LTRLREF lptr   =0;
static  i4_t     l_hcode=-1;
/* hash statistics - needed for smart hashing - right now it looks far from it */
static  i4_t    *h_stat = NULL;  

static  LTRLREF add_ltr __P((char *liter));
static  TXTREF  new_sect __P((i4_t sn));
static  word    hashcode __P((char *liter));
static  LTRLREF hash __P((char *string));
static  LTRLREF Hash(char *string,byte *new); /* new is return code */
/* 1 - new literal    */
/* 0 - old literal    */

static void
seg_switch(i4_t to_tree)
{
  static VADR hold_seg=TNULL;
  if ( (to_tree && hold_seg) || (to_tree==0 && hold_seg==TNULL))
    return;
  if ( to_tree )
    {
      hold_seg = GET_CURRENT_SEGMENT_ID;
      switch_to_segment( *Root_ptr(Current_Tree_Segment) );
    }
  else
    {
      switch_to_segment( hold_seg );
      hold_seg = TNULL;
    }
}

static LTRLREF 
hash(char *string)
{
  byte new;
  return  Hash(string,&new);
}

static LTRLREF 
Hash(char *string,byte *new)
{
  HASH ptrc,ptr0=NULL;
  word indc,ind0=0;
  word  l,code;
  HE e;
  *new=0;
  if (lptr)
    {
      l = strlen(STRING(lptr));
      code = l_hcode;
    }
  else
    {
      if(string==NULL)
        {
          yyerror("Internal error in hash.c: NULL literal");
          return TNULL;
        }
      l=strlen(string);
      if(l==0)
        {
          yyerror("Internal error in hash.c: dummy literal");
          return TNULL;
        }
      code=(lptr?l_hcode:hashcode(string));
    }
  
  indc=code;
  if( (HASHTBL) == TNULL)
    HASHTBL=new_sect(0);
  ptrc=(HASH)vpointer(HASHTBL);
  while( (ptrc->sect[indc].ptr!=TNULL)
	   && (ptrc->sect[indc].code!=code)
    )
    {
      indc=++indc % HASH_SECTION_SIZE;
      if(indc==code)
        {
          if(ptrc->next_sect==TNULL)
            ptrc->next_sect=new_sect(ptrc->sect_num+1);
          ptrc=(HASH)vpointer(ptrc->next_sect);
        }
    }
  while(1)
    {
      e=ptrc->sect[indc];
      if(e.ptr==TNULL)    /* free hole */
        {
          if(lptr)
            e.ptr=lptr;
          else  
            e.ptr=add_ltr(string);
          e.leng=l;
          e.code=code;
	  e.next=-1;
          *new=1;
          h_stat[code]++;
          if(ptr0)
            ptr0->sect[ind0].next=indc+ptrc->sect_num*HASH_SECTION_SIZE;
          ptrc->sect[indc]=e;
          return e.ptr;
        }
      if(lptr)
        {
          if(e.ptr==lptr)
            return e.ptr;
        }
      else
        if(e.leng==l)
          if(0==strcmp(STRING(e.ptr),string))
            return e.ptr;
      ind0=indc;
      ptr0=ptrc;
      if(e.next >= 0 )
        { /* test next literal with same code */
          indc=e.next-HASH_SECTION_SIZE*ptrc->sect_num;
          while(indc>HASH_SECTION_SIZE)
            {
              indc-=HASH_SECTION_SIZE;
              ptrc=(HASH)vpointer(ptrc->next_sect);
            }
        }
      else
        {        /* find free space */
          while(ptrc->next_sect)
	    ptrc=(HASH)vpointer(ptrc->next_sect);
          indc=code;
          while( ptrc->sect[indc].ptr!=TNULL)
            {
              indc=++indc % HASH_SECTION_SIZE;
              if(indc==code)
		{
		  ptrc->next_sect=new_sect(ptrc->sect_num+1);
		  ptrc=(HASH)vpointer(ptrc->next_sect);
		}
            }
        } /* else */
    } /* while(1) */
  /* unreached    */
  /* return NULL; */
}

static int
restruct_hash(i4_t rc)
{
  struct {
    LTRLREF l;
    i4_t     c;
  }  sv[HASH_SECTION_SIZE];
  
  HASH ptrc = NULL;
  word indc = 0;
  LTRLREF l = lptr;
  
  if(!rc)
    return 0;
  
  for ( ptrc=(HASH)vpointer(HASHTBL); ptrc; ptrc = (HASH)vpointer(ptrc->next_sect))
    {
      for( indc = 0 ; indc < HASH_SECTION_SIZE; indc ++)
        {
          sv[indc].l = ptrc->sect[indc].ptr;
          sv[indc].c = ptrc->sect[indc].code;
          ptrc->sect[indc].ptr = TNULL;
        }
      for( indc = 0 ; indc < HASH_SECTION_SIZE; indc ++)
        if(sv[indc].l)
          {
            lptr    = sv[indc].l;
            l_hcode = sv[indc].c;
            hash(STRING(lptr));
            lptr    = l;
          }
    }
  return 0;
}

static word
hashcode(char *s)
{
  typedef struct hc {
    i4_t        l_pos;
    i4_t        shift;
    struct hc *next;
  } hc_t;

  static byte  hcode=0;
  static byte  code[256];
  static hc_t *h_rule = NULL;
  static i4_t   h_wight = 0;
  
  word  rc=0;
  i4_t   p;
  hc_t *hc_c;
  
  if(hcode==0)
    {
      i4_t   wight,i;
      hc_t *ch;
      
      for(i=256;i--;)code[i]=0;
      code['_']=++hcode;
      for(i='0';i<='9';i++) code[i]=++hcode;
      for(i='A';i<='Z';i++) code[i]=++hcode;
      for(i='a';i<='z';i++) code[i]=++hcode;
      /* hcode==61 ~~ 6 bit */
      for(i=0;hcode>(1<<i);i++);
      hcode=i;
      h_rule = (hc_t*)xmalloc(sizeof(hc_t));
      wight = 1<<hcode;
      while (wight > HASH_SECTION_SIZE)
        {
          h_rule->shift --;
          wight >>=1;
        }
      while (2*wight < HASH_SECTION_SIZE)
        {
          h_rule->shift ++;
          wight <<=1;
        }
      for(ch = h_rule; ch->shift > hcode ; ch = ch->next)
        {
          ch->next = (hc_t*)xmalloc(sizeof(hc_t));
          ch->next->l_pos = ch->l_pos + 1;
          ch->next->shift = ch->shift - hcode;
        }
      if (h_wight!=wight)
        {
          if(h_wight==0)
            h_stat = (i4_t*)xmalloc(wight*sizeof(i4_t));
          else
            h_stat = (i4_t*)xrealloc((void*)h_stat,wight*sizeof(i4_t));
          h_wight = wight;
        }
    }

  while(s==0) /* request for check hash althorithm effectiveness */
    {
      i4_t imax,i;
      float avg, d2,d, maxx;
      hc_t *ch;
      
      for (avg = i = 0 ; i < h_wight; i++)
        avg += h_stat[i];
      avg /= h_wight;
      for (d2 = i = 0 ; i < h_wight; i++)
        { d = (h_stat[i] - avg)/avg; d2 = d*d; }
      d2 /= h_wight;
      if ( d2 < 0.04 ) /* if fluctuation is ok - just exit  */
        return restruct_hash(rc);
      if (rc > 10)
        {
          yyerror("cycle in hash beautification process");
          return restruct_hash(rc);
        }
      rc ++; /* we begin hash althorithm modification */
      /* fluctuation in hashing is too much - we shall to do intellectual correctness  */
      /* because 'intellectual' is too difficult  - let's do anything */
      /* weighted strengh */
      for(maxx = imax = i = 0; i < h_wight; i ++)
        {
          i4_t j;
          if (i != imax)
            for(j = 256; i--; )
              if (code[j] == (i>>h_rule->shift))
                code[j] = imax;
          maxx += h_stat[i]/avg;
          imax = maxx + 0.5;
        }
      if(d2 > 0.1) /* if dispersion is huge */
        { /* let's try one more way */
          if ( !h_rule->next || h_rule->next->l_pos > h_rule->l_pos+1)
            { /* let's compute value by pair of symbol */
              hc_t *cch = (hc_t*)xmalloc(sizeof(hc_t));
              cch->next = h_rule;
              cch->l_pos = h_rule->l_pos;
              h_rule->l_pos++;
              h_rule->shift --;
              cch->shift = h_rule->shift;
              h_rule = cch;
            }
          else
            for(ch = h_rule; ch; ch = ch->next) 
              ch->l_pos++;
        }
      for ( i  = 0; i < h_wight; i++)
        h_stat[i] = 0;
      {
        HASH ptrc = NULL;
        word indc = 0;

        for ( ptrc=(HASH)vpointer(HASHTBL); ptrc; ptrc = (HASH)vpointer(ptrc->next_sect))
          for( indc = 0 ; indc < HASH_SECTION_SIZE; indc ++)
            if (ptrc->sect[indc].ptr!=TNULL)
              h_stat [ptrc->sect[indc].code=hashcode(STRING(ptrc->sect[indc].ptr))]++;
      }
    }
  
  for(p=0,hc_c = h_rule; hc_c; hc_c = hc_c->next)
    {
      while (p<hc_c->l_pos && s[p]) p++;
      if (p < hc_c->l_pos && p>0)
        p--;
      rc += code[(byte)(s[p])] << hc_c->shift;
    }
  
  return rc % h_wight;
}

static TXTREF
new_sect(i4_t sn)
{
  word   size = sizeof(struct hash_sect);
  i4_t    i;
  TXTREF new_sect;
  
  seg_switch(1);
  new_sect = vmalloc(size)+ GET_CURRENT_SEGMENT_ID;
  ((HASH)vpointer(new_sect))->sect_num=sn;
  register_relocation_address(new_sect); /* next_sect */
  for(i=0;i<HASH_SECTION_SIZE;i++)
    register_relocation_address(new_sect + sizeof(TXTREF) + sizeof(i4_t) +
                                i*sizeof (HE) );
  seg_switch(0);
  return new_sect;
}

void
free_hash(void)
{
  TXTREF h;
  i4_t  i;
  seg_switch(1);
  while(HASHTBL)
    {
      h=HASHTBL;
      HASHTBL=((HASH)vpointer(h))->next_sect;
      for(i=0;i<HASH_SECTION_SIZE;i++)
          vmfree(((HASH)vpointer(h))->sect[i].ptr);
      vmfree(h);
    }
  seg_switch(0);
}

int
load_hash(LTRLREF l)                         /* load LITERAL to hash */
{                                            /* table                */
  lptr=l;
  if(l != hash(STRING(l)) )
    {
      fprintf(stderr,
              "\n%s:%d: Internal error in call function load_hash(%s)\n",
              __FILE__,__LINE__,STRING(l));
      yyfatal("Abort");
    }
  lptr=0;
  return 0;
}


LTRLREF
ltr_rec(char *liter)
{
  return hash(liter);
}

static LTRLREF
add_ltr(char *liter)
{
  VADR p;
  seg_switch(1);
  p=vm_ob_alloc(strlen(liter)+1,1)+GET_CURRENT_SEGMENT_ID;
  seg_switch(0);
  strcpy((char*)vpointer(p),liter);
  return (LTRLREF)p;
}

char *
ltrlref_to_string(register LTRLREF l)
{
  char *ptr;
  ptr = (char*)vpointer(l);
  if(!ptr)
    return "";
  return ptr;
}



	
