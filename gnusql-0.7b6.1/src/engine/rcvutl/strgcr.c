/*  strgcr.c  - Storage Crash Recovery Utility
 *              Kernel of GNU SQL-server. Recovery utilities    
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by  Vera Ponomarenko
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
 *  Contacts:   gss@ispras.ru
 *
 */

/* $Id: strgcr.c,v 1.252 1998/09/30 02:39:04 kimelman Exp $ */

#include "setup_os.h"
#include "xmem.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <assert.h>
#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>

#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#include "destrn.h"
#include "strml.h"
#include "puprcv.h"

#include "fdclrcv.h"
#include "f1f2decl.h"
#include "admdef.h"
#include "adfstr.h"

/**************************************************************/



#define  adfsize sizeof(struct ADF)

i4_t fdcurlj;
i4_t fd_lj;
i4_t msqidscr;
pid_t pidlj, pidmj, pidbf;
key_t keyscr, keylj, keymj, keybf;
u2_t RJZm, RJZl, MPAGEm, MPAGEl;
i4_t OPTBUFNUM, MAXNBUF, MAXTACT;
key_t FSEGNUM;
i4_t MAXTRANS;
pid_t strcr_pid;

extern i4_t ljmsize;
extern struct ldesind **TAB_IFAM;
extern i4_t TIFAM_SZ;
extern char **scptab;
extern i4_t minidnt;
extern u2_t trnum;
extern u2_t S_SC_S;
extern i4_t msqidl, msqidm, msqidb;

extern CPNM curcpn;
extern i2_t maxscan;


#define ARG(num, what) argum = (i4_t)(what);            \
                       sprintf (args[num], "%d", argum)

static void
ini_pupsi (void)
{
  char *args[20], mch[20][20];
  i4_t i;
  i4_t argum;  

  for (i = 0; i < 20; i++)
    args[i] = mch[i];

/*---------------INI MICROJ------------------------------*/
  if ((pidmj = fork ())< 0)
    {
      perror ("fork MJ");
      exit (1);
    }
  if (pidmj == 0)
    {
      args[0] = MJ;
      args[1] = MJFILE;
      sprintf (args[2], "%d", (i4_t)RJZm);
      sprintf (args[3], "%d", (i4_t)MPAGEm);
      sprintf (args[4], "%d", (i4_t)keymj);
      sprintf (args[5], "%d", (i4_t)keylj);
      args[6] = NULL;
      execvp (*args, args);
      perror ("SCR.INI: MJ doesn't exec");
      exit (1);
    }
  MSG_INIT (msqidm, keymj, "MJ");
  
/*----------------INI BUFFER----------------------*/
  
  if ((pidbf = fork ())< 0)
    {
      perror ("fork BUF");
      exit (1);
    }
  if (pidbf == 0)
    {
      struct dirent *dir;
      DIR *dp;
      i4_t seg_num;
      char name_BD_seg[1024];
      args[0] = BUF;
      ARG (1, keybf);
      ARG (2, keymj);
      ARG (3, OPTBUFNUM);
      ARG (4, MAXNBUF);
      ARG (5, MAXTACT);
      ARG (6, MAXTRANS);
      /* the first numeric name of the shared memory segment for PUPSI */
      ARG (7, FSEGNUM);
      /* BD segments numbers and names */
      if ((dp = opendir (DIR_SEGS)) == NULL)
	{
	  fprintf (stderr, "SCR: %s cannot open\n", DIR_SEGS);
	  exit (1);
	}
      dir = readdir (dp);
      dir = readdir (dp);
      for (i = 8; (dir = readdir (dp)) != NULL; i += 2)
	{
	  if (dir->d_ino == 0)
	    continue;
	  args[i] = dir->d_name + 3;
          seg_num = atoi (args[i]);
          ARG (i, seg_num);
	  sprintf (args[i + 1], "%s/%s", DIR_SEGS, dir->d_name);
          printf ("SCR.ini_pupsi: name = %s, segnum = %d\n", args[i + 1], seg_num);
	}
      printf ("SCR.ini_pupsi: i = %d\n", i);    
      if (i == 8)           /* no at all segments */
        {
          strcpy (name_BD_seg, DIR_SEGS);
          strcat (name_BD_seg, "/seg1");

          ini_BD_seg (name_BD_seg, S_SC_S);
          sprintf (args[8], "%d", 1);
          args[9] = name_BD_seg;
          i += 2;
        }
      closedir (dp);

      sprintf (args[i], "%d", (i4_t)strcr_pid);
      args[i + 1] = NULL;

      execvp (*args, args);
      perror ("SCR.INI: BUF doesn't exec");
      exit (1);
    }
  MSG_INIT (msqidb, keybf, "BUF");
  
/*-------------OPEN/INI LOGJ--------------------------*/
  if ((fd_lj = open (LJFILE, O_RDWR, 0644)) < 0)
    {
      ini_lj (LJFILE);
      if ((fd_lj = open (LJFILE, O_RDWR, 0644)) < 0)
        {      
          perror ("LJ: open");
          exit (1);
        }
    }
  if ((pidlj = fork ())< 0)
    {
      perror ("fork LJ");
      exit (1);
    }
  if (pidlj == 0)
    {
      args[0] = LJ;
      sprintf (args[1], "%d", fd_lj);
      sprintf (args[2], "%d", (i4_t)RJZl);
      sprintf (args[3], "%d", (i4_t)MPAGEl);
      sprintf (args[4], "%d", (i4_t)keylj);
      sprintf (args[5], "%d", (i4_t)keybf);
      sprintf (args[6], "%d", (i4_t)keyscr);      
      args[7] = NULL;
      execvp (*args, args);
      perror ("SCR.INI: LJ doesn't exec");
      exit (1);
    }
  MSG_INIT (msqidl, keylj, "LJ");
}

