/*
 *  vcblib.c - tree library of GNU SQL compiler
 *             Vocabulary support.
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming, 
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

/* $Id: vcblib.c,v 1.248 1998/09/29 22:23:52 kimelman Exp $ */

#include "trl.h"
#include "vcblib.h"
#include "trlinter.h"
#include "cycler.h"
#include "type_lib.h"
#include <assert.h>
#include "tassert.h"

static void sort_clmns __P((TXTREF *list));
void check_scan_cols(TXTREF tblptr);

static VCBREF
ind_table(VCBREF index)
{
  /*     table   column   colptr   given_index */
  return COL_TBL(OBJ_DESC(DOWN_TRN(index      )));
}

/*
 * Finding parameters, cursors etc. in vocabulary by only one name. 
 * Each of this kind of nodes stores name as a first parameter.     
 */

VCBREF
find_info( enum token code,LTRLREF l)     
{                                         
#define  VCB_NAME(v)  XLTR_TRN(v,0)       
  register VCBREF vcb;                    

  if     ( code==CURSOR )
    vcb=VCB_ROOT;
  else if( code==PARAMETER || code==SCAN )
    vcb=LOCAL_VCB_ROOT;
  else
    {
      if(code>=LAST_TOKEN)code=0;
      lperror("Internal error: trl.find_info: Unrecognized token code %s \n",
              NAME(code));
      return TNULL;
    }
  while(vcb)
    {
      if(CODE_TRN(vcb)==code)
        if(VCB_NAME(vcb)==l || l==TNULL)
          return vcb;
      vcb=RIGHT_TRN(vcb);
    }
  if( code == PARAMETER )
    vcb=VCB_ROOT;
  while(vcb)
    {
    if(CODE_TRN(vcb)==code)
      if(VCB_NAME(vcb)==l || l==TNULL)
        {
          vcb=copy_trn(vcb);
          RIGHT_TRN(vcb)=LOCAL_VCB_ROOT;
          LOCAL_VCB_ROOT=vcb;
          return vcb;
        }
      vcb=RIGHT_TRN(vcb);
    }
  return TNULL;
#undef VCB_NAME
}

/*
 * Finding entry in the vocabulary, which would be equivalent to the 
 * given one.
 */

VCBREF
find_entry(VCBREF v)                      
{                                         
  register enum token code;
  register VCBREF vcb;
  
  if(v==TNULL)return TNULL;
  code=CODE_TRN(v);
  switch(code)
    {
    case PARAMETER:
    case CURSOR:
      return find_info(code,XLTR_TRN(v,0));
    case SCAN:
      vcb=LOCAL_VCB_ROOT;
      while(vcb)
	{
          LTRLREF l = COR_NAME(v);

	  if(CODE_TRN(vcb)==SCAN)
	    if((COR_NO(vcb)   == COR_NO (v)) &&
	       (COR_TBL(vcb)  == COR_TBL(v)) &&
               (COR_NAME(vcb) == l)
               )
	      return vcb;
	  vcb=RIGHT_TRN(vcb);
	}
      break;
    case UNIQUE:
    case PRIMARY:
    case INDEX:
      vcb = find_entry(ind_table(v));
      if (!vcb)
        return vcb; /* even adequate table not exist */
      vcb = IND_INFO(vcb);
      while(vcb)
	{
	  CODE_TRN(v) = CODE_TRN(vcb);
	  if (trn_equal_p(v,vcb))
	    {
	      CODE_TRN(v) = code;
	      return vcb;
	    }
	  vcb = RIGHT_TRN(vcb);
	}
      CODE_TRN(v) = code;
      break;
    default:
      if(Is_Table(v))
	return find_table(code,TBL_FNAME(v),TBL_NAME(v));
      else if(Is_Column(v))
	{
	  vcb = COL_TBL(v);
	  if (!vcb)
	    return TNULL;
	  vcb=find_entry(vcb);
          /*
            if (!vcb) 
	    vcb = COR_TBL(v);
            */
	  return find_column(vcb,COL_NAME(v));
	}
      if(code<=UNKNOWN || code>=LAST_TOKEN)
	code=UNKNOWN;
      fprintf(STDERR,"trl.entry: Unrecognized token code %s \n",
	      NAME(code));
      yyerror("Internal error #2");
    }
  return TNULL;
}

/*
 * Finding tables, views and so on in the vocabulary by the desired code
 * and 2 given names
 */

