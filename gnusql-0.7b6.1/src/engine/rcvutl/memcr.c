
/*  memcr.c  - Memory Crash Recovery Utility
 *             Kernel of GNU SQL-server. Recovery utilities    
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

/* $Id: memcr.c,v 1.248 1998/09/30 02:39:04 kimelman Exp $ */

#include "setup_os.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#if HAVE_UNISTD_H 
#include <unistd.h>
#endif
#if HAVE_FCNTL_H 
#include <fcntl.h>
#endif

#include <sys/ipc.h>
#include <sys/msg.h>

#include <assert.h>

#include "destrn.h"
#include "strml.h"
#include "admdef.h"
#include "adfstr.h"
#include "fdclrcv.h"
#include "xmem.h"

key_t keylj, keymj, keybf, keymcr;
i4_t msqidmcr;
i4_t fdmj, fdcurlj;

extern CPNM curcpn;
extern i2_t maxscan;
extern char *ljpage;
extern struct ADREC bllj;
extern i4_t idtr;
extern struct ADBL adlj;

extern i4_t ljmsize;
extern struct ldesind **TAB_IFAM;
extern i4_t TIFAM_SZ;
extern char **scptab;
extern i4_t minidnt;
extern u2_t trnum;
extern u2_t S_SC_S;
extern i4_t msqidl, msqidm, msqidb;

static char mjpage[RPAGE];

static int
mrllb_tr(i4_t cidtr, struct ADBL adfix, struct ADBL cadlj, u2_t blsz, char *a,
         char type)
{
  i4_t newidtr;

  idtr = cidtr;
  if (cadlj.npage <= adfix.npage && cadlj.cm <= adfix.cm)
    {
      cadlj.npage = t2bunpack (a);
      a += size2b;
      cadlj.cm = t2bunpack (a);
      a += size2b;
      backactn (blsz, a, type);
    }
  while (cadlj.npage >= adfix.npage || cadlj.cm > adfix.cm)
    {
      rcv_LJ_GETREC (&bllj, &cadlj);
      a = bllj.block;
      type = *a++;
      if (type == GRLBLJ)
	continue;
      if (type == EOTLJ)
	{
	  getmint (a, bllj.razm - size1b);
	  continue;
	}
      newidtr = t4bunpack (a);
      a += size4b;
      cadlj.npage = t2bunpack (a);
      a += size2b;
      cadlj.cm = t2bunpack (a);
      if (cadlj.cm == 0)
	return (0);
    }
  r_tr (cadlj);
  return (0);
}

static void
mrollb_nftr(struct ADBL adfix, struct ADBL adstart, char *nftr, u2_t n)
{
  register u2_t i, NFTRANS, blsz;
  register char *a, type;
  struct ADBL cadlj;
  i4_t cidtr, newidtr;
  char *pnt, mas[RPAGE];

  NFTRANS = n / size4b;
  for (i = 0; i < NFTRANS; i++, nftr += size4b)
    {
      cidtr = t4bunpack (nftr);
      newidtr = cidtr;
      do
	{
	  cadlj = adstart;
	  blsz = LJ_prev (fdcurlj, &cadlj, &pnt, mas);
	  a = pnt;
	  type = *a++;
	  if (type == EOTLJ)
	    {
	      getmint (a, blsz - size1b);
	      continue;
	    }
	  newidtr = t4bunpack (a);
	  a += size4b;
	}
      while (newidtr != cidtr);
      mrllb_tr (cidtr, adfix, cadlj, blsz, a, type);
    }
}

#define GET_OFFBEG()     \
{                             \
  if (N == 1)                 \
    offbeg = RTPAGE + RTJOUR; \
  else                        \
    offbeg = RTPAGE;          \
}

