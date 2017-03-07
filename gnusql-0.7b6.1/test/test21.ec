/*
 * $Id: test21.ec,v 1.2 1998/09/29 21:26:58 kimelman Exp $
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

void
row(char *tab,char *own,int unt)
{
      PRINT_S (tab, "tabname");
      PRINT_S (own, "owner");
      PRINT_D (unt, "untabid");
      PRINT_END;
}

int
main(void)
{
  char tabname[100],owner[100],credate[100],cretime[100];
  char tabtype[5];
  int untabid,tabd,primindid,nrows;
  short segid,ncols;
  int i = 5 , j = i+1;

  $ WHENEVER SQLERROR GOTO errexit;

  /*---------------------------------------------*/
  $ declare c1 cursor for (
    select
       min (TABNAME), max (OWNER), avg (UNTABID)
--          avg(UNTABID),max(TABNAME), max(OWNER)
      from DEFINITION_SCHEMA.SYSTABLES
--      where UNTABID in ( :i, :j )
--      group by OWNER
    );
  
  printf("----c1------\n");
  $ whenever not found goto ec1;
  $ open c1 ;
  while(1)
    {
      float avg;
      $ fetch c1 into :tabname,:owner,:avg ;
      untabid = avg;
      row(tabname,owner,untabid);
    }
 ec1:
  $ close c1;
  printf("^^^^c1^^^^^^\n");
  /*---------------------------------------------*/

  fprintf(stderr,"%s: success\n",__FILE__);
  $ commit work;
  return 0;
  
errexit:
  fprintf(stderr,"%s: error (%d) : %s\n",
          __FILE__,gsqlca.sqlcode,gsqlca.errmsg);
  return 1;
}