static int
fin_pupsi (void)
{
  i4_t rep, repm, repb, repl, cd, i;
  struct msg_buf sbuf, rbuf;
  struct msqid_ds msqidds;

  repl = LOGJ_FIX ();
  cd = close (fdcurlj);

  sbuf.mtype = FINIT;
  if (msgsnd (msqidm, (void *) &sbuf, 0, 0) < 0)
    {
      perror ("SCR.msgsnd: MJ->FINIT");
      exit (1);
    }
  if (msgrcv (msqidm, (void *) &rbuf, 1, ANSMJ, 0) < 0)
    {
      perror ("SCR.msgrcv: FINIT MJ");
      exit (1);
    }
  repm = *rbuf.mtext;

  if (msgsnd (msqidb, (void *) &sbuf, 0, 0) < 0)
    {
      perror ("SCR.msgsnd: BUF->FINIT");
      exit (1);
    }
  if (msgrcv (msqidb, (void *) &rbuf, 1, ANSBUF, 0) < 0)
    {
      perror ("SCR.msgrcv: FINIT BUF");
      exit (1);
    }
  repb = *rbuf.mtext;
  i = msgctl (msqidl, IPC_RMID, &msqidds);
  i = msgctl (msqidm, IPC_RMID, &msqidds);
  i = msgctl (msqidb, IPC_RMID, &msqidds);

  if (cd == 0 && repm == 0 && repb == 0 && repl == 0)
    rep = 0;
  else
    rep = -1;
  kill (pidlj, SIGKILL);
  kill (pidmj, SIGKILL);
  kill (pidbf, SIGKILL);
  return (rep);
}

static void
open_cur_lj (char *file_name)
{
  if ( (fdcurlj = open (file_name, O_RDWR, 0644)) < 0)
    {
      printf ("current LJ: open %s", file_name);
      perror ("current LJ: open");
      exit (1);
    }
  printf ("current LJ  %s  fdcurlj = %d\n", file_name, fdcurlj);  
}

static key_t
get_msg_key(void)   /* this function has a complete static copy in adm.c */
{
  static key_t k = (key_t)-2;
  if (k==(key_t)-2)
#if HAVE_FTOK
    k = ftok(MJ, (getpid()&0xff));
#else
    k = FIRSTMQN;
#endif
  errno = 0;
  for(;;k++)
    {
      if ( msgget (k, DEFAULT_ACCESS_RIGHTS) >= 0 )
        continue;
      if (errno == ENOENT)
        return k++;
      else if (errno == ENOSPC)
        return -1;
    }
}

