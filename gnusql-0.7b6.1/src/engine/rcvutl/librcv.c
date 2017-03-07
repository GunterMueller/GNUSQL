/*  librcv.c  - Crash Recovery Utility Library 
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

/* $Id: librcv.c,v 1.253 1998/09/29 21:25:10 kimelman Exp $ */

#include "setup_os.h"
#if STDC_HEADERS
#include <sys/types.h>
#include <sys/stat.h>
#endif
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

#if HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif
#if HAVE_SYS_MSG_H
#include <sys/msg.h>
#endif
#if HAVE_SYS_SHM_H
#include <sys/shm.h>
#endif

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

/**************************************************************/

extern i4_t msqidl, msqidm, msqidb;
extern i4_t fdcurlj;

result_t gsqltrn_rc;
char ljpage[RPAGE];
u2_t pagenum;

extern i4_t N_AT_SEG;
extern COST cost;
extern struct ADREC bllj;
extern struct d_r_t *firstrel;
extern struct ADBL admj;
extern char *pbufmj;
extern char *pbuflj;
extern struct ldesind **TAB_IFAM;
extern i4_t TIFAM_SZ;
extern i4_t idtr;
extern struct ADBL adlj;
extern struct ADREC blmj;
extern char *bufmj;

extern unsigned ljmsize;
extern u2_t trnum;
extern struct ADREC blmj;
extern i4_t minidnt;

#define adfsize sizeof(struct ADF)

static void
rllbck_tr (i4_t fd, i4_t cidtr, struct ADBL cadlj)
{
  register char *a, type;
  i4_t newidtr;
  u2_t blsz;
  char *pnt, mas[RPAGE];

  for (; (blsz = LJ_prev (fd, &cadlj, &pnt, mas)) > 0;)
    {
      a = pnt;
      type = *a++;
      if (type == EOTLJ || type == GRLBLJ)
	continue;
      newidtr = t4bunpack (a);
      a += size4b;
      if (newidtr == cidtr)
	{
	  cadlj.npage = t2bunpack (a);
	  a += size2b;
	  cadlj.cm = t2bunpack (a);
	  a += size2b;
	  backactn (blsz, a, type);
	  break;
	}
    }
  idtr = cidtr;
  r_tr (cadlj);
}

static void
rollb_nftr(i4_t fd, struct ADBL cadlj, char *a, u2_t n)
{
  u2_t i, NFTRANS;
  i4_t cidtr;

  NFTRANS = n / size4b;
  for (i = 0; i < NFTRANS; i++, a += size4b)
    {
      cidtr = t4bunpack (a);
      rllbck_tr (fd, cidtr, cadlj);
    }
}

static void
frwrd_mtn (struct ADBL cadlj)
{
  char *a, type;
  u2_t blsz;
  char *pnt, mas[RPAGE];
  
  for (; (blsz = LJ_next (fdcurlj, &cadlj, &pnt, mas)) > 0;)
    {				/*The motion forward */
      a = pnt;
      type = *a++;
      if (type == CPRLJ)
        continue;
      if (type == EOTLJ)
	{
	  getmint (a, blsz - size1b);
	  continue;
	}
      if (type == GRLBLJ)
	{
	  adlj = cadlj;
	  blsz = LJ_prev (fdcurlj, &adlj, &pnt, mas);
	  a = pnt;
	  type = *a++;
	  if (type != EOTLJ)
	    fprintf (stderr, "Serious error! LIBRCV.frwrd_mtn: type |= EOTLJ (=%d) before GRLBLJ\n", type);
	  rollb_nftr (fdcurlj, adlj, a, blsz - size1b);
	  continue;
	}
      idtr = t4bunpack (a);
      forward_action (a + size4b, type, blsz - ljmsize);
    }
}

struct ADBL 
frwdmtn (void)
{
  struct ADBL cadlj;

  read_page (fdcurlj, 1, ljpage);
  cadlj.npage = 1;
  cadlj.cm = RTPAGE;

  frwrd_mtn (cadlj);
  return (adlj);    /* use adlj because cadlj doesn't change */
                    /* in frwrd_mnt but adlj   */  
}

