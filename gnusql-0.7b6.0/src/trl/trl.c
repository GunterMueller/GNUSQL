/*
 *  trl.c  - tree library support for GNU SQL precompiler
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

/* $Id: trl.c,v 1.248 1998/07/30 03:23:40 kimelman Exp $ */

#include "vmemory.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "trl.h"
#include "tassert.h"
#include "trlinter.h"
#include "tree_gen.h"
#include "type_lib.h"
#include "cycler.h"
#include "typeif.h"
#include <assert.h>
#include <fcntl.h>

/*******==================================================*******\
*******             tree  node description                 *******
\*******==================================================*******/
#define DEF_TOKEN(CODE,NAME,FORMAT,STRUCT,FLAGS)  \
      {NAME,FORMAT,STRUCT,FLAGS,0},

struct tkn_info token_info[NUM_TRN_CODE] =
{
#include "treecode.def"
  {"Last_token", "*", ' ', " ", 0}
};

/*******==================================================*******\
*******             tree  flags description                *******
\*******==================================================*******/

#define DEF_FLAG_BIT(CODE,NAME,CLASS,BIT_NO)  DEF_FLAG(CODE,NAME,CLASS)
#define DEF_FLAG(CODE,NAME,CLASS)             {NAME,CLASS,01L<<bt_##CODE},
struct flag_info flg_info[NUMBER_OF_FLAGS] =
{
#include "treeflag.def"
  {NULL, ' ', 0}
};


/*******==================================================*******\
*******           Static functions                         *******
\*******==================================================*******/
static VADR tree_vm_segment = (VADR) NULL;
static struct tree_root *trl_forest_descriptor = (struct tree_root *) NULL;

static void
trl_init (void)
{
  static i4_t init_trl = 0;
  register i4_t no;
  register enum token code;
  register enum flags f, fb;
  MASKTYPE msk;
  if (init_trl)
    return;
  for (code = 0; code < LAST_TOKEN; code++)
    {
      no = LENGTH (code) = strlen (FORMAT (code));
      msk = 0;
      if (code == OPERAND)
        continue;
      for (f = 0; f < LAST_FLAG; f++)
        if (compatible_tok_fl (code, f)) {
          if (TstF (msk, f) == 0L) {    /* if flag hasn't already been set */
            SetF (msk, f);              /* set it                          */
          } else {                      /* in other case                   */
            for (fb = 0; fb < f; fb++)  /* check again all previous flags  */
              if (TstF (FLAG_VALUE (f), fb))    /* to find source of       */
				                /* collision               */
                fprintf (STDERR,        /* and type report about it        */
                         "Internal error: misplaced flags '%s' and '%s'"
                         " for code '%s'\n",
                         NAME_FL (f), NAME_FL (fb), NAME (code)
                  );
          }
        }
    }
  init_trl = 1;
}

void
init_tree (void)
{
  if (!tree_vm_segment)
    install (NULL);
}

/*
 * 'change_tree_segment' set internal  'current' segment pointer to the segment
 * where 'ptr_inside' points into. The function also checks if this segment is 
 * really tree segment. 'Action' is a code of additional activity with this 
 * segment. 0 means just switch to this segment. 1 - means switch to the 
 * segment and add its descriptor to the list of acceptable tree segment. 
 * -1 means delete given segment from tree segment list
 */

