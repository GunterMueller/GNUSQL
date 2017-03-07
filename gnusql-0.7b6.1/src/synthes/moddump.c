/*
 *  moddump.c - dumping of executable module prepared by synthesator.
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Konstantin Dyshlevoj
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

/* $Id: moddump.c,v 1.251 1998/09/29 21:26:40 kimelman Exp $ */
  
#include "global.h"
#include "syndef.h"
#include <assert.h>

/* This definition must be declared latter than all variables definitions */
#define COMMAND(typ,var) struct S_##typ var; var = *V_PTR(cur, struct S_##typ); \
                         cur += sizeof(struct S_##typ); ALIGN(cur,SZ_LNG);

/*----------------------------------*/

void
DU_Print (VADR V_DU, FILE * out)
{
  data_unit_t *DU;
  
  DU = V_PTR (V_DU, data_unit_t);
  fprintf (out, "%i:%s ", V_DU, type2str(DU->type));
} /* DU_Print */

void
c_prn (VADR V_DU, FILE * out)
{
  data_unit_t *DU;
  TpSet *dat;
  
  DU = V_PTR (V_DU, data_unit_t);
  dat = &(DU->dat);
  if (NL_FL (dat) == NULL_VALUE)
    fprintf (out, ":\"NULL_VALUE\"");
  else
    switch (DU->type.code)
      {
      case T_STR :
	fprintf (out, ":\"%s\"", V_PTR (STR_VPTR (dat), char));
	break;
	
      case T_FLT :
	fprintf (out, ":\"%f\"", FLT_VL (dat));
	break;
        
      case T_SRT :
	fprintf (out, ":\"%hd\"", SRT_VL (dat));
	break;
	
      case T_INT :
	fprintf (out, ":\"%d\"", INT_VL (dat));
	break;
	
      case T_LNG :
	fprintf (out, ":\"%d\"", LNG_VL (dat));
	break;
	
      case T_DBL :
	fprintf (out, ":\"%e\"", DBL_VL (dat));
	break;
	
      default : break;
      }
  fprintf (out, ":%s ", type2str(DU->type));
} /* c_prn */
  
void
TrPrint (VADR off, i4_t spc, FILE * out)
{
  struct Node *Nd;
  i4_t i;
  
  if (!off)
    return;
  Nd = V_PTR (off, struct Node);
  for (i = 0; i < spc; i++)
    fprintf (out, " ");
  {
    char tpstr[40];
    conv_type (tpstr, &(Nd->Ty), TRANS_BACK);
    fprintf (out, "[%d: %s, Depend=%i, Handle=%i, Ty:%s, Ptr=%i",
	     off, NAME(Nd->code), Nd->Depend, Nd->Handle, tpstr, Nd->Ptr.off);

  }
  if (Nd->code == CONST)
    c_prn (Nd->Ptr.off, out);
  if (Nd->code == CALLPR)
    {
      PrepSQ *InitDt = V_PTR (Nd->Dn.off, PrepSQ);
      
      fprintf (out, ", ProcAdr =%i, GoUpNeed =%i]\n",
	       (i4_t)(InitDt->ProcAdr), InitDt->GoUpNeed);
      return;
    }
  else
    fprintf (out, "]\n");
    
  TrPrint (Nd->Dn.off, spc + 3, out);
  TrPrint (Nd->Rh.off, spc, out);
}

void
constr_print (VADR V_header, FILE *out)
{
  S_ConstrHeader *header;
  S_Constraint *constr;
  i4_t i, j;
  VADR *constr_arr;
  sql_type_t *tp;
  char *str = "";
  
  if (!V_header)
    return;
  header = V_PTR (V_header, S_ConstrHeader);
  fprintf (out, "\n  Constraints :\n  scan = %d untabid = %d tab_col_num = %d\n",
	   header->scan, header->untabid, header->tab_col_num);
  fprintf (out, "  old_vl_cnt = %d, old_vl_nums : (", header->old_vl_cnt);
  for (i = 0; i < header->old_vl_cnt; i++)
    fprintf (out, "%i ", V_PTR (header->old_vl_nums, i4_t)[i]);
  fprintf (out, ") old_vl_types : (");
  tp = V_PTR (header->old_vl_types, sql_type_t );
  for (i = 0; i < header->old_vl_cnt; i++)
    fprintf (out, "%s,", type2str(tp[i]));
  
  fprintf (out, ")\n  constr_arr : constr_num = %d\n", header->constr_num);
  
  constr_arr = V_PTR (header->constr_arr, VADR);
  for (j = 0; j < header->constr_num; j++)
    {
      constr = V_PTR (constr_arr[j], S_Constraint);
      fprintf (out, "\n    %s :", NAME (constr->constr_type));
      switch (constr->constr_type)
	{
	case FOREIGN :
	  fprintf (out, " Ind: unindid = %d untabid = %d\n",
		   (i4_t)V_PTR (constr->object, Indid)->unindid,
		   (i4_t)V_PTR (constr->object, Indid)->tabid.untabid);
	  break;
      
	case REFERENCE :
	  fprintf (out, " untabid = %d\n", (i4_t)V_PTR (constr->object, Tabid)->untabid);
	  break;
      
	case VIEW :
          str = "from WHERE in VIEW definition";
	case CHECK :
	  fprintf (out, "  checking tree %s:\n", str);
	  TrPrint (constr->object, 6, out);
	  break;
	  
	default : break;
	}
      if (constr->constr_type == FOREIGN || constr->constr_type == REFERENCE)
	{
	  fprintf (out, "      colnum = %d\n      col_arg_list : (", constr->colnum);
	  for (i = 0; i < constr->colnum; i++)
	    fprintf (out, "%i ", V_PTR (constr->col_arg_list, i2_t)[i]);
	  fprintf (out, ")\n      col_ref_list : (");
	  for (i = 0; i < constr->colnum; i++)
	    fprintf (out, "%i ", V_PTR (constr->col_ref_list, i2_t)[i]);
	  fprintf (out, ")\n");
	}
    }
} /* constr_print */

void
ModDump (FILE * out)
/* making module (from current segment) *
 * dump & putti–g it to file FName      */
{
  VADR cur, *ST, goto_place = VNULL;
  enum CmdCode code;
  i4_t SC, SectCnt;
  if (!(ST = V_PTR (*V_PTR ((VADR) 4, VADR), VADR)))
    return;
  SectCnt = (i4_t) (ST[0]);
  fprintf (out, "  Offs. from SectTable:\n");
  for (SC = 1; ST[SC]; SC++)
    if (SC <= SectCnt)
      fprintf (out, "    Sect %i =%i\n", SC, ST[SC]);
    else
      fprintf (out, "    Proc %i =%i\n", SC-SectCnt, ST[SC]);
  for (SC = 1; (cur = ST[SC]); SC++)
    {
      if (SC <= SectCnt)
	fprintf (out, "################### Section %i ##################\n",
		 SC);
      else
	fprintf (out, "################## Procedure %i #################\n",
		 SC-SectCnt);
      do
	{
	  fprintf (out, "\n-- %i ", cur);
	  code = *V_PTR (cur, enum CmdCode);
	  cur += sizeof (enum CmdCode);
          ALIGN(cur,SZ_LNG);
	  switch (code)
	    {
	      /*----------------------------------*/
	    case CursorHeader:
	      {
		COMMAND (CursorHeader, CrsHdr);

		fprintf (out, "-- CursorHeader\n");
		constr_print (CrsHdr.DelCurConstr, out);
		break;
	      }

	      /*----------------------------------*/
	    case Drop_Reg:
	      {
		i4_t i;
		COMMAND (Drop_Reg, DR);

		fprintf (out, "-- Drop_Reg:\nScanNum=%d TabdNum=%d\n",
			 DR.ScanNum, DR.TabdNum);
		fprintf (out, "List of Scan: (");
		for (i = 0; i < DR.ScanNum; i++)
		  fprintf (out, "%i, ", V_PTR (DR.Scan, VADR)[i]);
		fprintf (out, ")\nList of Tabd: (");
		for (i = 0; i < DR.TabdNum; i++)
		  fprintf (out, "%i, ", V_PTR (DR.Tabd, VADR)[i]);
		fprintf (out, ")\n");
		break;
	      }

	      /*----------------------------------*/
	    case OPSCAN:
	      {
		i4_t i;
		VADR *Tab_SP, *Ind_SP;
		Tabid *Tbd;
		Indid *Ind;
		COMMAND (OPSCAN, scan);

		fprintf (out, "-- OPSCAN:\n");
		fprintf (out, "Scan=%i Tid=%i OperType=%c mode=\\%i\n",
			 scan.Scan, scan.Tid, scan.OperType, scan.mode);
		fprintf (out, " nr=%i nl=%i nm=%i ic=%i Exit=%i\n",
			 scan.nr, scan.nl, scan.nm, scan.ic, scan.Exit);
		if (scan.OperType == 'i')
		  {
		    Ind = &(V_PTR (scan.Tid, UnId)->i);
		    fprintf (out, "    USING INDEX : unindid=%d untabid=%d\n",
			     (i4_t)Ind->unindid, (i4_t)Ind->tabid.untabid);
		  }
		else
		  {
		    assert (scan.OperType == 't');
		    Tbd = &(V_PTR (scan.Tid, UnId)->t);
		    fprintf (out, "    TABLE : untabid=%d\n", (i4_t)Tbd->untabid);
		  }
		fprintf (out, "List of rlist:\n(");
		for (i = 0; i < scan.nr; i++)
		  fprintf (out, "%i ", V_PTR (scan.rlist, i4_t)[i]);
		fprintf (out, ")\nList of mlist:\n(");
		for (i = 0; i < scan.nm; i++)
		  fprintf (out, "%i ", V_PTR (scan.mlist, i4_t)[i]);
		fprintf (out, ")\n");
		if (scan.nl)
		  {
		    fprintf (out, "List of Simple Predicates for Table:\n");
		    Tab_SP = V_PTR (scan.list, VADR);
		    for (i = 0; i < scan.nl; i++)
		      if (Tab_SP[i])
			{
			  fprintf (out, "for column %i :\n", i);
			  TrPrint (Tab_SP[i], 4, out);
			}
		  }
		if (scan.ic)
		  {
		    fprintf (out, "List of Simple Predicates for Index:\n");
		    Ind_SP = V_PTR (scan.range, VADR);
		    for (i = 0; i < scan.ic; i++)
		      if (Ind_SP[i])
			{
			  fprintf (out, "for column %i :\n", i);
			  TrPrint (Ind_SP[i], 4, out);
			}
		  }
		break;
	      }			/*end of SCAN */

	      /*--------------------------------------------------*/
	    case TMPSCN:
	      {
		i4_t i;
		VADR *Tab_SP, *Ind_SP;
		COMMAND (TMPSCN, scan);

		fprintf (out, "-- TMPSCN:\n");
		fprintf (out, "TidFrom=%i TidTo=%i OperType=%c nr=%i\n",
			 scan.TidFrom, scan.TidTo, scan.OperType, scan.nr);

		fprintf (out, "List of rlist:\n(");
		for (i = 0; i < scan.nr; i++)
		  fprintf (out, "%i ", V_PTR (scan.rlist, i4_t)[i]);

		if (scan.nl)
		  {
		    fprintf (out, "List of Simple Predicates for Table:\n");
		    Tab_SP = V_PTR (scan.list, VADR);
		    for (i = 0; i < scan.nl; i++)
		      if (Tab_SP[i])
			{
			  fprintf (out, "for column %i :\n", i);
			  TrPrint (Tab_SP[i], 4, out);
			}
		  }
		if (scan.ic)
		  {
		    fprintf (out, "List of Simple Predicates for Index:\n");
		    Ind_SP = V_PTR (scan.range, VADR);
		    for (i = 0; i < scan.ic; i++)
		      if (Ind_SP[i])
			{
			  fprintf (out, "for column %i :\n", i);
			  TrPrint (Ind_SP[i], 4, out);
			}
		  }
		break;
	      }			/*end of TMPSCN */

	      /*-----------------------------------*/
	    case TMPCRE:
	      {
		i4_t i;
		COMMAND (TMPCRE, tmp);

		fprintf (out, "-- TMPCRE:\n");
		fprintf (out, "Tid=%i colnum=%d nnulnum=%d\n",
			 tmp.Tid, tmp.colnum, tmp.nnulnum);

		fprintf (out, "List of coltype: (");
		for (i = 0; i < tmp.colnum; i++)
		  fprintf (out, "{%i,%i},", (V_PTR (tmp.coldescr, sql_type_t )[i]).code,
			   (V_PTR (tmp.coldescr, sql_type_t )[i]).len);
		fprintf (out, ")\n");
	      }
	      break;

	      /*-------------------------------------------*/
	    case SORTTBL:
	      {
		i4_t i;
		COMMAND (SORTTBL, scan);

		fprintf (out, "-- SORT:\n");
		fprintf (out, "TidIn=%i TidOut=%i fl=%c order=%c ns=%i\n",
		     scan.TidIn, scan.TidOut, scan.fl, scan.order, scan.ns);

		fprintf (out, "List of rlist:\n(");
		for (i = 0; i < scan.ns; i++)
		  fprintf (out, "%i ", V_PTR (scan.rlist, i4_t)[i]);
		fprintf (out, ")\n");
	      }
	      break;

	      /*-------------------------------------------*/
	    case MKUNION:
	      {
		COMMAND (MKUNION, scan);

		fprintf (out, "-- MKUNION:\n");
		fprintf (out, "TidIn1=%i TidIn2=%i TidOut=%i\n",
		     scan.TidIn1, scan.TidIn2, scan.TidOut);
	      }
	      break;

	      /*-------------------------------------------*/
	    case READROW:
	      {
		i4_t i;
		COMMAND (READROW, fndr);

		fprintf (out, "-- READROW:\n");
		fprintf (out, "Scan.off=%i nr=%i \n", fndr.Scan.off, fndr.nr);

		fprintf (out, "List offsets in ColPtr: (");
		for (i = 0; i < fndr.nr; i++)
		  DU_Print ((V_PTR (fndr.ColPtr.off, PSECT_PTR)[i]).off, out);
		fprintf (out, ")\nList of rlist: (");
		for (i = 0; i < fndr.nr; i++)
		  fprintf (out, "%i ", V_PTR (fndr.rlist.off, i4_t)[i]);
		fprintf (out, ")\n");
		break;
	      }			/*end of READROW */

	      /*-------------------------------------------*/
	    case FINDROW:
	      {
		i4_t i;
		COMMAND (FINDROW, fndr);

		fprintf (out, "-- FINDROW:\n");
		fprintf (out, "Scan=%i nr=%i Exit=%i\n",
			 fndr.Scan.off, fndr.nr, fndr.Exit.off);

		fprintf (out, "List of ColPtr: (");
		for (i = 0; i < fndr.nr; i++)
		  DU_Print ((V_PTR (fndr.ColPtr.off, PSECT_PTR)[i]).off, out);
		fprintf (out, ")\n");
		break;
	      }			/*end of FINDROW */

	      /*-------------------------------------------*/
	    case INSTAB:
	      {
		COMMAND (INSTAB, fndr);
		
		fprintf (out, "-- INSTAB:\n");
		fprintf (out, "TidFrom=%i (untabid = %d)  TidTo=%i (untabid = %d)\n",
			 fndr.TidFrom, (i4_t)V_PTR (fndr.TidFrom, Tabid)->untabid,
			 fndr.TidTo,   (i4_t)V_PTR (fndr.TidTo, Tabid)->untabid);
		break;
	      }			/*end of INSTAB */
		
	      /*-------------------------------------------*/
	    case INSROW:
	      {
		i4_t i;
		PSECT_PTR *Trs;
		sql_type_t *tp;
		COMMAND (INSROW, fndr);

		fprintf (out, "-- INSROW:\n");
		fprintf (out, "Tid=%i  nv=%i\nList of InsList:\n",
			 fndr.Tid.off, fndr.nv);
		fprintf (out, "List of coltype: (");
		tp = V_PTR (fndr.ClmTp.off, sql_type_t );
		for (i = 0; i < fndr.nv; i++)
		  fprintf (out, "%s,", type2str(tp[i]));
		fprintf (out, ")\n");
		Trs = V_PTR (fndr.InsList.off, PSECT_PTR);
		for (i = 0; i < fndr.nv; i++)
		  {
		    fprintf (out, "-------------Tree offset=%i-------------\n",
			     Trs[i].off);
		    TrPrint (Trs[i].off, 4, out);
		  }
		constr_print (fndr.Constr.off, out);
		break;
	      }			/*end of INSROW */

	      /*-------------------------------------------*/
	    case INSLIST:
	      {
		i4_t i;
		i2_t *len;
		sql_type_t *tp;
		VADR *cptr;
		COMMAND (INSLIST, fndr);

		fprintf (out, "-- INSLIST:\n");
		fprintf (out, "Tid=%i  nv=%i credat_adr=%i cretime_adr=%i\n",
			 fndr.Tid.off, fndr.nv, (i4_t)(fndr.credat_adr.off), 
			 (i4_t)(fndr.cretime_adr.off));
		fprintf (out, "List of coltype: (");
		tp = V_PTR (fndr.col_types.off, sql_type_t );
		for (i = 0; i < fndr.nv; i++)
		  fprintf (out, "%s,", type2str(tp[i]));
		fprintf (out, ")\n");
		if (fndr.len.off)
		  {
		    fprintf (out, "List of lengths:\n    (");
		    len = V_PTR (fndr.len.off, i2_t);
		    for (i = 0; i < fndr.nv; i++)
		      fprintf (out, "%d, ", len[i]);
		    fprintf (out, ")\n");
		  }
		fprintf (out,"List of col_ptr:\n     (");
		cptr = V_PTR (fndr.col_ptr.off, VADR);
		for (i = 0; i < fndr.nv; i++)
		  fprintf (out, "%i,", cptr[i]);
		fprintf (out, ")\n");
		break;
	      }			/*end of INSLIST */

	      /*-------------------------------------------*/
	    case MODROW:
	      {
		i4_t i;
		PSECT_PTR *Trs;
		sql_type_t *tp;
		COMMAND (MODROW, fndr);

		fprintf (out, "-- MODROW:\n");
		fprintf (out, "Scan = %i  nl = %i\n", fndr.Scan.off, fndr.nl);

		fprintf (out, "List of mlist: (");
		if (fndr.mlist.off)
		  for (i = 0; i < fndr.nl; i++)
		    fprintf (out, "%i ", V_PTR (fndr.mlist.off, i4_t)[i]);
		fprintf (out, ")\n");
		
		fprintf (out, "List of coltype: (");
		tp = V_PTR (fndr.ClmTp.off, sql_type_t );
		for (i = 0; i < fndr.nl; i++)
		  fprintf (out, "%s,", type2str(tp[i]));
		fprintf (out, ")\n");
		
		fprintf (out, "List of OutList: (");
		Trs = V_PTR (fndr.OutList.off, PSECT_PTR);
		for (i = 0; i < fndr.nl; i++)
		  {
		    fprintf (out, "-------------Tree offset=%i-------------\n",
			     Trs[i].off);
		    TrPrint (Trs[i].off, 4, out);
		  }
		
		constr_print (fndr.Constr.off, out);
		break;
	      }			/*end of MODROW */

	      /*-------------------------------------------*/
	    case DELROW:
	      {
		COMMAND (DELROW, cls);
		fprintf (out, "-- DELROW(Scan=%i)\n", cls.Scan);
		constr_print (cls.Constr, out);
		break;
	      }			/*end of DELROW */

	      /*-------------------------------------------*/
	    case CLOSE:
	      {
		COMMAND (CLOSE, cls);
		fprintf (out, "-- CLOSE ( Scan=%i )\n", cls.Scan);
		break;
	      }			/*end of CLOSE */

	      /*-------------------------------------------*/
	    case DROPBASE:
	      {
		COMMAND (DROPBASE, tmp);
		fprintf (out, "-- DROPBASE ( untabid=%i )\n", tmp.untabid);
	      }
	      break;

	      /*-------------------------------------------*/
	    case DROPTTAB:
	      {
		COMMAND (DROPTTAB, tmp);
		fprintf (out, "-- DROPTTAB ( Tid=%i )\n", tmp.Tid);
	      }
	      break;

	      /*-------------------------------------------*/
	    case MKGROUP:
	      {
		i4_t i;
		COMMAND (MKGROUP, cls);

		fprintf (out, "-- MKGROUP:\n");
		fprintf (out, "TidIn=%i TidOut=%i order=%c ng=%i\n",
			 cls.TidIn, cls.TidOut, cls.order, cls.ng);

		fprintf (out, "List of glist:\n(");
		for (i = 0; i < cls.ng; i++)
		  fprintf (out, "%i ", V_PTR (cls.glist, i4_t)[i]);

		fprintf (out, ")\n nf=%i\n", cls.nf);
		fprintf (out, "List of flist:\n(");
		for (i = 0; i < cls.nf; i++)
		  fprintf (out, "%i ", V_PTR (cls.flist, i4_t)[i]);

		fprintf (out, ")\nList of fl:\n(");
		for (i = 0; i < cls.nf; i++)
		  fprintf (out, "%i ", V_PTR (cls.fl, char)[i]);
		fprintf (out, ")\n");
		break;
	      }			/*end of GROUP */

	      /*-------------------------------------------*/
	    case FUNC:
	      {
		i4_t i;
		VADR *Tab_SP, *Ind_SP;
		COMMAND (FUNC, cls);

		fprintf (out, "-- FUNC:\n");
		fprintf (out, "Tid=%i OperType=%c nl=%i Exit=%i\n",
			 cls.Tid.off, cls.OperType, cls.nl, cls.Exit.off);

		if (cls.nl)
		  {
		    fprintf (out, "List of Simple Predicates for Table:\n");
		    Tab_SP = V_PTR (cls.list.off, VADR);
		    for (i = 0; i < cls.nl; i++)
		      if (Tab_SP[i])
			{
			  fprintf (out, "for column %i :\n", i);
			  TrPrint (Tab_SP[i], 4, out);
			}
		  }
		if (cls.ic)
		  {
		    fprintf (out, "List of Simple Predicates for Index:\n");
		    Ind_SP = V_PTR (cls.range.off, VADR);
		    for (i = 0; i < cls.ic; i++)
		      if (Ind_SP[i])
			{
			  fprintf (out, "for column %i :\n", i);
			  TrPrint (Ind_SP[i], 4, out);
			}
		  }
		
		fprintf (out, "List of flist:\n(");
		for (i = 0; i < cls.nf; i++)
		  fprintf (out, "%i ", V_PTR (cls.flist.off, i4_t)[i]);

		fprintf (out, ")\nList of fl:\n(");
		for (i = 0; i < cls.nf; i++)
		  fprintf (out, "%i ", V_PTR (cls.fl.off, char)[i]);

		fprintf (out, ")\nList of colval: (");
		for (i = 0; i < cls.nf; i++)
		  DU_Print ((V_PTR (cls.colval.off, PSECT_PTR)[i]).off, out);
		fprintf (out, ")\n");
		break;

	      }			/*end of FUNC */

	      /*-------------------------------------------------*/
	    case until:
	      {
		COMMAND (until, utl);

		fprintf (out, "-- until:\n");
		fprintf (out, "Offs of: cond=%i ret=%i\n", utl.cond.off,
			 utl.ret.off);
		TrPrint (utl.cond.off, 4, out);
		break;
	      }			/*end of until */

	      /*-------------------------------------------*/
	    case RetPar:
	      {
		i4_t i;
		COMMAND (RetPar, retp);

		fprintf (out, "-- RetPar:\n");
		fprintf (out, ", ExitFlag = %d OutNum=%i \nList of OutList:\n",
			 (i4_t)(retp.ExitFlag), retp.OutNum);
		for (i = 0; i < retp.OutNum; i++)
		  {
		    fprintf (out, "-------------Tree offset=%i-------------\n",
			     (V_PTR (retp.OutList.off, PSECT_PTR)[i]).off);
		    TrPrint ((V_PTR (retp.OutList.off, PSECT_PTR)[i]).off, 4, out);
		  }
		break;
	      }			/*end of RetPar*/

	      /*-------------------------------------------*/
	    case SavePar:
	      {
		i4_t i;
		COMMAND (SavePar, savp);

		fprintf (out, "-- SavePar:\n");
		fprintf (out, "ParamNum=%i \nList of PlaceOff: (", savp.ParamNum);
		for (i = 0; i < savp.ParamNum; i++)
		  DU_Print ((V_PTR (savp.PlaceOff.off, PSECT_PTR)[i]).off, out);
		fprintf (out, ")\n");
		break;
	      }			/*end of SavePar*/

	      /*-------------------------------------------*/
	    case COND_EXIT:
	      {
		COMMAND (COND_EXIT, CE);

		fprintf (out, "-- COND_EXIT:\n");
		fprintf (out, "-------------Tree offset=%i-------------\n",
			 CE.Tree.off);
		TrPrint (CE.Tree.off, 4, out);
		break;
	      }			/*end of COND_EXIT */

	      /*-------------------------------------------*/
	    case next_vm_piece:
	      {
		cur = goto_place;
		fprintf (out, "--  next_vm_piece: %i\n", goto_place);
		break;
	      }			/*end of next_vm_piece */

	      /*-------------------------------------------*/
	    case GoTo:
	      {
		COMMAND (GoTo, savv);
		
		goto_place = savv.Branch.off;
		fprintf (out, "-- GoTo : %i\n", goto_place);
		break;
	      }			/*end of GOTO */

	      /*-------------------------------------------*/
	    case SetHandle:
	      fprintf (out, "-- SetHandle:\n");
	      {
		i4_t i;
		COMMAND (SetHandle, sh);

		fprintf (out, "Bit=%i TrCnt=%d\nList of Trees: (", sh.Bit, sh.TrCnt);
		for (i = 0; i < sh.TrCnt; i++)
		  fprintf (out, "%i ", (V_PTR (sh.Trees.off, PSECT_PTR)[i]).off);
		fprintf (out, ")\n");
		break;
	      }			/*end of SetHandle */

	      /*-------------------------------------------*/
	    case end:
	      fprintf (out, "-- END:\n");
	      break;		/*end of END */

	      /*-------------------------------------------*/
	    case ERROR:
	      {
		COMMAND (ERROR, er);

		fprintf (out, "-- ERROR = %d\n", er.Err);
		break;		/*end of ERROR */
	      }

	      /*-------------------------------------------*/
	    case CRETAB:
	      {
		i4_t i;
		COMMAND (CRETAB, tmp);

		fprintf (out, "-- CRETAB:\n");
		fprintf (out, "tabname='%s' owner='%s'\n",
			 V_PTR (tmp.tabname, char), V_PTR (tmp.owner, char));
		fprintf (out, "segid=%d colnum=%d nncolnum=%d tabid=%d\n",
			 tmp.segid, tmp.colnum, tmp.nncolnum, (i4_t)(tmp.tabid));

		fprintf (out, "List of coltype: (");
                {
                  sql_type_t *tp = V_PTR (tmp.coldescr, sql_type_t);
                  for (i = 0; i < tmp.colnum; i++)
                    fprintf (out, "%s,", type2str(tp[i]));
                }
                fprintf (out, ")\n");
                  
	      }
	      break;

	      /*-------------------------------------------*/
	    case CREIND:
	      {
		i4_t i;
		COMMAND (CREIND, tmp);

		fprintf (out, "-- CREIND:\n");
		fprintf (out, "tabid=%d indid=%d uniq_fl=\\%i colnum=%d\n",
			 (i4_t)(tmp.tabid), (i4_t)(tmp.indid), tmp.uniq_fl, tmp.colnum);

		fprintf (out, "List of columns' numbers : (");
		for (i = 0; i < tmp.colnum; i++)
		  fprintf (out, "%i ", V_PTR (tmp.clmlist, i4_t)[i]);
		fprintf (out, ")\n");
	      }
	      break;

	      /*-------------------------------------------*/
	    case PRIVLG:
	      {
		i4_t i;
		COMMAND (PRIVLG, tmp);

		fprintf (out, "-- PRIVLG:\n");
		fprintf (out, "untabid=%d owner='%s' user='%s'\n",
			 *V_PTR(tmp.untabid, i4_t), V_PTR(tmp.owner, char), 
			 V_PTR(tmp.user, char));
		if (tmp.privcodes)
		  fprintf (out,"  privcodes='%s'\n", V_PTR(tmp.privcodes, char));
		if (tmp.un)
		  {
		    fprintf (out, "UPDATE: count=%d , columns : (", tmp.un);
		    for (i = 0; i < tmp.un; i++)
		      fprintf (out, "%i ", V_PTR (tmp.ulist, i4_t)[i]);
		    fprintf (out, ")\n");
		  }
		if (tmp.rn)
		  {
		    fprintf (out, "REFERENCES: count=%d , columns : (", tmp.rn);
		    for (i = 0; i < tmp.rn; i++)
		      fprintf (out, "%i ", V_PTR (tmp.rlist, i4_t)[i]);
		    fprintf (out, ")\n");
		  }
	      }
	      break;

	    default:
	      fprintf (out, "? ?!! UNRECOGNIZED COMMAND ? ?!!\n");
	    }
	}
      while (code != end);
    }				/* end of 'for' with ST */
  fprintf (out, "************** end of module.");
} /* ModDump */
