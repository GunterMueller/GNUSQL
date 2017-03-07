/*
 * $Id: testd2.ec,v 1.2 1998/09/29 21:27:14 kimelman Exp $
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

extern char *err_str[];

#define G(p)					\
{						\
  fprintf(stderr,"%s running...",#p);		\
  int_res = p;					\
  if(int_res<0)					\
    goto errexit;				\
}

#define G2(s)					\
{						\
  fprintf(stderr,"execute '%s' \n",s);		\
  G(SQL__execute_immediate(s));			\
}

int
main(void)
{
  char s[100],*str[] =
  { "\n"
    "CREATE TABLE TTBL1 (\n"
    "   k  int not null primary key,\n"
    "   t0 char(10),\n"
    "   k2 smallint not null unique,\n"
    "   t1 char(20) default user,\n"
    "   unique (t0,t1)\n"
    ")\n"
    ,
    "INSERT INTO TTBL1(k,t0,k2)"
    "VALUES ( 10, 'aaa', 2) "
    ,
    "DROP TABLE TTBL1"
  };
  int int_res, i_ind;

  for ( i_ind = 0; i_ind < sizeof(str)/sizeof(char*) ; i_ind++)
    {
      G2(str[i_ind]);
      _SQL_commit();
    }
  
  return 0;
errexit:

  if (int_res < 0)
    printf ("SQL_ERROR: '%s'\n", gsqlca.errmsg);
  else
    {
      int_res = SQL__execute_immediate ("COMMIT WORK");
      if (int_res < 0)
        printf ("SQL_ERROR: '%s'\n", gsqlca.errmsg);
    }
  return 0;
}