static TXTREF 
change_tree_segment(TXTREF ptr_inside,i4_t action)
{
  static struct t_seg { 
    TXTREF         tree_id;
    struct t_seg  *next,
                  *prev;
  }	                *head=NULL;
  register TXTREF  	 seg;
  register struct t_seg *cur;
  
  trl_init ();
  assert( (-1<=action) && (action <=1));
  seg = ptr_inside ? get_vm_segment_id(ptr_inside) : TNULL;
  for( cur = head; cur && cur->tree_id != seg ; cur = cur->next);
  if ( !cur && action != 1 )/* on attempt to switch to or delete             */
                            /* unregistered segment                          */
    return tree_vm_segment; /* do nothing - even don't become angry          */
  if (cur && action == -1 ) /* delete some segment                           */
    {
      if (cur->prev == NULL)
	head=cur->next;
      else 
	{
	  cur->prev->next=cur->next;
	  if (cur->next) 
	    cur->next->prev=cur->prev;
	}
      xfree(cur);
      cur = head;
    }
  else if ( !cur && action == 1 ) /* add segment which hasn't been found     */
    {
      cur=xmalloc(sizeof(*cur));
      cur->tree_id = seg;
      if (head)
	{
	  cur->next = head;
	  cur->next->prev = cur;
	}
      head = cur;
    }
  /* right now 'cur' points to desirable segment so let's switch to it       */
  if(!cur)  /* if there is no available segment                              */
    {
      tree_vm_segment = TNULL;
      trl_forest_descriptor = 0;
    }
  else /* if everything is ok */
    {
      VADR vo = GET_CURRENT_SEGMENT_ID;
      tree_vm_segment=cur->tree_id;
      switch_to_segment (tree_vm_segment);
      seg = resolve_local_reference ("_TRL_descriptor");
      if (!seg) 
	{
	  seg = tree_vm_segment + vm_ob_alloc (sizeof(struct tree_root), 
					       sizeof(TXTREF));
	  register_export_address (seg,"_TRL_descriptor");
	  assert ( seg == resolve_local_reference ("_TRL_descriptor") );
          {
            /*
             * because 'struct tree_root' is collection of TXTREFs we need
             * to register them as relocate.
             */
            i4_t i = sizeof(struct tree_root)/sizeof(TXTREF);
            while(i--)
              register_relocation_address( seg + i*sizeof(TXTREF));
          }
        }
      switch_to_segment (vo);
      trl_forest_descriptor = (struct tree_root *) vpointer (seg);
    }
  return tree_vm_segment;
}

/*******==================================================*******\
*******           GLOBAL functions                         *******
\*******==================================================*******/

TXTREF *
Root_ptr (enum kind_of_root sel)
{
  switch(sel)
    {
    case Tree_Root:
      return &(trl_forest_descriptor->root);
    case Vcb_Root:
      return &(trl_forest_descriptor->vcbroot);
    case G_Author:
      return &(trl_forest_descriptor->author);
    case Hash_Tbl:
      return &(trl_forest_descriptor->hash_tbl);
    case Current_Tree_Segment:
      return &(tree_vm_segment);
    }
  return (TXTREF*)NULL;
}

TXTREF  
set_current_tree_segment(TXTREF ptr_inside)
{
  return change_tree_segment(ptr_inside,0);
}

TXTREF 
get_current_tree_segment(void)
{
  return tree_vm_segment;
}

/*
 * create new tree segment, mark them as a current one and return identifier 
 * of it.
 */
 
TXTREF 
create_tree_segment(void)
{
  register TXTREF tr; 
  if ( (tr = create_segment () ) == TNULL)
    return TNULL;
  assert( tr == change_tree_segment(tr,1));
  GL_AUTHOR = ltr_rec (get_user_name ());
  return tree_vm_segment;
}

/* do the same but load tree from buffer. */

TXTREF 
load_tree_segment(void *tree_buffer,i4_t len)
{
  register TXTREF tr; 
  if ( (tr = link_segment (tree_buffer, len)) == TNULL)
    return TNULL;
  assert( tr == change_tree_segment(tr,1));
  return tree_vm_segment; 
}

#define CHECK_TREE_DISPOSE_INTEGRITY

void   
dispose_tree_segment(TXTREF ptr_inside)
{
  VADR   sid;
  TXTREF cs=tree_vm_segment,ds;
  if (ptr_inside == TNULL)
    return;
  sid = get_vm_segment_id(ptr_inside);
  if (cs == sid) cs = 0;
  if (sid != change_tree_segment(ptr_inside,0))
    return;
#ifdef CHECK_TREE_DISPOSE_INTEGRITY
  /* free space */
  while (get_statement (1))
    del_statement (1);
  free_line (VCB_ROOT);
  free_hash ();
  ds = tree_vm_segment;
  {
    VADR seg = resolve_local_reference ("_TRL_descriptor");
    if (seg && ds == get_vm_segment_id(seg)) 
      vmfree(seg);
  }
#endif
  change_tree_segment(ptr_inside,-1);
  if (cs && cs != tree_vm_segment)
    change_tree_segment(cs,0);
#ifdef CHECK_TREE_DISPOSE_INTEGRITY
/*
 * The code below isn't really need but should work if all logic  is 
 * correct. Now it isn't so. Therefore we have to avoid this testing 
 * now to be able to check other parts of system.
 */
  if (!unlink_segment (ds, 0))
    {
      yyerror ("Note! freing memory isn't clean");
      errors--;               /* !!!!!!! */
      unlink_segment (ds, 1);
    }
#else
  unlink_segment (ds, 1);
#endif 
}

