/*
 *  intlib.c  -  interpretator library of GNU SQL server
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Konstantin Dyshlevoi.
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

/* $Id: intlib.c,v 1.253 1998/09/29 22:23:40 kimelman Exp $ */

#include "global.h"
#include "inprtyp.h"
#include "assert.h"
#include "sql.h"

/* In all leafs field Ptr points to element of type data_unit_t      *
 * where the type of the value is filled by codegen.              */

/* Type of result of subtree must be put into all according nodes *
 * ( if it's the logic value =>  T_BOOL).                         *
 * in nodes for subquery :.Rh- reference to programm for          *
 * handling this subquery.                                        */

extern     data_unit_t *data_st;
StackUnit *stack = NULL;
extern i4_t dt_ind, dt_max;
i4_t        st_ind, st_max;

data_unit_t  *ins_col_values = NULL;
i4_t       numb_ins_col = 0;

static data_unit_t *del_col_values = NULL;
static i4_t numb_del_col = 0;
tab_stat_info *fir_tab_stat = NULL, *cur_tab_stat = NULL;

#define OLDSTAT(op,func) ((col_info[i]).op##_##func)
#define NEWVAL(op)  (op##_col_values[i])

#define DO_CHANGE(op)			\
{					\
  DO_CHANGE_(min, LT, op);		\
  DO_CHANGE_(max, GT, op);		\
}
/* func = 'min' or 'max'; comp = 'LT' or 'GT'; op = 'ins' or 'del' */

#define DO_CHANGE_(func, comp, op)	        \
{						\
  if (use_values)				\
    {						\
      if (NEWVAL(op).dat.nl_fl == REGULAR_VALUE && \
          OLDSTAT(op,func).dat.nl_fl != NULL_VALUE) \
        {					\
          if (OLDSTAT(op,func).dat.nl_fl == REGULAR_VALUE) \
            MK_BIN_OPER (comp, NEWVAL(op), OLDSTAT(op,func)); \
          if (OLDSTAT(op,func).dat.nl_fl == UNKNOWN_VALUE || COMP_RESULT) \
            OLDSTAT(op,func) = NEWVAL(op);	\
        }					\
    }						\
  else						\
    OLDSTAT(op,func).dat.nl_fl = NULL_VALUE;	\
}

i4_t
handle_statistic (i4_t untabid, i4_t tab_col_num,
		  i4_t mod_col_num, i4_t *mod_col_list, i4_t use_values)
/* if use_values == FALSE => there isn't info about old or new values */
{
  i4_t i, error, col_num;
  col_change_info *col_info;
  
  if (!untabid)
    return 0;
  
  /* pointer to info for current table setting to cur_tab_stat : */
  
  if (!cur_tab_stat || cur_tab_stat->untabid != untabid)
    {
      for (cur_tab_stat = fir_tab_stat; cur_tab_stat;
					cur_tab_stat = cur_tab_stat->next)
	if (cur_tab_stat->untabid == untabid)
	  break;
      if (!cur_tab_stat)
	{
	  cur_tab_stat = TYP_ALLOC (1, tab_stat_info);
	  cur_tab_stat->untabid = untabid;
	  cur_tab_stat->ncols = tab_col_num;
	  col_info = cur_tab_stat->col_info =
            TYP_ALLOC (tab_col_num, col_change_info);
	  for (i = 0; i < tab_col_num; i++)
            {
              col_info[i].ins_min.dat.nl_fl = UNKNOWN_VALUE;
              col_info[i].ins_max.dat.nl_fl = UNKNOWN_VALUE;
              col_info[i].del_min.dat.nl_fl = UNKNOWN_VALUE;
              col_info[i].del_max.dat.nl_fl = UNKNOWN_VALUE;
            }
	  cur_tab_stat->next = fir_tab_stat;
	  fir_tab_stat = cur_tab_stat;
	}
    }
  
  col_info = cur_tab_stat->col_info;
  if (mod_col_list) /* UPDATE */
    for (i = 0; i < mod_col_num; i++)
      {
	col_num = mod_col_list[i];
	
	DO_CHANGE (ins);
        DO_CHANGE (del);
      }
  else
    {
      if (mod_col_num) /* INSERT */
	{
          (cur_tab_stat->nrows_change)++;
	  
	  for (i = 0; i < tab_col_num; i++)
            DO_CHANGE (ins);
	}
      else /* DELETE */
	{
          (cur_tab_stat->nrows_change)--;
	  
	  for (i = 0; i < tab_col_num; i++)
            DO_CHANGE (del);
	}
    }
  return 0;
} /* handle_statistic */

