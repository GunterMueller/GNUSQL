/*
 * $Id: test22.ec,v 1.2 1998/09/29 21:26:59 kimelman Exp $
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
  int untabid, i = 8;
  char grantee[100],grantor[100],tabauth[100]; 
  short colno;

  EXEC SQL
  DECLARE CURS1 CURSOR FOR
    (
      SELECT UNTABID, GRANTEE, GRANTOR, TABAUTH
      FROM DEFINITION_SCHEMA.SYSTABAUTH
      WHERE

       TABAUTH NOT LIKE  'S' ESCAPE 'S'
--     NOT
--     (  TABAUTH = 'Su'
--      OR
--     UNTABID < 9 ) --:i
    )
;

  EXEC SQL
  DECLARE CURS2 CURSOR FOR
    (
      SELECT UNTABID, COLNO, GRANTEE, GRANTOR, COLAUTH
      FROM DEFINITION_SCHEMA.SYSCOLAUTH
--     WHERE 
--       GRANTEE='kml'
--     AND
--       COLNO > 9--:i
    )
;

  $ WHENEVER SQLERROR GOTO errexit;
  $ WHENEVER NOT FOUND GOTO exit;
  $ open CURS1;
  while(1)
    {
      $ fetch CURS1 into :untabid,:grantee,:grantor,:tabauth ;
      fprintf(stderr,"untabid='%d'grantee='%s' grantor ='%s' tabauth='%s'\n",
              untabid,grantee,grantor,tabauth);
    }
 exit:
  fprintf(stderr,"End of Table 1\n");
  $ close CURS1;

  $ WHENEVER NOT FOUND GOTO exit1;
  $ open CURS2;
  while(1)
    {
      $ fetch CURS2 into :untabid,:colno,:grantee,:grantor,:tabauth ;
      fprintf(stderr,"untabid='%d' colno=%d grantee='%s' grantor ='%s' tabauth='%s'\n",
              untabid,colno,grantee,grantor,tabauth);
    }
 exit1:
  fprintf(stderr,"End of Table 2\n");
  $ close CURS2;
  
  fprintf(stderr,"%s: success\n",__FILE__);
  $ commit work;
  return 0;
  
errexit:
  fprintf(stderr,"%s: error (%d) : %s\n",
          __FILE__,gsqlca.sqlcode,gsqlca.errmsg);
  return 1;
}










