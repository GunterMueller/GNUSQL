/*
 *  trns.c  - Transaction service top routine
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Vera Ponomarenko
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

/* $Id: trns.c,v 1.257 1998/09/29 21:25:49 kimelman Exp $ */

#include "setup_os.h"
#include "xmem.h"

#include <sys/types.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "../admdef.h"
#include "destrn.h"
#include "strml.h"
#include "expop.h"
#include "fdcltrn.h"
#include "dessrt.h"
#include "fdclsrt.h"

#ifndef CLK_TCK
#define CLK_TCK 60
#endif

/*------ Transaction globals ------------------*/

i4_t ljrsize;
i4_t riskdmsz;
i4_t  IAMM = 0;
COST cost = 0L;
CPNM curcpn = 1;
struct des_nseg desnseg;
struct ADREC bllj;
struct ADREC blmj;
char *pbuflj = bllj.block;
char *bufmj = blmj.block;
char *pbufmj;
struct d_r_t *firstrel = NULL;
i2_t maxscan = DTSCAN;
i4_t idtr;
struct ADBL adlj;
struct ADBL admj;
u2_t extent_size;
struct dmbl_head *ffrdmbl;
struct dmbl_head *lfrdmbl;
i4_t fd_tmp_seg;
char name_tmp_seg[42];
i4_t    freext;
i4_t    sizeof_tmp_seg;  /* temporary file size in bytes */

extern i4_t ljmsize;
extern struct ldesind **TAB_IFAM;
extern i4_t TIFAM_SZ;
extern char **scptab;
extern i4_t minidnt;
extern u2_t trnum;
extern u2_t S_SC_S;
extern i4_t msqidl, msqidm, msqidb;

/*---------------------------------------------*/
pid_t parent;
static key_t keyadm, keylj, keymj, keybf, keysn, keytrn;
i4_t msqida, msqids, msqidt;

extern int errno;
i4_t TRNUM, N_AT_SEG = 0;

char *savestring (char *);

#define ARG(num, what, type)   sscanf (args[num], "%d", &argum); \
                               what = (type)(argum)


#define PRINT(x, y) /*PRINTF((x, y))*/
#define TEMP_SEG     DBAREA "/tseg"

