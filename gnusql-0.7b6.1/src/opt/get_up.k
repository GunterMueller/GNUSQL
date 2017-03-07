/*
 *  get_up.k  - getting up nested tables and views
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
 *  Contacts: gss@ispras.ru
 *
 */

/* $Id: get_up.k,v 1.248 1998/09/29 21:26:23 kimelman Exp $ */

#include "cycler.h"
#include "opt.h"
#include "cnf.h"
#include "tree_gen.h"
#include "tassert.h"
#include <assert.h>


typedef struct {
  enum token  frame_code;
  TXTREF      from;
  TXTREF      where;
  TXTREF      add_to_where;
  TXTREF      exists;
} frame_t;

DECL_STACK(nt,frame_t);

/* the following two routines was wriiten to use from debugger */

static void
debug_frame(frame_t *t)
{
#define F          fprintf
#define E          STDERR
#define BRANCH(v)  F(E,"\n%s:",#v)
#define BRANCH1(v) F(E,"\n%s:",#v); debug_trn(t->v);
  
  BRANCH (frame_code);        F(E,"%s\n",NAME(t->frame_code));
  BRANCH1(from);
  BRANCH1(where);
  BRANCH1(add_to_where);
  BRANCH1(exists);
#undef F
#undef E
#undef BRANCH
#undef BRANCH1
}

void
debug_nt_stack(int i)
{
  int depth;
  frame_t *st_frame;
  
  for(depth=0;depth < st_depth(nt_stack) && depth < i ;depth++)
    {
      fprintf(STDERR,
              "---------------------------%d-%d-%d-%d-------------\n",
              depth,depth,depth,depth);
      st_frame=SPTR(nt,depth);
      debug_frame(st_frame);
    }
}

#define LD (flag==CYCLER_LD)
#define DR (flag==CYCLER_DR)
#define RL (flag==CYCLER_RL)

#define CLEAR(v) bzero(&v,sizeof(v))