static int
MJ_prev (i4_t fd, struct ADBL *adj, char **pnt, char *mas)
{
  u2_t N, offbeg, n, blsz, n1;
  char *end_of_record;

  N = adj->npage;
  n = adj->cm;
  GET_OFFBEG();
  if (n == offbeg && N == 1)
    {
      *pnt = mjpage + RTPAGE;
      return (0);
    }
  n -= offbeg;
  if (n < RTBLK)
    {				/* the endtop-block places in two pages*/
      char buff[size2b];
      n1 = RTBLK - n;
      bcopy (mjpage + RTPAGE, buff + n1, n);
      N--;
      read_page (fd, N, mjpage);
      bcopy (mjpage + RPAGE - n1, buff, n1);
      blsz = t2bunpack (buff);
      end_of_record = mjpage + RPAGE - n1;
      GET_OFFBEG();
      n = RPAGE - offbeg - n1;
    }
  else
    {				/* the top-block places in (N)-page */
      end_of_record = mjpage + offbeg + n - RTBLK;
      blsz = t2bunpack (end_of_record);
      n -= RTBLK;
      if (n == 0)
        {
          N--;
          read_page (fd, N, mjpage);
          GET_OFFBEG();
          n = RPAGE - offbeg;
          end_of_record = mjpage + RPAGE;
        }
    }
  assert (blsz < RPAGE);
  if (n < blsz)
    {               /* block-record places in two page */
      n1 = blsz - n;
      bcopy (mjpage + RTPAGE, mas + n1, n);
      N--;
      read_page (fd, N, mjpage);
      GET_OFFBEG();
      bcopy (mjpage + RPAGE - n1, mas, n1);
      *pnt = mas;
      n = RPAGE - offbeg - n1;
    }
  else
    {
       *pnt = end_of_record - blsz;
       n -= blsz;
       if (n == 0 && N != 1)
         {
           bcopy (*pnt, mas, blsz);
           *pnt = mas;
           N--;
           read_page (fd, N, mjpage);
           GET_OFFBEG();
           n = RPAGE - offbeg;
         }
    }
  adj->npage = N;
  adj->cm = n + offbeg;
  return (blsz);
}

static void
get_last_page (i4_t fd, char *jpage, u2_t *pn)	
{     /* Search the last page of a journal */
  i4_t PCP, RFILE;
  u2_t N;
  i4_t NV;
  struct stat buf;

  if (fstat (fd, &buf) < 0)
    {
      perror ("MCR.get_last_page: fstat");
      exit (1);
    }
  RFILE = buf.st_size / (RPAGE);
  N = *pn;
  read_page (fd, N, jpage);
  NV = t4bunpack (jpage);
  while (N != RFILE)
    {
      PCP = *(jpage + size4b + size2b);
      if (PCP == SIGN_NOCONT)
	break;
      read_page (fd, ++N, jpage);
      if (NV != t4bunpack (jpage))
	{
	  read_page (fd, --N, jpage);
	  break;
	}
    }
  *pn = N;
}