static struct d_r_t *
cr_rd(struct id_rel *idr)
{
  char *a, *asp;
  struct A pg;
  struct d_r_t *desrel;
  u2_t *ai;
  unsigned char t;

  asp = getpg (&pg, idr->urn.segnum, idr->pagenum, 's');
  ai = (u2_t *) (asp + phsize) + idr->index;
  a = asp + *ai;
  t = *a & MSKCORT;
  if (t == IND)
    {			/* indirect reference */
      u2_t pn2, ind2;
      char *asp = NULL;
      ind2 = t2bunpack (a + 1);
      pn2 = t2bunpack (a + 1 + size2b);
      putpg (&pg, 'n');
      while ((asp = getpg (&pg, pg.p_sn, pn2, 's')) == NULL);
      ai = (u2_t *) (asp + phsize) + ind2;
      a = asp + *ai;
      assert ((*a & CREM) != 0 && *ai != 0);
    }
  desrel = crtfrd (idr, a);
  putpg (&pg, 'n');
  return (desrel);
}

u2_t
LJ_next (i4_t fd, struct ADBL *adj, char **pnt, char *mas)
{
  u2_t N, offbeg, n, blsz, off, n1;
  char *a, *beg_of_record;
  
  N = adj->npage;
  offbeg = adj->cm;
  if (pagenum != N)
    read_page (fd, N, ljpage);
  a = ljpage + size4b;
  off = t2bunpack (a);
  a += size2b;
  if (off == offbeg && *a == 0)
    return (0);
  a = ljpage + offbeg;
  if (offbeg + RTBLK > RPAGE)
    {				/* the top-block places in two pages*/
      char buff[size2b];
      n = RPAGE - offbeg;
      bcopy (a, buff, n);
      N++;
      read_page (fd, N, ljpage);
      n1 = RTBLK - n; 
      bcopy (ljpage + RTPAGE, buff + n, n1);
      blsz = t2bunpack (buff);
      n = RTPAGE + n1;
      beg_of_record = ljpage + n;
    }
  else
    {				/* the top-block places in (N)-page */
      blsz = t2bunpack (a);
      beg_of_record = a + RTBLK;
      n = offbeg + RTBLK;
    }
  if (n + blsz > RPAGE)
    {			/* block-record places in two page*/
      n = RPAGE - n;
      bcopy(beg_of_record, mas, n);
      N++;
      read_page (fd, N, ljpage);
      n1 = blsz - n;
      bcopy (ljpage + RTPAGE, mas + n, n1);
      *pnt = mas;
      n = RTPAGE + n1;
    }
  else
    {
      *pnt = beg_of_record;
      n += blsz;
    }
  if (n + RTBLK > RPAGE)
    {				/* the endtop-block places in two pages*/
      N++;
      offbeg = RTPAGE + RTBLK - (RTBLK - n);
    }
  else
    offbeg = n + RTBLK;
  adj->npage = N;
  adj->cm = offbeg;
  return (blsz);
}

static void
rollback_action (char *a)
{
  char type;
  struct ADBL cadlj;
  struct ADREC bllj;
  
  cadlj.npage = t2bunpack (a);
  a += size2b;
  cadlj.cm = t2bunpack (a);
  a += size2b;
  rcv_LJ_GETREC (&bllj, &cadlj);
  a = bllj.block;
  type = *a++;
  backactn (bllj.razm, a + size4b + 2 * size2b, type);
}

