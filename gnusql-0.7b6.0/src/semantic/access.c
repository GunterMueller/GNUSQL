/*
 *   access.c - priviledge checking and some semantic
 *              constrait checking 
 *              GNU SQL server
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Eugene W. Woynov, 1994
 *  Rewritten by Michael Kimelman, 1995
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
 *  Contacts:  gss@ispras.ru
 *
 */

/* $Id: access.c,v 1.246 1997/04/24 17:46:44 kml Exp $ */

#include "setup_os.h"

#if STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
# endif
char *strchr ();
#endif

#include "global.h"
#include "seman.h"
#include "funall.h"
#include "cycler.h"
#include "tassert.h"
#include <assert.h>

#define ACT_SELECT       "S"
#define ACT_INSERT       "I"
#define ACT_DELETE       "D"
#define ACT_UPDATE       "U"
#define ACT_GRANT        "G"
#define ACT_REFERENCES   "R"

typedef struct 
{
  char      author[7];
  i4_t      *ulist;
  i4_t       ui;
  i4_t      *rlist;
  i4_t       ri;
  TXTREF    tbl;
  TXTREF    tblptr;
} tbl_pvlg_t;

#define AUTH_CHK(n,a) \
       auth_chk(n,a,NAME(CODE_TRN(cnode)),LOCATION_TRN(cnode))

#define TYPE_ERR(msg)   \
       SERROR_MSG(1,LOCATION_TRN(cnode),msg)

#define THE_SAME(s1,s2)  (!strcmp(s1,s2)) /* ==0 */

static TXTREF
check_author(tbl_pvlg_t *p)
{
  if(!p || !p->tbl)
    return TNULL;
  switch(CODE_TRN(p->tbl))
    {
    case TMPTABLE:
      if (!THE_SAME(p->author,ACT_SELECT))
        goto end; /* can't be done */
    case VIEW:
      if (TstF_TRN(VIEW_QUERY(p->tbl),RO_F) &&
          !THE_SAME(p->author,ACT_SELECT) &&
          !THE_SAME(p->author,ACT_REFERENCES))
        goto end;
    case TABLE:
      if (THE_SAME (STRING (GL_AUTHOR), STRING (TBL_FNAME (p->tbl))))
        p->tbl = TNULL;
      else if ( THE_SAME ("DEFINITION_SCHEMA" ,STRING (TBL_FNAME (p->tbl))) &&
                THE_SAME ("INFORMATION_SCHEMA",STRING (GL_AUTHOR)))
        p->tbl = TNULL;
      else if (0==tabpvlg ((TBL_TABID (p->tbl)).untabid,STRING (GL_AUTHOR),
                           p->author, p->ui, p->ulist, p->ri, p->rlist))
        p->tbl = TNULL;
      break;
    default:
      yyfatal("");
    }
  
end:
  
  if (p->ulist)
    xfree (p->ulist);
  if (p->rlist)
    xfree (p->rlist);
  p->author[0] =0;
  p ->ui = p->ri = 0;
  return p->tbl;
}

static void
check_update(TXTREF asslist, tbl_pvlg_t *p)
{
  TXTREF  assign;
  i4_t     i;
  enum token code = CODE_TRN(asslist);
  

  TASSERT((code == ASSLIST) ||
          (code == UPDATE)  ||
          (code == REFERENCE),
          asslist);
    
  assign = DOWN_TRN (asslist);

  if (code == REFERENCE)
    {
      TASSERT(p->rlist == NULL, asslist);
      p->ri = ARITY_TRN(asslist);
      p->rlist = (i4_t *) xmalloc (p->ri * sizeof (i4_t));
    }
  else
    {
      TASSERT(p->ulist == NULL, asslist);
      p->ui = ARITY_TRN(asslist);
      p->ulist = (i4_t *) xmalloc (p->ui * sizeof (i4_t));
    }
  
  for (i=0;assign;i++, assign = RIGHT_TRN (assign))
    {
      TXTREF scol = TNULL, tabl = TNULL;
      enum token code1 = CODE_TRN(assign);
      if (code == ASSLIST)
        {
          TASSERT(code1 == ASSIGN, asslist);
          scol = OBJ_DESC (DOWN_TRN (assign));
          TASSERT(CODE_TRN(scol) == SCOLUMN,asslist);
          tabl = COR_TBL (COL_TBL (scol));
        }
      else /* UPDATE or REFERENCE */
        {
          TASSERT(code1 == COLPTR, asslist);
          scol = OBJ_DESC (assign);
          TASSERT(CODE_TRN(scol) == COLUMN,asslist);
          tabl = COL_TBL (scol);
        }
      TASSERT(tabl,assign);
      if(p->tbl == TNULL)
        p->tbl = tabl;
      TASSERT( tabl == p->tbl, asslist);
      if(code==REFERENCE)
        p->rlist[i] = COL_NO (scol);
      else
        p->ulist[i] = COL_NO (scol);
    }
  {
    char *act = (CODE_TRN(asslist)==REFERENCE?ACT_REFERENCES : ACT_UPDATE);
    if (!strstr(p->author,act))
      strcat(p->author,act);
  }
}