VCBREF
find_table(enum token code,LTRLREF l,LTRLREF l1)
{
  register VCBREF vcb;
  if((l==TNULL) && (l1==TNULL))return TNULL;

  if     ( code==VIEW || code==TABLE || code==ANY_TBL )
    vcb=VCB_ROOT;
  else if( code==TMPTABLE )
    vcb=LOCAL_VCB_ROOT;
  else
    {
      char err[100];
      if(code>=LAST_TOKEN)code=0;
      
      sprintf(err,"find_table: Unrecognized token code %s \n",NAME(code));
      yyerror(err);
      return TNULL;
    }
  while(vcb)
    {
      register enum token Code=CODE_TRN(vcb);
      if( Code==code || ( code==ANY_TBL && Is_Table(vcb) ))
        if( (TBL_FNAME(vcb)==l || l==TNULL) && (TBL_NAME(vcb)==l1 || l1==TNULL) )
          return vcb;
      vcb=RIGHT_TRN(vcb);
    }
  return TNULL;
}

static void
add_to_the_end(TXTREF *root,TXTREF node)
{
  register TXTREF c;
  if(*root==TNULL)
   *root=node;
  else
    {
      for(c=*root;RIGHT_TRN(c);c=RIGHT_TRN(c));
      RIGHT_TRN(c)=node;
    }
  RIGHT_TRN(node) = TNULL;
}

/*
 * add information to the vocabulary with checking for existing of equivalent
 * entries
 */

VCBREF
add_info_l(VCBREF v)
{
  register VCBREF vcb;
  register enum token code;
  register i4_t  ins_no;
  if(v==TNULL)return v;
  code = CODE_TRN(v);

  switch(code)
    {
    case CURSOR:
      vcb=find_info(code,CUR_NAME(v));
      if(vcb!=TNULL) return vcb;
      add_to_the_end(&VCB_ROOT,v);
      break;
    case VIEW:
    case TABLE:
    case ANY_TBL:
      vcb=find_table(code,TBL_FNAME(v),TBL_NAME(v));
      if(vcb!=TNULL) return vcb;
      add_to_the_end(&VCB_ROOT,v);
      break;
    case TMPTABLE:
      vcb=find_table(code,TBL_FNAME(v),TBL_NAME(v));
      if(vcb!=TNULL) return vcb;
      if(TBL_FNAME(v)==TNULL)
        TBL_FNAME(v)=GL_AUTHOR;
      if(TBL_NAME(v)==TNULL)
        {
          static int tmptblno=0;
          char b[40];
          sprintf(b,"tmp%d",++tmptblno);
          TBL_NAME(v)=ltr_rec(b);
        }
      add_to_the_end(&LOCAL_VCB_ROOT,v);
      break;
    case PARAMETER:
      vcb=find_info(code,PAR_NAME(v));
      if(vcb!=TNULL) return vcb;
      add_to_the_end(&LOCAL_VCB_ROOT,v);
      break;
    case SCAN: 
      vcb=LOCAL_VCB_ROOT;
      ins_no = 1;
      while(vcb)
	{
	  if((CODE_TRN(vcb)==SCAN) && (COR_TBL(vcb)==COR_TBL(v)))
            {
              if (COR_NO(vcb) == COR_NO(v))
                return vcb;
              else if ( ins_no <= COR_NO(vcb) )
                ins_no = COR_NO(vcb) + 1;
            }
	  vcb=RIGHT_TRN(vcb);
	}
      if(COR_NO(v) <0)
        COR_NO(v)=ins_no;
      add_to_the_end(&LOCAL_VCB_ROOT,v);
      break;
    case COLUMN:
      {
        TXTREF tbl = COL_TBL(v);
        vcb=find_column(tbl,COL_NAME(v));
        if(vcb!=TNULL)return vcb;
        if(TstF_TRN(tbl,CHECKED_F)) /* if table contains information   */
          {                          /*  from database                   */
            yyerror("Internal error:"
                    " it's impossible to change table information now");
            return TNULL;
          }
        add_to_the_end(&TBL_COLS(COL_TBL(v)),v);
        TBL_NCOLS(tbl)++;
        break;
      }
    case SCOLUMN:
      {
        TXTREF scan = COL_TBL(v);
        vcb=find_column(scan,COL_NAME(v));
        if(vcb!=TNULL)
          return vcb;
        vcb = find_column(COR_TBL(scan),COL_NAME(v));
        if (!vcb)
          {
            CODE_TRN(v) = COLUMN;
            COL_TBL(v) = COR_TBL(scan);
            add_info(v); /* if it's impossible we never back here */
          }
        add_column(scan,COL_NAME(v));
        return find_column(scan,COL_NAME(v));
      }
    case PRIMARY:
    case UNIQUE:
    case INDEX:
      vcb=find_entry(v);
      if(vcb!=TNULL)
	{
	  CODE_TRN(vcb) = CODE_TRN(v);
	  return vcb;
	}
      SetF_TRN(v,VCB_F);
      add_to_the_end(&IND_INFO(ind_table(v)),v);
      break;
    default:
      if(code>=LAST_TOKEN)code=0;
      fprintf(STDERR,"trl.add_info: Unrecognized token code %s \n",
	      NAME(code));
      yyerror("Internal error #2");
      return TNULL;
    }
  return v;
}

