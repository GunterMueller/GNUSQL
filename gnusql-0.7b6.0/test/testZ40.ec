/*
 * $Id: testZ40.ec,v 1.1 1998/01/19 06:14:48 kml Exp $
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
  char t1[100];
  long cnt;
  int k1,j=0;

  EXEC SQL
  DECLARE CURS1 CURSOR FOR
   ( SELECT  k1+:j, t1
    FROM ZTBL0
--  WHERE k1 == 10
    )
    ;
  
 EXEC SQL
  DECLARE CURS2 CURSOR FOR
 (   SELECT  k1+:j, t1
    FROM ZTBL0
--  WHERE k1 == 10
  )  
   ;
 
  $ WHENEVER SQLERROR GOTO errexit;
  $ WHENEVER NOT FOUND GOTO exit;

  fprintf(stderr,"Deleting each 5th row of ZTBL0\n");
  $ open CURS1;
  while(1)
    {
      $ fetch CURS1 into :k1, :t1;
      printf("k1='%d',t1='%s'\n",k1,t1);
      if ( k1 == 5)
        {
          
        EXEC SQL
           DELETE FROM ZTBL0
          WHERE CURRENT OF CURS1 ;
        
        }
      t1[0]=0;
    }


 exit:
  fprintf(stderr,"End of Table ZTBL0\n");

  $ close CURS1;

  $ WHENEVER NOT FOUND GOTO exit1;

  fprintf(stderr,"Second pass - show updated table\n");
  $ OPEN CURS2;
  
  while(1) 
    {
      $ fetch CURS2 into :k1, :t1;
      printf("k1='%d',t1='%s'\n",k1,t1);
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
