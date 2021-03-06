/*
 * $Id: test31.ec,v 1.2 1998/09/29 21:27:01 kimelman Exp $
 *
 * This file is a part of GNU SQL Server
 *
 * Copyright (c) 1996-1998, Free Software Foundation, Inc
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
  char tabname[100],owner[100],credate[100],cretime[100];
  char tabtype[5];
  int i,untabid,tabd,primindid,nrows;
  short segid,ncols,rowsize,nindexes; 

  EXEC SQL 
    DECLARE CURS1 CURSOR FOR
    (
     SELECT UNTABID
     FROM DEFINITION_SCHEMA.SYSTABLES
     WHERE
     UNTABID - 1 NOT BETWEEN NROWS - 5 AND NROWS + 5 --17 AND 19 
     and
     NOT (UNTABID BETWEEN 1 AND 8)
     AND
     UNTABID NOT IN (
                     SELECT UNTABID
                     FROM DEFINITION_SCHEMA.SYSCOLUMNS
                     WHERE UNTABID > 15
                     )
     AND
     UNTABID NOT IN (2, 4)
     AND
     TABNAME NOT LIKE  'S' ESCAPE 'S' -- LIKE 'S' ESCAPE 'S'
     )
    ;
 
  $ WHENEVER SQLERROR GOTO errexit;
  $ WHENEVER NOT FOUND GOTO exit;
 
  $ open CURS1;
  while(1)
    {
      $ fetch CURS1 into :untabid ;
      printf("untabid='%d',\n",untabid);
    }

exit:
  fprintf(stderr,"End of Table DEFINITION_SCHEMA.SYSTABLES\n");

  $ close CURS1;

  fprintf(stderr,"%s: success\n",__FILE__);
  $ commit work;
  return 0;
  
errexit:
  fprintf(stderr,"%s: error (%d) : %s\n",
          __FILE__,gsqlca.sqlcode,gsqlca.errmsg);
  return 1;
}
