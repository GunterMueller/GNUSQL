/*
 * $Id: test50.ec,v 1.1 1998/01/19 06:14:44 kml Exp $
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

int
main(void)
{
  char str[100];
  int int1;

  EXEC SQL
  DECLARE CURS1 CURSOR FOR
    (
      SELECT UNTABID, TABNAME  --, MAX (TABNAME) --, MIN (UNTABID)--COUNT ( DISTINCT TABNAME ) --,TABNAME  -- COUNT (*) !!
      FROM DEFINITION_SCHEMA.SYSTABLES
      WHERE
        UNTABID > 5 --< 9
      AND
        TABNAME >= 'SYSCOLUMNS'
    )
--    GROUP BY UNTABID
 ORDER BY UNTABID
; 


  $ WHENEVER SQLERROR GOTO errexit;
  $ WHENEVER NOT FOUND GOTO exit;
  $ open CURS1;
  while(1)
    {
      $ fetch CURS1 into :int1, :str ;
      fprintf(stderr,"min (UNTABID)='%d' TABNAME='%s'\n",int1, str);
    }
 exit:
  fprintf(stderr,"End of Table\n");
  $ close CURS1;
  fprintf(stderr,"%s: success\n",__FILE__);
  $ commit work;
  return 0;
  
errexit:
  fprintf(stderr,"%s: error (%d) : %s\n",
          __FILE__,gsqlca.sqlcode,gsqlca.errmsg);
  return 1;
}



