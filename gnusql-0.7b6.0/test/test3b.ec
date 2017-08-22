/*
 * $Id: test3b.ec,v 1.2 1998/06/01 15:13:08 kimelman Exp $
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
main(int argc, char *argv[])
{
  char  COLNAME[20];
  int   UNTABID,tab;
  short COLNO;
  short NCOLS;
  int   NROWS;
  int   NNULCOLNUM;
  char  TABNAME[20];
  int   ind,ind1;
  
  tab = 3;
  if(argc>1)
    tab = atoi(argv[1]);
  
  $ WHENEVER SQLERROR  GOTO errexit;
  
  $ DECLARE CURS3 CURSOR FOR
    (SELECT COLNAME, UNTABID, COLNO
     FROM DEFINITION_SCHEMA.SYSCOLUMNS
     WHERE
       UNTABID = :tab and COLNO < 2
    )
    ;
  $ DECLARE CURST CURSOR FOR
    (SELECT UNTABID, TABNAME, NCOLS,NROWS, NNULCOLNUM
     FROM DEFINITION_SCHEMA.SYSTABLES
    )
    ;
  
  $ WHENEVER NOT FOUND GOTO exitt;
  $ open CURST;
  printf("\n|--------------------------------------------|\n");
  while(1)
    {
      $ fetch CURST into :UNTABID, :TABNAME, :NCOLS,:NROWS :ind, :NNULCOLNUM :ind1;
      
      printf("| %3d | %18s | %3d | %3d | %3d |\n",
             UNTABID, TABNAME, NCOLS,NROWS, NNULCOLNUM);
    }
exitt:
  printf("|--------------------------------------------|\n");
  fprintf(stderr,"End of Table SYSTABLES\n");
  $ close CURST;
  
  $ WHENEVER NOT FOUND GOTO exit3;
  $ open CURS3;
  printf("\n|--------------------------------|\n");
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









