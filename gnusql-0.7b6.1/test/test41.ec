/*
 * $Id: test41.ec,v 1.2 1998/09/29 21:27:04 kimelman Exp $
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
  int untabid,tabd,primindid,nrows, nrows_ind, primindid_ind, nnulcolnum, tabd_ind, segid_ind;
  short segid,ncols;
  int i = 8;

  EXEC SQL
  DECLARE CURS1 CURSOR FOR
    (
      SELECT TABNAME, OWNER, UNTABID
           ,SEGID,PRIMINDID,
           TABTYPE,TABD,CREDATE,CRETIME,NCOLS,NROWS, NNULCOLNUM
      FROM DEFINITION_SCHEMA.SYSTABLES
      WHERE 
        UNTABID > (SELECT MAX (UNTABID) / 4 --AVG (UNTABID)
                  FROM DEFINITION_SCHEMA.SYSCOLUMNS)
      AND
        (NCOLS - 2) IN (SELECT UNTABID
                      FROM DEFINITION_SCHEMA.SYSTABLES)
    )
;

  $ WHENEVER SQLERROR GOTO errexit;
  $ WHENEVER NOT FOUND GOTO exit;
  $ open CURS1;
  while(1)
    {
      $ fetch CURS1 into :tabname, :owner, :untabid , :segid :segid_ind,
                         :primindid :primindid_ind, :tabtype, :tabd  :tabd_ind,
                         :credate, :cretime, :ncols,
                         :nrows :nrows_ind, :nnulcolnum;

      PRINT_S (tabname, "tabname");
      PRINT_S (owner, "owner");
      PRINT_D (untabid, "untabid");
      PRINT_ID (segid, "segid");
      PRINT_ID (primindid, "primindid");
      PRINT_ID (tabd, "tabd");
      PRINT_S (tabtype, "tabtype");
      PRINT_S (credate, "credate");
      PRINT_S (cretime, "cretime");
      PRINT_D (ncols, "ncols");
      PRINT_ID (nrows, "nrows");
      PRINT_D (nnulcolnum, "nnulcolnum");

      PRINT_END;
    }
 exit:
  fprintf(stderr,"End of Table SYSTABLES\n");
  $ close CURS1;
  fprintf(stderr,"%s: success\n",__FILE__);
  $ commit work;
  return 0;
  
errexit:
  fprintf(stderr,"%s: error (%d) : %s\n",
          __FILE__,gsqlca.sqlcode,gsqlca.errmsg);
  return 1;
}









