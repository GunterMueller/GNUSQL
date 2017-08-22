/*
 * $Id: test10.ec,v 1.1 1998/01/19 06:14:41 kml Exp $
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
  int untabid, i = 8;
  char grantee[100],grantor[100],colauth[100] ;
  short colno;


 EXEC SQL
  DECLARE CURS1 CURSOR FOR
  (  (
      SELECT UNTABID , COLNO, GRANTEE, GRANTOR, COLAUTH
      FROM DEFINITION_SCHEMA.SYSCOLAUTH)
    UNION --ALL
    (  SELECT UNTABID , COLNO, GRANTEE, GRANTOR, COLAUTH
      FROM DEFINITION_SCHEMA.SYSCOLAUTH
 )  )
 UNION ALL
 (   (
      SELECT UNTABID , COLNO, GRANTEE, GRANTOR, COLAUTH
      FROM DEFINITION_SCHEMA.SYSCOLAUTH )
    UNION
    ( SELECT UNTABID , COLNO, GRANTEE, GRANTOR, COLAUTH
      FROM DEFINITION_SCHEMA.SYSCOLAUTH
      GROUP BY UNTABID , COLNO, GRANTEE, GRANTOR, COLAUTH
    )
 )
;

  $ WHENEVER SQLERROR GOTO errexit;

  $ WHENEVER NOT FOUND GOTO exit1;
  $ open CURS1;
  while(1)
    {
      $ fetch CURS1 into :untabid ,:colno,:grantee,:grantor,:colauth ;
#if 0
     fprintf(stderr,"untabid='%d'\n", untabid);
#else
      fprintf(stderr,"untabid='%d' colno=%d grantee='%s' grantor ='%s' colauth='%s'\n",
              untabid,colno,grantee,grantor,colauth);
#endif
    }
 exit1:
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