static int
process_statistic (int change_in_db)
{
  col_change_info *col_info;
  int i, error, nrows;
  
  while(fir_tab_stat)
    {
      cur_tab_stat = fir_tab_stat;
      if (change_in_db)
        {
          if (cur_tab_stat->nrows_change)
            {
              if ((nrows = get_nrows (cur_tab_stat->untabid)) < 0)
                return nrows;
              
              if ((error = put_nrows (cur_tab_stat->untabid,
                                      nrows + cur_tab_stat->nrows_change)))
                return error;
            }
          
          for (i = 0, col_info = cur_tab_stat->col_info;
               i < cur_tab_stat->ncols; i++, col_info++)
            if ((error = put_col_stat (cur_tab_stat->untabid, i, col_info)))
              return error;
        }
      fir_tab_stat = cur_tab_stat->next;
      xfree (cur_tab_stat->col_info);
      xfree (cur_tab_stat);
    }
  return 0;
} /* process_statistic */

i4_t
change_statistic (void)
{
  return process_statistic(1);
}

int
drop_statistic (void)
{
  return process_statistic(0);
}

i4_t
check_and_put (i4_t mod_col_num, i4_t *mod_col_list, TRNode **tree_ptr_arr,
	       data_unit_t **res_data_arr, S_ConstrHeader *header)
{
  int               i, j, error, reslt ;
  i4_t              read_num, *read_num_arr, max_clm_num, cur_colnum;
  char              null_fl;
  S_Constraint     *constr;
  VADR             *V_constr_arr;
  int              *arg_num_list, *ref_num_list, tab_col_num;
  sql_type_t       *read_type_arr;
  static data_unit_t **read_adrs     = NULL;
  static int        read_adrs_lng = 0;
  
  if (!header)
    return 0;
  
  read_num = header->old_vl_cnt;
  tab_col_num = header->tab_col_num;
  CHECK_ARR_SIZE (ins_col_values, numb_ins_col, tab_col_num, data_unit_t);
  bzero (ins_col_values, tab_col_num * sizeof (data_unit_t));
  if (res_data_arr)
    *res_data_arr = ins_col_values;
  
  /* new values forming : */
  
  for (i = 0; i < mod_col_num; i++)
    {
      error = calculate (tree_ptr_arr[i], NULL, 0, 0, NULL);
      if (error < 0)
	return error;
      ins_col_values[(mod_col_list) ? mod_col_list[i] : i] = DtNextEl;
    }
  
  /* old values reading from scan : */
  
  if (read_num)
    {
      read_num_arr = V_PTR (header->old_vl_nums, i4_t);
      read_type_arr = V_PTR (header->old_vl_types, sql_type_t);
      CHECK_ARR_SIZE (read_adrs, read_adrs_lng, read_num, data_unit_t *);
      CHECK_ARR_SIZE (del_col_values, numb_del_col, tab_col_num, data_unit_t);
      for (i = 0; i < tab_col_num; i++)
	del_col_values[i].type.code = 0;
      
      for (i = 0; i < read_num; i++)
	{
	  read_adrs[i] = del_col_values + read_num_arr[i];
	  read_adrs[i]->type = read_type_arr[i];
	}
	  
      if ((error = read_row (*V_PTR (header->scan, Scanid), read_num,
			     read_num_arr, read_adrs)) < 0)
	return error;
	
      /* if there don't exist some new values then they are equal to old ones */
      for (i = 0; i < tab_col_num; i++)
	if (!(ins_col_values[i].type.code) && (del_col_values[i].type.code))
	  ins_col_values[i] = del_col_values[i];
    }
  
  /* changing statistic info about table & columns */
  error = handle_statistic (header->untabid, tab_col_num,
                            mod_col_num, mod_col_list, TRUE);
  if (error < 0)
    return error;
  
  if (!(header->constr_num))
    return 0;
  
  /* constraints checking : */
  
  V_constr_arr = V_PTR (header->constr_arr, VADR);
  for (i = 0; i < header->constr_num; i++)
    {
      constr = V_PTR (V_constr_arr[i], S_Constraint);
      arg_num_list = V_PTR (constr->col_arg_list, i4_t);
      ref_num_list = V_PTR (constr->col_ref_list, i4_t);
      cur_colnum = constr->colnum;
      
      switch (constr->constr_type)
	{
	case FOREIGN :
	  /* checking of NULL-values in referencing columns list */
	  for (j = 0; j < cur_colnum; j++)
	    {
	      MK_UN_OPER (ISNULL, ins_col_values[arg_num_list[j]]);
	      if (COMP_RESULT)
		/* there is NULL value */
		break;
	    }
	  if (j < cur_colnum)
	    /* not all values of referencing columns are NOT NULL */
	    break;
	  
	  CHECK_ARR_SIZE (read_adrs, read_adrs_lng, cur_colnum, data_unit_t *);
	  for (j = 0; j < cur_colnum; j++)
	    read_adrs[j] = ins_col_values + arg_num_list[j];
	      
	  if (ind_read (V_PTR (constr->object, Indid), cur_colnum,
			read_adrs, 0, NULL, NULL, NULL, RSC))
	    return -REF_VIOL;
	  break;
	  
	case REFERENCE :
	  /* changing of values concerned by constraint checking */
	  if (mod_col_list)
	    {
	      for (j = 0; j < cur_colnum; j++)
		{
		  MK_BIN_OPER (EQU, ins_col_values[arg_num_list[j]],
			       del_col_values[arg_num_list[j]]);
		  if (!COMP_RESULT)
		    /* old & new values are not equal */
		    break;
		}
	      if (j >= cur_colnum)
		/* old & new values of columns from constraint are all equal */
		break;
	    }
	  
	  CHECK_ARR_SIZE (read_adrs, read_adrs_lng, cur_colnum, data_unit_t *);
	  for (j = 0, max_clm_num = 0; j < cur_colnum; j++)
	    {
	      if (max_clm_num < ref_num_list[j])
		max_clm_num = ref_num_list[j];
	      read_adrs[j] = del_col_values + arg_num_list[j];
	    }
	      
	  if (!tab_read (V_PTR (constr->object, Tabid), cur_colnum,
			 max_clm_num, ref_num_list, read_adrs, 0, NULL, NULL))
	    return -REF_VIOL;
	  break;
	
	case VIEW :
	case CHECK :
          error = calculate (V_PTR (constr->object, TRNode),&reslt, 0, 0, &null_fl);
          if (error < 0)
            return error;
          if ((null_fl != REGULAR_VALUE) || (reslt == 0))
            return (constr->constr_type == CHECK) ? -CHECK_VIOL : -VIEW_VIOL;
	  break;
	  
	default :
          break;
	}
    }
  return 0;
} /* check_and_put */