TXTREF 
get_up_nested_table(TXTREF node,i4_t flag)
{
  frame_t *cf = NULL;

  if (flag<=0)
    {
      return cycler(node,get_up_nested_table,CYCLER_LD + CYCLER_DR - flag);
    }
#if 0
  fprintf(STDERR,"\nnt:%s:%s:",NAME(CODE_TRN(node)),
          (LD?"LD":(DR?"DR":(RL?"RL":"**"))));
#endif
  
  switch(CODE_TRN(node))
    {
    case SELECT:
    case UPDATE:
    case DELETE:
    case INSERT:
      if (!Is_Statement(node))
        break;
    case QUERY:
    case SUBQUERY:
      if(LD)
	{
	  frame_t frame;
	  CLEAR(frame);
          frame.frame_code = CODE_TRN(node);
	  PUSHS(nt,frame);
	}
      else if(DR)
	{
          TXTREF v;
          i4_t    ok = 0;
          
	  frame_t frame;
	  POPS(nt,frame);

          /* delete all scans (and underlied temporary tables)
             which has already transformed to join */
          while (!ok)
            for(ok=1,v = LOCAL_VCB_ROOT;v;v = RIGHT_TRN(v))
              if(CODE_TRN(v)==SCAN && TstF_TRN(v,INDICATOR_F))
                {
                  TXTREF t = COR_TBL(v);
                  ok = 0;
                  COR_TBL(v)=TNULL;
                  del_info(v);
                  if(t && CODE_TRN(t) == TMPTABLE)
                    {
                      TXTREF from = DOWN_TRN(VIEW_QUERY(t));
                      DOWN_TRN(from) = TNULL;
                      ARITY_TRN(from) = 0;
                      del_info(t);
                    }
                  break;
                }
          /* lets process where */
          if (frame.add_to_where) /* if there is something to do */
            {
              TXTREF aw   = frame.add_to_where;
              i4_t    done = 0;
              if(frame.where==TNULL)
                {
                  if (ARITY_TRN(aw) > 1 )
                    {
                      DOWN_TRN(aw) =
                        rule_nested_assotiative(&done, #(And { (DOWN:list <aw>) }));
                      ARITY_TRN(aw) = 1;
                    }
                  add_child(node,aw);
                  frame.where = aw;
                }
              else
                {
                  DOWN_TRN(frame.where) =
                    rule_nested_assotiative(&done, #(And { (DOWN <frame.where>) (DOWN:list <aw>) }));
                  free_node(aw);
                }
            }
          if (st_depth(nt_stack) > 0)
            {
              cf = SPTR(nt,0);
              if ( cf->exists ) /* we are in EXISTS subquery */
                { 
                  /* check for possibilities to transform subquery to join */
                  /*
                    to be filled later
                  */
                }
            }
        }
      break;
    case FROM:
      if (LD)
        {
          cf = SPTR(nt,0);
          cf->from = node;
        }
      break;
    case WHERE:
      if (LD)
        {
          cf = SPTR(nt,0);
          cf->where = node;
        }
      break;
    case EXISTS:
      if (LD)
        { /* mark frame that we come to exist subquery */
          TXTREF pred;
          cf = SPTR(nt,0);
          pred = DOWN_TRN(cf->where);
          if ( pred != node ) /* if exist is NOT the only condition in query */
            {
              if (CODE_TRN(pred) != AND ) /* and there is no conjunction */
                break;
              for (pred = DOWN_TRN(pred) ; pred && pred != node ; pred = RIGHT_TRN(pred));
              if (pred == node)
                cf->exists = node;
            }
        }
      else if(DR)
        {
          cf = SPTR(nt,0);
          if (cf->exists)
            {
              if (cf->exists != node)
                {
                  free_tree(node);
                  node = cf->exists;
                }
            }
        }
      break;
    case TBLPTR: 
      if (LD)
        {
          TXTREF tbl;
          cf = SPTR(nt,0);
          if (cf->from==TNULL)
            cf->from = node;
          else
            TASSERT(CODE_TRN(cf->from)==FROM,cf->from);
          
          tbl = COR_TBL(TABL_DESC(node));
          
          if(CODE_TRN(tbl)==TABLE)
            break;
          
          /* view must be substituted earlier */
          TASSERT(CODE_TRN(tbl)==TMPTABLE,node);
          /* process nested queries */
          VIEW_QUERY(tbl) = get_up_nested_table(VIEW_QUERY(tbl),0);
          
          if(CODE_TRN(VIEW_QUERY(tbl)) != QUERY)
            break;
          /* we can substitute nested table expression */
          SetF_TRN(TABL_DESC(node),INDICATOR_F); /* set mark for replacement */
          { /* here we need to copy tblptrs to new place */
            TXTREF to, tc,t1;
            t1 = tc = TNULL;
            /*       tblptr   from     query      TMPTABLEorVIEW */
            for(to = DOWN_TRN(DOWN_TRN(VIEW_QUERY(tbl           )));
                to;
                to=RIGHT_TRN(to))
              {
                if(tc)
                  tc = RIGHT_TRN(tc) = copy_tree(to);
                else
                  t1 = tc            = copy_tree(to);
              }
            RIGHT_TRN(tc) = RIGHT_TRN(node);
            free_node(node);
            node = t1;
          }
          /* add where of upped query to us */
          {
            TXTREF sel,where;
            sel = RIGHT_TRN(DOWN_TRN(VIEW_QUERY(tbl)));
            where = RIGHT_TRN(sel);
            if (where)
              {
                RIGHT_TRN(sel) = TNULL;
                ARITY_TRN(VIEW_QUERY(tbl)) --;
                if (cf->add_to_where==TNULL)
                  cf->add_to_where = where;
                else
                  {
                    RIGHT_TRN(DOWN_TRN(where)) = DOWN_TRN(cf->add_to_where);
                    DOWN_TRN(cf->add_to_where) = DOWN_TRN(where);
                    ARITY_TRN(cf->add_to_where) ++;
                    free_node(where);
                  }
              }
          }
        }
      break;
    case COLPTR:
      if (LD)
	{
          TXTREF scan = COL_TBL(OBJ_DESC(node));
          if ( TstF_TRN(scan, INDICATOR_F)) 
            { /* do replacement */
              /*           1st sel  selection from     query      tmptable scan     */
              TXTREF sel = DOWN_TRN(RIGHT_TRN(DOWN_TRN(VIEW_QUERY(COR_TBL  (scan)))));
              i4_t    i   = COL_NO(OBJ_DESC(node));
              while(i--)
                sel = RIGHT_TRN(sel);
              sel = copy_tree(sel);
              RIGHT_TRN(sel) = RIGHT_TRN(node);
              free_node(node);
              node = sel;
            }
	}
      break;
    case GRANT:
    case REVOKE:
    case CREATE:
    case ALTER:
    case DROP:
      cycler_skip_subtree = 1;
      break;
    default: break;
    }
  return node;
}
