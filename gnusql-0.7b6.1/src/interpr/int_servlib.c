/*
 *  int_servlib.c  - top level functions of GNU SQL interpretator server
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Konstantin Dyshlevoi
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

/* $Id: int_servlib.c,v 1.250 1998/09/29 21:26:12 kimelman Exp $ */

#include "global.h"
#include "gsqltrn.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "sql.h"
#include "inprtyp.h"
#include "inpop.h"
#include "svr_lib.h"
#include "type_lib.h"

extern data_unit_t *data_st;	/*[MAX_STACK]*/
extern StackUnit *stack;	/*[MAX_STACK]*/
extern i4_t dt_ind, dt_max;
extern i4_t st_ind, st_max;
extern pid_t parent;            /* administrator process id */
extern i4_t msqidt;              
extern i4_t minidnt;

extern i4_t       res_hole_cnt;
extern parm_t    *res_hole;

#define RET            return 
#define CH_ERR_IN(cd)  if (chck_err(cd)) RET 

i4_t cnt_res;

/*
 * Receiving of argumens to interpretator process and result sending.     
 * Returns SQLCODE in result (=0 if O'K, < 0 if error, =100 if EndOfScan) 
 */

static void
do_insn(insn_t *in)
{
  parm_t *args = (in->parms.parm_row_t_len) ? in->parms.parm_row_t_val : NULL;
  
  int i, error;
  char *cur;			/* current pointer */
  enum CmdCode code;
  VADR *SectTable, *arr;
  Scanid *cur_scan;
  UnId *cur_tid;
  i4_t sectnum;
  i4_t command;

  struct S_CursorHeader *CHE  = NULL;
  struct S_Drop_Reg     *DR   = NULL;
  struct S_SavePar      *SPAR = NULL;
  
  cnt_res = 0;
  SQLCODE = 0;
  InitDat;
  InitStack;
  
  start_processing();
  
  sectnum = in->sectnum;
  
  if (sectnum < 0)		/* commit or rollback */
    {
      if (sectnum == -1)
	rollback (0);
      else if (sectnum==-2)
	{
	  change_statistic ();
	  commit ();
	}
      else
        yyfatal("unrecognized section number");
        
      RET;
    }

  command = in->command;
  switch_to_segment ((VADR)(in->vadr_segm));
  
  SectTable = V_PTR (*V_PTR ((VADR) 4, VADR), VADR);
  /* first element of SectTable - sections number */
  if (((i4_t *) (SectTable))[0] <= sectnum)
    CH_ERR_IN (-ER_5);

  cur = V_PTR (SectTable[sectnum + 1], char);
  
#define READ_CMD(adr, cmnd) code = *((enum CmdCode *) cur);                   \
                            if (code == cmnd)                                 \
                              { int sz;                                       \
                                sz = sizeof (enum CmdCode); ALIGN(sz,SZ_LNG); \
                                cur += sz;                                    \
                                adr = (struct S_##cmnd *) cur;                \
                                sz = SIZES (S_##cmnd); ALIGN(sz,SZ_LNG);      \
                                cur += sz;                                    \
			      }
  
  READ_CMD(CHE, CursorHeader);
  READ_CMD(DR, Drop_Reg);
  READ_CMD(SPAR, SavePar);
  
#undef READ_CMD
  
  if (CHE)
    {
      /* value of OpFl is :                          *
       * 0 - cursor wasn't opened;                   *
       * 1 - cursor was opened but wasn't used ;     *
       * 2 - cursor was opened and was used ;        *
       * 3 - cursor was opened but there was already *
       *     SQLCODE =100 (scanning is finished)     */

      if (!DR)
	CH_ERR_IN (-ER_8);
      switch (command)
	{
	case 0:		/* OPEN CURSOR */
	  if (CHE->OpFl)
	    CH_ERR_IN (-ER_OpCur);
	  (CHE->OpFl)++;
	  break;

	case 1:		/* FETCH */
	  if (!(CHE->OpFl))
	    CH_ERR_IN (-ER_Fetch);
	  if (CHE->OpFl == 3) /* end of scan was already found */
	    CH_ERR_IN (-ER_EOSCAN);
	  CHE->OpFl = 2;
	  cur = CHE->Cur;
	  SPAR = NULL;
	  break;

	case 2:		/* CLOSE CURSOR */
	  if (!(CHE->OpFl))
	    CH_ERR_IN (-ER_Close);
	  if (CHE->OpFl > 1) /* there was work with cursor  *
		              * ( not only "open cursor" )  */ 
	    {
	      if (DR->ScanNum)
		{
		  arr = V_ADR(DR,Scan,VADR);
		  for (i = 0; i < DR->ScanNum; i++)
		    {
		      cur_scan = V_PTR(arr[i], Scanid);
		      if ( *cur_scan >= 0)
			{
			  error = closescan (*cur_scan);
			  CH_ERR_IN (error);
			  *cur_scan = -1;
			}
		    }
		}
	      if (DR->TabdNum)
		{
		  arr = V_ADR(DR,Tabd,VADR);
		  for (i = 0; i < DR->TabdNum; i++)
		    {
		      cur_tid = V_PTR(arr[i], UnId);
		      if ((cur_tid->t).untabid)
			{
			  error = dropttab (&(cur_tid->t));
			  CH_ERR_IN (error);
			  (cur_tid->t).untabid = 0;
			}
		    }
		}
	    }
	  (CHE->OpFl) = 0;
	  RET;

	case 3:		/* DELETE : POSITIONED */
	  if ((CHE->OpFl) != 2)
	    CH_ERR_IN (-ER_Delete);
	  if (!DR || (DR->ScanNum != 1))
	    CH_ERR_IN (-ER_8);
	  error = check_and_put (0, NULL, NULL, NULL,
				 V_ADR (CHE, DelCurConstr, S_ConstrHeader));
	  CH_ERR_IN (error);
	  
	  error = delrow (*V_PTR (*V_ADR (DR,Scan,VADR), Scanid));
	  CH_ERR_IN (error);
	  RET;

	default:
	  CH_ERR_IN (-ER_6);	/* ERROR */
	}			/* switch, if */
    }				/* if */
  /* cur = pointer to first command */

  if (SPAR || args)
    {
      if (!(SPAR && args))
	CH_ERR_IN (-ER_CLNT);
	
      /* fprintf (STDOUT, " INTERPR: CODE=%d\n", code); */
      
      if (SPAR->flag != 'y')
	{
	  off_adr (&(SPAR->PlaceOff), SPAR->ParamNum);
	  SPAR->flag = 'y';
	}
      
      for (i = 0; i < SPAR->ParamNum; i++)
	{
	  error = rpc_to_DU (args + i, PTR (SPAR, PlaceOff, i, data_unit_t));
	  /* what I need to do when error > 0 ( string was shortened ) ? */
	  if (error < 0)
	    CH_ERR_IN (error);
	}
    }
  
  if (CHE && !command) /* OPEN CURSOR */
    {
      CHE->Cur = cur;
      RET;
    }
  
  error = proc_work (cur, (CHE) ? &(CHE->Cur) : NULL, 0);
  
  if (SQLCODE == 100 && CHE) /* if eoscan found in cursor */ 
    CHE->OpFl = 3; /* saving information about End Of Scan for CURSOR */
  
  RET;
} /* do_insn */

#undef RET

/* Initialization of interpretator's work with module.    *
 * Returns virtual address of loaded module or error (<0) */
result_t*
module_init(string_t *in, struct svc_req *rqstp)
{
  FILE *fmodule;
  char *beg;
  i4_t leng;

  gsqltrn_rc.sqlcode = -MDLINIT;
  fmodule = fopen (*in, "r");
  if (fmodule)
    {
      fseek (fmodule, 0, SEEK_END);
      leng = ftell (fmodule);
      fseek (fmodule, 0, SEEK_SET);
      beg = xmalloc (leng);
      fread (beg, leng, 1, fmodule);
      fclose (fmodule);
      gsqltrn_rc.sqlcode = 0;
      gsqltrn_rc.info.rett = RET_SEG;
      gsqltrn_rc.info.return_data_u.segid = link_segment (beg, leng);
    }
  /* what about correlation between current BD and module ? */
  return &gsqltrn_rc;
} /* module_init */

static void
put_in_row(parm_row_t *r)
{
  void   *ptr;
  i4_t     i;
  
  assert(cnt_res);
  
  ptr = xmalloc ( sizeof(parm_t)*cnt_res);
  bcopy(res_hole,ptr,sizeof(parm_t)*cnt_res);
  
  r->parm_row_t_len = cnt_res;
  r->parm_row_t_val = ptr;
  for (i=0; i < cnt_res; i++)
    {
      parm_t *e = &(r->parm_row_t_val[i]);
      if( (e->indicator == 0) && (e->value.type == SQLType_Char))
        {  /* if data is a valuable string -- save copy of the string */
          void *p = xmalloc(e->value.data_u.Str.Str_len);
          bcopy(e->value.data_u.Str.Str_val,p,e->value.data_u.Str.Str_len);
          e->value.data_u.Str.Str_val = p;
        }
    }
}

result_t *
execute_stmt (insn_t *insn, struct svc_req *rqstp)
{
  insn_t     *cinsn;
  i4_t count, total;
  
  SQLCODE = 0;
  for(cinsn = insn,total = 0; cinsn ; cinsn = cinsn -> next )
    total += 1 + cinsn->options;
  if ( total > 1) /* request for table */
    {
      i4_t tbl_width=0;
      gsqltrn_rc.info.rett = RET_TBL;
      gsqltrn_rc.info.return_data_u.tbl.tbl_len = total;
      gsqltrn_rc.info.return_data_u.tbl.tbl_val = xmalloc ( sizeof(parm_row_t)*total);
      count = 0;
      for (cinsn = insn;
           cinsn && (SQLCODE==0) && (count<total);
           cinsn = cinsn -> next)
        while ((cinsn->options-- >=0) && (count<total))
          {
            do_insn(cinsn);
            if (SQLCODE<0)
              goto table_exit;
            if (cnt_res)
              {
                if (!tbl_width) tbl_width = cnt_res;
                assert(tbl_width == cnt_res);
                put_in_row(&(gsqltrn_rc.info.return_data_u.tbl.tbl_val[count++]));
              }
          }
    table_exit:
      if (!count)
        gsqltrn_rc.info.rett = RET_VOID;
      else if (count>1)
        gsqltrn_rc.info.return_data_u.tbl.tbl_len = count;
      else /* if count == 1 */
        {
          parm_row_t *row = gsqltrn_rc.info.return_data_u.tbl.tbl_val;
          bcopy(row,&(gsqltrn_rc.info.return_data_u.row),sizeof(parm_row_t));
          xfree(row);
          gsqltrn_rc.info.rett = RET_ROW;
        }
    }
  else if (total==1)
    {
      do_insn(insn);
      if (cnt_res)
        {
          if (SQLCODE>=0)
            {
              gsqltrn_rc.info.rett = RET_ROW;
              put_in_row(&gsqltrn_rc.info.return_data_u.row);
            }
          else
            gsqltrn_rc.info.rett = RET_VOID;
        }
      else
        assert(gsqltrn_rc.info.rett == RET_VOID);
    }
  gsqltrn_rc.sqlcode = SQLCODE;
  return &gsqltrn_rc;
}