/*
 * print error message to stderr & roll back of transaction    
 * if it's needed & return in this case TRUE else FALSE   
 * remark : if cd==EOSCAN (end of scan) this is not error
 */
int
chck_err (int cd)
{
  if (!cd)
    return 0;
  
  if (cd == -ER_EOSCAN)
    {
      SQLCODE = 100;
      return 0;
    }
  
  if (cd < 0)
    SQLCODE = cd;
  else
    {
      SQLCODE = -ER_SYER;
      rollback (cd);
    }
  return 1;
}				/* "chck_err" */

/*
 * change virtual addresses in array arr_ptr 
 * of length nr to ordinary addresses
 */
void 
off_adr (PSECT_PTR * arr_ptr, i4_t nr)
{
  PSECT_PTR *pptr;
  int i;
  
  if (nr)
    {
      assert (arr_ptr->off);
      arr_ptr->adr = V_PTR (arr_ptr->off, char);
      pptr = (PSECT_PTR *) (arr_ptr->adr);
      for (i = 0; i < nr; i++, pptr++)
	pptr->adr = V_PTR (pptr->off, char);
    }
  else
    arr_ptr->adr = NULL;
}				/* "off_adr" */

/*
 * implementation of LIKE predicate
 * if there is no ESCAPE in LIKE => z=0              
 * values of strings-arguments can't be unknown here
 */