static struct ADBL 
recov_mj (char *mj_name)
{
  register char *a, *b, *d, *asp, *beg;
  struct ADBL adfix, cadmj;
  i4_t idm, cidm;
  u2_t sn, pn, fs, off, blsz, prevpn, size;
  i2_t shsize;
  char *pnt, type, prmod;
  char mas[RPAGE];
  struct A pg;

  if ((fdmj = open (mj_name, O_RDWR, 0644)) < 0)
    {
      perror ("MCR: open current MJ");
      exit (1);
    }
  pn = 1;
  get_last_page (fdmj, mjpage, &pn);
  cadmj.npage = pn;
  cadmj.cm = t2bunpack (mjpage + size4b);
  asp = NULL;
  for (prevpn = (u2_t) ~ 0, prmod = 'n'; (blsz = MJ_prev (fdmj, &cadmj, &pnt, mas)) > 0;)
    {
      a = pnt;
      while (blsz != 0)
	{
	  a += adjsize;
	  type = *a++;
          assert (type == OLD || type == SHF || type == COMBR || type == COMBL);
	  idm = t4bunpack (a);
	  a += size4b;
	  sn = t2bunpack (a);
          assert (sn != 0);
	  a += size2b;
	  pn = t2bunpack (a);
	  a += size2b;
	  if (pn != prevpn)
	    {
	      if (prevpn != (u2_t) ~ 0)
		putpg (&pg, prmod);
	      asp = getpg (&pg, sn, pn, 'x');
	      prmod = 'n';
	      prevpn = pn;
	    }
	  cidm = ((struct p_head *) asp)->idmod;
	  if (idm <= cidm)
	    prmod = 'm';
	  off = t2bunpack (a);
	  a += size2b;
	  fs = t2bunpack (a);
	  a += size2b;
	  size = adjsize + 1 + size4b + 4 * size2b;
	  if (type == OLD)
	    {
	      if (idm <= cidm)
                bcopy (a, asp + off, fs);
              a += fs;
	      size += fs;
	    }
	  else
	    {
	      size += size2b;
              shsize = t2bunpack (a);
              a += size2b;
              if (type == SHF)
                {
                  if (idm <= cidm)
                    {
                      beg = asp + off;
                      if (shsize < 0)
                        {
                          shsize = -shsize;
                          bcopy (beg + shsize, beg, fs);
                        }
                      else
                        {
                          for (b = beg + fs - 1, d = b - shsize; fs != 0; fs--)
                            *b-- = *d--;
                        }
                    }
                }
	      else 
		{
		  if (idm <= cidm)
		    {
                      beg = asp + off;
                      if (type == COMBR)
                        {
                          b = beg - fs;
                          bcopy (b - shsize, b, fs);
                        }
                      else
                        {
                          for (b = beg + fs - 1, d = b + shsize; fs != 0; fs--)
                            *d-- = *b--;
                        }
                      bcopy (a, beg, shsize);
		    }
                  size += shsize;
                  a += shsize;
		}
	    }
	  blsz -= size;
	}
    }
  if (prevpn != (u2_t) ~ 0)
    putpg (&pg, prmod);
  bcopy (pnt, (char *) &adfix, adjsize);
  close (fdmj);
  return (adfix);
}