/* 
 * add info to vocabulary and return status
 */

CODERT
add_info(VCBREF v)                       
{
  enum token code = CODE_TRN(v);
  VCBREF v1=add_info_l(v);
  if(v1==v)
    return 0;
  if (code == SCOLUMN)
    {
      if (CODE_TRN(v)==SCOLUMN && trn_equal_p(v,v1))
        return 0;
      if (CODE_TRN(v)==COLUMN)
        {
          TXTREF vv=find_column(COR_TBL(COL_TBL(v1)),COL_NAME(v1));
          if(vv==v)
            return 0;
        }
    }
  fprintf(STDERR,"\n trl.add_info: Internal error "
          "\n -- duplicate vocabulary info \n");
  yyfatal("Abort.");
  return -1; /* unreachable */
}

/*
 * delete given node from vocabulary
 */

CODERT
del_info(VCBREF v)                       
{
  register VCBREF *Vcb,vcb,cv;
  register enum token code;
  if(v==TNULL) return 0;     
  if(!TstF_TRN(v,VCB_F))
    {
      fprintf(STDERR,"Impossible delete non vocabulary entry in del_info\n");
      return 1;
    }
  code = CODE_TRN(v);
  cv=TNULL;
  switch(code)
    {
    case CURSOR:
    case VIEW:
    case TABLE:
    case ANY_TBL:
      Vcb=&VCB_ROOT;
      break;
    case SCAN:
    case PARAMETER:
    case TMPTABLE:
      Vcb=&LOCAL_VCB_ROOT;
      break;
    case COLUMN:
      Vcb=&TBL_COLS(COL_TBL(v));
      break;
    case SCOLUMN:
      Vcb=&COR_COLUMNS(COL_TBL(v));
      break;
    case PRIMARY:
    case UNIQUE:
    case INDEX:
      Vcb=&IND_INFO(ind_table(v));
    default:
      fprintf(STDERR,"trl.del_info: Unrecognized token code =%d \n",
                     (i4_t)code);
      yyerror("Internal error #2");
      return -1;
    }
  vcb=*Vcb;
  cv=TNULL;
  if(vcb==v)
    vcb=RIGHT_TRN(cv=v);
  else LPROC(cv,vcb)
    if(RIGHT_TRN(cv)==v)
      {
        RIGHT_TRN(cv)=RIGHT_TRN(v);
        break;
      }
  if(cv==TNULL && code==PARAMETER)
    {
      Vcb=&VCB_ROOT;
      vcb=*Vcb;
      if(vcb==v)
        vcb=RIGHT_TRN(cv = v);
      else LPROC(cv,vcb)
        if(RIGHT_TRN(cv)==v)
          {
            RIGHT_TRN(cv)=RIGHT_TRN(v);
            break;
          }
    }
  *Vcb=vcb;
  if(cv==TNULL)
    {
      fprintf(STDERR,"Vocabulary entry not found in vocabulary !!!!\n");
      debug_trn(v);
      return -1;
    }
  else
    {
      RIGHT_TRN(v)=TNULL;
      if (CODE_TRN(v) == COLUMN)
        TBL_NCOLS(COL_TBL(v))--;
      free_line(v);
    }
  return 0;
}

/* 
 * Finding column of the given table by name or by column number (DB info)
 * tbl - reference to table;
 * l   - name of column ;
 * num - DB number of column;
 * fl  - switcher for finding column by name or by number 
 */

static VCBREF
find_column_A(TXTREF tbl,LTRLREF l,i4_t num,i4_t fl)
{
  register TXTREF vcb;
  register enum token code;
  if(tbl==TNULL) return TNULL;
  code=CODE_TRN(tbl);
  
  if(l==TNULL && fl==0)return TNULL;

  switch(code)
  {
  case SCAN:
       vcb=COR_COLUMNS(tbl);
       break;
  default:
      if(Is_Table(tbl))
        {
          vcb=TBL_COLS(tbl);
          break;          
        }
      if(code>=LAST_TOKEN)code=0;
      fprintf(STDERR,"trl.find_column: Unrecognized token code %s \n",
                     NAME(code));
      yyerror("Internal error #2");
      return TNULL;
  }
  for(; vcb ;vcb=RIGHT_TRN(vcb))
    if( ((fl==0) && (l==COL_NAME(vcb))) ||
        ((fl!=0) && (num==COL_NO(vcb)))
      )
      return vcb;
  return TNULL;
}