static int
eval_like (char *x, char *y, char z)
{
  char *px = x, *py = y;
  while ((*px && *py) || (*py == '%'))
    {
      if (*py == '%')
	{
	  py++;
	  do
	    if (eval_like (px, py, z))
	      return 1;
	  while (*px++);
	  return 0;
	}
      if (*py != '_')
	{
	  if (*py == z)	 /* if z=0, this condition is false always */
	    py++;
	  if (*px != *py)
	    break;
	}
      px++;
      py++;
    }				/* while */
  return !(*px || *py);
}				/* "like" */

void
debug_itree(TRNode *cur,int indent)
{
  static char spaces[] = "                                                                          ";
  printf("%s",spaces+sizeof(spaces)-1-indent*2);
  printf("%p:%s (Depend:%d,Handle:%d): *(%s)%d = ",cur,NAME(cur->code),cur->Depend,cur->Handle,
         type2str(cur->Ty),cur->Ptr.off);
  if (cur->Ptr.off==0)
    printf("null reference\n");
  else if (cur->Ty.code == SQLType_0)
    printf("...\n");
  else
    {
      TpSet *d = & V_PTR(cur->Ptr.off,data_unit_t)->dat;
      if ( d->nl_fl == NULL_VALUE )
        printf("NULL value\n");
      else if  ( d->nl_fl == REGULAR_VALUE )
        {
          switch (cur->Ty.code)
            {
            case SQLType_Char:
              if (d->val.StrPtr.off < 16*1024/* HACK!!! */ )
                printf("\"%s\"\n",V_PTR(d->val.StrPtr.off,char));
              else
                printf("\"%s\"\n",d->val.StrPtr.adr);
              break;
            case SQLType_Real:
              printf("%f\n",d->val.Flt);
              break;
            default:
              printf("%ld\n",(long)d->val.Lng);
            }
        }
      else
        printf("UNKNOWN value\n");
    }
  
  if (cur->Arity==0 || cur->code==CALLPR)
      return;
  
  printf("%s{{\n",spaces+sizeof(spaces)-1-(indent+1)*2);
  
  for ( cur = V_PTR(cur->Dn.off,TRNode); cur ; cur = V_PTR(cur->Rh.off,TRNode) )
    debug_itree(cur,indent+2);
  
  printf("%s}}\n",spaces+sizeof(spaces)-1-(indent+1)*2);
}

void
debug_data_stack(int depth)
{
  int ind;
  if (dt_ind<0)
    return;
  if ( dt_ind - depth < 0 ) depth = dt_ind ;
  printf("-<<<< data_stack(%d) <<<<\n",dt_ind);
  for ( ind = dt_ind; ind >= 0 && ind >=dt_ind - depth ; ind --)
    {
      TpSet *d = & data_st[ind].dat;
      printf("%d:\t",dt_ind - ind);
      
      printf("%s = ",type2str(data_st[ind].type));
      if ( d->nl_fl == NULL_VALUE )
        printf("NULL value\n");
      else if  ( d->nl_fl == REGULAR_VALUE )
        {
          switch (data_st[ind].type.code)
            {
            case SQLType_Char:
              if (d->val.StrPtr.off < 16*1024/* HACK!!! */ )
                printf("\"%s\"\n",V_PTR(d->val.StrPtr.off,char));
              else
                printf("\"%s\"\n",d->val.StrPtr.adr);
              break;
            case SQLType_Real:
              printf("%f\n",d->val.Flt);
              break;
            default:
              printf("%ld\n",(long)d->val.Lng);
            }
        }
      else
        printf("UNKNOWN value\n");
    }
  printf("->>>> data_stack>>>>>\n");
}

static int
put_data_next (TRNode * cur)
{
  DtPush;
  switch (cur->code)
    {
    case USERNAME :
      NlFlCur = REGULAR_VALUE;
      StrCur = current_user_login_name;
      CodCur = T_STR;
      LenCur = strlen(current_user_login_name);
      break;
      
    case EXISTS   :
      NlFlCur = REGULAR_VALUE;
      LngCur  = TRUE  ;       /* result of SubQuery is already calculated */
      CodCur  = T_BOOL;
      LenCur  = 0;
      
      /* saving of SubQuery result : */
      LngPred = TRUE;
      break;
      
    case COLUMN :
      /* column pointer in tree from CHECK constraint */
      assert(ins_col_values!=NULL);
      DtCurEl = ins_col_values[(int)(cur->Ptr.off)];
      break;
      
    default     :
      DtCurEl = *TP_ADR (cur, Ptr, data_unit_t);
      CHECK_FLAG (DtCurEl);
      if (NlFlCur == UNKNOWN_VALUE)
	return -ER_1;
      break;
    } /* switch */
  return 0;
}				/* put_next_data */