static void
ini_mcru (void)
{
  struct ADF adf;
  i4_t fdaf;

    /* Getting queue for MCR */
  if ((msqidmcr = msgget (keymcr, IPC_CREAT | DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("ADM: Getting queue for MCR");
      exit (1);
    }
  if ((msqidm = msgget (keymj, DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("MCR.msgget: Get queue for MJ");
      exit (1);
    }

  if ((msqidl = msgget (keylj, DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("MCR.msgget: Get queue for LJ");
      exit (1);
    }

  if ((msqidb = msgget (keybf, DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("MCR.ini_mcru.msgget: Get queue for BUF");
      exit (1);
    }
  if ((fdaf = open (ADMFILE, O_RDWR, 0644)) < 0)
    {
      perror ("ADMFILE: open error");
      exit (1);
    }
  if (read (fdaf, (char *) &adf, sizeof (struct ADF)) != sizeof (struct ADF))
    {
      perror ("ADMFILE: read error");
      exit (1);
    }
  minidnt = 1;
  close (fdaf);
}

static void
mfrwrd_mtn (struct ADBL cadlj, struct ADBL last_adlj, char *idtr_pnt, u2_t n)
{
  char *a, type, *cur_idtr_pnt, *eo_idtr_pnt;
  u2_t blsz;
  i4_t cidtr;
  char *pnt, mas[RPAGE];

  eo_idtr_pnt = idtr_pnt + n;
  while (cadlj.npage != last_adlj.npage || cadlj.cm != last_adlj.cm)
    {
      blsz = LJ_next (fdcurlj, &cadlj, &pnt, mas);
      a = pnt;
      type = *a++;
      if (type == CPRLJ)
        continue;
      if (type == EOTLJ)
	{
	  getmint (a, blsz - size1b);
	  continue;
	}
      idtr = t4bunpack (a);
      for (cur_idtr_pnt = idtr_pnt; cur_idtr_pnt < eo_idtr_pnt; cur_idtr_pnt += size4b)
        {
          cidtr = t4bunpack (cur_idtr_pnt);
          if (cidtr == idtr)
            goto end;
        }
      forward_action (a + size4b, type, blsz - ljmsize);
    end:;
    }
}

int
main (int argc, char **argv)
{
  register char *a;
  u2_t n, blsz;
  struct ADBL cadlj, ladlj, adfix;
  char type, *pnt, mas[RPAGE];
  i4_t preot = 0, repl;
  u2_t pn, s_sc_s;
  struct msg_buf sbuf;

  MEET_DEBUGGER;
  
  setbuf (stdout, NULL);
  fprintf (stderr, "MCR.main.b: Memory Crash Recovery Utility is running\n");
  sscanf (argv[3], "%d", &keymj);
  sscanf (argv[4], "%d", &keylj);
  sscanf (argv[5], "%d", &keybf);
  sscanf (argv[6], "%d", &keymcr);
  {
    i4_t x;
    sscanf (argv[7], "%d", &x);
    s_sc_s=x;
  }
  trnum = CONSTTR;
  S_SC_S = s_sc_s;
  
  TIFAM_SZ = 2;
  TAB_IFAM = (struct ldesind **) xmalloc (TIFAM_SZ * sizeof (struct ldesind **));
  TAB_IFAM[0] = NULL;
  TAB_IFAM[1] = NULL;
  tab_difam (1);
  ljmsize = size1b + size4b + 2 * size2b + 2 * size2b + size2b + size4b + 2 * size2b;
  scptab = (char **) xmalloc (chpsize * maxscan);
  for (n = 0; (i2_t) n < maxscan; n++)
    scptab[n] = NULL;
  

/*------------Recovery by MICROJ------------------*/

  ini_mcru ();
  adfix = recov_mj (argv[1]);
  if (adfix.cm == 0)
    goto end;

/*------------Recovery by LOGJ------------------*/

/* The search of the last EOTLJ from the end of LJ */
/* If EOTLJ is after adfix, rollback unfinished transactions */

  if ((fdcurlj = open (argv[2], O_RDWR, 0644)) < 0)
    {
      perror ("MCR: open current LJ");
      exit (1);
    }
  pn = adfix.npage;
  get_last_page (fdcurlj, ljpage, &pn);
  adlj.npage = pn;
  adlj.cm = t2bunpack (ljpage + size4b);
  cadlj = adlj;
  while ( cadlj.npage != adfix.npage || cadlj.cm != adfix.cm)
    {
      blsz = LJ_prev (fdcurlj, &cadlj, &pnt, mas);
      a = pnt;
      type = *a++;
      if (type == EOTLJ)
	{
	  preot = 1;
	  ladlj = cadlj;
	  getmint (a, blsz - size1b);
	  n = blsz - size1b;
	  mrollb_nftr (adfix, cadlj, a, n);
          mfrwrd_mtn (adfix, ladlj, a, n);
	  break;
	}
    }
  if (preot == 0)
    ladlj = bmtn_feot (fdcurlj, adfix);
  rcv_wmlj (&ladlj);		/* The record about global rollback */
  rcv_ini_lj ();
  repl = LOGJ_FIX ();
  dubl_segs ();
end:
  /*  ini_mj (argv[1]);*/
  sbuf.mtype = ANSADM;
  sbuf.mtext[0] = 0;
  if (msgsnd (msqidmcr, (MSGBUFP)&sbuf, 1, 0) < 0)
    {
      perror ("MCR.msgsnd: Answer ADM");
      exit (1);
    }

  fprintf (stderr, "MCR: Memory Crash Recovery has finished successfully\n");
  sleep (1);
  return 0;
}