void *
extract_tree_segment(TXTREF ptr_inside,i4_t *len)
{
  change_tree_segment(ptr_inside,-1);
  return export_segment (ptr_inside, len, 1);
}

#define ERR(cond,msg) if (cond) { yyerror(msg); rc = SQL_ERROR; goto ERR_proc; }

CODERT
install (char *s)
{
  void *dump = NULL;
  i4_t   len = 0;
  FILE *fd; 
  CODERT rc = SQL_SUCCESS; 
  if (s)
    {                           /* loading tree binary image from file */
      fd= fopen (s, "r");
      ERR(fd == NULL,"Can't open tree binary file");
      fseek (fd, 0, SEEK_END);
      len = ftell (fd);
      fseek (fd, 0, SEEK_SET);
      dump = xmalloc (len);
      fread (dump, len, 1, fd);
      fclose (fd);
      if(!load_tree_segment(dump,len))
	{
	  fprintf (STDERR,
		   "Error occured during loading tree from module '%s'",
		   s);
	  return SQL_ERROR;
	}
    }
  else if(!create_tree_segment())
    return SQL_ERROR;
ERR_proc:
  return rc;
}

CODERT
finish (char *s)
{
  void  *dump = NULL;
  i4_t    len = 0;
  FILE  *fd;
  CODERT rc = SQL_SUCCESS; 
  if (s)
    {
      dump = extract_tree_segment (tree_vm_segment, &len);
      ERR(!dump,"Error occured during packing tree to binary array");
      fd = fopen (s, "w");
      ERR(!fd,"Can't open output binary file");
      fwrite (dump, len, 1, fd);
      fclose (fd);
    }
  /* changed by dkv */
  /*
  else
  dispose_tree_segment(tree_vm_segment);
  */
ERR_proc:
  if (dump)
    xfree(dump);
  return rc;
}

#undef ERR

/*******==================================================*******\
*******      Constructors and destructors                  *******
\*******==================================================*******/

i4_t 
trn_size(enum token code, MASKTYPE msk)
{
  register i4_t size;
  
  size = SIZE_TRN (code);
  if (TstF (msk, PATTERN_F))
    size += ADD_TRN_SIZE(code);
  if (TstF(msk,ACTION_F))
    size += ADD_TRN_SIZE(code);
  return size;
 }

TXTREF
gen_node1 (enum token code, MASKTYPE msk)
{
  static   i4_t scan_no = 0;
  register i4_t size;
  register TXTREF node = TNULL;
  VADR     old ;
  init_tree ();
  if (code == UNKNOWN || code == NIL_CODE)
    return TNULL;
  
  size = trn_size(code,msk);
  /*-----------------------------*/
  old = GET_CURRENT_SEGMENT_ID;
  switch_to_segment (tree_vm_segment);
  node = GET_CURRENT_SEGMENT_ID + vm_ob_alloc (size, sizeof (tr_union));
  switch_to_segment (old);
  /*-----------------------------*/
  Ptree (node)->code = code;
  MASK_TRN (node) = msk;
  switch (code)
    {
    case NOOP:
      XLNG_TRN(node,0) = 1;
      break;
    case USERNAME:
      USR_TYPE(node) = pack_type(SQLType_Char,MAX_USER_NAME_LNG,0);
      break;
    case SCAN:
      {
        char str[100];
        sprintf(str,"scan%d",scan_no++);
        COR_NAME(node) = ltr_rec(str);
      }
      COR_NO(node) = -1;
    case INDEX:
    case UNIQUE:
    case PRIMARY:
    case CURSOR:
    case PARAMETER:
      SetF_TRN (node, VCB_F);
      break;
    default:
      if (Is_Table (node) || Is_Column (node))
        SetF_TRN (node, VCB_F);
      if (Is_Column (node))
        COL_NO(node) = -1;
    }
  {
    register char  *p;
    register i4_t   i;
	
    for ( p=FORMAT(code),i=-1; *p; p++,i++) 
      switch(*p){
      case 't':case 'v':case 'V':case 'L':case 'T':case 's':
      case 'r':case 'd':case 'P':case 'N':
	register_relocation_address( node + sizeof(tree_t) + i * sizeof(tr_union) );
      }
  }
  return node;
}

TXTREF
gen_node (enum token code)
{
  return gen_node1 (code, 0);
}