/*
 * Bits from Handle are added to all node right from cur
 */
static void
traverse_handle (TRNode * subnode, MASK handle)
{
  while (RhExist (subnode))
    {
      subnode = NextRh (subnode);
      subnode->Handle = BitOR (subnode->Handle, handle);
    }
}				/* traverse_handle */

/*
 * CALCULATE: 
 * tree tr calculation & putting the result into res_ptr    *
 * accordingly needed length res_size & type res_type       *
 * res_size is used only if res_type == T_STR,              *
 * if res_type==0 => result type isn't changed              *
 * & it's length for strings isn't more than res_size.      *
 * if null_fl != NULL => here will be result description    *
 * (REGULAR_VALUE or NULL_VALUE) (if UNKNOWN_VALUE => error)*
 *                                                          *
 * Propositions :                                           *
 * 1. In all leafs: Dn.off=NULL                             *
 * 2. .Ptr is used in all nodes :                           *
 *      in leafs - for data referencing (data_unit_t);      *
 *      in other nodes - for referencing to saved result    *
 *                       of previous calculating of subtree *
 * This function returns value of SQLCODE:                  *
 * 0 - if it's all right,                                   *
 * < 0 - if error,                                          *
 * > 0 - if result is string and this value = lenght of     *
 *       this string. It is greater than res_size           *
 *       (so string was shortened).
 */

