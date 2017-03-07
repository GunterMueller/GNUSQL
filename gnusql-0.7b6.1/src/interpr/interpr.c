/*
 *  interpr.c  -  main interpretator module of GNU SQL server
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

/* $Id: interpr.c,v 1.251 1998/09/29 22:23:40 kimelman Exp $ */

/* in values of type PSECT_PTR always : .adr=NULL <==> .off==VNULL        */
/* If there is a SavePar command, it must be the first command of section */ 
/* If error => RetPar ( query result returning) isn't working             */
/* negative value in scan hole <==>  scan wasn't opened corrrectly        */
/* zero value in field "untabid" of Tabid <==> corresponding temporary
                                             table doesn't exist at this time */

#include "setup_os.h"
#if HAVE_TIME_H
#include <time.h>
#endif

#include "global.h"
#include "inprtyp.h"
#include "gsqltrn.h"
#include "sql.h"
#include "expop.h"

#define PRINT(x, y) /* printf (x, y) */

extern data_unit_t *ins_col_values;
extern i4_t       numb_ins_col;
extern data_unit_t *data_st;
extern i4_t       dt_ind, dt_max;
extern i4_t       cnt_res;
extern i4_t       res_hole_cnt;
extern parm_t    *res_hole;

struct sqlca gsqlca;

#define CHECK_ERR(cd)  if (chck_err(cd)) return (cd)