void
forward_action (char *a, char type, u2_t n)
{
  u2_t sn;
  i4_t rn, ordrn;
  struct d_r_t *desrel;
  struct des_tid tid;
  struct id_rel idr;
  struct full_des_tuple dtuple;

  if (type == RLBLJ || type == RLBLJ_AS_OP)
    rollback_action (a);
  a += 2 * size2b;
  sn = idr.urn.segnum = dtuple.sn_fdt = t2bunpack (a);
  assert (sn != 0);
  a += size2b;
  rn = idr.urn.obnum = dtuple.rn_fdt = t4bunpack (a);
  a += size4b;
  idr.pagenum = t2bunpack (a);
  a += size2b;
  idr.index = t2bunpack (a);
  a += size2b;
  if (rn != RDRNUM)
    {
      tid.tpn = t2bunpack (a);
      a += size2b;
      tid.tindex = t2bunpack (a);
      a += size2b;
      for (desrel = firstrel; desrel != NULL; desrel = desrel->drlist)
        if (desrel->desrbd.relnum == rn)
          break;
      if (desrel == NULL)
        desrel = cr_rd (&idr);
    }
  else
    {
      tid.tpn = idr.pagenum;
      tid.tindex = idr.index;
      desrel = NULL;
    }
  dtuple.tid_fdt = tid;
  if (type == DELLJ)
    redo_dltn (&dtuple, desrel, a);
  else if (type == INSLJ)
    {
      struct des_tid newtid;

      newtid = ordins (&idr, a, n, 'n');
      if (newtid.tpn != tid.tpn || newtid.tindex != tid.tindex)
        fprintf (stderr,
                 "LIBRCV.frwrd_mtn: Internal error! tids don't match pn=%d\n",
                 tid.tpn);
      if (rn != RDRNUM)
        proind (ordindi, desrel, desrel->desrbd.indnum, a, &tid);
      else
        {
          struct ldesind *di;
          ordrn = t4bunpack (a + scscal (a));
          idr.urn.obnum = ordrn;
          desrel = crtfrd (&idr, a);
          for (di = desrel->pid; di != NULL; di = di->listind)
            crindci (di);
        }
    }
  else
    {			/* The modification */
      u2_t corsize;
      struct des_tid ref_tid;
      corsize = get_placement (sn, &tid, &ref_tid);
      n -= corsize;
      ordmod (&dtuple, &ref_tid, corsize, a + corsize, n);
      if (rn != RDRNUM)
        mproind (desrel, desrel->desrbd.indnum, a, a + corsize, &tid);
      else
        {
          ordrn = t4bunpack (a + scscal (a));
          desrel = firstrel;
          for (; desrel->desrbd.relnum != ordrn; desrel = desrel->drlist);
          if (desrel == NULL)
            perror ("LIBRCV.frwrd_mtn: The corresponding desrel is absent\n");
          
          if (type == DLILJ) /*nead to delete an index */
            redo_dind (desrel, a + corsize);
          else if (type == CRILJ)
            redo_cind (desrel, a + corsize);
          else if (type == ADFLJ)
            {		/*nead to add fields */
              struct d_r_bd drbd;
              u2_t fn;
              a += scscal (a) + corsize;
              drbdunpack (&drbd, a);
              fn = drbd.fieldnum;
              desrel->desrbd.fieldnum = fn;
              dfunpack (desrel, fn * rfsize, a + drbdsize);
            }
        }
    }
}

void
getmint (char *a, u2_t  n)
{
  register u2_t i, NFTRANS;
  i4_t cidtr;

  NFTRANS = n / size4b;
  if (NFTRANS == 0)
    return;
  minidnt = t4bunpack (a);
  a += size4b;
  for (i = 1; i < NFTRANS; i++, a += size4b)
    {
      cidtr = t4bunpack (a);
      if (cidtr < minidnt)
	minidnt = cidtr;
    }
}