int
calculate (TRNode * tr, void *res_ptr, i4_t res_size,
	   byte res_type, char *null_fl)
{
  enum sit_t {
    OPERN,
    LEAF,
    DONE
  };
  
  enum sit_t     sit;
  MASK           handle;
  TRNode        *cur;
  TRNode        *pred;
  int            res = 0;
#define GO_UP(cur,handle) { cur=stack[st_ind].TP; handle=cur->Handle; StPop; }

  cur = tr;
  handle = 0;
  assert (cur != NULL);
  sit = (Leaf(tr)) ? LEAF : OPERN;

  while (1)
    {
      /*
        debug_itree(cur,0);
        debug_data_stack(dt_ind+2);
      */
      switch (sit)
	{
	case OPERN: /* cur - pointer to operation node     */
	  /* handle value is stored in all operation nodes */
	  handle = BitAND (handle, cur->Depend);
	  handle = cur->Handle = BitOR (handle, cur->Handle);
	  if (handle || (cur->Ptr.off == VNULL))
	    {
	      /* we have to recalculate subtree */
	      if (cur->code == CALLPR) /* procedure call */
		{
		  PrepSQ *init_data = V_PTR (cur->Dn.off, PrepSQ);
                  
		  DtPush;
		  DtCurEl = init_data->ToDtStack;
                  
		  res = proc_work (V_PTR (init_data->ProcAdr, char), NULL, handle);
		  if (res < 0)
		    return res;
		  else
		    SQLCODE = 0;
		  
		  if (NlFlCur == UNKNOWN_VALUE)
		    /* in SubQuery for comparison predicate there isn't any row */
		    NlFlCur = NULL_VALUE;
	      
		  if (init_data->GoUpNeed)
		    {
		      DtPredEl = DtCurEl;
		      DtPop;
		      GO_UP(cur,handle);
		    }
		  sit = DONE;
		  break;
		}
	      
	      StPush;
	      StcCur.TP = cur;
	      StcCur.flg = 0;
	      
	      cur = NextDn (cur);
	      if (Leaf (cur))	/* not operation  */
		sit = LEAF;
	    }
	  else
	    sit = LEAF;
	  break;
	  
	case LEAF:
          res = put_data_next (cur);
          /* printf("res=%d\n",res); */
	  if (res)
	    return res;
	  sit = DONE;
	  break;
	  
	case DONE:  /* cur is pointer to node, subquery for which        *
		     * is already calculated. Result is in head of stack */
	  cur->Handle = 0;
	  if ((cur->Ptr.off != VNULL) && !(Leaf (cur)))
	    *TP_ADR (cur, Ptr, data_unit_t) = DtCurEl;
	  if (cur == tr)
	    {			/* the work with tree is finished          *
	                         * next two operations are putting the     *
				 * result into res_ptr, if res_ptr != NULL */
	      if (res_ptr)
		res = put_dat (DU_PTR_VL (DtCurEl), LenCur, CodCur, NlFlCur,
			       (char *) res_ptr, res_size, res_type, null_fl);
	      else /*  res_ptr==NULL*/
		if (null_fl)
		  *null_fl = NlFlCur;
	      DtPop;
	      return res;
	    }			/* if  */
	  
	  pred = StcCur.TP;
	  
	  if (StcCur.flg == 0)
            {
              /* ------------ Optimisation of calculation --------------- */
              /* The most left element of operation *
               * "pred" is in stack Data now        *
               * the logic expression's calculation may be finished here
               */
	      if (pred->Arity == 1) /* unary operation */
		{
		  res = oper (pred->code, 1);
		  if (res < 0)
		    return res;
		  GO_UP(cur,handle);
		  break;
		}
	      
	      if (pred->code == IN) /* IN predicate with values list */
		{
		  int null_flag = FALSE;
		  TRNode *where = NextRh (cur);
		  
		  if (!where)
		    return -ER_1;
		  if (NlFlCur == REGULAR_VALUE)
		    /* if value expression in IN is NULL value => *
		     * the result of IN is NULL too               */
		    {
		      DtPush;
		      do {
			DtCurEl = DtPredEl;
			put_data_next (where);
			oper (EQU, 2);
			if (!null_flag && (NlFlCur == NULL_VALUE))
			  null_flag = TRUE;
		      } while (!LngCur && (where = NextRh (where)));
		      if (null_flag && !LngCur)
			NlFlCur = NULL_VALUE;
		      DtPredEl = DtCurEl;
		      DtPop;
		    }
		  GO_UP(cur,handle);
		  break;
		}
	      
	      if (NlFlCur == REGULAR_VALUE)
                {
                  if ((pred->code == OR) || (pred->code == AND))
                    if (((pred->code == OR) && (LngCur == 1)) ||
                        ((pred->code == AND) && (LngCur == 0)))
                      {
                        traverse_handle (cur, handle);
                        GO_UP(cur,handle);
                        break;
                      }
                    else
                      {
                        if (RhExist (cur))
                          DtPop;
                      }
                  else
                    (StcCur.flg)++;
                }
	      else /* NlFlCur == NULL_VALUE */
                {
                  if (Is_Comp_Code (pred->code) || Is_Arithm_Code (pred->code))
                    {
                      traverse_handle (cur, handle);
                      GO_UP(cur,handle);
                      break;
                    }
                  else
                    (StcCur.flg)++;
                }
                /* -------------- End of optimisation part ---------------- */
	    }
	  else
	    {	/* the operand isn't most left => operation "pred" must *
		 * be executed here  for two arguments from stack Data  */
	      if (pred->code == LIKE)
		{
		  if ((NlFlPred != REGULAR_VALUE) || (NlFlCur != REGULAR_VALUE))
		    NlFlPred = NULL_VALUE;
		  else
		    {
                      assert ( pred->Arity == 2 || pred->Arity == 3 );
		      LngPred = eval_like (StrPred, StrCur,
                                           ((pred->Arity == 2) ? 0 : ADR (NextRh (cur), Ptr)[0]));
		      /* if pred->Arity==2 : there is no ESCAPE */
		      CodPred = T_BOOL;
		    }
		  DtPop;
		  GO_UP(cur,handle);
		  break;
		}
	      else
		if (( res = oper (pred->code, 2) ) < 0)
		  return res;
	    } /* if else */
	  if (RhExist (cur))
	    {
	      cur = NextRh (cur);
	      sit = (Leaf (cur) ? LEAF : OPERN ) ;
	      break;
	    }
	  else
	    GO_UP(cur,handle);
	  break;
	}			/* switch  */
    }				/* while     */
#undef GO_UP
}				/* "calculate" */

/*
 * simbol ch is put into ch_ptr either as right half (how=0) 
 * or as left half (how=1)of char.
 */
static void 
put_as (char ch, char *ch_ptr, i4_t how)
{
  assert (ch);
  *ch_ptr |= (how) ? ch << 4 : ch;
}				/* put_as */