int
main (int argc,char **argv)
{
  struct ADBL cadlj, ladlj;
  char *dname;
  i4_t fdaf, segnum, n;
  struct ADF p2;
  DIR *dp;
  struct dirent *dir;
  struct msqid_ds msqidds;
  char name[1024];

  setbuf (stdout, NULL);
  
  ljmsize = size1b + size4b + 2 * size2b + 2 * size2b + size2b + size4b + 2 * size2b;
  scptab = (char **) xmalloc (chpsize * maxscan);
  for (n = 0; n < maxscan; n++)
    scptab[n] = NULL;
  trnum = CONSTTR;
  strcr_pid = getpid();

/*------------- Copy BD's segments ---------------*/

  if ((segnum = dir_copy (DIR_REP_SEGS, DIR_SEGS)) < 0)
    fprintf (stderr, "SCR: Can't copy SEGS\n");

/*------------OPEN AND READ ADMFILE------------------*/
  if ((fdaf = open (ADMFILE, O_RDWR, 0644)) < 0)
    {
      ini_adm_file (ADMFILE);
      if ((fdaf = open (ADMFILE, O_RDWR, 0644)) < 0)
        {      
          perror ("ADMFILE: open error");
          exit (1);
        }
    }
  if (read (fdaf, (char *) &p2, adfsize) != adfsize)
    {
      perror ("ADMFILE: read error");
      exit (1);
    }
  minidnt = 1;
  MAXTRANS = p2.maxtrans;
  RJZm = p2.mjred;
  RJZl = p2.ljred;
  MPAGEm = p2.mjmpage;
  MPAGEl = p2.ljmpage;
  S_SC_S = p2.sizesc;
  OPTBUFNUM = p2.optbufnum;
  MAXNBUF = p2.maxnbuf;
  MAXTACT = p2.maxtact;
  FSEGNUM = p2.fshmsegn;
  
  TIFAM_SZ = 2;
  TAB_IFAM = (struct ldesind **) xmalloc (TIFAM_SZ * sizeof (struct ldesind **));
  TAB_IFAM[0] = NULL;
  TAB_IFAM[1] = NULL;
  tab_difam (1);
  
  keyscr  = get_msg_key();  
  keylj   = get_msg_key();
  keymj   = get_msg_key();
  keybf   = get_msg_key();

  /* Getting queue for rcvsc */
  if ((msqidscr = msgget (keyscr, IPC_CREAT | DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("SCR: Getting queue for rcvsc");
      exit (1);
    }  

/*------------Recovery by LJ's repository------------------*/

  if ((dp = opendir (DIR_REP_LJS)) == NULL)
    {
      fprintf (stderr, "SCR: %s cannot open\n", DIR_REP_LJS);
      exit (1);
    }
  dir = readdir (dp);
  dir = readdir (dp);
  while ((dir = readdir (dp)) != NULL)
    {
      if (dir->d_ino == 0)
	continue;
      dname = dir->d_name;
      copy (dname, DIR_REP_LJS, DIR_JOUR);
      ini_pupsi ();
      strcpy (name, DIR_JOUR);
      strcat (name, "/");
      strcat (name, dname);
      open_cur_lj (name);
      cadlj = frwdmtn ();
      fin_pupsi ();
      dubl_segs ();

      if (unlink (name) < 0)
	{
	  fprintf (stderr, "SCR: %s cannot delete\n", name);
	  exit (1);
	}
    }
  closedir (dp);

  ini_pupsi ();
  fdcurlj = fd_lj;
  cadlj = frwdmtn ();
  ladlj = bmtn_feot (fdcurlj, cadlj);

  close (fdaf);
  rcv_wmlj (&ladlj);		/* The record about global rollback */
  fin_pupsi ();
  dubl_segs ();
  msgctl (msqidscr, IPC_RMID, &msqidds);  
  fprintf (stderr, "SCR: Storage Crash Recovery has finished successfully\n");
  return 0;
}