void
r_tr (struct ADBL cadlj)
{
  char *a, type;
  struct ADREC bllj;

  while ( cadlj.cm != 0 )
    {
      rcv_LJ_GETREC (&bllj, &cadlj);
      a = bllj.block;
      type = *a++;
      a += size4b;
      cadlj.npage = t2bunpack (a);
      a += size2b;
      cadlj.cm = t2bunpack (a);
      a += size2b;
      backactn (bllj.razm, a, type);
    }
}
struct ADBL 
bmtn_feot (i4_t fd, struct ADBL cadlj)
    /* The motion back to the first EOTLJ */
{
  char *a, type;
  i4_t prnftr = 0;
  struct ADBL ladlj;
  u2_t blsz;
  char *pnt, mas[RPAGE];

  ladlj = cadlj;
  while ((blsz = LJ_prev (fd, &cadlj, &pnt, mas)) > 0)
    {
      a = pnt;
      type = *a++;
      if (type == GRLBLJ)
	{
	  ladlj = cadlj;
	  prnftr = 1;
	  continue;
	}
      if (type == EOTLJ)
	{
	  getmint (a, blsz - size1b);
	  if (prnftr == 0)
	    rollb_nftr (fd, cadlj, a, blsz - size1b);
	  break;
	}
      idtr = t4bunpack (a);
      backactn (blsz, a + size4b + 2 * size2b, type);
      ladlj = cadlj;
    }
  return (ladlj);
}

void
backactn (u2_t blsz, char * a, char type)
{
  u2_t sn;
  i2_t n;
  struct d_r_t *desrel;
  struct des_tid tid;
  i4_t rn;
  struct id_rel idr;
  struct full_des_tuple dtuple;

  if (type == RLBLJ || type == CPRLJ || type == RLBLJ_AS_OP)
    return;
  sn = idr.urn.segnum = dtuple.sn_fdt = t2bunpack (a);
  assert (sn != 0);
  a += size2b;
  rn = idr.urn.obnum = dtuple.rn_fdt = t4bunpack (a);
  a += size4b;
  idr.pagenum = t2bunpack (a);
  a += size2b;
  idr.index = t2bunpack (a);
  a += size2b;
  if (rn != RDRNUM)
    {
      tid.tpn = t2bunpack (a);
      a += size2b;
      tid.tindex = t2bunpack (a);
      a += size2b;
      for (desrel = firstrel; desrel != NULL; desrel = desrel->drlist)
	if (desrel->desrbd.relnum == rn)
	  break;
      if (desrel == NULL)
	desrel = cr_rd (&idr);
    }
  else
    {
      tid.tpn = idr.pagenum;
      tid.tindex = idr.index;
      desrel = NULL;
    }
  dtuple.tid_fdt = tid; 
  n = blsz - ljmsize;
  if (type == DELLJ)
    {
      redo_insrtn (&dtuple, desrel, n, a);
    }
  else if (type == INSLJ)
    {
      redo_dltn (&dtuple, desrel, a);
    }
  else
    {				/* The modification */
      u2_t corsize;
      struct des_tid ref_tid;
      
      corsize = get_placement (sn, &tid, &ref_tid);
      n -= corsize;
      ordmod (&dtuple, &ref_tid, corsize, a, n);
      if (rn != RDRNUM)
	{
	  mproind (desrel, desrel->desrbd.indnum, a + n, a, &tid);
	}
      else
	{
          i4_t ordrn;
	  ordrn = t4bunpack (a + scscal(a));
          idr.urn.obnum = ordrn;
	  desrel = firstrel;
	  for (; desrel->desrbd.relnum != ordrn; desrel = desrel->drlist);
	  if (desrel == NULL)
	    desrel = crtfrd (&idr, a + corsize);    /* create a new table descriptor
                                              corresponding aftermodification state */
	  if (type == CRILJ)
	    {			/* nead to delete an index */
	      redo_dind (desrel, a);
	    }
          else if (type == DLILJ)
            {				/*nead to create an index */
              redo_cind (desrel, a);
            }
	  else if (type == ADFLJ)
	    {			/*nead to delete fields */
              struct d_r_bd drbd;
	      a++;
	      drbdunpack (&drbd, a);
	      desrel->desrbd.fieldnum = drbd.fieldnum;
	    }
	}
    }
}

