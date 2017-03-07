/*
 * $Id: testTV20.ec,v 1.2 1998/09/29 21:27:11 kimelman Exp $
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
  char s[100];
  int i, i_ind, s_ind;

  EXEC SQL
  DECLARE CURS1 CURSOR FOR
    (
      SELECT i, s
      FROM VIEW1
--      WHERE 
--        i < 10
    )
;

  $ WHENEVER SQLERROR GOTO errexit;
  $ WHENEVER NOT FOUND GOTO exit;
  $ open CURS1;

  while(1)
    {
      $ fetch CURS1 into :i :i_ind, :s :s_ind;

      PRINT_ID (i, "untabid");
      PRINT_IS (s, "colname");
      PRINT_END;
    }
 exit:
  fprintf(stderr,"The End\n");
  $ close CURS1;


 EXEC SQL
   DELETE FROM  VIEW1
   WHERE i <= 9;  

  $ WHENEVER NOT FOUND GOTO exit1;
  $ open CURS1;

  while(1)
    {
      $ fetch CURS1 into :i :i_ind, :s :s_ind;

      PRINT_ID (i, "untabid");
      PRINT_IS (s, "colname");
      PRINT_END;
    }

 exit1:
  fprintf(stderr,"The End\n");
  $ close CURS1;
  fprintf(stderr,"%s: success\n",__FILE__);
  $ commit work;
  return 0;
  
errexit:
  fprintf(stderr,"%s: error (%d) : %s\n",
          __FILE__,gsqlca.sqlcode,gsqlca.errmsg);
  return 1;
}
