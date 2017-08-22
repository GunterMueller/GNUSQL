/*
 * $Id: test3a.ec,v 1.1 1998/01/19 06:14:44 kml Exp $
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
--   WHERE
--     UNTABID = 2    and
--     UNTABID > 10   and
--     COLNAME = 'k1' and
--     COLNAME >= :str AND
--       UNTABID < :i
--     OR
--       UNTABID > 100
    )
    ;
  
  $ WHENEVER SQLERROR GOTO errexit;
  $ WHENEVER NOT FOUND GOTO exit;
  printf(" /----------------------------\\\n");
  printf("/          SYSCOLUMNS           \\ \n");
  $ open CURS1;
  printf("|--------------------------------|\n");
  while(1)
    {
      $ fetch CURS1 into :COLNAME, :UNTABID, :COLNO, :COLTYPE, :COLTYPE1,
	:COLTYPE2, :DEFVAL :DEFVAL_ind, :DEFNULL,
	:VALNO :VALNO_ind, :MINVAL :MINVAL_ind, :MAXVAL :MAXVAL_ind;
      
      printf("| %3d | %18s | %3d |\n",UNTABID,COLNAME,COLNO);
    }
exit:
  printf("|--------------------------------|\n");
  $ close CURS1;
  printf("\\         SYSCOLUMNS           / \n");
  printf(" \\----------------------------/\n");
  
  EXEC SQL
    DECLARE CURS2 CURSOR FOR
    (SELECT COLNAME, UNTABID, COLNO, COLTYPE, COLTYPE1,
     COLTYPE2, DEFVAL, DEFNULL, VALNO, MINVAL, MAXVAL
     FROM DEFINITION_SCHEMA.SYSCOLUMNS
     WHERE
--     UNTABID = 2    and
       UNTABID > 10  
--     COLNAME = 'k1' and
--     COLNAME >= :str AND
--       UNTABID < :i
--     OR
--       UNTABID > 100
    )
    ;
  
  $ WHENEVER NOT FOUND GOTO exit2;
  $ open CURS2;
  printf(" /----------------------------\\\n");
  printf("/          SYSCOLUMNS           \\ \n");
  printf("|--------------------------------|\n");
  while(1)
    {
      $ fetch CURS2 into :COLNAME, :UNTABID, :COLNO, :COLTYPE, :COLTYPE1,
	:COLTYPE2, :DEFVAL :DEFVAL_ind, :DEFNULL,
	:VALNO :VALNO_ind, :MINVAL :MINVAL_ind, :MAXVAL :MAXVAL_ind;
      
      printf("| %3d | %18s | %3d |\n",UNTABID,COLNAME,COLNO);
    }
exit2:
  printf("|--------------------------------|\n");
  $ close CURS2;
  printf("\\         SYSCOLUMNS           / \n");
  printf(" \\----------------------------/\n");
  
  EXEC SQL
    DECLARE CURS3 CURSOR FOR
    (SELECT COLNAME, UNTABID, COLNO
     FROM DEFINITION_SCHEMA.SYSCOLUMNS
     WHERE
       UNTABID <= 4
    )
    ;
  
  $ WHENEVER NOT FOUND GOTO exit3;
  $ open CURS3;
  printf("|--------------------------------|\n");
  while(1)
    {
      $ fetch CURS3 into :COLNAME, :UNTABID, :COLNO;
      
      printf("| %3d | %18s | %3d |\n",UNTABID,COLNAME,COLNO);
    }
exit3:
  printf("|--------------------------------|\n");
  fprintf(stderr,"End of Table SYSCOLUMNS\n");
  $ close CURS3;
  
  fprintf(stderr,"%s: success\n",__FILE__);
  $ commit work;
  return 0;
  
errexit:
  fprintf(stderr,"%s: error (%d) : %s\n",
          __FILE__,gsqlca.sqlcode,gsqlca.errmsg);
  return 1;
}