u2_t
LJ_prev (i4_t fd, struct ADBL *adj, char **pnt, char *mas)
{
  u2_t N, n, blsz, n1;
  char *end_of_record;
  
  N = adj->npage;
  n = adj->cm;
  if (n == RTPAGE && pagenum == 1)
    return (0);
  if (pagenum != N)
    read_page (fd, N, ljpage);
  n -= RTPAGE;
  if (n < RTBLK)
    {				/* the endtop-block places in two pages*/
      char buff[RTBLK];
      n1 = RTBLK - n;
      bcopy (ljpage + RTPAGE, buff + n1, n);
      N--;
      read_page (fd, N, ljpage);
      end_of_record = ljpage + RPAGE - n1;
      bcopy (end_of_record, buff, n1);
      blsz = t2bunpack (buff);
      n = RPAGE - RTPAGE - n1;
    }
  else
    {				/* the top-block places in (N)-page */
      end_of_record = ljpage + n + RTPAGE - RTBLK;
      blsz = t2bunpack (end_of_record);
      n -= RTBLK;
      if (n == 0)
        {
          N--;
          read_page (fd, N, ljpage);
          n = RPAGE - RTPAGE;
          end_of_record = ljpage + RPAGE;
        }
    }
  if (n < blsz)
    {				/* block-record places in two pages */
      n1 = blsz - n;
      bcopy (ljpage + RTPAGE, mas + n1, n);
      N--;
      read_page (fd, N, ljpage);
      bcopy (ljpage + RPAGE - n1, mas, n1);
      *pnt = mas;
      n = RPAGE - RTPAGE - n1;
    }
  else
    {
      *pnt = end_of_record - blsz;
      n -= blsz;
      if (n == 0)
        {
          N--;
          n = RPAGE - RTPAGE;
        }
    }
  if (n < RTBLK)
    {				/* the endtop-block places in two pages */
      N--;
      n1 = RTBLK - n;
      n = RPAGE - RTPAGE - n1;
    }
  else
    n -= RTBLK;
  adj->npage = N;
  adj->cm = n + RTPAGE;
  return (blsz);
}

static void
write_page(i4_t fd, u2_t N, char *buf)
{
  if (lseek (fd, (i4_t) (RPAGE * (N - 1)), SEEK_SET) < 0)
    {
      perror ("LIBRCV: lseek");
      exit (1);
    }
  if (write (fd, buf, RPAGE) != RPAGE)
    {
      perror ("LIBRCV: read");
      exit (1);
    }
}

static void
do_cont (void)
{
  register char *a;
  i4_t NB;

  NB = t4bunpack (ljpage);
  *(ljpage + size4b + size2b) = SIGN_CONT;
  write_page (fdcurlj, pagenum, ljpage);
  a = ljpage;
  t4bpack (NB, a);
  a += size4b;
  t2bpack (RTPAGE, a);
  a += size2b;
  *a = SIGN_NOCONT;
  pagenum++;
}

static char *
write_topblock(u2_t size, u2_t off, char *a)
{
  if (off + RTBLK > RPAGE)
    {				/* the top-block places in two pages*/
      u2_t n, n1;
      char buff[size2b];
      n = RPAGE - off;
      t2bpack (size, buff);
      bcopy (buff, a, n);
      do_cont ();
      a = ljpage + RTPAGE;
      n1 = RTBLK - n;
      bcopy (buff + n, a, n1);
      a += n1;
    }
  else
    {				/* the top-block places in (N)-page */
      t2bpack (size, a);
      a += size2b;
    }
  return (a);
}

void
rcv_wmlj (struct ADBL *cadlj)
{
  u2_t off, razm;
  register char *p;

  read_page (fdcurlj, cadlj->npage, ljpage);
  off = cadlj->cm;;
  razm = 1;
  p = write_topblock (razm, off, ljpage + off);
  if (p + razm > ljpage + RPAGE)
    {				
      do_cont ();
      p = ljpage + RTPAGE;
    }
  *p++ = (char) GRLBLJ;
  p = write_topblock (razm, p - ljpage, p);
  off = p - ljpage;
  t2bpack (off, ljpage + size4b);
  *(ljpage + size4b + size2b) = SIGN_NOCONT;
  write_page (fdcurlj, pagenum, ljpage);
  ftruncate (fdcurlj, pagenum * RPAGE);
}

