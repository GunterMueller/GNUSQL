/*
 * $Id: test30.ec,v 1.2 1998/06/01 15:13:07 kimelman Exp $
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
  char tabname[100],owner[100],credate[100],cretime[100];
  char tabtype[5];
  int i, untabid,tabd,primindid,nrows, nnulcolnum, ind, ind1;
  short segid,ncols;

  EXEC SQL
  DECLARE CURS1 CURSOR FOR
    (
      SELECT  TABNAME,OWNER,UNTABID,TABTYPE,
            CREDATE,CRETIME,NCOLS,NROWS, NNULCOLNUM
      FROM DEFINITION_SCHEMA.SYSTABLES
--      WHERE   UNTABID < 100
--        AND UNTABID >= 2
--        and TABNAME    is not null
--        and OWNER      is not null
--        and UNTABID    is not null
--        and SEGID      is not null
--        and TABD       is not null
--        and PRIMINDID  is not null
--        and TABTYPE    is not null
--        and CREDATE    is not null
--        and CRETIME    is not null
--        and NCOLS      is not null
--        and NROWS      is not null
--        and NNULCOLNUM is not null
    ) ;
  
  $ WHENEVER SQLERROR GOTO errexit;
  $ WHENEVER NOT FOUND GOTO exit;

  $ open CURS1;
  
  while(1)
    {
      $ fetch CURS1 into :tabname,:owner,:untabid,:tabtype,
               :credate,:cretime,:ncols,:nrows :ind ,:nnulcolnum :ind1;
      
      PRINT_S (tabname, "tabname");
      PRINT_S (owner, "owner");
      PRINT_D (untabid, "untabid");
      PRINT_S (tabtype, "tabtype");
      PRINT_S (credate, "credate");
      PRINT_S (cretime, "cretime");
      PRINT_D (ncols, "ncols");
      PRINT_D (nrows, "nrows");
      PRINT_D (nnulcolnum, "nnulcolnum");
      PRINT_END;
    }
exit:
  fprintf(stderr,"End of Table SYSTABLES\n");
  $ close CURS1;
  
  fprintf(stderr,"%s: success\n",__FILE__);
  fprintf(stderr,"%s: success\n",__FILE__);
  $ commit work;
  return 0;
  
errexit:
  fprintf(stderr,"%s: error (%d) : %s\n",
          __FILE__,gsqlca.sqlcode,gsqlca.errmsg);
  return 1;
}