void
trans_init (i4_t argc,char *args[])
{
  i4_t n;
  struct des_exns *desext;
  struct msg_buf sbuf;
  i4_t extssize, s_sc_s;
  char **s;
  i4_t argum;
  
  setbuf (stdout, NULL);
  ARG(1, keytrn, key_t);
  ARG(2, keysn, key_t);
  ARG(3, keylj, key_t);
  ARG(4, keymj, key_t);
  ARG(5, keybf, key_t);
  ARG(6, TRNUM, i4_t);
  trnum = TRNUM + CONSTTR;
  ARG(7, minidnt, i4_t);
  ARG(8, extssize, i4_t);
  extent_size = extssize;
  ARG(9, s_sc_s, i4_t);
  ARG(13, parent, pid_t);
  ARG(14, keyadm, key_t);
  
  S_SC_S = s_sc_s;
  if ((msqidt = msgget (keytrn, IPC_CREAT | DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("TRN.msgget: Queue for TRN");
      exit (1);
    }
  
  MSG_INIT (msqidl, keylj,  "LJ");
  MSG_INIT (msqidm, keymj,  "MJ");
  MSG_INIT (msqida, keyadm, "ADM");
  MSG_INIT (msqidb, keybf,  "BUF");
  MSG_INIT (msqids, keysn,  "SYN");

  TIFAM_SZ = 2;
  TAB_IFAM = (struct ldesind **) xmalloc (TIFAM_SZ * sizeof (struct ldesind **));
  TAB_IFAM[0] = NULL;
  TAB_IFAM[1] = NULL;
  tab_difam (1);
  adlj.cm = 0;
  ljmsize = size1b + size4b + 2 * size2b + size2b + size4b + 2 * size2b + tidsize;
  ljrsize = size1b + size4b + 2 * size2b;
  scptab = (char **) xmalloc (chpsize * maxscan);
  for (n = 0; n < maxscan; n++)
    scptab[n] = NULL;
  freext = 0;
  sizeof_tmp_seg = 0; 
  desnseg.lexnum = 0;
  desnseg.mexnum = DEXTD;
  desnseg.dextab = (struct des_exns *) xmalloc (dexsize * DEXTD);
  for (n = 0, desext = desnseg.dextab; n < DEXTD; n++, desext++)
    desext->efpn = (u2_t) ~ 0;
  desnseg.mtobnum = TOBPTD;
  desnseg.tobtab = (char **) xmalloc (chpsize * TOBPTD);
  for (n = 0, s = desnseg.tobtab; n < TOBPTD; n++)
    s[n] = NULL;

  sbuf.mtype = START;
  t2bpack (trnum, sbuf.mtext);
  __MSGSND(msqids, &sbuf, size2b, 0,"TRN.msgsnd: START to SYN");
  
  fd_tmp_seg = -1;            /* Temporary segment of the transaction is not open */
  strcpy (name_tmp_seg, TEMP_SEG);
  strcat (name_tmp_seg, args[6]);
} /* trans_init */

#undef ARG

i4_t
svpnt (void)
{
  struct msg_buf sbuf;
  struct id_rel idr;
  sbuf.mtype = SVPNT;
  t2bpack (trnum, sbuf.mtext);
  __MSGSND(msqids, &sbuf, size2b, 0,"TRN.msgsnd: SVPNT to SYN");
  __MSGRCV(msqids, &sbuf, cpnsize, trnum, 0,"TRN.msgrcv: SVPNT from SYN");
  curcpn = *(CPNM *) sbuf.mtext;
  wmlj (CPRLJ, ljrsize + cpnsize, &adlj, &idr, NULL, curcpn);
  return (curcpn);
}

i4_t
killtran (void)
{
  struct msg_buf sbuf;
  u2_t n, num_tob;
  struct des_tob *dt;
  
  if (IAMM != 0)
    {
      t2bpack (WLJET, sbuf.mtext);
      ADM_SEND (TRNUM, size2b, "TRN.msgsnd: WLJET to ADM");
      __MSGRCV(msqidl, &sbuf, 0, trnum, 0,"TRN.msgrcv: Answer from LJ");
    }
  num_tob = desnseg.mtobnum;
  for (n = 0; n < num_tob; n++)
    {
      dt = (struct des_tob *) *(desnseg.tobtab + n);
      if (dt != NULL)
        {
          delscd (dt->osctob, (char *) dt);
          xfree ((void *) dt);
        }
    }
  sbuf.mtype = COMMIT;
  t2bpack (trnum, sbuf.mtext);
  if (fd_tmp_seg > -1)
    unlink(name_tmp_seg);
  __MSGSND(msqids, &sbuf, size2b, 0,"TRN.msgsnd: COMMIT to SYN");
  __MSGRCV(msqids, &sbuf, 0, trnum, 0,"TRN.msgrcv: COMMIT from SYN");
  return (OK);
}

i4_t
closesc (i4_t scnum)
{
  char **t, sctype;
  struct d_mesc *scpr;
  struct d_r_t *desrel;
  struct d_sc_i *scind;
  struct ldesscan *desscan;
  
  if (scnum > maxscan)
    return (-ER_NDSC);
  t = scptab + scnum;
  scpr = (struct d_mesc *) * t;
  if (scpr == NULL)
    return (-ER_NDSC);
  if ((sctype = scpr->obsc) == SCR)
    {				/* relation scan */
      desrel = (struct d_r_t *) scpr->pobsc;
      desrel->oscnum--;
      scind = (struct d_sc_i *) scpr;
      desscan = &scind->dessc;
      if (desscan->cur_key != NULL)
	{
	  xfree ((void *) desscan->cur_key);
	}
    }
  else if (sctype == SCI)
    {				/* index scan */
      scind = (struct d_sc_i *) scpr;
      desscan = &scind->dessc;
      if (desscan->cur_key != NULL)
	{
	  xfree ((void *) desscan->cur_key);
	}
      desscan->pdi->oscni--;
    }
  else if (sctype == SCTR || sctype == SCF)
    {
      struct des_tob *dt;
      dt = (struct des_tob *) scpr->pobsc;
      dt->osctob--;
    }
  else
    return (-ER_NDSC);
  xfree ((void *) *t);
  *t = NULL;
  return (OK);
}

i4_t
mempos (i4_t scnum)
{
  struct d_mesc *scpr;
  char sctype;
  
  scpr = (struct d_mesc *) * (scptab + scnum);
  if (scnum >= maxscan || scpr == NULL)
    return (-ER_NDSC);
  if ((sctype = scpr->obsc) == SCTR)
    {				/* temporary relation scan */
      struct d_sc_r *screl;
      screl = (struct d_sc_r *) scpr;
      screl->memtid = screl->curtid;
    }
  else if (sctype == SCI || sctype == SCR)
    {				/* index scan */
      struct d_sc_i *scind;
      scind = (struct d_sc_i *) scpr;
      scind->dessc.mtidi = scind->dessc.ctidi;
    }
  else if (sctype == SCF)
    {
      struct d_sc_f *scfltr;
      scfltr = (struct d_sc_f *) scpr;
      scfltr->mpnf = scfltr->pnf;
      scfltr->mofff = scfltr->offf;
    }
  return (OK);
}

i4_t
curpos (i4_t scnum)
{
  struct d_mesc *scpr;
  char sctype;
  
  scpr = (struct d_mesc *) * (scptab + scnum);
  if (scnum >= maxscan || scpr == NULL)
    return (-ER_NDSC);
  if ((sctype = scpr->obsc) == SCTR)
    {				/* temporary relation scan */
      struct d_sc_r *screl;
      screl = (struct d_sc_r *) scpr;
      screl->curtid = screl->memtid;
    }
  else if (sctype == SCI || sctype == SCR)
    {				/* index scan */
      struct d_sc_i *scind;
      scind = (struct d_sc_i *) scpr;
      scind->dessc.ctidi = scind->dessc.mtidi;
    }
  else if (sctype == SCF)
    {
      struct d_sc_f *scfltr;
      scfltr = (struct d_sc_f *) scpr;
      scfltr->pnf = scfltr->mpnf;
      scfltr->offf = scfltr->mofff;
    }
  return (OK);
}

void
modmes (void)
{
  struct msg_buf sbuf;

  if (IAMM != 0)
    return;
  t2bpack (IAMMOD, sbuf.mtext);
  ADM_SEND (TRNUM, size2b, "TRN.msgsnd: IAMMOD to ADM");
  __MSGRCV(msqidt, &sbuf, size4b, (i4_t) trnum, 0,"TRN.msgrcv: IAMMOD from ADM");
  idtr = t4bunpack (sbuf.mtext);
  IAMM++;
  return;
}

i4_t
uniqnm (void)
{
  i4_t uniq_name;
  struct msg_buf sbuf;
  t2bpack (UNIQNM, sbuf.mtext);
  ADM_SEND (TRNUM, size2b, "TRN.msgsnd: uniqnm to ADM");
  __MSGRCV(msqidt, &sbuf, size4b, (i4_t) trnum, 0,"TRN.msgrcv: uniqnm from ADM");
  uniq_name = t4bunpack (sbuf.mtext);
  return (uniq_name);
}

CPNM
sn_lock (struct id_rel *pidrel, i4_t t, char *lc, i4_t sz)
{
  char *p;
  struct msg_buf sbuf;
  CPNM cpn;

  sbuf.mtype = LOCK;
  p = sbuf.mtext;
  BUFPACK(trnum,p);
  BUFPACK(*pidrel,p);
  BUFPACK(cost,p);
  *p++ = t;
  BUFPACK(sz,p);
  bcopy (lc, p, sz);
  p += sz;
  __MSGSND(msqids, &sbuf, p - sbuf.mtext, 0,"TRN.msgsnd: LOCK to SYN");
  __MSGRCV(msqids, &sbuf, cpnsize, trnum, 0,"TRN.msgrcv: LOCK from SYN");
  cpn = *(CPNM *) sbuf.mtext;
  return (cpn);
}

void
sn_unltsp (CPNM cpn)
{
  struct msg_buf sbuf;
  char *p;
  
  sbuf.mtype = UNLTSP;
  p = sbuf.mtext;
  BUFPACK(trnum,p);
  BUFPACK(cpn,p);
  __MSGSND(msqids, &sbuf, cpnsize + size2b, 0,"TRN.msgsnd: UNLTSP to SYN");
  __MSGRCV(msqids, &sbuf, 0, trnum, 0,"TRN.msgrcv: UNLTSP from SYN");
}

/*
static void
sort (i4_t type, u2_t sn, u2_t * fpn, struct des_field *df, u2_t fn, u2_t fdfn,
      u2_t * fsrt, u2_t kn, char prdbl, char *drctn, u2_t * lpn)
{
  u2_t n;
  char *p;
  struct msg_buf sbuf;
  COST cost1;

  sbuf.mtype = type;
  p = sbuf.mtext;
  BUFPACK(trnum,p);
  BUFPACK(*fpn,p);
  BUFPACK(fn,p);
  BUFPACK(fdfn,p);
  for (; fn != 0; fn--, df++)
    BUFPACK(*df,p);
  if (type == FLSORT)
    BUFPACK(sn,p);
  BUFPACK(kn,p);
  for (n = kn; n != 0; n--, fsrt++)
    BUFPACK(*fsrt,p);
  *p++ = prdbl;
  bcopy (drctn, p, kn);
  p += kn;
  
  PRINTF (("TRN.sort: trnum = %d\n", trnum));
  
  __MSGSND(msqidq, &sbuf, p - sbuf.mtext, 0,"TRN.msgsnd: SORT to SRT");
  __MSGRCV(msqidq, &sbuf, 2 * size2b + sizeof (COST), trnum, 0,
           "TRN.msgrcv: SORT from SRT");
  p = sbuf.mtext;
  BUFUPACK(p,*fpn);
  BUFUPACK(p,*lpn);
  BUFUPACK(p,cost1);
  cost += cost1;
}

void
srtr_trsort (u2_t * fpn, struct fun_desc_fields *desf,
             u2_t * fsrt,u2_t kn, char prdbl, char *drctn, u2_t * lpn)
{
  u2_t fn, fdf;
  struct des_field * df;
  fn = desf->f_fn;
  fdf = desf->f_fdf;
  df = desf->df_pnt;
  sort (TRSORT, 0, fpn, df, fn, fdf, fsrt, kn, prdbl, drctn, lpn);
}

void
srtr_flsort (u2_t sn, u2_t * fpn, struct fun_desc_fields *desf,
             u2_t * mfn, u2_t kn, char prdbl, char *drctn, u2_t * lpn)
{
  u2_t fn, fdf;
  struct des_field * df;
  fn = desf->f_fn;
  fdf = desf->f_fdf;
  df = desf->df_pnt;
  sort (FLSORT, sn, fpn, df, fn, fdf, mfn, kn, prdbl, drctn, lpn);
}
*/

void
srtr_tid (struct des_tob *dt)
{
  u2_t fpn, lpn;
  fpn = dt->firstpn;
  lpn = tidsort (&fpn); 
  dt->firstpn = fpn;
  dt->lastpn = lpn;
}

/*
void
srtr_tid (struct des_tob *dt)
{
  char *p;
  struct msg_buf sbuf;
  COST cost1;
  u2_t fpn, lpn;

  sbuf.mtype = TIDSRT;
  p = sbuf.mtext;
  BUFPACK(trnum,p);
  BUFPACK(dt->firstpn,p);
  __MSGSND(msqidq, &sbuf, 2 * size2b, 0,"TRN.msgsnd: SORT to SRT");
  __MSGRCV(msqidq, &sbuf, 2 * size2b + sizeof (cost1), trnum, 0,
           "TRN.msgrcv: SORT from SRT");
  p = sbuf.mtext;
  BUFUPACK(p,fpn);
  BUFUPACK(p,lpn);
  dt->firstpn = fpn;
  dt->lastpn = lpn;
  BUFUPACK(p,cost1);
  cost += cost1;
}
*/

void
LJ_GETREC (struct ADBL *pcadlj)
{
  char *p;
  struct msg_buf sbuf;

  sbuf.mtype = GETREC;
  p = sbuf.mtext;
  t2bpack (trnum, p);
  p += size2b;
  bcopy ((char *) pcadlj, p, adjsize);
  __MSGSND(msqidl, &sbuf, adjsize + size2b, 0,"TRN.msgsnd: LJ GETREC");
  __MSGRCV(msqidl, &sbuf, sizeof (struct ADREC), trnum, 0,"TRN.msgrcv: Answer from LJ");
  p = sbuf.mtext;
  bllj.razm = t2bunpack (p);
  p += size2b;
  bcopy (p, pbuflj, bllj.razm);
}

void
read_tmp_page (u2_t pn, char *buff)
{
  if (fd_tmp_seg < 0)
    {
      printf ("TRN.read_tmp_page: temp seg is not open");
      return;
    }
  if (lseek (fd_tmp_seg, BD_PAGESIZE * pn, SEEK_SET) < 0)
    {
      perror ("TRN.read_tmp_page: temp seg lseek error ");
      exit (1);
    }
  if (read (fd_tmp_seg, buff, BD_PAGESIZE) < 0)
    {
      printf ("TRN.read_tmp_page: temp seg read error errno=%d\n", errno);
      exit (1);
    }
  cost += 1;
}

void
write_tmp_page (u2_t pn, char *buff)
{
  if (fd_tmp_seg < 0)
    if ((fd_tmp_seg = open (name_tmp_seg, O_RDWR|O_CREAT, DEFAULT_ACCESS_RIGHTS)) < 0)
      {
        printf ("TRN.write_tmp_page: temp seg open error\n");
	exit (1);
      }
  if (lseek (fd_tmp_seg, BD_PAGESIZE * pn, SEEK_SET) < 0)
    {
      perror ("TRN.write_tmp_page: temp seg lseek error ");
      exit (1);
    }
  if (write (fd_tmp_seg, buff, BD_PAGESIZE) != BD_PAGESIZE)
    {
      perror ("TRN.write_tmp_page: temp seg write error ");
      exit (1);
    }
  cost += 1;
}

char *
get_new_tmp_page (struct A *ppage, u2_t pn)
{
  char *asp;

  asp = xmalloc (BD_PAGESIZE);
  ppage->p_shm = asp;
  ppage->p_sn = NRSNUM;
  ppage->p_pn = pn;
  return (asp);
}

char *
get_tmp_page (struct A *ppage, u2_t pn)
{
  char *asp;

  asp = ppage->p_shm;
  if (asp == NULL)
    asp = get_new_tmp_page (ppage, pn);
  read_tmp_page (pn, asp);
  return (asp);
}

void
put_tmp_page (struct A *ppage, char type)
{
  if (type == 'm')
    write_tmp_page (ppage->p_pn, ppage->p_shm);
}

void
BUF_endop (void)
{
  struct msg_buf sbuf;

  sbuf.mtype = ENDOP;
  __MSGSND(msqidb, &sbuf, 0, 0,"TRN.msgsnd: ENDOP to BUF");
  assert (N_AT_SEG == 0);
}

void
error (char *s)
{
  printf ("error: %s\n", s);
  exit (0);
}