void
free_node (TXTREF node)
{
  if (!node)
    return;
  if ( tree_memory_mode_state )
    {
      fprintf (STDERR, ">>>>>>>Freeing node at %X ...", (u4_t) node);
      debug_trn (node);
      fprintf (STDERR, "<<<<<<<======================\n");
    }
  if (!Ptree(node))
    {
      fprintf (STDERR, "warning (internal problem): "
               "attemp to free unused memory\n");
      return;
    }
  vmfree (node);
}

TXTREF
gen_vect (i4_t len)              /* generate vector     */
{
  register i4_t s;
  register TXTREF vec;

  init_tree ();
  s = sizeof (struct trvec_def) + (len - 1) * sizeof (tr_union);
    {
      VADR old = GET_CURRENT_SEGMENT_ID;
      switch_to_segment (tree_vm_segment);
      vec = GET_CURRENT_SEGMENT_ID + vm_ob_alloc (s, sizeof (tr_union));
      switch_to_segment (old);
    }
  VLEN (vec) = len;
  return vec;
}

TXTREF
realloc_vect (TXTREF n, i4_t newlen)
{
  register TXTREF v;
  register i4_t l;
  v = gen_vect (newlen);
  for ( l = gmin(VLEN(n),newlen)  ; l--;)
    VOP (v, l) = VOP (n, l);
  free_vect (n);
  return v;
}

void
free_vect (TXTREF vec)
{
  vmfree (vec);
}

PNODE
Pnode (register TXTREF n)
{
  return (PNODE) vpointer (n);
}

TXTREF
gen_const_node (SQLType code, char *info)
{
  TXTREF n;
  n = gen_node1 (CONST, 0);
  if (info)
    CNST_NAME (n) = ltr_rec (info);
  switch (code)
    {
    case SQLType_Char:
      CNST_STYPE (n) = pack_type (code, info ? strlen (info) : 0 , 0);
      break;
    case SQLType_Int:
    case SQLType_Long:
    case SQLType_Short:
      if ( strlen(info) > 5 )
        CNST_STYPE (n) = pack_type (SQLType_Int, 0, 0);
      else
        CNST_STYPE (n) = pack_type (SQLType_Short, 0, 0);
      break;
    case SQLType_Num:
#if 1
      {
        register i4_t l, i = strlen (info);
        register char *p = info + i;
        for (l = i; i >= 0; p--, i--)
          if (*p == '.')
            break;
        CNST_STYPE (n) = pack_type (code, l - 1, l - i - 1);
      }
      break;
#else
      code = SQLType_Real;
#endif
    default:
      CNST_STYPE (n) = pack_type (code, 0, 0);
    }
  return n;
}

/*******==================================================*******\
*******   Common tree and vocabulary structure support     *******
\*******==================================================*******/

CODERT
add_statement (TXTREF p)
{
  TXTREF cp;
  if (CODE_TRN(p) != CUR_AREA)
    SetF_TRN(p,HAS_VCB_F);
  if (!ROOT)
    {
      ROOT = p;
      STMT_UID (p) = 0;
    }
  else
    {
      cp = ROOT;
      while (RIGHT_TRN (cp) != (TNULL))
        cp = RIGHT_TRN (cp);
      RIGHT_TRN (cp) = p;
      STMT_UID (p) = STMT_UID (cp) + 1;
    }
  RIGHT_TRN (p) = TNULL;
  return 0;
}

CODERT
del_statement (i4_t number)
{
  register TXTREF pt = TNULL, t = ROOT;
  if (number <= 0)
    yyfatal ("Internal error: attempt to delete statement with ID < 0");
  while (--number && t)
    {
      /* if(STMT_UID(t)==number) return t; */
      pt = t;
      t = RIGHT_TRN (t);
    }
  if (!t)
    return 0;
  if (!pt)
    ROOT = RIGHT_TRN (t);
  else
    RIGHT_TRN (pt) = RIGHT_TRN (t);
  free_tree (t);
  return 0;
}

TXTREF
get_statement (i4_t number)
{
  register TXTREF t = ROOT;
  while (--number && t)
    {
      /* if(STMT_UID(t)==number) return t; */
      t = RIGHT_TRN (t);
    }
  return t;
}