#define COMP(oprn, elem, to) { MK_BIN_OPER (oprn, *du, cond->elem); to = COMP_RESULT; }

static int
where_in_SP (data_unit_t * du, SP_Cond * cond)
/* returns position code for value from DU. This position is     *
 * determined by simple predicate stored already in Cond         *
 * There is EQU in Cond:  |   else - accordingly interval :      *
 *   -----*-----          |  ---------[-------------]----------  *
 *     0  1  2            |     3     4      5      6     7      */
/* Values in Cond can't be unknown ( == NULL_VALUE)              */ 
{
  static char code[4][4] = {
    {5, 5, 6, 7},
    {3, -1, -1, -1},
    {4, -1, -1, -1},
    {5, 5, 6, 7}};
  int error;
  int where_left, where_right;	/* position's codes for date from DU */
  where_left = 0;             	/* accordingly interval from *Cond   */
  where_right = 0;              /* 0 - there exist not a bound;      *
                                 *                                   *
                                 *     1        2       3            *
                                 * _____________*______________      */
  
  if (cond->LeftCode)
    {
      /* putting to where_left : result of "DU > Cond.Left" */
      COMP(GT, Left, where_left);
      
      if (where_left)
	where_left = 3;
      else 
	if (cond->LeftCode == GT)
	  where_left = 1;
	else
	  {
	    /* putting to where_left : result of "DU < Cond.Left" */
	    COMP(LT, Left, where_left);
      
	    if (!where_left)
	      where_left = 2;
	  }
      if (cond->LeftCode == EQU)
	return where_left - 1;
    }
  if (((!where_left) || (where_left > 2)) && (cond->RightCode))
    {
      /* putting to where_right : result of "DU < Cond.Right" */
      COMP(LT, Right, where_right);

      if (!where_right)
        {
          if (cond->RightCode == LT)
            where_right = 3;
          else
            {
              /* putting to where_right : result of "DU > Cond.Right" */
              COMP(GT, Right, where_right);
              
              where_right += 2;
            }
        }
    }
  return code[where_left][where_right];
}				/* where_in_SP */

#define RET(res)     { xfree (column_SP); return res; }
#define LEFT_COND    (column_SP[col_num].LeftCode)
#define RIGHT_COND    (column_SP[col_num].RightCode)
#define ISNULL_COND (column_SP[col_num].NullCode)
#define LEFT_EXPR    (column_SP[col_num].Left)
#define RIGHT_EXPR   (column_SP[col_num].Right)
#define COND_TYPE     (column_SP[col_num].Type)

