/*
 * $Id: test70.ec,v 1.2 1998/09/29 21:27:05 kimelman Exp $
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
  char str[100];
  short i=7,k=2;
  char typ;
  int typ1;
  float j=0;
  long cnt;

  EXEC SQL
  DECLARE CURS1 CURSOR FOR
    (
     SELECT AVG (UNTABID) -- COUNT (*) --AVG (NCOLS)
     --COUNT (*) --, MIN (COLNAME) -- ,MAX(COLNAME) ,
                         --UNTABID,COLNO, COLTYPE, COLTYPE1
      FROM DEFINITION_SCHEMA.SYSTABLES --SYSCOLUMNS
--      WHERE 
--        COLNAME>='S' --'SYSCOLUMNS'
--       AND
--         UNTABID>0 --:i
--       AND
--         (COLNAME < (SELECT MAX(COLNAME)
--                     FROM DEFINITION_SCHEMA.SYSTABLES ))
--     GROUP BY UNTABID, COLNAME  --,UNTABID,
    )
  ;
  
  $ WHENEVER SQLERROR GOTO errexit;
  $ WHENEVER NOT FOUND GOTO exit;
  $ open CURS1;
  while(1)
    {
      *str=0;
      $ fetch CURS1 into :j; /*, :str; , :k, :typ, :typ1; */
      fprintf(stderr,"AVG (UNTABID) = %g \n",j);
      /*    fprintf(stderr,"UNTABID=%d, MIN (COLNAME)='%s'\n",j,str);*/
      /*
        fprintf(stderr,"UNTABID=%d, COLNAME='%s' COLNO=%d "
        "COLTYPE='%d' COTYPE1=%d\n",j,str,k,typ,typ1);
        */
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