void
free_line (TXTREF c)
{
  register TXTREF t;
  if ( ! c )
    return;
  if (!Ptree(c))
    {
      fprintf (STDERR, 
	       "warning (internal problem): attemp to free unused memory\n");
      return;
    }
  if ( TstF_TRN (c, VCB_F))
    {
      enum token code = CODE_TRN(c);
      if (Is_Table (c))
	{
	  if (((code == VIEW) ||
	       (code == TMPTABLE)) &&
	      (VIEW_QUERY (c)))
	    {
	      t = VIEW_QUERY (c);
	      VIEW_QUERY (c) = TNULL;
	      free_tree (t);
	    }
	  else if (( code == TABLE) &&
		   (TBL_CONSTR (c) ))
	    {
	      t = TBL_CONSTR (c); 
	      TBL_CONSTR (c) = TNULL;
	      free_line (t);
	    }
	}
      else if (code == COLUMN)
	{
          t = COL_DEFAULT(c); 
	  COL_DEFAULT (c) = TNULL;
	  free_node (t);
	}
      else if ( code == UNIQUE || code == PRIMARY || code == INDEX)
	{
	  t=DOWN_TRN(c);
	  DOWN_TRN(c) = TNULL;
	  free_line(t);
	}
    }
  t = RIGHT_TRN(c);
  RIGHT_TRN(c) = TNULL;
  free_line(t);
  if (Is_Scan (c))
    {
      COR_TBL(c) = TNULL;
      t = COR_COLUMNS(c);
      COR_COLUMNS(c) = TNULL;
      free_line (t);
    }
  else if (Is_Table (c))
    {
      free_line (IND_INFO(c));
      IND_INFO(c) = TNULL;
      t = TBL_COLS (c);
      TBL_COLS(c) = TNULL;
      free_line (t);
    }
  if (TstF_TRN (c, VCB_F))
    free_node (c);
  else
    free_tree (c);
}

void
free_tree (TXTREF root)
{
  register TXTREF t;
  if (!root)
    {
      fprintf (STDERR, "warning (internal problem): "
               "null pointer is tried to free\n");
      return;
    }
  if (!Ptree(root))
    {
      fprintf (STDERR, "warning (internal problem): "
               "attemp to free unused memory\n");
      return;
    }
  
  if (TstF_TRN (root, VCB_F))
    return;
  if (HAS_DOWN (root))
    {
      t = DOWN_TRN (root);
      DOWN_TRN(root) = TNULL;
      ARITY_TRN(root) = 0;
      free_line(t);
    }
  
  {
    register char *fmt = FORMAT (CODE_TRN (root));
    register i4_t   i, l;
    for (i = 0; *fmt; fmt++, i++)
      {
#define TST_PAT_BIT(n) TST_BIT(XLNG_TRN(root,PTRN_OP(CODE_TRN(root),n)),PTRN_BIT(n))
        if ( TstF_TRN(root,ACTION_F) && TST_PAT_BIT(i) )
          continue;
        switch (*fmt)
          {
          case 'N':
            if (XTXT_TRN (root, i))
	      {
		free_line (XTXT_TRN (root, i));
		XTXT_TRN(root,i) = TNULL;
	      }
            break;
          case 't':
	  case 'P':
            if (XTXT_TRN (root, i))
	      {
		free_tree (XTXT_TRN (root, i));
		XTXT_TRN(root,i) = TNULL;
	      }
            break;
          case 'T':
            if (XVEC_TRN (root, i))
              for (l = 0; l < XLEN_VEC (root, i); l++)
		{
		  free_tree (XVECEXP (root, i, l));
		  XVECEXP (root, i, l) = TNULL;
		}
          case 'L':
            if (XVEC_TRN (root, i))
	      {
		free_vect (XVEC_TRN (root, i));
		XVEC_TRN (root, i) = TNULL;
	      }
            break;
          default:
            break;
          }
      }
  }
  if (Is_Statement (root))
    {
      if (CODE_TRN (root) == CUR_AREA)
	del_info (STMT_VCB (root));
      else
	free_line (STMT_VCB (root));
      STMT_VCB(root) = TNULL;
    }
  if ( CODE_TRN(root) == NOOP)
    {
      XLNG_TRN(root,0)--;
      if (XLNG_TRN(root,0)>0)
	return;
    }
  free_node (root);
}

/*******==================================================*******\
*******            Other functions                         *******
\*******==================================================*******/

i4_t
compatible_tok_fl (enum token code, enum flags fcode)
{
  register char *p;
  if (code == OPERAND)
    return 1;
  for (p = FLAGS (code); *p; p++)
    if (*p == CLASS (fcode))
      return 1;
  return 0;
}

