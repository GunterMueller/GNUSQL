/*
 * $Id: test40.ec,v 1.2 1998/09/29 21:27:03 kimelman Exp $
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

int
main(void)
{
  char str1[100], str2[100];
  short i=3,k=2, cn;
  int j=0;
  long cnt;

  $ whenever sqlerror goto errexit;
  EXEC SQL
  DECLARE CURS1 CURSOR FOR
    (
      SELECT DISTINCT COLNAME, TABNAME, t.UNTABID, COLNO
      FROM DEFINITION_SCHEMA.SYSCOLUMNS c, DEFINITION_SCHEMA.SYSTABLES t
--    WHERE 
--      t.UNTABID = c.UNTABID
--        AND
--      COLNAME >= 'E'
--        AND
--      t.UNTABID < 10
    )
;

  $ select count(*) into :j 
      FROM DEFINITION_SCHEMA.SYSCOLUMNS c, DEFINITION_SCHEMA.SYSTABLES t
--    WHERE 
--    t.UNTABID < c.UNTABID
--        AND
--      COLNAME >= 'E'
--        AND
--      t.UNTABID < 10
    ;

  printf("Expected number of rows : %d \n",j);
  
  $ WHENEVER NOT FOUND GOTO exit;
  $ open CURS1;
  while(1) 
    {
      $ fetch CURS1 into :str1, :str2, :j,:cn;
      fprintf(stderr,"colname='%s', tabname=%s, untabid='%d', colno='%d'\n",
              str1, str2, j, cn);
    }
 exit:
  fprintf(stderr,"End of Curs 1\n");

  $ close CURS1;

  fprintf(stderr,"%s: success\n",__FILE__);
  $ commit work;
  return 0;
  
errexit:
  fprintf(stderr,"%s: error (%d) : %s\n",
          __FILE__,gsqlca.sqlcode,gsqlca.errmsg);
  return 1;
}