void
rcv_LJ_GETREC (struct ADREC *bllj, struct ADBL *pcadlj)
{
  char *p;
  struct msg_buf sbuf;

  sbuf.mtype = GETREC;
  t2bpack (trnum, sbuf.mtext);
  bcopy ((char *) pcadlj, sbuf.mtext, adjsize);
  if (msgsnd (msqidl, (void *) &sbuf, adjsize + size2b, 0) < 0)
    {
      perror ("LIBRCV.msgsnd: LJ GETREC");
      exit (1);
    }
  if (msgrcv (msqidl, (void *) &sbuf, sizeof (struct ADREC), trnum, 0) < 0)
    {
      perror ("LIBRCV.msgrcv: Answer from LJ");
      exit (1);
    }
  p = sbuf.mtext;
  bllj->razm = t2bunpack (p);
  p += size2b;
  bcopy (p, bllj->block, bllj->razm);
}

i4_t
dir_copy (char *dir_from, char *dir_to)
{
  DIR *dp;
  struct dirent *dir;
  i4_t num;

  if ((dp = opendir (dir_from)) == NULL)
    {
      fprintf (stderr, "LIBRCV: %s cannot open\n", dir_from);
      exit (1);
    }
  dir = readdir (dp);
  dir = readdir (dp);
  for (num = 0; (dir = readdir (dp)) != NULL; num++)
    {
      if (dir->d_ino == 0)
	continue;
      copy (dir->d_name, dir_from, dir_to);
    }
  closedir (dp);
  return (num);
}

void
copy (char *name, char *dir_from, char *dir_to)
{
  i4_t i;
  pid_t pidcp;
  char *args[4], mch[4][1024];
  i4_t status;

  for (i = 0; i < 4; i++)
    args[i] = mch[i];
  if ((pidcp = fork ())< 0)
    {
      perror ("fork cp");
      exit (1);
    }
  if (pidcp == 0)
    {
      args[0] = CP;
      sprintf (args[1], "%s/%s", dir_from, name);
      sprintf (args[2], "%s/%s", dir_to, name);
      args[3] = NULL;
      execvp (*args, args);
      perror ("LIBRCV.copy: CP doesn't exec");
      exit (1);
    }
  while (waitpid (-1, &status, 0) != pidcp);
}

i4_t
LOGJ_FIX (void)
{
  struct msg_buf sbuf;
  sbuf.mtype = BEGFIX;
  if (msgsnd (msqidl, (void *) &sbuf, 0, 0) < 0)
    {
      perror ("LIBRCV.msgsnd: LOGJ->FIX");
      exit (1);
    }
  if (msgrcv (msqidl, (void *) &sbuf, 1, ANSLJ, 0) < 0)
    {
      perror ("LIBRCV.msgrcv: BEGFIX LJ");
      exit (1);
    }
  return ( (i4_t)*sbuf.mtext);
}

/* Read "N"-page from JRN-basefile into buf */

void
read_page(i4_t fd, u2_t N, char *buf)
{
  if (lseek (fd, (i4_t) (RPAGE * (N - 1)), SEEK_SET) < 0)
    {
      perror ("LIBRCV: lseek");
      exit (1);
    }
  if (read (fd, buf, RPAGE) != RPAGE)
    {
      perror ("LIBRCV: read");
      exit (1);
    }
  pagenum = N;
}


void
dubl_segs (void)
{
  dir_copy (DIR_SEGS, DIR_DUB_SEGS);
  dir_copy (DIR_SEGS, DIR_REP_SEGS);
}

void
rcv_ini_lj ()
{
  struct msg_buf sbuf;

  sbuf.mtype = INILJ;
  t2bpack (trnum, sbuf.mtext);
  if (msgsnd (msqidl, (void *) &sbuf, size2b, 0) < 0)
    {
      perror ("LIBRCV.msgsnd: LJ INILOGJ");
      exit (1);
    }
  if (msgrcv (msqidl, (void *) &sbuf, 0, trnum, 0) < 0)
    {
      perror ("LIBRCV.msgrcv: Answer from LJ on INILOGJ");
      exit (1);
    }
}