TXTREF
copy_tree (TXTREF src)
{
  TXTREF dest, last_oper, src_oper;

  if(!src)
    return TNULL;
  
  last_oper = TNULL;
  dest = copy_trn (src);
  
  if (dest && HAS_DOWN (dest))
    {
      ARITY_TRN (dest) = ARITY_TRN (src);
      
      for (src_oper = DOWN_TRN (src);
           src_oper;
           src_oper = RIGHT_TRN (src_oper)
           )
        if (last_oper)
          last_oper = RIGHT_TRN (last_oper) = copy_tree (src_oper);
        else
          last_oper = DOWN_TRN (dest)       = copy_tree (src_oper);
      
      RIGHT_TRN (last_oper) = TNULL;
    }
  return dest;
}

#define CHECK_VCB {copy = add_info_l(copy);TASSERT(copy == tmp_trn,copy);}

TXTREF
copy_trn(register TXTREF orig)
{
  register TXTREF copy;
  register enum token code;
    
  DECL_STACK (copy_trn_st, TXTREF);
  
  if (!orig)
    return orig;
  /* Vocabulary check */
  code = CODE_TRN (orig);
  copy = TNULL;

 
  if (TstF_TRN (orig, VCB_F))
    {
      TXTREF c;
      copy = find_entry(orig);
      if (copy)      /* local copy */
        return copy;
      COPY_NODE (code, orig, copy);
      switch(code)
        {
        case TABLE:
        case VIEW:
        case TMPTABLE:
        case ANY_TBL:
          TBL_FNAME(copy) = ltr_rec(STRING(TBL_FNAME(orig)));
          TBL_NAME(copy) = ltr_rec(STRING(TBL_NAME(orig)));
          break;
        case SCAN:
          COR_NAME(copy) = (COR_NAME(orig)?ltr_rec(STRING(COR_NAME(orig))):TNULL);
          COR_TBL(copy)  = copy_trn(COR_TBL(orig));
          break;
        case PRIMARY:
        case UNIQUE:
        case INDEX:
          break;
        case COLUMN:
        case SCOLUMN:
          COL_TBL(copy) = copy_trn(COL_TBL(copy));
        case PARAMETER:
        case CURSOR:
          /* col_name is equivalent here to par_name, CUR_name */
          COL_NAME(copy) = ltr_rec(STRING(COL_NAME(orig)));
          break;
        default:
          lperror("unexpected vcb node: '%s' ",NAME(code));
          return copy;
        }
      c = find_entry(copy);
      if(!c && code==SCOLUMN)
        c = add_info_l(copy);
      if (c)      /* has already copyed to new segment */
        {
          if(c!=copy)
            free_node(copy);
          return c;
        }
    }
  if (TstF_TRN (orig, MARK_F))
    {
      yyerror("attempt to copy cycled tree");
      ClrF_TRN (orig, MARK_F);
      debug_trn (orig);
      SetF_TRN (orig, MARK_F);
      return TNULL;
    }
  if (Is_Statement (orig))
    {
      PUSHS (copy_trn_st, LOCAL_VCB_ROOT);
      LOCAL_VCB_ROOT = TNULL;
    }
  if (!copy)
    COPY_NODE (code, orig, copy);
  if (TstF_TRN (orig, VCB_F) && Is_Table(copy))
    {
      TXTREF tmp_trn = copy;
      if(TstF_TRN(orig,CHECKED_F))
        ClrF_TRN(copy,CHECKED_F);
      CHECK_VCB;
    }
  SetF_TRN (orig, MARK_F);
  {
    register char *fmt;
    register i4_t i;
    fmt = FORMAT (code);
    for (i = 0; *fmt; fmt++, i++)
      switch (*fmt)
        {
        case 's':
        case 'S':
	  XLTR_TRN(copy,i) = ((XLTR_TRN(orig,i)  != 0) ?
			      ltr_rec(STRING(XLTR_TRN(orig,i))) : 
			      TNULL);
          break;
        case 'N':
          {
            TXTREF v,v1;
            
            v=XTXT_TRN (orig, i);
            v1 = XTXT_TRN (copy, i) = copy_trn (v);
            while(v && RIGHT_TRN(v))
              {
                v = RIGHT_TRN(v);
                RIGHT_TRN(v1) = copy_trn(v);
                v1 = RIGHT_TRN(v1);
              }
          }
          break;
        case 't':
        case 'v':
        case 'P':
          XTXT_TRN (copy, i) = copy_trn (XTXT_TRN (orig, i));
          break;
        case 'V':
          XTXT_TRN (copy, i) = TNULL;
          break;
        case 'r':
          RIGHT_TRN (copy) = TNULL;
          break;
        case 'd':
          DOWN_TRN (copy) = TNULL;
          break;
        case 'a':
          ARITY_TRN (copy) = 0;
          break;
        case 'L':
        case 'T':
          {
            register TXTREF vec, vec1;
            register i4_t j;
            vec = XTXT_TRN (orig, i);
            j = VLEN (vec);
            XTXT_TRN (copy, i) = vec1 = gen_vect (j);
            while (j--)
              if (*fmt == 'T')
                VOP (vec1, j).txtp = copy_trn (VOP (vec, j).txtp);
              else
                VOP (vec1, j) = VOP (vec, j);
            break;
          }
        default:
          break;
        }
  }
  /* Vocabulary check */
  if (TstF_TRN (copy, VCB_F))
    {
      TXTREF tmp_trn = copy;
      switch (code)
        {
        case COLUMN:
          break;
        case SCOLUMN:           /* link all backward reference */
        case PARAMETER:
        case CURSOR:
        case SCAN:
          CHECK_VCB;
          break;
        default:
          if (Is_Table (copy))
            {
	      if(TstF_TRN(orig,CHECKED_F))
		SetF_TRN(copy,CHECKED_F);
            }                   /* Is_Table */
        }
    }
#undef CHECK_VCB
  ClrF_TRN (orig, MARK_F);
  if (!copy)
    {
      debug_trn (orig);
      yyfatal("Panic: null copy of subtree (see above) produced");
    }
  if (Is_Statement (copy))
    {
      STMT_VCB (copy) = LOCAL_VCB_ROOT;
      POPS (copy_trn_st, LOCAL_VCB_ROOT);
    }
  return copy;
}

