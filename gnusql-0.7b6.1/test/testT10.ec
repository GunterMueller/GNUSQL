/*
 * $Id: testT10.ec,v 1.2 1998/09/29 21:27:08 kimelman Exp $
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
  char str[100];
  long cnt;
  int j, j_ind, str_ind;

  $ WHENEVER SQLERROR GOTO errexit;



 EXEC SQL
 INSERT INTO TBL0 ( k1, t1)
 VALUES ( 140, '' )
;
/**/

 EXEC SQL
 INSERT INTO TBL0 ( k1, t1)
 VALUES ( 160, 'MSU' )
;

 EXEC SQL
 INSERT INTO TBL0 ( k1, t1)
 VALUES ( 141,NULL )
;

 EXEC SQL
   INSERT INTO TBL0  
   VALUES ( 42,USER )
   ;
 
 EXEC SQL
   INSERT INTO TBL0 (k1) 
   VALUES ( 43 )
   ;
 
 EXEC SQL
   INSERT INTO TBL0 ( k1, t1)
   SELECT UNTABID, TABNAME 
   FROM DEFINITION_SCHEMA.SYSTABLES
--     WHERE UNTABID < 8
--         AND
--	   TABNAME > 'aa'
;

/**/

 EXEC SQL
 INSERT INTO TBL0 (k1, t1)
   SELECT k1+100, 'NEW'
    FROM TBL0
;
*/
/**/


  EXEC SQL
 INSERT INTO TBL0 ( k1, t1)
   (SELECT UNTABID, TABNAME
    FROM DEFINITION_SCHEMA.SYSTABLES
--     WHERE UNTABID > 8
--       AND
--	 TABNAME > 'aa'
   )
    ;

 EXEC SQL
  DECLARE CURS1 CURSOR FOR
    (
      SELECT  k1, t1
      FROM TBL0
    )
    
;

  $ WHENEVER NOT FOUND GOTO exit;
 
  $ open CURS1;
  while(1)
    {
      $ fetch CURS1 into :j :j_ind, :str :str_ind;
      PRINT_ID (j, "k1");
      PRINT_IS (str, "t1");
      PRINT_END;
    }


 exit:
  fprintf(stderr,"End of Table TBL0\n");
  fprintf(stderr,"%s: success\n",__FILE__);
  $ commit work;
  return 0;
  
errexit:
  fprintf(stderr,"%s: error (%d) : %s\n",
          __FILE__,gsqlca.sqlcode,gsqlca.errmsg);
  return 1;
}
