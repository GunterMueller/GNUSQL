/*
 * $Id: test33.ec,v 1.2 1998/09/29 21:27:02 kimelman Exp $
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

int TABFROM = -1, TABTO    = -1,
    INDTO   = -1, CHCONID  = -1,
    CONSIZE = -1, FRAGSIZE = -1;

short NCOLS    = -1, COLNOFR1 = -1, COLNOFR2 = -1, COLNOFR3 = -1,
  COLNOFR4 = -1, COLNOFR5 = -1, COLNOFR6 = -1, COLNOFR7 = -1,
  COLNOFR8 = -1, COLNOTO1 = -1, COLNOTO2 = -1, COLNOTO3 = -1,
  COLNOTO4 = -1, COLNOTO5 = -1, COLNOTO6 = -1,
  COLNOTO7 = -1, COLNOTO8 = -1, FRAGNO   = -1;

int ind;

int
main(void)
{
  
  EXEC SQL
    DECLARE CURS1 CURSOR FOR
    (
     SELECT TABFROM, TABTO, INDTO, NCOLS, COLNOFR1, COLNOFR2, COLNOFR3,
       COLNOFR4, COLNOFR5, COLNOFR6, COLNOFR7, COLNOFR8, COLNOTO1,
       COLNOTO2, COLNOTO3, COLNOTO4, COLNOTO5, COLNOTO6,
       COLNOTO7, COLNOTO8
     FROM DEFINITION_SCHEMA.SYSREFCONSTR
    )
    ;
  
  EXEC SQL
    DECLARE CURS2 CURSOR FOR
    (
      SELECT UNTABID, CHCONID,  CONSIZE, NCOLS, COLNO1, COLNO2, COLNO3,
	COLNO4, COLNO5, COLNO6, COLNO7, COLNO8
      FROM DEFINITION_SCHEMA.SYSCHCONSTR
    )
    ;

  EXEC SQL
    DECLARE CURS3 CURSOR FOR
    (
      SELECT CHCONID,  FRAGNO, FRAGSIZE
      FROM DEFINITION_SCHEMA.SYSCHCONSTRTWO
    )
    ;

  $ WHENEVER SQLERROR GOTO errexit;
  $ WHENEVER NOT FOUND GOTO exit;
  $ open CURS1;

  printf (" SYSREFCONSTR :\n");
  while(1)
    {
      $ fetch CURS1 into
        :TABFROM,  :TABTO,    :INDTO,    :NCOLS,    :COLNOFR1 :ind, :COLNOFR2 :ind,
	:COLNOFR3 :ind, :COLNOFR4 :ind, :COLNOFR5 :ind, :COLNOFR6 :ind, :COLNOFR7 :ind, :COLNOFR8 :ind,
	:COLNOTO1 :ind, :COLNOTO2 :ind, :COLNOTO3 :ind, :COLNOTO4 :ind, :COLNOTO5 :ind, :COLNOTO6 :ind,
	:COLNOTO7 :ind, :COLNOTO8 :ind;

      PRINT_D (TABFROM , "TABFROM" );
      PRINT_D (TABTO   , "TABTO"   );
      PRINT_D (INDTO   , "INDTO"   );
      PRINT_D (NCOLS   , "NCOLS"   );
      PRINT_D (COLNOFR1, "COLNOFR1");
      PRINT_D (COLNOFR2, "COLNOFR2");
      PRINT_D (COLNOFR3, "COLNOFR3");
      PRINT_D (COLNOFR4, "COLNOFR4");
      PRINT_D (COLNOFR5, "COLNOFR5");
      PRINT_D (COLNOFR6, "COLNOFR6");
      PRINT_D (COLNOFR7, "COLNOFR7");
      PRINT_D (COLNOFR8, "COLNOFR8");
      PRINT_D (COLNOTO1, "COLNOTO1");
      PRINT_D (COLNOTO2, "COLNOTO2");
      PRINT_D (COLNOTO3, "COLNOTO3");
      PRINT_D (COLNOTO4, "COLNOTO4");
      PRINT_D (COLNOTO5, "COLNOTO5");
      PRINT_D (COLNOTO6, "COLNOTO6");
      PRINT_D (COLNOTO7, "COLNOTO7");
      PRINT_D (COLNOTO8, "COLNOTO8");

      PRINT_END;
    }
 exit:
  $ close CURS1;

  $ WHENEVER NOT FOUND GOTO exit1;
  $ open CURS2;

  printf ("\n\n SYSCHCONSTR :\n\n");
  while(1)
    {
      $ fetch CURS2 into
        :TABFROM,  :CHCONID,  :CONSIZE,  :NCOLS,    :COLNOFR1 :ind, :COLNOFR2 :ind,
	:COLNOFR3 :ind, :COLNOFR4 :ind, :COLNOFR5 :ind, :COLNOFR6 :ind, :COLNOFR7 :ind, :COLNOFR8 :ind;

      PRINT_D (TABFROM , "UNTABID");
      PRINT_D (CHCONID , "CHCONID");
      PRINT_D (CONSIZE , "CONSIZE");
      PRINT_D (NCOLS   , "NCOLS"  );
      PRINT_D (COLNOFR1, "COLNO1" );
      PRINT_D (COLNOFR2, "COLNO2" );
      PRINT_D (COLNOFR3, "COLNO3" );
      PRINT_D (COLNOFR4, "COLNO4" );
      PRINT_D (COLNOFR5, "COLNO5" );
      PRINT_D (COLNOFR6, "COLNO6" );
      PRINT_D (COLNOFR7, "COLNO7" );
      PRINT_D (COLNOFR8, "COLNO8" );

      PRINT_END;
    }
 exit1:
  $ close CURS2;

  $ WHENEVER NOT FOUND GOTO exit2;
  $ open CURS3;

  printf ("\n\n SYSCHCONSTRTWO :\n\n");
  while(1)
    {
      $ fetch CURS3 into :CHCONID, :FRAGNO, :CONSIZE;

      PRINT_D (CHCONID, "CHCONID");
      PRINT_D (FRAGNO, "FRAGNO");
      PRINT_D (CONSIZE, "FRAGSIZE");

      PRINT_END;
    }
 exit2:
  $ close CURS3;

  fprintf(stderr,"%s: success\n",__FILE__);
  $ commit work;
  return 0;
  
errexit:
  fprintf(stderr,"%s: error (%d) : %s\n",
          __FILE__,gsqlca.sqlcode,gsqlca.errmsg);
  return 1;
}