/* 
 * Return 1 if X and Y are identical-looking trn's. This is the Lisp function
 * EQUAL for TXTREF arguments.  
 */

i4_t
trn_equal_p (TXTREF x, TXTREF y)
{
  register i4_t i;
  register enum token code;
  register char *fmt;
  register TXTREF parmx, parmy;

  if (x == y)
    return 1;
  if (x == 0 || y == 0)
    return 0;

  code = CODE_TRN (x);
 /* call itself and swap parameters to avoid NOOPs */
  if (code == NOOP)
    return trn_equal_p (y,DOWN_TRN(x));

  /* Trn's of different codes cannot be equal.  */
  if (code != CODE_TRN (y))
    return 0;

  {
    MASKTYPE msk, msk1, msk2;
    msk1 = 0;
    SetF (msk1, PATTERN_F);
    msk1 = 2 * msk1 - 1;

    msk = MASK_TRN (x);
    msk CLRVL msk1;             /* clear all bits less than PATTERN_F */
    ClrF (msk, MARK_F);

    msk2 = MASK_TRN (y);
    msk2 CLRVL msk1;            /* clear all bits less than PATTERN_F */
    ClrF (msk2, MARK_F);
    if (msk != msk2)
      return 0;
  }

  /* Compare the elements.  If any pair of corresponding elements fail to
     match, return 0 for the whole things.  */

  fmt = FORMAT (code);
  for (i = LENGTH (code) - 1; i >= 0; i--)
    {
      switch (fmt[i])
        {
        case 'i':
        case 'f':
        case 'l':
        case 'a':               /* because they must has equal number of params*/
          if (XLNG_TRN (x, i) != XLNG_TRN (y, i))
            return 0;
          break;
		  
        case 's':               /* because identical literals save just once   */
          if (XLNG_TRN (x, i) == XLNG_TRN (y, i))
	    break;
          if (strcmp(STRING(XLTR_TRN (x, i)),STRING(XLTR_TRN (y, i))))
            return 0;
	  break;

        case 't':
        case 'v':
        case 'P':
          if (trn_equal_p (XTXT_TRN (x, i), XTXT_TRN (y, i)) == 0)
            return 0;
          break;

        case 'V':
        case 'p':
        case '0':
        case 'd':
        case 'r':
	case 'x':
        case 'R':
	  break;
	  
        case 'y':
	  if (XLNG_TRN (x, i) != XLNG_TRN(y, i))
	    return 0;
	  break;
      
        case 'L':
	  {
	    TXTREF patvect=XVEC_TRN(x,i);
	    TXTREF invect=XVEC_TRN(y,i);
	    i4_t j=VLEN(patvect);
	    if(j!=VLEN(invect))
	      return 0;
	    for(;j;j--)
	      if(VOP(patvect,j).l != VOP(invect,j).l)
		return 0;
	  }
	    break;
        case 'T':               /* array of longs and TXTREF's */
	   { 
	    TXTREF patvect=XVEC_TRN(x,i);
	    TXTREF invect=XVEC_TRN(y,i);
	    i4_t j=VLEN(patvect);
	    if(j!=VLEN(invect))
	      return 0;
	    for(;j;j--)
	      if(VOP(patvect,j).txtp != VOP(invect,j).txtp)
		return 0;
	  }
          break;

        default:
          yyfatal ("TRL.trn_equal_p: unexpected format character");
        }
    }
  /* param's check */
  if (HAS_DOWN (x))
    for (
      i = ARITY_TRN (x),
	parmx = DOWN_TRN (x),
	parmy = DOWN_TRN (y);
      i && parmx && parmy;
      i--,
	parmx = RIGHT_TRN (parmx),
	parmy = RIGHT_TRN (parmy)
      )
      if (trn_equal_p (parmx, parmy) == 0)
        return 0;
  return 1;
}


