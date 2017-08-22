/*
 * $Id: test3c.ec,v 1.1 1998/01/19 06:14:44 kml Exp $
 *
 * This file is a part of GNU SQL Server
 *
 * Copyright (c) 1996, Free Software Foundation, Inc
 * Developed at Institute of System Programming of Russian Academy of Science
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contacts: gss@ispras.ru
 */
#include <stdio.h>
#include <stdlib.h>
#include "tests.h"
#include "typepi.h"

int
main(void)
{
  int i = 5;
  char str[] = "SZ" /* "TABD" "UNTABID" */;
  char COLNAME[20], COLTYPE[2], DEFVAL[500],
    DEFNULL[2], MAXVAL[500], MINVAL[500];
  int  UNTABID, COLTYPE1, COLTYPE2, VALNO;
  short COLNO;
  int  DEFVAL_ind, VALNO_ind, MAXVAL_ind, MINVAL_ind;
  
  EXEC SQL
    DECLARE CURS1 CURSOR FOR
    (SELECT COLNAME, UNTABID, COLNO, COLTYPE, COLTYPE1,
     COLTYPE2, DEFVAL, DEFNULL, VALNO, MINVAL, MAXVAL
     FROM DEFINITION_SCHEMA.SYSCOLUMNS
--     WHERE UNTABID = 2
--     WHERE UNTABID > 10
--     AND COLNAME = 'k1'
--   WHERE COLNAME >= :str AND UNTABID < :i --OR UNTABID > 100
    )
    ;
  
  $ WHENEVER SQLERROR GOTO errexit;
  $ WHENEVER NOT FOUND GOTO exit;
  $ open CURS1;
  while(1)
    {
      $ fetch CURS1 into :COLNAME, :UNTABID, :COLNO, :COLTYPE, :COLTYPE1,
	:COLTYPE2, :DEFVAL :DEFVAL_ind, :DEFNULL,
	:VALNO :VALNO_ind, :MINVAL :MINVAL_ind, :MAXVAL :MAXVAL_ind;
      
      PRINT_S (COLNAME, "colname");
      PRINT_D (UNTABID, "untabid");
      PRINT_D (COLNO, "colno");
      PRINT_D (*COLTYPE, "coltype");
      PRINT_D (COLTYPE1, "coltype1");
      PRINT_D (COLTYPE2, "coltype2");
      PRINT_IS (DEFVAL, "defval");
      PRINT_S (DEFNULL, "defnull");
      PRINT_ID (VALNO, "valno");
      switch (*COLTYPE)
	{
	case T_STR :
	  PRINT_IS (MINVAL, "minval");
	  PRINT_IS (MAXVAL, "maxval");
	  break;
	  
	case T_SRT :
	  PRINT_IDI ((*(short*)MINVAL), MINVAL_ind, "minval");
	  PRINT_IDI ((*(short*)MAXVAL), MAXVAL_ind, "maxval");
	  break;
	  
	case T_INT :
	  PRINT_IDI ((*(int*)MINVAL), MINVAL_ind, "minval");
	  PRINT_IDI ((*(int*)MAXVAL), MAXVAL_ind, "maxval");
	  break;
	  
	case T_LNG :
	  PRINT_IDI ((*(long*)MINVAL), MINVAL_ind, "minval");
	  PRINT_IDI ((*(long*)MAXVAL), MAXVAL_ind, "maxval");
	  break;
	  
	case T_FLT :
	  PRINT_IDI ((*(float*)MINVAL), MINVAL_ind, "minval");
	  PRINT_IDI ((*(float*)MAXVAL), MAXVAL_ind, "maxval");
	  break;
	}
      
      PRINT_END;
    }
exit:
  fprintf(stderr,"End of Table SYSCOLUMNS\n");
  $ close CURS1;
  fprintf(stderr,"%s: success\n",__FILE__);
  $ commit work;
  return 0;
  
errexit:
  fprintf(stderr,"%s: error (%d) : %s\n",
          __FILE__,gsqlca.sqlcode,gsqlca.errmsg);
  return 1;
}
