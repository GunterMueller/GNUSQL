/*
 * $Id: testM2.ec,v 1.2 1998/09/29 21:27:07 kimelman Exp $
 *
 * This file is a part of GNU SQL Server
 *
 * Copyright (c) 1996-1998, Free Software Foundation, Inc
 * Developed at Institute of System Programming of Russian Academy of Science
 * This file is written by Michael Kimelman
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
#include <signal.h>
#include <sys/wait.h>

$ whenever sqlerror goto errexit;

int
show_1(void)
{
  char t1[40];
  int k1;
  
  $ declare show1 cursor for
    ( select k1,t1 
      from tbl1 )
    ;
  $ open show1;
  fputs("---------------------c1---------------------------\n",stdout);
  while(1) 
    {
      $ fetch show1 into :k1, :t1;
      if(SQLCODE==100) break;
      printf("\n|%7d|%40s| ",k1,t1);
    }
  fputs("\n---------------------c1---------------------------\n",stdout);
  $ close show1;
  return 0;
errexit:
  return SQLCODE;
}

static volatile int lock = 0;

void
catch(int sig)
{
  
  if (sig==SIGUSR1)
    lock = 0;
  return;
}

void 
wait_usr(void)   
{ 
  lock = 1; 
  signal(SIGUSR1,catch); 
  while(lock) 
    sleep(1);
}

#define EXIT         { if (cld) { kill(cld,SIGKILL); cld = 0; } return 1;     }

int
main(int argc,char **argv)
{
  int   pass = 0;
  int   i,j,k;
  pid_t cld = 0;
  
  printf("pass %d...\n",pass++);
  
  printf("let's see the junk\n");
  if (show_1())
    EXIT;       

  printf("delete it now ...\n");
  $ delete from tbl1 ;

  printf("and see again...\n");
  if (show_1())
    EXIT;       
  $ commit work;

  cld = fork();
  if (cld==0)
    {
      printf("son: before insert ...\n");
      for(i = 0; i < 10 ; i++ )
        {
          int v;
          char s[50];

          v=i+1;
          sprintf(s,"k=%10d",v);
      
          $ insert into tbl1 (k1,t1)
            values (:v,:s );
          if (i == 5)
            {
              printf("Son: let's see can the parent read something ...\n");
              kill(getppid(),SIGUSR1);
              sleep(100);
/*              wait_usr(); */
              printf("Son: let's insert 5 values more ..\n");
            }
        }
      printf("son: after insert ...\n");
      $ commit work;
      printf("Son: and how about seeing now - after my death!!\n");
      return 0;           /* rollback on exit */
    }
  
  printf("Hi: I'm the parent and going to wait ...\n");
  wait_usr();
  printf("Hi: I'm the parent and going to show something ...\n");
  if (show_1())
    EXIT;
/*  kill(cld,SIGUSR1); */
  wait(NULL);
  cld = 0;
  
  if (show_1())
    EXIT;         /* should be automatically disconnect with       */

  $ commit work;
  
  return 0;
errexit:
  EXIT;
}