VCBREF
find_column(TXTREF tbl,LTRLREF l) 
{
  return find_column_A(tbl,l,0,0); 
}

VCBREF 
find_column_by_number(TXTREF tbl,i4_t num)
{
  return find_column_A(tbl,0,num,1);
}

CODERT
add_column(TXTREF tbl,LTRLREF l)       
{
  register TXTREF vcb,newv;
  register i4_t    flag;
  register enum token code;
  if(tbl==TNULL)return SQL_ERROR;
  if(l==TNULL)return SQL_ERROR;
  code=CODE_TRN(tbl);
  if(find_column(tbl,l))
    {
      lperror("Warning: add duplicate column '%s' to next table:\n",
              STRING(l));
      debug_trn(tbl);
      return SQL_ERROR;
    }
  switch(code)
  {
  case SCAN:
       vcb=find_column(COR_TBL(tbl),l);
       if(!vcb){
         flag=add_column(COR_TBL(tbl),l);
         if(flag)
           return flag;
         vcb=find_column(COR_TBL(tbl),l);
       }
       COPY_NODE(SCOLUMN,vcb,newv);
       COL_TBL(newv)=tbl;
       add_to_the_end(&COR_COLUMNS(tbl),newv);
       sort_clmns(&COR_COLUMNS(tbl));
       break;
  case TABLE:
  case VIEW:
  case ANY_TBL:
  case TMPTABLE:
       if(TstF_TRN(tbl,CHECKED_F)) /* if table contains information   */
       {                          /*  from database                   */
         yyerror("Internal error: it's impossible to change table information now");
         return -1;
       }
       newv=gen_node(COLUMN);
       COL_TBL(newv)=tbl;
       COL_NAME(newv)=l;
       add_to_the_end(&TBL_COLS(tbl),newv);
       TBL_NCOLS(tbl)++; 
       break;
  default:
       if(code>=LAST_TOKEN)code=0;
       fprintf(STDERR,"trl.add_column: Unrecognized token code %s \n",
                      NAME(code));
       yyerror("Internal error #2");
       return -1;
  }
  return 0;
}


#define NTYPE(n)  *(node_type(n))
#define is_compatible(a,b)  is_casted(NTYPE(a),NTYPE(b))

sql_type_t *
node_type(TXTREF node)
{
  switch(CODE_TRN(node))
    {
    case COLPTR:    
    case PARMPTR:   
    case OPTR:      return node_type(OBJ_DESC(node));
    case COLUMN:
    case SCOLUMN:   return &COL_TYPE(node);
    case PARAMETER: return &PAR_TTYPE(node);
    case CONST:     return &CNST_TTYPE(node);
    case NULL_VL:   return &NULL_TYPE(node);
    case USERNAME:  return &USR_TYPE(node);
    default:
      if(Is_Operation(node) && Is_Typed_Operation(node))
        return &OPRN_TYPE(node);
      lperror("unexpected node's type requested: '%s'",NAME(CODE_TRN(node))); 
      yyfatal("");
    }
  /* unreachable code */
  return NULL;
}

static void
type_comp(char *msg,TXTREF e,TXTREF t)
{
  if( !is_compatible(e,t) )
    {
      char err[256];
      sprintf(err,"%s\n%s(%s) - ",msg,NAME(CODE_TRN(t)),
              type2str(NTYPE(t)));
      sprintf(err + strlen(err),"%s(%s)\n",NAME(CODE_TRN(e)),
              type2str(NTYPE(e)));
      yyerror(err);
      toggle_mark1f(1);
      debug_trn(t);
      toggle_mark1f(1);
      debug_trn(e);
    }
  else if ((NTYPE(t)).code==SQLType_0)
    {
      extern void put_type_to_tree  __P((TXTREF arg_node, sql_type_t *need_type));
      put_type_to_tree(t,node_type(e));
    }
}

#define SWAP(typ,a,b) { typ iv=a; a=b; b=iv;}

