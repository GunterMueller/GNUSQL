/*
 *  const.h -  identifiers of system DB catalogs columns of
 *             GNU SQL server
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Olga Dmitrieva, 1994
 *  Modified by Michael Kimelman, 1996
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

#ifndef __const_h__
#define __const_h__

/* $Id: const.h,v 1.246 1998/09/29 21:25:51 kimelman Exp $ */

#define BASE_DAT  "db/base.dat"

#define SYSADM    "DEFINITION_SCHEMA"

#define SYSTABLES      systabtabid
#define SYSCOLUMNS     syscoltabid   
#define SYSINDEXES     sysindtabid
#define SYSTABAUTH     tabauthtabid   
#define SYSCOLAUTH     colauthtabid   
#define SYSREFCONSTR   refconstrtabid
#define SYSCHCONSTR    chconstrtabid
#define SYSCHCONSTRTWO chcontwotabid
#define SYSVIEWS       viewstabid
/*-----------------------------------------------*/
#define TABLES_COLNO      13
#define COLUMNS_COLNO     11
#define INDEXES_COLNO     16
#define TABAUTH_COLNO      4
#define COLAUTH_COLNO      5
#define REFCONSTR_COLNO   20
#define CHCONSTR_COLNO    12
#define CHCONSTRTWO_COLNO  4
#define VIEWS_COLNO        4

#define MAX_COLNO         20

#define TABLES_NULNO       3
#define COLUMNS_NULNO      4
#define INDEXES_NULNO      3
#define TABAUTH_NULNO      4
#define COLAUTH_NULNO      5
#define REFCONSTR_NULNO    5
#define CHCONSTR_NULNO     5
#define CHCONSTRTWO_NULNO  4
#define VIEWS_NULNO        4

/*-------------------------------------------------*/
#define SYSTABNAME     0
#define SYSTABOWNER    1
#define SYSTABUNTABID  2
#define SYSTABSEGID    3
#define SYSTABTABD     4
#define SYSTABCLUSTIND 5
#define SYSTABTABTYPE  7
#define SYSTABNCOLS    10
#define SYSTABNROWS    11
#define SYSTABNNULCOLNUM 12

/*-------------------------------------------------*/
#define SYSCOLCOLNAME  0
#define SYSCOLUNTABID  1
#define SYSCOLCOLNO    2
#define SYSCOLCOLTYPE  3
#define SYSCOLCOLTYPE1 4
#define SYSCOLCOLTYPE2 5
#define SYSCOLDEFVAL   6
#define SYSCOLDEFNULL  7
#define SYSCOLVALNO    8
#define SYSCOLMIN      9
#define SYSCOLMAX     10

/*-----------------------------------------------*/
#define SYSTABAUTHUNTABID  0
#define SYSTABAUTHGRANTEE  1
#define SYSTABAUTHTABAUTH  3
/*-----------------------------------------------*/
#define SYSCOLAUTHUNTABID  0
#define SYSCOLAUTHCOLNO    1
#define SYSCOLAUTHGRANTEE  2
#define SYSCOLAUTHCOLAUTH  4
/*-----------------------------------------------*/
#define SYSREFCONSTRTABFROM  0
#define SYSREFCONSTRTABTO    1
#define SYSREFCONSTRINDTO    2
#define SYSREFCONSTRNCOLS    3
#define SYSREFCONSTRCOLNOFR1 4
#define SYSREFCONSTRCOLNOFR2 5
#define SYSREFCONSTRCOLNOFR3 6
#define SYSREFCONSTRCOLNOFR4 7
#define SYSREFCONSTRCOLNOFR5 8
#define SYSREFCONSTRCOLNOFR6 9
#define SYSREFCONSTRCOLNOFR7 10
#define SYSREFCONSTRCOLNOFR8 11
#define SYSREFCONSTRCOLNOTO1 12
#define SYSREFCONSTRCOLNOTO2 13
#define SYSREFCONSTRCOLNOTO3 14
#define SYSREFCONSTRCOLNOTO4 15
#define SYSREFCONSTRCOLNOTO5 16
#define SYSREFCONSTRCOLNOTO6 17
#define SYSREFCONSTRCOLNOTO7 18
#define SYSREFCONSTRCOLNOTO8 19
/*-----------------------------------------------*/
#define SYSCHCONSTRUNTABID   0
#define SYSCHCONSTRCHCONID   1
#define SYSCHCONSTRCONSIZE   2
#define SYSCHCONSTRNCOLS     3
#define SYSCHCONSTRCOLNO1    4
#define SYSCHCONSTRCOLNO2    5
#define SYSCHCONSTRCOLNO3    6
#define SYSCHCONSTRCOLNO4    7
#define SYSCHCONSTRCOLNO5    8
#define SYSCHCONSTRCOLNO6    9
#define SYSCHCONSTRCOLNO7    10
#define SYSCHCONSTRCOLNO8    11
/*-----------------------------------------------*/
#define SYSCHCONSTR2CHCONID   0
#define SYSCHCONSTR2FRAGNO    1
#define SYSCHCONSTR2FRAGSIZE  2
#define SYSCHCONSTR2FRAG      3
/*-----------------------------------------------*/
#define SYSVIEWSCHCONID      0
#define SYSVIEWSFRAGNO       1
#define SYSVIEWSFRAGSIZE     2
#define SYSVIEWSFRAG         3
/*-----------------------------------------------*/
#define SYSINDEXUNTABID      0
#define SYSINDEXTYPE         3
#define SYSINDEXNCOL         4
#define SYSINDEXCOLNO1       5
#define SYSINDEXCOLNO2       6
#define SYSINDEXCOLNO3       7
#define SYSINDEXCOLNO4       8
#define SYSINDEXCOLNO5       9
#define SYSINDEXCOLNO6       10
#define SYSINDEXCOLNO7       11
#define SYSINDEXCOLNO8       12
#define SYSINDEXUNINDID      15
/*------------------------- E N D -----------------------*/

#endif
