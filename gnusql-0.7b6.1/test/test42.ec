/*
 * $Id: test42.ec,v 1.2 1998/09/29 21:27:04 kimelman Exp $
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
  char tabname[100],owner[100],inxname[100],colname[100];
  char tabtype[5];
  int untabid,tabd,primindid,nrows;
  short segid,ncols;

  EXEC SQL
    DECLARE CURS2 CURSOR FOR
    (
      SELECT TABNAME, t.UNTABID, INXNAME, COLNAME
      --,COLNAME,t.UNTABID --OWNER,UNTABID
      FROM DEFINITION_SCHEMA.SYSCOLUMNS c, DEFINITION_SCHEMA.SYSINDEXES i,
           DEFINITION_SCHEMA.SYSTABLES  t
      WHERE
      t.UNTABID = c.UNTABID
      AND
      t.UNTABID = i.UNTABID
      AND
      i.COLNO1 IN (SELECT COLNO
                   FROM DEFINITION_SCHEMA.SYSCOLUMNS cb,
                        DEFINITION_SCHEMA.SYSTABLES tb
                   WHERE
                        DEFINITION_SCHEMA.SYSTABLES.UNTABID = t.UNTABID
                    AND t.UNTABID > 2
                    AND cb.UNTABID = tb.UNTABID
                    AND COLNAME = c.COLNAME
                   )
      AND
      t.UNTABID = ANY ( 
                       SELECT UNTABID
                       FROM DEFINITION_SCHEMA.SYSCOLUMNS
                       --          WHERE UNTABID < 5
                       WHERE COLNAME = 'NCOLS'
                       --'COLNOTO1' --'UNTABID' --'COLNOTO1' --'ABCD'
                       )
    )
    ;

  EXEC SQL
    DECLARE CURS1 CURSOR FOR
    (
      SELECT TABNAME, TA.UNTABID, INXNAME, COLNAME
      --,COLNAME,TA.UNTABID --OWNER,UNTABID
      FROM
          DEFINITION_SCHEMA.SYSCOLUMNS A,
          DEFINITION_SCHEMA.SYSINDEXES i,
          DEFINITION_SCHEMA.SYSTABLES TA
      WHERE
          TA.UNTABID = A.UNTABID
        AND
          TA.UNTABID = i.UNTABID
        AND
          i.COLNO1 IN
           (SELECT COLNO
            FROM DEFINITION_SCHEMA.SYSCOLUMNS B, DEFINITION_SCHEMA.SYSTABLES TB
            WHERE
                  TB.UNTABID = TA.UNTABID
              AND TA.UNTABID > 2
              AND B.UNTABID = TB.UNTABID
              AND COLNAME = A.COLNAME
           )
        AND
          TA.UNTABID = ANY
           (SELECT UNTABID
            FROM DEFINITION_SCHEMA.SYSCOLUMNS
            WHERE
              -- UNTABID < 5
              COLNAME = 'NCOLS' --'COLNOTO1' --'UNTABID' --'COLNOTO1' --'ABCD'
           )
    )
  ;

  $ WHENEVER SQLERROR GOTO errexit;
  $ WHENEVER NOT FOUND GOTO exit;
  $ open CURS2;
  while(1)
    {
      $ fetch CURS2 into :tabname,:untabid, :inxname, :colname;

      PRINT_S (tabname, "tabname");
      PRINT_D (untabid, "untabid");
      PRINT_S (inxname, "inxname");
      PRINT_S (colname, "colname");
      PRINT_END;
    }
exit:
  fprintf(stderr,"End of Table \n");
  $ close CURS2;
  fprintf(stderr,"%s: success\n",__FILE__);
  $ commit work;
  return 0;
  
errexit:
  fprintf(stderr,"%s: error (%d) : %s\n",
          __FILE__,gsqlca.sqlcode,gsqlca.errmsg);
  return 1;
}