/*******==================================================*******\
*******            Compiler limit functions                *******
\*******==================================================*******/

i4_t
trl_wrn (char *msg, char *file, i4_t line, trn ptr)
{
  fprintf (STDERR, "\n\n%s:%d: %s \n\n", file, line, msg);
  debug_trn_d (ptr);
  return 0;
}

i4_t
trl_err (char *msg, char *file, i4_t line, trn ptr)
{
  trl_wrn (msg, file, line, ptr);
  yyfatal ("Abort");            /* ==> Exit */
  /* Unreachable */
  assert(0);
  return 0;
}

i4_t
fmt_eq (char *fmt, char *tst)
{
  register char *f = fmt, *t = tst;
  while (*t)
    if (*(f++) != *(t++))
      return 0;
  return 1;
}


#ifdef CHECK_TRL

trn
test_exist (trn node, i4_t n, char *_FILE__, i4_t _LINE__)
{
  register i4_t l;
  enum token code = CODe_TRN1 (node);
  l = LENGTH (code);
  if (Tstf_TRN (node, PATTERN_F))
    l += PTRN_ELEM (code);
  if (Tstf_TRN (node, ACTION_F))
    l += PTRN_ELEM (code);
  if (n < l)
    return node;
  trl_err ("TRL: number of operand is too much in ", _FILE__, _LINE__, node);
  /* unreachable code */
  return 0;
}

trn
test_node (trn node, char *f, i4_t l)
{
  register enum token code;
  if (node == NULL)
    trl_err ("TRL: null tree reference ", f, l, NULL);
  code = node->code;
  if (code <= UNKNOWN || code >= LAST_TOKEN)
    trl_err ("TRL: unexpected tree code ", f, l, NULL);
  return node;
}

i4_t
tstv_exist (trvec vec, i4_t n, char *_FILE__, i4_t _LINE__)
{
  if (n < VLEn1 (vec))
    return n;
  fprintf (STDERR,
        "\n\nInternal error: %s:%d: number of vector element is too much\n",
           _FILE__, _LINE__);
  yyfatal ("Abort");            /* ==> Exit */
  /* Unreached */
  return 0;
}

trvec
tstv_vec (trvec vec, char *f, i4_t l)
{
  if (vec)
    return vec;
  trl_err ("TRL: null vector reference ", f, l, NULL);
  /* unreachable code */
  return NULL;
}

tr_union *
xOp_parm (trn node, i4_t n, char c, char *_FILE__, i4_t _LINE__)
{
  register char *fmt = FORMAT (CODe_TRN1 (node));
  register i4_t nn = n;
  if (fmt[nn] != c)
    {
      for (nn = 0; fmt[nn]; nn++)
        if (fmt[nn] == c)
          break;
      if (fmt[nn] == 0)
        trl_err ("TRL: incorrect format request in ", _FILE__, _LINE__, node); /* ==>exit */
    }
  return &(node->operands[nn]);
}

tr_union *
xOp_parms (trn node, i4_t n, char *s, char *_FILE__, i4_t _LINE__)
{
  if (fmt_eq (FORMAT (CODe_TRN1 (node)) + n, s))
    return &(node->operands[n]);
  trl_err ("TRL: incorrect struct format request in ", _FILE__, _LINE__, node);
  return NULL;
}

#endif