i4_t
proc_work (char *begin, char **Exi, MASK Handle)
{      /* Command's interpretator for work with procedure from      *
	* address begin                                             *
	* Exi - address where to put returning point                *
	* (if Exi != NULL). Here is stored (after function`s work): *
	*  Returns error code : = 0 if O'k, < 0 else.               */
  static char *dba_name = NULL;
  char *cur = begin, *owner, **colptr;
  i4_t i, reslt, error = 0, spn, typ;
  i2_t *len;
  enum CmdCode code;
  i4_t bufsize, rngsize, Sc_num = 0, Tbl_num = 0;
  char *bufptr = NULL;
  char *rngptr = NULL;
  MASK dop;
  char tp, null_fl, retpar_done_fl = 0;
  VADR       *Sc_to_close = NULL;
  VADR       *Tbl_to_drop = NULL;
  Scanid     *cur_scan;
  UnId       *cur_tid;
  data_unit_t   *data_arr;
  sql_type_t *col_types;

  if (!dba_name)
    {
      char *ss=current_user_login_name;
      current_user_login_name = NULL;
      dba_name = get_user_name ();
      current_user_login_name = ss;
    }
  
#define SHIFT_PTR(ptr,size) {  int sz; sz = size; ALIGN(sz, SZ_LNG); ptr += sz; }
#define CASE(sn,vn)    case sn: { \
   struct S_##sn *vn; vn  = (struct S_##sn*)cur; \
   SHIFT_PTR(cur, sizeof(struct S_##sn)); /*printf("intepretation:'%s' - begin \n",#sn);*/
#define ENDCASE(sn) /*printf("intepretation:'%s' - end \n",#sn);*/ break; }

  while (1)
    {
      code = *((enum CmdCode *) cur);
      PRINT (" INTERPR: CODE=%d\n", code);
      SHIFT_PTR(cur,sizeof (enum CmdCode));
      switch (code)
	{
          /*---------------------------------------------------*/
          CASE(FINDROW,FIND);
          {
            if ((FIND->flag) != 'y')
              {
                off_adr (&(FIND->ColPtr), FIND->nr);
                FIND->Scan.adr = ADR (FIND, Scan);
                FIND->Exit.adr = ADR (FIND, Exit);
                FIND->flag = 'y';
              }
            SQLCODE = 0;
            error = findrow (SCN (FIND), (data_unit_t **) (FIND->ColPtr.adr));
            CHECK_ERR (error);
            if (error == -ER_EOSCAN)
              cur = (char *) (FIND->Exit.adr);
          }
          ENDCASE(FINDROW);
          /*---------------------------------------------------*/
          CASE(READROW,RROW);
          {
            if (RROW->flag != 'y')
              {
                off_adr (&(RROW->ColPtr), RROW->nr);
                RROW->Scan.adr = ADR (RROW, Scan);
                RROW->rlist.adr = ADR (RROW, rlist);
                RROW->flag = 'y';
              }
            SQLCODE = 0;
            error = read_row (SCN (RROW), RROW->nr, (i4_t *) (RROW->rlist.adr),
                              (data_unit_t **) (RROW->ColPtr.adr));
            if (error == -ER_EOSCAN)
              error = -ER_NCR;
            CHECK_ERR (error);
          }
          ENDCASE(READROW);
          /*---------------------------------------------------*/
          CASE(until,UNT);
          {
            if (UNT->flag != 'y')
              {
                UNT->cond.adr = ADR (UNT, cond);
                UNT->ret.adr = ADR (UNT, ret);
                UNT->flag = 'y';
              }
            error = calculate ((TRNode *) (UNT->cond.adr),
                               &reslt, 0, 0, &null_fl);
            CHECK_ERR (error);
            if ((null_fl != REGULAR_VALUE) || (reslt == 0))
              cur = UNT->ret.adr;
          }
          ENDCASE(until);
          /*---------------------------------------------------*/
          CASE(COND_EXIT,C_EXIT);
          {
            if (C_EXIT->flag != 'y')
              {
                C_EXIT->Tree.adr = ADR (C_EXIT, Tree);
                C_EXIT->flag = 'y';
              }
            error = calculate ((TRNode *) (C_EXIT->Tree.adr),
                               &reslt, 0, 0, &null_fl);
            CHECK_ERR (error);
            if (reslt)
              {
                for (i = 0; i < Sc_num; i++)
                  {
                    cur_scan = V_PTR(Sc_to_close[i], Scanid);
                    if ( *cur_scan >= 0)
                      {
                        error = closescan (*cur_scan);
                        *cur_scan = -ER_EOSCAN;
                        CHECK_ERR (error);
                      }
                  }
                for (i = 0; i < Tbl_num; i++)
                  {
                    cur_tid = V_PTR(Tbl_to_drop[i], UnId);
                    if ((cur_tid->t).untabid)
                      {
                        error = dropttab (&(cur_tid->t));
                        (cur_tid->t).untabid = 0;
                        CHECK_ERR (error);
                      }
                  }
                return 0;
              }
          }
          ENDCASE(COND_EXIT);
          /*---------------------------------------------------*/
          CASE(OPSCAN,SCA);
          {
            V_CONDBUF (bufsize, bufptr, SCA, nl, list);
            V_SCN (SCA) = -ER_EOSCAN;
            spn = 0;
            switch (SCA->OperType)
              {
              case CH_TAB:
                V_SCN (SCA) = opentscan (Tid_V_ADR (SCA, Tid), &spn,
                                         SCA->mode, SCA->nr,
                                         V_ADR (SCA, rlist, i4_t),
                                         bufsize, bufptr, SCA->nm,
                                         V_ADR (SCA, mlist, i4_t));
                break;
              case CH_INDX:
                /* form condition buffer and 'break' if conditions
                   aren't compatible */
                V_CONDBUF (rngsize, rngptr, SCA, ic, range);
                V_SCN (SCA) = openiscan (Iid_V_ADR (SCA, Tid), &spn,
                                         SCA->mode, SCA->nr,
                                         V_ADR (SCA, rlist, i4_t),
                                         bufsize, bufptr, rngsize, rngptr,
                                         SCA->nm, V_ADR (SCA, mlist, i4_t));
                break;
              case CH_FLTR :
#if 0
                V_SCN(SCA) = openfscan(Fid_V_ADR(SCA,Tid), SCA->mode,
                                       SCA->nr, V_ADR(SCA,rlist,i4_t),
                                       bufsize, bufptr, SCA->nm,
                                       V_ADR(SCA,mlist,i4_t));
#endif
              default:
                CHECK_ERR (-ER_8);
              }
            if (bufptr)
              {
                xfree (bufptr);
                bufptr = NULL;
              }
            if (rngptr)
              {
                xfree (rngptr);
                rngptr = NULL;
              }
            
            if (V_SCN (SCA) < 0)
              CHECK_ERR (V_SCN (SCA));
            if (spn > 0)
              CHECK_ERR (spn);
            if (V_SCN (SCA) == -ER_EOSCAN)
              cur = V_ADR (SCA, Exit, char);
          }
          ENDCASE(OPSCAN);
          /*---------------------------------------------------*/
          CASE(SetHandle,SHL);
          {
            if ((SHL->flag) != 'y')
              {
                off_adr (&(SHL->Trees), SHL->TrCnt);
                SHL->flag = 'y';
              }
            dop = (SHL->Bit) ? SHL->Bit : Handle;
            for (i = 0; i < SHL->TrCnt; i++)
              PTR (SHL, Trees, i, TRNode)->Handle |= dop;
          }
          ENDCASE(SetHandle);
          /*---------------------------------------------------*/
	case end:
	  if (Exi) /* CURSOR */
	    *Exi = cur;
	  else
	    if ((SQLCODE == 100) && (retpar_done_fl))
	      /* in operator SELECT the result was reached */
	      SQLCODE = 0;
	  return 0;
          /*---------------------------------------------------*/
          CASE(GoTo,GOTO);
          {
            if (GOTO->flag != 'y')
              {
                GOTO->Branch.adr = ADR (GOTO, Branch);
                GOTO->flag = 'y';
              }
            cur = GOTO->Branch.adr;
          }
          ENDCASE(GoTo);
          /*---------------------------------------------------*/
          CASE(MODROW,MDR);
          {
            if (MDR->flag != 'y')
              {
                off_adr (&(MDR->OutList), MDR->nl);
                MDR->Scan.adr = ADR (MDR, Scan);
                MDR->ClmTp.adr = ADR (MDR, ClmTp);
                MDR->mlist.adr = ADR (MDR, mlist);
                MDR->Constr.adr = ADR (MDR, Constr);
                MDR->flag = 'y';
              }
            
            error = check_and_put (MDR->nl, (i4_t *) (MDR->mlist.adr),
                                   (TRNode **) (MDR->OutList.adr), &data_arr,
                                   (S_ConstrHeader *) (MDR->Constr.adr));
            CHECK_ERR (error);
            
            error = mod_data (SCN (MDR), MDR->nl, data_arr, (sql_type_t *) (MDR->ClmTp.adr),
                              (i4_t *) (MDR->mlist.adr));
            CHECK_ERR (error);
          }
          ENDCASE(MODROW);
          /*---------------------------------------------------*/
          CASE(INSROW,INS);
          {
            if (INS->flag != 'y')
              {
                off_adr (&(INS->InsList), INS->nv);
                INS->Tid.adr = ADR (INS, Tid);
                INS->ClmTp.adr = ADR (INS, ClmTp);
                INS->Constr.adr = ADR (INS, Constr);
                INS->flag = 'y';
              }
            
            error = check_and_put (INS->nv, NULL, (TRNode **) (INS->InsList.adr),
                                   &data_arr, (S_ConstrHeader *) (INS->Constr.adr));
            CHECK_ERR (error);
            
            error = ins_data (Tid_ADR (INS), INS->nv, data_arr,
                              (sql_type_t *) (INS->ClmTp.adr));
            CHECK_ERR (error);
          }
          ENDCASE(INSROW);
          /*---------------------------------------------------*/
          CASE(INSLIST,INL);
          {
            if (INL->flag != 'y')
              {
                off_adr (&(INL->col_ptr), INL->nv);
                INL->Tid.adr = ADR (INL, Tid);
                INL->credat_adr.adr = ADR (INL, credat_adr);
                INL->cretime_adr.adr = ADR (INL, cretime_adr);
                INL->col_types.adr = ADR (INL, col_types);
                INL->len.adr = ADR (INL, len);
                INL->flag = 'y';
                CHECK_ERR (error);
              }
            
            if (INL->credat_adr.adr)
              curdate (INL->credat_adr.adr);
            if (INL->cretime_adr.adr)
              curtime (INL->cretime_adr.adr);
            
            CHECK_ARR_SIZE (ins_col_values, numb_ins_col, INL->nv, data_unit_t);
            bzero (ins_col_values, INL->nv * sizeof (data_unit_t));
            colptr = (char **) (INL->col_ptr.adr);
            col_types = (sql_type_t *) (INL->col_types.adr);
            len = (i2_t *) (INL->len.adr);
            for (i = 0; i < INL->nv; i++)
              {
                ins_col_values[i].type = col_types[i];
                error = mem_to_DU ((colptr[i]) ? REGULAR_VALUE : NULL_VALUE,
                                   col_types[i].code, (len) ? len[i] : -1,
                                   colptr[i], ins_col_values + i);
                CHECK_ERR (error);
              }
            
            error = handle_statistic (Tid_ADR (INL)->untabid,
                                      INL->nv, INL->nv, NULL, TRUE);
            CHECK_ERR (error);
            
            error = ins_data (Tid_ADR (INL), INL->nv, ins_col_values, col_types);
            CHECK_ERR (error);
          }
          ENDCASE(INSLIST);
          /*---------------------------------------------------*/
          CASE(INSTAB,INST);
          {
            error = instttab (V_ADR (INST, TidFrom, Tabid), V_ADR (INST, TidTo, Tabid),
                              0, NULL, 0, NULL, NULL);
            CHECK_ERR (error);
          }
          ENDCASE(INSTAB);
          /*---------------------------------------------------*/
          CASE(DELROW,DROW);
          {
            error = check_and_put (0, NULL, NULL, NULL,
                                   V_ADR (DROW, Constr, S_ConstrHeader));
            CHECK_ERR (error);
            
            error = delrow (V_SCN (DROW));
            CHECK_ERR (error);
          }
          ENDCASE(DELROW);
          /*---------------------------------------------------*/
          CASE(TMPSCN,REL);
          {
            V_CONDBUF (bufsize, bufptr, REL, nl, list);
            
            switch(REL->OperType)
              {
              case 't': /* build table */
                error = bldtttab (Tid_V_ADR (REL, TidFrom),
                                  Tid_V_ADR (REL, TidTo), REL->nr,
                                  V_ADR (REL, rlist, i4_t),
                                  bufsize, bufptr, NULL);
                break;
              case 'i': /* build index */
                V_CONDBUF (rngsize, rngptr, REL, ic, range);
                error = bldtitab (Iid_V_ADR (REL, TidFrom),
                                  Tid_V_ADR (REL, TidTo), REL->nr,
                                  V_ADR (REL, rlist, i4_t), bufsize, bufptr,
                                  rngsize, rngptr, NULL);
                break;
              case 'f' : /* build filter */
#if 0
                error = bldtftab(Fid_V_ADR(REL,TidFrom),
                                 Tid_V_ADR(REL,TidTo),REL->nr,
                                 V_ADR(REL,rlist,i4_t),
                                 bufsize,bufptr,NULL);
#endif
              default:
                CHECK_ERR (-ER_8);
              }
            if (bufptr)
              {
                xfree (bufptr);
                bufptr = NULL;
              }
            if (rngptr)
              {
                xfree (rngptr);
                rngptr = NULL;
              }
            
            CHECK_ERR (error);
            
          }
          ENDCASE(TMPSCN);
          /*---------------------------------------------------*/
          CASE(SORTTBL,SOR);
          {
            error = sorttab (Tid_V_ADR (SOR, TidIn), Tid_V_ADR (SOR, TidOut),
                             SOR->ns, V_ADR (SOR, rlist, i4_t), SOR->order, SOR->fl);
            CHECK_ERR (error);
          }
          ENDCASE(SORTTBL);
          /*---------------------------------------------------*/
          CASE(MKUNION,UNI);
          {
            error = uidnsttab (Tid_V_ADR (UNI, TidIn1), Tid_V_ADR (UNI, TidIn2),
                               Tid_V_ADR (UNI, TidOut), BLDUN);
            CHECK_ERR (error);
          }
          ENDCASE(MKUNION);
          /*---------------------------------------------------*/
          CASE(RetPar,PRET);
          {
            if (PRET->flag != 'y')
              {
                off_adr (&(PRET->OutList), PRET->OutNum);
                PRET->flag = 'y';
              }
            
            /* PRET->ExitFlag == FALSE => operator SELECT */
            if (!PRET->ExitFlag)
              {
                if (!retpar_done_fl)
                  retpar_done_fl++;
                else		 /* one row was already extracted */
                  CHECK_ERR (-ER_SEL);
              }
            
            cnt_res = PRET->OutNum;
            if (res_hole_cnt < cnt_res)
              {
                i4_t need_sz = cnt_res * sizeof (parm_t);
		
                if (res_hole)
                  {
                    res_hole = (parm_t*) xrealloc (res_hole, need_sz);
                    bzero (res_hole, need_sz);
                  }
                else
                  res_hole = (parm_t*) xmalloc (need_sz);
		
                res_hole_cnt = cnt_res;
              }
	    
            for (i = 0; i < cnt_res; i++)
              {
                typ = TYP (PRET, OutList, i).code;
                error = calculate (PTR (PRET, OutList, i, TRNode), NULL, 0, 0, NULL);
                CHECK_ERR (error);
                
                error = DU_to_rpc (PtrDtNextEl, res_hole + i, typ);
                CHECK_ERR (error);
              }			/* for */
            
            if (PRET->ExitFlag) /* CURSOR */
              {
                if (Exi)
                  *Exi = cur;
                return 0;
              }
          }
          ENDCASE(RetPar);
          /*---------------------------------------------------*/
          CASE(TMPCRE,RNEW);
          {
            error = crettab (Tid_V_ADR (RNEW, Tid), RNEW->colnum,
                             RNEW->nnulnum, V_ADR (RNEW, coldescr, sql_type_t));
            CHECK_ERR (error);
          }
          ENDCASE(TMPCRE);
          /*---------------------------------------------------*/
          CASE(MKGROUP,GRP);
          {
            error = make_group (Tid_V_ADR (GRP, TidIn),
                                Tid_V_ADR (GRP, TidOut), GRP->ng,
                                V_ADR (GRP, glist, i4_t), GRP->order,
                                GRP->nf, V_ADR (GRP, flist, i4_t),
                                V_ADR (GRP, fl, char));
            CHECK_ERR (error);
          }
          ENDCASE(TMPCRE);
          /*---------------------------------------------------*/
          CASE(FUNC,FUN);
          {
            if ((FUN->flag) != 'y')
              {
                off_adr (&(FUN->colval), FUN->nf);
                FUN->Tid.adr = ADR (FUN, Tid);
                FUN->list.adr = ADR (FUN, list);
                FUN->range.adr = ADR (FUN, range);
                FUN->flist.adr = ADR (FUN, flist);
                FUN->fl.adr = ADR (FUN, fl);
                FUN->Exit.adr = ADR (FUN, Exit);
                FUN->flag = 'y';
              }
            CONDBUF (bufsize, bufptr, FUN, nl, list);
	    
            SQLCODE = 0;
            if (FUN->OperType == CH_INDX)
              {
                CONDBUF (rngsize, rngptr, FUN, ic, range);
                
                error = funci (Iid_ADR (FUN), NULL, bufsize, bufptr, rngsize,
                               rngptr, (data_unit_t **)(FUN->colval.adr), FUN->nf,
                               (i4_t *)(FUN->flist.adr), (char *)(FUN->fl.adr));
              }
            else
              {
                if (FUN->OperType != CH_TAB)
                  CHECK_ERR (-ER_8);
                error = func (Tid_ADR (FUN), NULL, bufsize, bufptr,
                              (data_unit_t **)(FUN->colval.adr), FUN->nf,
                              (i4_t *)(FUN->flist.adr), (char *)(FUN->fl.adr));
              }
            
            if (bufptr)
              {
                xfree (bufptr);
                bufptr = NULL;
              }
            if (rngptr)
              {
                xfree (rngptr);
                rngptr = NULL;
              }
            
            CHECK_ERR (error);
          }
          ENDCASE(FUNC);
          /*---------------------------------------------------*/
          CASE(CLOSE,CLS);
          {
            error = closescan (V_SCN (CLS));
            V_SCN (CLS) = -ER_EOSCAN;
            CHECK_ERR (error);
          }
          ENDCASE(CLOSE);
          /*---------------------------------------------------*/
          CASE(DROPTTAB,DRT );
          {
            error = dropttab (Tid_V_ADR (DRT, Tid));
            Tid_V_ADR (DRT, Tid) -> untabid = 0;
            CHECK_ERR (error);
	  }
          ENDCASE(DROPTTAB);
          /*---------------------------------------------------*/
          CASE(DROPBASE,DRB );
          {
            error = drop_table (DRB->untabid);
            CHECK_ERR (error);
	  }
          ENDCASE(DROPBASE);
          /*---------------------------------------------------*/
          CASE(ERROR,ERR );
          {
            if (Exi)
              *Exi = cur - sizeof (enum CmdCode);
            CHECK_ERR (ERR->Err);
            return 0;
	  }
          ENDCASE(ERROR);
          /*---------------------------------------------------*/
          CASE(Procedure,PRC );
          {
            error = proc_work (V_ADR (PRC, ProcBeg, char), NULL, 0);
            CHECK_ERR (error);
	  }
          ENDCASE(Procedure);
          /*---------------------------------------------------*/
          CASE(CRETAB,CTB );
          {
            owner = V_ADR (CTB, owner, char);
            if (strcmp (owner, current_user_login_name) &&
                strcmp (current_user_login_name, dba_name))
              CHECK_ERR (-ER_PRIVLG);
            if (existtb (owner, V_ADR (CTB, tabname, char), NULL, &tp))
              CHECK_ERR (-ER_CRETAB);
            error = creptab (V_ADR (CTB, tabid, Tabid), CTB->segid,
                             CTB->colnum, CTB->nncolnum,
                             V_ADR (CTB, coldescr, sql_type_t));
            if (error < 0)
              error = -ER_CRETAB;
            CHECK_ERR (error);
	  }
          ENDCASE(CRETAB);
          /*---------------------------------------------------*/
          CASE(CREIND,CIND );
          {
            error = creind (V_ADR (CIND, indid, Indid), V_ADR (CIND, tabid, Tabid),
                            CIND->uniq_fl, 0, CIND->colnum, V_ADR (CIND, clmlist, i4_t));
            if (error < 0)
              error = -ER_CREIND;
            CHECK_ERR (error);
	  }
          ENDCASE(CREIND);
          /*---------------------------------------------------*/
          CASE(PRIVLG,PRIV );
          {
            if (strcmp(current_user_login_name, dba_name) &&
                ( strcmp(V_ADR (PRIV, user, char), current_user_login_name) ||
                  ((!(PRIV->owner) ||
                    strcmp(V_ADR (PRIV, owner, char), current_user_login_name)) &&
                   tabpvlg (*V_ADR (PRIV, untabid, i4_t),
                            V_ADR (PRIV, user, char), V_ADR (PRIV, privcodes, char),
                            PRIV->un, (PRIV->un) ? V_ADR (PRIV, ulist, i4_t) : NULL,
                            PRIV->rn, (PRIV->rn) ? V_ADR (PRIV, rlist, i4_t) : NULL)) ))
              CHECK_ERR (-ER_PRIVLG);
	  }
          ENDCASE(PRIVLG);
          /*---------------------------------------------------*/	  
          CASE(Drop_Reg,DREG );
          {
            Sc_num = DREG->ScanNum;
            if (Sc_num)
              Sc_to_close = V_ADR (DREG, Scan, VADR);
            
            Tbl_num = DREG->TabdNum;
            if (Tbl_num)
              Tbl_to_drop = V_ADR (DREG, Tabd, VADR);
	  }
          ENDCASE(Drop_Reg);
          /*---------------------------------------------------*/	  
	default:
	  CHECK_ERR (-ER_8);

	} /* switch    */
    }	  /* while     */
}	  /* proc_work */
