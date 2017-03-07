/*
 *  testd1.ec - stripped DB monitor 
 *            
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming, Russia
 *  This file is written by Michael Kimelman.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Contact:  gss@ispras.ru
 *
 */

/* $Id: testd1.ec,v 1.3 1998/09/29 21:27:13 kimelman Exp $ */

#include <stdlib.h>
#include <string.h>
#include "sql.h"
#include "dyn_funcs.h"
#include <assert.h>
  

static char *statement_scanner_buffer=NULL;
int ARGC;
char **ARGV;

int
SCANNER_NAME(void)
{
  return 1;
}

int
print_width(dyn_sql_type_code_t t,int len)
{
  switch(t)
    {
    case SQL__Long  : 
    case SQL__Real  : return 10;
    case SQL__Short : return  5;
    case SQL__Double: return 15;
    default:
    }
  return len; 
}

int
SQL_connect(char *host,char *user,char *passwd)
{
  return 0;
}

void
error_rprt(char *st,int rc, char *stmt)
{
  fprintf(stderr,"\n#### Error occured in '%s'\n%s\nat \"%s\"\n",
          stmt,gsqlca.errmsg,st);
}

int
main(int argc, char *argv[])
{
  static char buffer[4096*4];
  int stmt = 0;
  int rc;
  int i,j;
  static char *sps =
    "                                                                       "
    "                                                                       "
    "                                                                       "
    "                                                                       "
    "                                                                       "
    ;

  statement_scanner_buffer = buffer;
  
#define CHECK(st) if((rc=SQL__##st)!=0){ error_rprt(#st,rc,statement_scanner_buffer); goto error; }
  sprintf(buffer,"select * from %s.%s",
          (argc>2?argv[2]:"DEFINITION_SCHEMA"),
          (argc>1?argv[1]:"SYSTABLES"));
  while(stmt==0)
    {
      char *b;
      int   l = strlen(statement_scanner_buffer);
      int  *clms = NULL;
      
      SQL_DESCR in,out;
      
      if (statement_scanner_buffer == NULL)
        continue;
      if (strncmp (statement_scanner_buffer,"select",strlen("select")) &&
          strncmp(statement_scanner_buffer,"SELECT",strlen("select")))
        {
          /* if not select statement */
          CHECK(execute_immediate(statement_scanner_buffer));
          stmt++;
          continue;
        }

      /* put select in parenthesys */
      b = malloc (l+3);
      b[0] = '(';
      strcpy(b+1,statement_scanner_buffer);
      strcat(b+1+l,")");
      statement_scanner_buffer = b;
          
      b = malloc(20);
      sprintf(b,"%dth Stmt",stmt++);

      CHECK(prepare(b,statement_scanner_buffer));
      CHECK(allocate_cursor(b,"CURSOR"));
      in  = SQL__allocate_descr("IN",0);
      out = SQL__allocate_descr("OUT",0);

      CHECK(describe(b,1,in));
      CHECK(describe(b,0,out));

      clms = (int*)malloc(out->count*sizeof(int));
      
      /* open cursor and write headers */
      j=1;
      fprintf(stdout,"\n|");
      for( i=0; i < out->count; i++)
        {
          int jj, k = 0;
          sql_descr_element_t *e = &(out->values[i]);
          if (e->unnamed)
            jj=fprintf(stdout," col_%03d ",i);
          else
            jj=fprintf(stdout," %s ",e->name);
          k = print_width(e->type,e->length) - jj;
          jj += fprintf(stdout,"%s|",sps+strlen(sps)- (k>0?(k>=20?20:k):0) );
          clms [i] = jj - 1;
          j += jj;
        }

      /* -------------------------------------------------*/
      fprintf(stdout,"\n");
      for(i=j;i--;)
        fprintf(stdout,"-");

      if(in->count)
        {
          CHECK(open_cursor("CURSOR",in));
        }
      else
        {
          CHECK(open_cursor("CURSOR",NULL));
        }
        
      while(1)
        {
          char oline[1024];
          for( i=0; i < out->count; i++)
            out->values[i].indicator = 0;

          CHECK(fetch("CURSOR",out));
          
          if(SQLCODE!=0)
            break;
          
          sprintf(oline,"\n|");
          for (i=0; i < out->count; i++)
            {
              int k ;
              int kk = 0 ;
              char buf[4096] ;
              sql_descr_element_t *e = &(out->values[i]) ;
              
              if (e->indicator < 0)
                {
                  if (!e->nullable)
                    if (e->name)
                      fprintf(stderr,"\nindicator < 0 for 'not null' field '%s'",e->name);
                    else
                      fprintf(stderr,"\nindicator < 0 for 'not null' field '%d'",i);
                  sprintf(buf,"null");
                }
              else switch(e->type)
                {
                case SQL__Char:
                case SQL__CharVar:
                  sprintf(buf,"%s",(char*)e->data);
                  break;
                case SQL__Double: /* DOUBLE */
                case SQL__Float: /* FLOAT  */
                  sprintf(buf,"%g",*((double*)(e->data)));
                  break;
                case SQL__Real: /* REAL   */
                  sprintf(buf,"%g",*((float*)(e->data)));
                  break;
                case SQL__Int: /* int */
                  sprintf(buf,"%X",*((int*)(e->data)));
                  break;
                case SQL__Short: /* small */
                  sprintf(buf,"%d",(int)*((short*)(e->data)));
                  break;
                default:
                  sprintf(buf," **%d** ",e->type);
                }
              assert(kk==0);
              k = (clms[i] - strlen(buf))/2; 
              if (k > 4*clms[i])
                {
                  strcat(oline,"!!");
                  k = 0;
                }
              k = sprintf(oline+strlen(oline),"%s%s",
                          sps + strlen(sps) - (k>0?(k>20?20:k):0),buf);
              k = clms[i] - k;
              sprintf(oline+strlen(oline),"%s|",
                      sps + strlen(sps) - (k>0?(k>20?20:k):0));
            }
          fprintf(stdout,"%s",oline);
        }
      
      /* -------------------------------------------------*/
      fprintf(stdout,"\n");
      for(i=j;i--;)
        fprintf(stdout,"-");
      fprintf(stdout,"\n\n");
      CHECK(close_cursor("CURSOR"));
      CHECK(deallocate_prepare(b));
    error:
      SQL__deallocate_descr(&in);
      SQL__deallocate_descr(&out);
      xfree(clms);
      xfree(b);
      b = NULL;
    }
  _SQL_commit();
  return 0;
}