static TXTREF
check_act (TXTREF node, char *author)
{
  tbl_pvlg_t t1;

  bzero(&t1,sizeof(t1));
  
  if ( THE_SAME(author,ACT_UPDATE) || THE_SAME(author,ACT_REFERENCES))
    {
      check_update(node,&t1);
      return check_author(&t1);
    }

  switch(CODE_TRN(node))
    {
    case SCAN:
      t1.tbl = COR_TBL(node); break;
    case TABLE:
    case VIEW:
      t1.tbl = node; break;
    default:
      yyfatal("Unexpected node checked");
    }
  strcpy(t1.author,author);
  node = check_author(&t1);
  return node;
}

static void
auth_chk(TXTREF node,char *act,char *text,i4_t where)
{
  TXTREF errtbl;

  errtbl  = check_act (node, act);
  if (errtbl)
    {
      char err[100];
      sprintf(err,"permission denied to %s table '%s.%s'",
              text,
              STRING (TBL_FNAME(errtbl)),
              STRING(TBL_NAME(errtbl)));
      SERROR_MSG(1,where,err);
    }
}

TXTREF
ac_proc(TXTREF cnode,i4_t f)
{
  TXTREF t;
  if (f==0)
    cnode = cycler(cnode, ac_proc, CYCLER_LD);
  else if (f == CYCLER_LD)
    switch(CODE_TRN(cnode))
      {
      case FROM:
        for(t=DOWN_TRN(cnode);t; t = RIGHT_TRN(t))
          auth_chk(TABL_DESC(t),ACT_SELECT,"select",LOCATION_TRN(t));
        cycler_skip_subtree = 1;
        break;
      case TBLPTR:
        t = cnode;
        auth_chk(TABL_DESC(t),ACT_SELECT,"select",LOCATION_TRN(t));
        break;
      default: break;
      }
  return cnode;
}

CODERT
access_main (TXTREF root)
{
  static TXTREF cursor;
  static TXTREF gr_table;
  TXTREF cnode;


  for (cnode = root; cnode; cnode = RIGHT_TRN (cnode))
    {
      if(Is_Statement(cnode))   
        ac_proc(cnode,0);      /* check selection rights */
      switch(CODE_TRN(cnode))
        {
        case CUR_AREA:
          cursor = STMT_VCB(cnode);
          access_main(DOWN_TRN(cnode));
          cursor = TNULL;
          break;
        case DECL_CURS:
        case FETCH:
          break;
        case SELECT:
          if (gr_table)
            AUTH_CHK(gr_table,ACT_SELECT);
          break;
        case DELETE:
          AUTH_CHK((gr_table? gr_table : TABL_DESC(DOWN_TRN(cnode))),ACT_DELETE);
          break;
        case INSERT:
          AUTH_CHK((gr_table? gr_table : TABL_DESC(DOWN_TRN(cnode))),ACT_INSERT);
          break;
        case UPDATE:
          AUTH_CHK( (gr_table? cnode : RIGHT_TRN(DOWN_TRN(cnode))), ACT_UPDATE);
          break;
        case ALTER:
        case CREATE:
          if(CODE_TRN(CREATE_OBJ(cnode))==VIEW)
            /* check rights to see source tables for view */
            ac_proc(VIEW_QUERY(CREATE_OBJ(cnode)),0);
          break;
        case GRANT:
          {
            TXTREF refer = RIGHT_TRN(DOWN_TRN(cnode));
            TASSERT(CODE_TRN(refer) == PRIVILEGIES, cnode);
            gr_table = TABL_DESC(DOWN_TRN(cnode));
            if (ARITY_TRN(refer)>0) 
              access_main(DOWN_TRN(refer));
            else /* check all privileges */
              auth_chk(gr_table,"SDIURG","do all",LOCATION_TRN(cnode));
            gr_table = TNULL;
          }  
          break;
        case REFERENCE:
          AUTH_CHK( cnode, ACT_REFERENCES);
          break;
        case DROP:
          if (strcmp (STRING (GL_AUTHOR), STRING (TBL_FNAME (CREATE_OBJ(cnode)))))
            {
              char err[100];
              sprintf(err,"permission denied to DROP table '%s.%s'",
                      STRING(TBL_FNAME(CREATE_OBJ(cnode))),
                      STRING(TBL_NAME (CREATE_OBJ(cnode))));
              SERROR_MSG(1,LOCATION_TRN(cnode),err);
            }
          break;
        case REVOKE: /* nothing to check - in the worst case we'll do useless scan
                      * of priviledge table at runtime
                      */
          break;
        default:
          debug_trn(cnode);
          yyfatal("Unexpected Statement node");
        }
    }
  return SQL_SUCCESS;
}