int
condbuf (char **bufptr, int number_of_columns, VADR * trees)
/* returns :                                *
 * 1. as function :                         *
 *    0  if conditions are not compatible   *
 *    -error_code if error                  *
 *    length of buffer if success           *
 * 2. in bufptr : buffer address            */
{
  static int act_tab[6][8] =
  {				/* 0  1  2  3  4  5  6  7 */
                        /* EQU */ {3, 0, 3, 3, 1, 1, 1, 3},
                        /* NE  */ {0, 0, 0, 0, 0, 0, 0, 0},
                        /* LE  */ {3, 0, 0, 3, 4, 2, 0, 0},
                        /* GE  */ {0, 0, 3, 0, 0, 1, 4, 3},
                        /* LT  */ {3, 3, 0, 3, 3, 2, 2, 0},
                        /* GT  */ {0, 3, 3, 0, 1, 1, 3, 3}
  };

  static char code[7][7] =
  {				/* 0     EQU  NE  LE   GE  LT   GT */
                       /*  0  */ {ANY,   EQ,  0, SMLEQ, 0, SML, 0},
                       /* EQU */ {EQ,    0,   0, EQ,    0, EQ,  0},
                       /* NE  */ {NEQ,   0,   0, 0,     0, 0,   0},
                       /* LE  */ {0,     0,   0, 0,     0, 0,   0},
                       /* GE  */ {GRTEQ, 0,   0, SESE,  0, SES, 0},
                       /* LT  */ {0,     0,   0, 0,     0, 0,   0},
                       /* GT  */ {GRT,   0,   0, SSE,   0, SS,  0}
  };

#define NIL_CODE  (EQU - 1)
  
  SP_Cond *column_SP; 
  int err, wh, col_num, act, size;
  char *curptr;

  assert (number_of_columns);
  size = 0;
  column_SP = GET_MEMC (SP_Cond, number_of_columns);
  for (col_num = 0; col_num < number_of_columns; col_num++)
    {
      TRNode *current_tree;
      current_tree = V_PTR (trees[col_num], TRNode);
      if (current_tree)
	COND_TYPE = NextDn (current_tree)->Ty;
      for (; current_tree; current_tree = NextRh (current_tree))
	{
          enum token cur_code;
	  cur_code = current_tree->code;
	  
	  switch (cur_code)
	    {
	    case ISNULL :
	      if (LEFT_COND || RIGHT_COND || ISNULL_COND == NEQUN )
		/* conditions are not compatible */
		RET (0);
	      ISNULL_COND = EQUN;
	      break;
	      
	    case ISNOTNULL :
	      if (ISNULL_COND == EQUN)
		/* conditions are not compatible */
		RET (0);
	      ISNULL_COND = NEQUN;
	      break;
	      
	    default : /* comparison predicate */
	      if (ISNULL_COND == EQUN)
		/* conditions are not compatible */
		RET (0);
              {
                char null_fl;
                
                err = calculate (NextRh (NextDn (current_tree)),
                                 NULL, 0, 0, &null_fl);
                if (err < 0)
                  RET (err);
                if (null_fl == UNKNOWN_VALUE)
                  RET (-ER_1);
                if (null_fl == NULL_VALUE)
                  RET (0);
              }
	      /* if success : data is in DtNextEl */
	      if (cur_code == NE)
		act = 1;
	      else
		{
                  data_unit_t saved_data;
                  
		  saved_data = DtNextEl;
                  wh = where_in_SP (&saved_data, column_SP + col_num);
		  if (wh < 0)
		    RET ((wh == -1) ? -ER_1 : wh);
		  /* wh - relation code for current condition    *
		   * accordingly already stored simple predicate */
		  act = act_tab[cur_code - EQU][wh];
		  switch (act)
		    {
		    case 0:	/* nothing to do */
		      break;
		    case 1:	/* changing of left bound of Simple Predicate */
		      LEFT_COND = cur_code;
		      LEFT_EXPR = saved_data;
		      break;
		    case 2:	/* changing of right bound of Simple Predicate */
		      RIGHT_COND = cur_code;
		      RIGHT_EXPR = saved_data;
		      break;
		    case 3:	/* conditions are not compatible */
		      RET (0);
		      break;
		    case 4:	/* making of predicate EQU */
		      LEFT_COND = EQU;
		      RIGHT_COND = 0;
		      LEFT_EXPR = saved_data;
		      break;
		    }
		}			/* else */
	    } /* swith */
	}			/* for */
      if (LEFT_COND)
	size += BUF_SIZE_VL (LEFT_EXPR, COND_TYPE);
      if (RIGHT_COND)
	size += BUF_SIZE_VL (RIGHT_EXPR, COND_TYPE);
    }				/* for */

  {
    int j;
    j = number_of_columns / 2 + 1;
    size += j;
    *bufptr = GET_MEMC (char, size);
    curptr = *bufptr + j;
    put_as (ENDSC, *bufptr + j - 1, number_of_columns == 2 * j - 1);
  }

  for (col_num = 0; col_num < number_of_columns; col_num++)
    {
      char oper_code;
      if (LEFT_COND)
	if ((err = DU_to_buf (&LEFT_EXPR, &curptr, &COND_TYPE)) < 0)
	  RET (err);
      
      if ((LEFT_COND != EQU) && (LEFT_COND != NE) && RIGHT_COND)
	if ((err = DU_to_buf (&RIGHT_EXPR, &curptr, &COND_TYPE)) < 0)
	  RET (err);

      oper_code = code[(LEFT_COND)  ? LEFT_COND - NIL_CODE  : 0]
                      [(RIGHT_COND) ? RIGHT_COND - NIL_CODE : 0];
      assert (oper_code);
      put_as ((oper_code == ANY && ISNULL_COND) ? ISNULL_COND : oper_code,
              *bufptr + col_num / 2, col_num % 2);
    }				/* for */
  RET (size);
}				/* condbuf */

#undef RET
#undef LEFT_COND
#undef RIGHT_COND
#undef ISNULL_COND
#undef LEFT_EXPR
#undef RIGHT_EXPR
#undef COND_TYPE

