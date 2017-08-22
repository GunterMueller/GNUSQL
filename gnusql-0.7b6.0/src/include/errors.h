/* 
 *  errors.h -  file with errors description for GNU SQL compiler
 *                
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
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

/* $Id: errors.h,v 1.247 1997/11/05 16:01:20 kml Exp $ */

#ifdef DEF_ERR

DEF_ERR (OK, "OK")
DEF_ERR (ER_NU, "The key of this tuple isn't unique")
DEF_ERR (ER_NCF, "Incorrect conditions")
DEF_ERR (ER_NDR, "Incorrect relation identifier")
DEF_ERR (ER_NMS, "Incorrect scan mode")
DEF_ERR (ER_NCR, "Current tuple is absent")
DEF_ERR (ER_NIOB, "Incorrect object identifier")
DEF_ERR (ER_NDSC, "Incorrect scan identifier")
DEF_ERR (ER_NDI, "Incorrect index identifier")
DEF_ERR (ER_N_SORT, "Not sorted")
DEF_ERR (ER_H_DBL, "Has doublicates")
DEF_ERR (ER_D_DRCTN, "Sort directions are not matched")
DEF_ERR (ER_N_EQV, "Objects are not equivalent")
DEF_ERR (ER_EMFL, "The filter is empty")
DEF_ERR (ER_EOSCAN, "End of scan")
DEF_ERR (ER_NO_SUCH_SEG, "No such segment")
DEF_ERR (ER_SYER, "Synchronization error")
DEF_ERR (ER_FATAL, "Fatal error")
DEF_ERR (ER_1, "Incorrect data in the tree")
DEF_ERR (ER_3, "Incorrect operation in the tree")
DEF_ERR (ER_4, "Initialization error")
DEF_ERR (ER_5, "Incorrect section number")
DEF_ERR (ER_6, "Incorrect cursor operation")
DEF_ERR (ER_8, "Incorrect command code in module")
DEF_ERR (ER_NOMEM, "Memory exhausted")
DEF_ERR (ER_RET, "SubQuery handling error")
DEF_ERR (ER_SQ, "More than one row in SubQuery")
DEF_ERR (ER_OpCur, "Attemp to open unclosed cursor")
DEF_ERR (ER_Fetch, "Attemp to fetch data by unopened cursor")
DEF_ERR (ER_Close, "Attemp to close unopened cursor")
DEF_ERR (ER_Delete, "Attemp to positional delete before cursor open")
DEF_ERR (ER_CRETAB, "Table creation error")
DEF_ERR (ER_DRTAB, "Table dropping error: integrity violation")
DEF_ERR (ER_CREIND, "Index creation error")
DEF_ERR (ER_PRIVLG, "There is not enough priveleges")
DEF_ERR (ER_BD, "Incorrect information from the low level")
DEF_ERR (ER_CLNT, "Client calling error")
DEF_ERR (ER_SERV, "Incorrect work with server")
DEF_ERR (ND_INDIC, "NULL result for parameter without indicator")
DEF_ERR (NOCRTR, "Transactiion can't be created now")
DEF_ERR (NEED_WAIT, "There is no result yet")
DEF_ERR (MEM_ERR, "Memory allocation error")
DEF_ERR (MDLINIT, "Module initialization error")
DEF_ERR (NOTRNANS, "Client can't receive answer from server-transaction")
DEF_ERR (TRN_INIT, "Transaction initialization error")
DEF_ERR (TRN_EXITED, "Transaction is finished")
DEF_ERR (NULLRES, "Empty result from interpretator")
DEF_ERR (TRN_ID, "Dispatcher: Incorrect identifier of transaction process")
DEF_ERR (ER_SEL, "More than one row in the result of operator SELECT")
DEF_ERR (ER_BUF, "Inetrnal buffer overflow (copy.c)")
DEF_ERR (CHECK_VIOL, "CHECK constraint violation")
DEF_ERR (REF_VIOL, "RERERENCE constraint violation")
DEF_ERR (VIEW_VIOL, "WITH CHECK OPTION constraint violation")
DEF_ERR (DS_STMT, "Invalid SQL statement name")
DEF_ERR (DS_CUR, "Invalid SQL cursor name")
DEF_ERR (DS_CURST, "Invalid cursor state")
DEF_ERR (DS_DESCR, "Dynamic SQL: Incorrect descriptor")
DEF_ERR (DS_DESCRLEN, "Dynamic SQL: static size of descriptor is too small")
DEF_ERR (DS_NMARG, "Dynamic SQL: Incorrect name-argument")
DEF_ERR (DS_CURDECL, "Dynamic SQL: Redeclaration of cursor name")
DEF_ERR (DS_SYNTER, "Syntax error or access rule violation in dynamic SQL statement")
DEF_ERR (TNERR, "Incorrect table name in dynamic SQL statement")
DEF_ERR (DS_EXE, "More than one row in the result of dynamic SQL execute statement")
DEF_ERR (DS_NOPAR,  "Dynamic SQL error - USING clause required for dynamic parameters")
DEF_ERR (DS_NORES,  "Dynamic SQL error - USING clause required for result fields")
DEF_ERR (DS_CNT,    "Dynamic SQL error - invalid descriptor count")
DEF_ERR (DS_BADTAR, "Dynamic SQL error - USING clause does not match target specification")
DEF_ERR (DS_BADPAR, "Dynamic SQL error - USING clause does not match parameter specification")
DEF_ERR (ER_COMP_FATAL, "SQL Compiler fatal (prbably internal) error")
#endif
