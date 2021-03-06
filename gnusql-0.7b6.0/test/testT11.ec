/*
 * $Id: testT11.ec,v 1.1 1998/01/19 06:14:47 kml Exp $
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
  char tailb[100];
  long cnt;
  float taila;
  int k1;
  short k2;
  $ WHENEVER SQLERROR GOTO errexit;

 EXEC SQL
 INSERT INTO TBL1
 VALUES (60, 7, 5.5, 'D')
;

 EXEC SQL
 INSERT INTO TBL1 ( taila, tailb, k1, k2)
 VALUES (5.5, 'usr', 60, 5)
;

 EXEC SQL
 INSERT INTO TBL1 (k1)
 VALUES (500)
;

  EXEC SQL
  INSERT INTO TBL1 --( k1, k2, taila, tailb)
   SELECT UNTABID, UNTABID, 7 + COLNO, TABNAME 
    FROM DEFINITION_SCHEMA.SYSTABLES
     WHERE TABNAME > 'SYSD'
         AND
	   TABNAME > 'aa'
;

EXEC SQL
 INSERT INTO TBL1 
   SELECT k1+100, k2+2, taila + 1.9 ,tailb 
    FROM TBL1
;

 EXEC SQL
  DECLARE CURS1 CURSOR FOR
    (
      SELECT  k1, k2, taila, tailb
      FROM TBL1
    )
    
;

  $ WHENEVER NOT FOUND GOTO exit;
 
  $ open CURS1;
  while(1)
    {
      $ fetch CURS1 into :k1, :k2, :taila, :tailb ;
      printf("k1 = '%d', k2 = '%d', taila = '%f', tailb = '%s'\n", k1, k2, taila, tailb);
    }


 exit:
  fprintf(stderr,"End of Table TBL1\n");
  fprintf(stderr,"%s: success\n",__FILE__);
  $ commit work;
  return 0;
  
errexit:
  fprintf(stderr,"%s: error (%d) : %s\n",
          __FILE__,gsqlca.sqlcode,gsqlca.errmsg);
  return 1;
}
