/*
 * $Id: testd4.ec,v 1.3 1998/09/29 21:27:14 kimelman Exp $
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
#include "dyn_funcs.h"

#define G(p)					\
{						\
  fprintf(stderr,"%s running...",#p);		\
  int_res = p;					\
  if(int_res<0)					\
    goto errexit;				\
}

int
main(void)
{
  char s[100],*op,*op1,*op2,*str1 =
    "CREATE TABLE TTBL1(k int not null primary key, t0 char(10), k2 smallint not null unique, t1 char(20) default user, unique (t0,t1))" ,
    *str2= "INSERT INTO TTBL1(k,t0,k2)"
    "VALUES ( 10, 'aaa', 2) "
    ,
    *str3= "DROP TABLE TTBL1"
    ;
  int int_res, i_ind, s_ind;
  {
    SQL_DESCR descr1,descr2;
    
    SQL__prepare(op, str1);
    descr1=SQL__allocate_descr("IN",0);
    descr2=SQL__allocate_descr("OUT",0);    
    SQL__describe(op, 1, descr1);    
    SQL__describe(op, 0, descr2);    
    SQL__execute(op, descr1, descr2);
  }
  
  /* PREPARE op FROM  str1
     EXECUTE op USING SQL DESCRIPTOR "IN"
     ;*/
  _SQL_commit();
  
  SQL__prepare(op1,str2);

  {
    SQL_DESCR tmp_in=SQL__allocate_descr("IN" ,0);
    SQL_DESCR tmp_out=SQL__allocate_descr("OUT" ,0);
    SQL_DESCR tmp;int flag=0, i=0;
    SQL__describe(op1, 1, tmp_in);
    SQL__describe(op1, 0, tmp_out);
    tmp=tmp_in;
    tmp_in=SQL__get_descr("INP");
    SQL__execute(op1, tmp_in, tmp_out);
    flag=1;
    tmp=tmp_out;
  }
  _SQL_commit();
  
  SQL__prepare(op2,str3);
  {
    SQL_DESCR tmp_in=SQL__allocate_descr("IN" ,0);
    SQL_DESCR tmp_out=SQL__allocate_descr("OUT" ,0);
    SQL_DESCR tmp;int flag=0, i=0;
    SQL__describe(op2, 1, tmp_in);
    SQL__describe(op2, 0, tmp_out);
    tmp=tmp_in;
    tmp_in=SQL__get_descr("INPUT");
    SQL__execute(op2, tmp_in, tmp_out);
    flag=1;
    tmp=tmp_out;
  }
  _SQL_commit();
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