void
sort_list_by_col_no(TXTREF *list,TXTREF *l2,TXTREF *l3,
		    i4_t is_scols,char *msg2,char *msg3)
{
   TXTREF *elist, *el2 = NULL, *el3 = NULL, ce;
   i4_t    n,i,j,doit,*nlist;
   n=0;
   for(ce=*list;ce;ce=RIGHT_TRN(ce))
     n++;

    if (!n)
      return;
    
   elist = (TXTREF*)xmalloc(sizeof(TXTREF)*n);
   nlist = (i4_t*)xmalloc(sizeof(i4_t)*n);
   for(i=0,ce=*list;ce;ce=RIGHT_TRN(ce),i++)
     {
       elist[i]=ce;
       nlist[i]=COL_NO(is_scols?ce:OBJ_DESC(ce));
     }
   if(l2)
     {
       el2 = (TXTREF*)xmalloc(sizeof(TXTREF)*n);
       for(i=0,ce=*l2;ce && i<n;ce=RIGHT_TRN(ce),i++)
         el2[i]=ce;
       assert(i==n && ce==TNULL);
     }
   if(l3)
     {
       el3 = (TXTREF*)xmalloc(sizeof(TXTREF)*n);
       for(i=0,ce=*l3;ce && i<n;ce=RIGHT_TRN(ce),i++)
         el3[i]=ce;
       assert(i==n && ce==TNULL);
     }
   for(doit=1,j=n;j && doit--;j--) 
     for(i=1;i<j;i++)
       if(nlist[i-1]>nlist[i])
         {
	    SWAP(i4_t,nlist[i],nlist[i-1]);
	    SWAP(TXTREF,elist[i],elist[i-1]);
	    if(l2)
	      SWAP(TXTREF,el2[i],el2[i-1]);
	    if(l3)
	      SWAP(TXTREF,el3[i],el3[i-1]);
	    doit = 1;
         }
   if(l2)
     {
       for(i=0;i<n;i++)
         type_comp(msg2,elist[i],el2[i]);
       *l2   = el2[0];
       for(i=0;i<n-1;i++)
         RIGHT_TRN(el2[i])=el2[i+1];
       RIGHT_TRN(el2[n-1])   = TNULL;
       xfree(el2);
     }
   if(l3)
     {
       for(i=0;i<n;i++)
         type_comp(msg3,elist[i],el3[i]);
       *l3 = el3[0];
       for(i=0;i<n-1;i++)
         RIGHT_TRN(el3[i])=el3[i+1];
       RIGHT_TRN(el3[n-1]) = TNULL;
       xfree(el3);
     }
   *list = elist[0];
   for(i=0;i<n-1;i++)
     RIGHT_TRN(elist[i])=elist[i+1];
   RIGHT_TRN(elist[n-1]) = TNULL;
   xfree(elist);
   xfree(nlist);
}

static void
sort_clmns(TXTREF *list)
{
  sort_list_by_col_no(list,NULL,NULL,1,NULL,NULL);
}

i4_t 
count_list(TXTREF list)
{
  register i4_t i;
  for(i=0;list;list=RIGHT_TRN(list))
    i++;
  return i;
}

void
check_scan_cols(TXTREF tblptr)
{
  TXTREF scan;
  
  TASSERT(CODE_TRN(tblptr) == TBLPTR, tblptr);
  
  scan = TABL_DESC(tblptr);
  /* check table columns */
  sort_clmns(&(TBL_COLS(COR_TBL(scan))));
  if(count_list(TBL_COLS(COR_TBL(scan))) != TBL_NCOLS(COR_TBL(scan)))
    {
      yyfatal("table columns haven't been uploaded yet");
    }
  sort_clmns(&(COR_COLUMNS(scan)));
  if (count_list(COR_COLUMNS(scan)) != TBL_NCOLS(COR_TBL(scan)))
    {
      TXTREF a,b,a0=TNULL;
      a=COR_COLUMNS(scan);
      b=TBL_COLS(COR_TBL(scan));
      while(b)
        {
          if(!a || COL_NO(a) > COL_NO(b))
            { /* add skipped column to scan column list */
              extern TXTREF copy_tree(TXTREF); /* kitty/conv.c */
              TXTREF c;

              COPY_NODE (SCOLUMN, b, c);
              COL_TBL (c) = scan;
              COL_DEFAULT(c) = copy_tree(COL_DEFAULT(c));
              
              if(a0) COL_NEXT(a0)      = c;
              else   COR_COLUMNS(scan) = c;
              COL_NEXT(c) = a;
              a0 =c;
            }
          else if (COL_NO(a) == COL_NO(b))
            a=COL_NEXT(a0=a);
          else /* IF COL_NO(a) < COL_NO(b) */
            yyfatal("unexpected column number in scan ");
          b=COL_NEXT(b);
        }
    }
}
