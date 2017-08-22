/*
 * adm.c - Storage and Transaction Synchrohization Management 
 *         System Administrator of GNU SQL server
 *
 * This file is a part of GNU SQL Server
 *
 * Copyright (c) 1996, 1997, Free Software Foundation, Inc
 * Developed at the Institute of System Programming 
 * This file is written by Vera Ponomarenko
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
 *
 */

/* $Id: adm.c,v 1.256 1998/08/22 04:15:33 kimelman Exp $ */

#include <setup_os.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/times.h>

#ifdef HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif

#ifdef HAVE_SYS_MSG_H
#include <sys/msg.h>
#endif

#if STDC_HEADERS
#include <string.h>
#endif

/* for uname */
#if HAVE_UNAME
#include <sys/utsname.h>
#endif

#include <signal.h>
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

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

#if HAVE_FCNTL_H 
#include <fcntl.h>
#endif

#include "dispatch.h"
#include "global.h"
#include "rnmtp.h"
#include "pupsi.h"
#include "inpop.h"
#include "expop.h"
#include "adfstr.h"
#include "rcvutl/puprcv.h"
#include "totdecl.h"

#define GDBINIT_FILE ".gdb"
#define EXIT  finit(1)

#define PRINT(x, y) PRINTF((x,y));

/* maximum of time for waiting exiting of all transactions */
#define KILL_WAIT 10 

#define nul     0xffff
#define EOTLJ   0		/* end of transaction */

#include "strml.h"

struct msg_buf  sbuf;
struct msg_buf  rbuf;
struct des_trn *tabtr = NULL;


#define TRN_SEND_NEED
#include "admdef.h"
#undef TRN_SEND_NEED

#include "gspstr.h"

extern int errno;
extern i4_t default_num;

debug_pid_t volatile *debuggers_pids = NULL;

i4_t sbuf_sz, rbuf_sz;
pid_t adm_pid;          /* proc id of administrator                       */

char nocrtr_fl = 0;     /* = 1 if dispatcher may not create a transaction */
char finit_fl  = 0;     /* = 1 if after last transaction finish           */
		        /*     dispatcher ought to exit                   */
char fix_lj_fl = 0;     /* = 1 if after last transaction finish           */
		        /*     dispatcher ought to fix log journal        */
char *fix_path = NULL;  /* path to fix log journal                        */

key_t keyadm, keylj, keymj, keybf, keymcr, keysn, *keytrn = NULL;
i4_t   fdlj, fdaf, fdto;
volatile pid_t pidlj = 0, pidmj = 0, pidbf = 0, pidsn = 0, pidmcr = 0;

volatile i4_t finit_done = 0;
volatile i4_t children   = 0;

i4_t msgida = -1, msgidl = -1, msgidm = -1, msgidb = -1;
i4_t msgidc = -1, msgids = -1;


#define  adfsize sizeof(struct ADF)

struct ADF p2;
u2_t  *trn_ids = NULL;		/* transaction numbers       */
i4_t   transaction_number;      /* active transaction number */
i4_t   maxtrans, maxusetran;
i4_t   sizext;			/* extent-size in block      */
u2_t   sizesc;			/* scale-size in block       */
i4_t   uidconst;
i4_t   unname;
i4_t   nunname;
i4_t   uidt;
i4_t   idt;
i4_t   minid;
i4_t   numtr;
i4_t   rjzm, rjzl, mpagem, mpagel;
i4_t   vnlj;

extern void   sig_usr_hnd  __P((i4_t sig_num));
extern i4_t   cp_lj_reg  __P((i4_t to_sz, char *to));
extern void   finit  __P((i4_t err));
static i4_t   ini  __P((void));
void          kill_all (i4_t err);

static void
adm_exit(void)
{
  finit(0);
}

int
main (int argc,char *argv[])
{
  struct sigaction act;
  sigset_t set;

  printf ("\n   GNU SQL server (version %s on %s)",VERSION,HOST);
  printf ("\n   Copyright (c) 1996, 1997 Free Software Foundation, Inc.\n\n");
  
  fix_adm_port(argv[1]); /* argv[1] is adm rpc program identifier */
  
  adm_pid = getpid();
  
  sigemptyset (&set);
  sigaddset (&set, SIGUSR1);
  sigaddset (&set, SIGCHLD);
  
  ATEXIT(adm_exit);
  
#if 0 /*SA_SIGINFO*/
  act.sa_handler   = NULL;
  act.sa_mask      = set;
  act.sa_flags     = SA_SIGINFO;
  act.sa_sigaction = sig_usr_hnd;
#else
  act.sa_handler   = sig_usr_hnd;
  act.sa_mask      = set;
  act.sa_flags     = 0;
#endif
  
  if (sigaction (SIGUSR1, &act, NULL) ||
      sigaction (SIGCHLD, &act, NULL) ||
      sigaction (SIGALRM, &act, NULL)   )
    EXIT;
  
  sbuf_sz = rbuf_sz = BD_PAGESIZE;
  
  setbuf (stdout, NULL);
  
  ini ();

  printf ("\n<<<<<  Server is ready >>>>>>\n");
  ADM_RPC_START; /* rpcgen generated main */
  /* unreachable */
  return 0;
} /* main */

static void 
iamm (u2_t num) /* I'm modifying transaction */
{
  tabtr[num].uidtr = uidt++;
  if (uidt == idt)
    {
      idt += uidconst;
      p2.uid = idt;
      if (lseek (fdaf, 0, SEEK_SET) < 0)
	{
	  perror ("ADM.lseek: lseek admfile");
	  return;
	}
      if (write (fdaf, (char *) &p2, adfsize) != adfsize)
	{
	  perror ("ADMFILE");
	  return;
	}
    }
}

static void 
uniqnm (u2_t num)
{
  unname++;
  if(unname == nunname)
    {
      nunname += uidconst;
      p2.unname= unname;
      if(lseek(fdaf,0,SEEK_SET) <0 )
        {
          perror("ADM.lseek: lseek admfile");
          exit(1);
        }
      if(write(fdaf,(char *)&p2,adfsize) !=adfsize)
        {
          perror("ADMFILE");
          exit(1);
        }
    }
  PRINTF (("ADM.uniqnm.e: NUM=%d,UNNAME=%d\n", num, (i4_t)unname));
}

/* ? ? write to logic log "end of transaction" mark.
 *    "num" - transaction identifier
 */
static void 
wljetr (u2_t num)
{
  i4_t i, blsz;
  char *pnt;
  i4_t uid;

  PRINTF (("ADM.wljet.b: numtr=%d, FIR_TRN_NUM = %d, maxusetran = %d\n",
          num, FIR_TRN_NUM, maxusetran));
  t2bpack (num + CONSTTR, sbuf.mtext);
  pnt = sbuf.mtext + sizeof(u2_t) + sizeof(u2_t);
  *pnt++ = EOTLJ;
  for (i = FIR_TRN_NUM, blsz = 1; i <= maxusetran; i++)
    {
      uid = tabtr[i].uidtr;
      if ((uid != 0) && (i != num))
	{
          t4bpack (uid, pnt);
          pnt +=sizeof(i4_t);
          blsz += sizeof(i4_t);
	}
    }
  sbuf.mtype = PUTOUT;
  t2bpack (blsz, sbuf.mtext + sizeof(u2_t));
  if (msgsnd (msgidl, (MSGBUFP)&sbuf, sizeof(u2_t) + sizeof(u2_t) + blsz, 0) < 0)
    {
      perror ("ADM.msgsnd: LOGJ->PUTOUT");
      return;
    }
  PRINTF (("ADM.wljet.e: recsize=%d\n", blsz));
} /* wljetr */

#define NUMTR_DEC \
{ numtr--; if (maxusetran==numtr) for(;!(tabtr[maxusetran].idprtr);maxusetran--);}

void 
endotr (i2_t num,i4_t exit_code)
{
  i4_t i, msgid;
  struct msqid_ds msqidds;
  i4_t all_done;
  i4_t pr = 0;

  if(exit_code)
    {
      PRINT ("ADM.endotr: some recovery for trn %d must be here\n", num);
      kill_all(-1); /* stops everything and restart services with MCR */
    }
  if (tabtr[num].uidtr == minid)
    { /* if fiinished transaction has minID identifier */
      tabtr[num].uidtr = 0;
      for (i = FIR_TRN_NUM; i <= maxusetran; i++)	/* search new minID */
        if (tabtr[i].uidtr != 0)
          {
            minid = tabtr[i].uidtr;
            for (; i <= maxusetran; i++)	/* search new minID */
              if (tabtr[i].uidtr < minid)
                minid = tabtr[i].uidtr;
            pr = 1;
            break;
          }
      if (pr == 0)
        minid = uidt;
      
      for (i = FIR_TRN_NUM; i <= maxusetran; i++) /*sending of min identifier*
				    	           *of transaction to all    *
					           *modifying transactions   */
	if (tabtr[i].uidtr != 0)
	  {
	    t4bpack (minid, sbuf.mtext);
	    TRN_SEND (GETMIT, sizeof(i4_t));
	  }
    }
  
  if ((msgid = tabtr[num].msgidtrn))
    {
      msgctl (msgid, IPC_RMID, &msqidds);
      tabtr[num].msgidtrn = 0;
      keytrn[num] = 0;

    }
  
  PRINTF (("ADM.endotr: TRN N=%d uniq_name = %d is killed\n",
          num, (i4_t)tabtr[num].uidtr));
  tabtr[num].idprtr = 0;
  NUMTR_DEC;
  
  /* RPC-Unregistration of transaction server */
  SVC_UNREG(DEFAULT_TRN +num, 1); /* Number of version of interpretator & *
                                   * compiler must = 1                    */
  
  tabtr[num].cretime = 0;
  if (nocrtr_fl)
    {
      all_done = 1;
      
      for (i = FIR_TRN_NUM; i <= maxusetran; i++)
	if (tabtr[i].idprtr)
	  if (tabtr[i].cretime)
	    {
	      all_done = 0;
	      break;
	    }
      
      if (all_done)
	{
          nocrtr_fl = 0;
	  if (fix_lj_fl)
	    copylj();
	  if (finit_fl)
	    finit(0);
	}
    }
  transaction_number--;

  PRINT ("ADM.endotr.e: NUM=%d\n", num);
  PRINT ("ADM.endotr.e: NUMTR=%d\n", numtr);
}


static void 
errfu (char c, char tperr)
{
  char pr;
  PRINTF (("ADM.ERRFU.b: c=%c\n", c));
  switch (c)
    {
    case 'm':
      pr = MJ_PPS;
      break;
    case 'l':
      pr = LJ_PPS;
      break;
    case 'b':
      pr = BF_PPS;
      break;
    case 's':
      pr = SN_PPS;
      break;
    default:
      break;
    }
  /* OUT_ERR(tperr,pr,0); */
}

static void 
copy (void)
{
  char file_name[1024];
  char arr[32];

  PRINTF (("ADM.COPY.b:\n"));

  sprintf (arr, "%d", (i4_t)vnlj);
  strcpy (file_name, ARCHIVE);
  strcat (file_name, "/ljour.");
  strcat (file_name, arr);
  
  cp_lj_reg (strlen (file_name), file_name);

  PRINTF (("ADM.COPY.e: %s\n", file_name));
}

static void 
nobuf (void)
{
  printf ("ADM.NOBUF: BUF doesn't get buffer\n");
}

void 
realop (i4_t op, u2_t num, char *b)
{
  sigset_t set;
  
  PRINT ("ADM.realop: op  = %d\n", op )
  /* sleep (2);   @ need to delete @ */
  PRINT ("ADM.realop: NUM = %d\n", (i4_t)num)
    
  if (!op)
    return;
  
  PRINT ("ADM.realop: op  = %d\n", op )
   sigprocmask (0, NULL, &set);
  PRINT ("ADM.realop: presence flag of SIGUSR1: %d\n", sigismember(&set, SIGUSR1)) 
  
  switch (op)
    {
    case RES_READY :
      if (tabtr[num].res_ready == 2) /* it was request from client */
	tabtr[num].res_ready = 1;
      break;
    
    case IAMMOD:
      iamm (num);
      sbuf.mtype = num + CONSTTR;
      t4bpack (tabtr[num].uidtr, sbuf.mtext);
      if (msgsnd (tabtr[num].msgidtrn, (MSGBUFP)&sbuf, sizeof(i4_t), 0) < 0)
	{
	  perror ("ADM.msgsnd: Answer to IAMMOD");
	  return;
	}
      break;
    case UNIQNM:
      uniqnm (num);
      sbuf.mtype = num + CONSTTR;
      t4bpack (unname, sbuf.mtext);
      if (msgsnd (tabtr[num].msgidtrn, (MSGBUFP)&sbuf, sizeof(i4_t), 0) < 0)
	{
	  perror ("ADM.msgsnd: Answer to UNIQNM");
	  return;
	}
      break;
    case WLJET:
      wljetr (num);
      break;
    case ERRFU:
      errfu (*b, *(b + 1));
      break;
    case COPY:
      copy ();
      break;
    case NOBUF:
      nobuf ();
      break;
    case READY:
      break;
    default:
      break;
    }
}

#define TO_TABTR(pid, msgid)  \
  { tabtr[numtr].uidtr = 0;     \
    tabtr[numtr].idprtr = pid;          \
    tabtr[numtr].res_ready = 0;         \
    tabtr[numtr].cretime = time(NULL);  \
    tabtr[numtr++].msgidtrn = msgid;    \
    maxusetran++; \
  }

#define ARG(num, what) sprintf (args[num], "%d", (i4_t)(what))

static int
logj_fix (void)
{
  sbuf.mtype = BEGFIX;
  if (msgsnd (msgidl, (MSGBUFP)&sbuf, 0, 0) < 0)
    {
      perror ("ADM.msgsnd: LOGJ->FIX");
      return -1;
    }
  if (msgrcv (msgidl, (MSGBUFP)&sbuf, 1, ANSLJ, 0) < 0)
    {
      perror ("ADM.msgrcv: BEGFIX LJ");
      return -1;
    }
  return ((i4_t) *sbuf.mtext);
}

static pid_t
fork_service(char **args,i4_t take_care_of_fail)
{
  pid_t chld;
  
#ifndef NOT_DEBUG
  { 
    char **arg;
    fputs ("##>>>",stderr);
    for(arg=args;*arg;arg++)
      fprintf(stderr,"%s ",*arg);
    fputs ("\n",stderr);
  }
#endif

  children++;
  
  chld = fork();
  if (chld < 0) /* if fork failed */
    {
      lperror("fork '%s'",*args);
      if (take_care_of_fail)  
        return chld;          /* return error code          */
      EXIT;
    }
  else if (chld==0)     
    { /* child process -- replace by '*args' */
      execvp (*args, args);
      lperror ("ADM.fork_service: '%s' doesn't exec",*args);
      exit(1);
    }
  return chld;
}

#ifndef NOT_DEBUG

static void
run_debugger (pid_t debuggee, char **args,char *to_debugger,char *x_server)
{
  pid_t  debugger;
  char * prog = *args;
  i4_t    i;
  
  { /* prepare debugger script -- written generally for gdb */
    FILE * fl;

    fl = fopen (GDBINIT_FILE, "w");
    fprintf (fl,
             "attach %d\n"                  /* attach at MEET_DEBUGGER     */
             "break exit\n"
             "break abort\n"
             "break lperror\n"
             "%s\n"                         /* set local settings          */
             "break wait_debugger\n"        /* clear wait flag             */
             "cont\n"                       /* lets catch it by debugger   */
             "set status = 0\n"
             "finish\n"                     /* go up to 'main'             */
             ,(i4_t)debuggee
             ,(to_debugger?to_debugger:""));
    fclose (fl);
  }
  
  if (!x_server)
    x_server = getenv("DISPLAY");
  if (!x_server)
    x_server = ":0.0";

  args[i=1] = "-display";
  args[++i] = x_server;
  args[++i] = "-bg";
  args[++i] = "Gray";
#if defined(HAVE_DDD)
  args[0] = HAVE_DDD;
  args[++i] = prog;
  args[++i] = "-nx";
  args[++i] = "-command=" GDBINIT_FILE;
#elif defined(HAVE_XXGDB)
  args[0] = HAVE_XXGDB;
  args[++i] = "-nx";
  args[++i] = "-command=" GDBINIT_FILE;
  args[++i] = prog;
#elif  defined(HAVE_XTERM)
  args[0] = HAVE_XTERM;
  args[++i] = "-e";
#if defined(HAVE_GDB)
  args[++i] = HAVE_GDB;
#elif defined(HAVE_DBX) 
  args[++i] = HAVE_DBX;
#else
# error configuraton - !NOT_DEBUG && !HAVE_GDB && !HAVE_DBX 
#endif
  args[++i] = prog;
  args[++i] = "-x";
  args[++i] = GDBINIT_FILE;
#else
# error configuraton - !NOT_DEBUG && !HAVE_XTERM && !HAVE_DDD && !HAVE_XXGDB 
#endif
  args[++i] = NULL;

  debugger = fork_service(args,(to_debugger!=NULL));
  if (debugger>0)
    { /* add new entry to debuggers list */
      debug_pid_t *new_d = xmalloc (sizeof(debug_pid_t));
      new_d->debugger = debugger;
      new_d->to_debug = debuggee;
      new_d->next = debuggers_pids;
      if (debuggers_pids)
        debuggers_pids->prev = new_d;
      debuggers_pids = new_d;
    }
}

#endif /* NOT_DEBUG */

static pid_t
run_chld (i4_t run_gdb,char **args,char *to_debugger,char *x_server)
{
  pid_t  chld;

#ifndef NOT_DEBUG
  if (run_gdb<0)
    {
      i4_t c;
      printf ("DO you want to start gdb for '%s' ? ##>>>>",*args);
      do 
        c = getchar();
      while ( c == '\n' );
      if (c != 'y')
        run_gdb = 0;
    }
  if (run_gdb)
    { /* add debugger keyword */
      char **p;
      for ( p = args; *p; p++ );
      *p = "DebugIt";
      *(++p) = NULL;
    }  
#endif

  chld = fork_service(args,(to_debugger!=NULL));
#ifndef NOT_DEBUG
  if (run_gdb && chld > 0)
    run_debugger (chld,args,to_debugger,x_server);
#endif
  
  return chld;
}

static key_t
get_msg_key(void)   /* this function has a complete static copy in strgcr.c */
{
  static key_t k = (key_t)-2;
  if (k==(key_t)-2)
#if HAVE_FTOK
    k = ftok(MJ, 'D');
#else
    k = FIRSTMQN;
#endif
  errno = 0;
  for(;;)
    {
      if ( msgget (k, DEFAULT_ACCESS_RIGHTS) < 0 )
        {
        if (errno == ENOENT)
          return k++;
        else if (errno == ENOSPC)
          return -1;
        }
      k++;
    }
}

static i4_t 
ini (void)
{
  i4_t i;
  char rep, repmj, *args[20];
  i4_t optbufnum, maxnbuf, maxtact;
  key_t fsegnum;
  char mas[sizeof(i4_t)];
  char argarea[20][20];
  
#define INITARG for(i = 0 ; i < 20; i++) args[i] = argarea[i];

  rep = 0;
  
  
  /*------------OPEN,READ AND WRITE ADMFILE------------------*/
  PRINTF (("ADM.ini: Open, read and write ADMFILE %s\n", ADMFILE));
  if ((fdaf = open (ADMFILE, O_RDWR, 0644)) < 0)
    {
      perror ("ADMFILE: open error");
      EXIT;
    }
  if (read (fdaf, (char *) &p2, adfsize) != adfsize)
    {
      perror ("ADMFILE: read error");
      EXIT;
    }
  idt       = p2.uid;
  uidconst  = p2.uidconst;
  nunname   = p2.unname;
  sizext    = p2.sizext / BD_PAGESIZE;
  maxtrans  = p2.maxtrans;
  rjzm      = p2.mjred;
  rjzl      = p2.ljred;
  mpagem    = p2.mjmpage;
  mpagel    = p2.ljmpage;
  sizesc    = p2.sizesc;
  optbufnum = p2.optbufnum;
  maxnbuf   = p2.maxnbuf;
  maxtact   = p2.maxtact;
  fsegnum   = p2.fshmsegn;
  PRINTF (("ADM.ini: was reading from ADMFILE:\n"));
  PRINTF (("  UID=%d, UIDCONST=%d, MAXTRANS=%d, SIZEXT=%d "
	  "UNNAME=%d, RJZm=%d, RJZl=%d, MPAGEm=%d,"
	  " MPAGEl=%d, MAXNBUF=%d,  SIZESC=%d\n", 
	  (i4_t)idt, (i4_t)uidconst, maxtrans, sizext, 
	  (i4_t)nunname, rjzm, rjzl,
	  mpagem, mpagel, maxnbuf, sizesc));
  uidt = idt;
  if (uidt == 0)
    uidt++;
  idt += uidconst;
  p2.uid = idt;
  unname = nunname;
  if (unname == 0)
    unname++;
  nunname += uidconst;
  p2.unname = nunname;
  if (lseek (fdaf, 0, SEEK_SET) < 0)
    {
      perror ("ADM.lseek: lseek admfile");
      EXIT;
    }
  if (write (fdaf, (char *) &p2, adfsize) != adfsize)
    {
      perror ("ADMFILE: write error");
      EXIT;
    }
  
  keyadm = get_msg_key();
  keylj  = get_msg_key();
  keymj  = get_msg_key();
  keybf  = get_msg_key();
  keysn  = get_msg_key();
  keymcr = get_msg_key();
  
  /* Getting queue for administraror */
  if ((msgida = msgget (keyadm, IPC_CREAT | DEFAULT_ACCESS_RIGHTS)) < 0)
    {
      perror ("ADM: Getting queue for adm");
      EXIT;
    }
    
  keytrn = (key_t *) xmalloc (maxtrans * sizeof (key_t));
  
  tabtr = (struct des_trn *) xmalloc (maxtrans * sizeof (struct des_trn));
  trn_ids = (u2_t *) xmalloc (maxtrans * sizeof(u2_t)); /* all trn_ids[i] = 0 */
  minid = uidt;
  maxusetran = FIR_TRN_NUM;
  numtr = 1;
  
/*---------------INI MICROJ------------------------------*/
  INITARG;
  args[0] = MJ;
  args[1] = MJFILE;
  sprintf (args[2], "%d", rjzm);
  sprintf (args[3], "%d", mpagem);
  sprintf (args[4], "%d", (i4_t)keymj);
  sprintf (args[5], "%d", (i4_t)keylj);
  args[6] = NULL;
  pidmj = run_chld(-1,args,NULL,NULL);
/*---------------INI MICROJ 2 ----------------------------*/

  PRINTF (("ADM.ini: before MSG_INIT ini MJ keymj = %d\n", keymj));
  MSG_INIT (msgidm, keymj, "MJ");
  PRINTF (("ADM.ini: after MSG_INIT ini MJ\n"));
  if (msgrcv (msgidm, (MSGBUFP)&rbuf, 1, ANSMJ, 0) < 0)
    perror ("ADM.msgrcv: INIT MJ");
  repmj = *rbuf.mtext;
  PRINTF (("ADM.ini: INI repmj = %d\n", repmj));
  sbuf.mtype = INIMJ;
  if (msgsnd (msgidm, (MSGBUFP)&sbuf, 0, 0) < 0)
    {
      perror ("ADM.msgsnd: MJ->INIMJ");
      exit (1);
    }
  
  /*----------------INI BUFFER----------------------*/
  PRINTF (("ADM.ini: INI BUFFER>\n"));
  

  /*vvvvvvvvvvvvvvv  start BUF   vvvvvvvvvvvvvv*/
  INITARG;
  args[0] = BUF;
  ARG (1, keybf);
  ARG (2, keymj);
  ARG (3, optbufnum);
  ARG (4, maxnbuf);
  ARG (5, maxtact);
  ARG (6, maxtrans);
  /* the first numeric name of the   *
   * shared memory segment for PUPSI */
  ARG (7, fsegnum);
  /* BD segments numbers and names */
  ARG (8, 1);
  args[9] = SEG1;
  ARG (10, adm_pid);
  args[11] = NULL;
  pidbf = run_chld(-1,args,NULL,NULL);
  /*^^^^^^^^^^^^^^^^^^ start BUF ^^^^^^^^^^^^^^^^^^^^^*/

  PRINTF (("ADM.ini: INI BUF\n"));  
  MSG_INIT (msgidb, keybf, "BUF");
  
  /*-------------OPEN AND INI LOGJ--------------------------*/
  PRINTF (("ADM.ini: Open and INI LOGJ\n"));
  if ((fdlj = open (LJFILE, O_RDWR, 0644)) < 0)
    {
      perror ("LJ: open");
      EXIT;
    }
  if (read (fdlj, mas, sizeof(i4_t)) != sizeof(i4_t))
    {
      perror ("LJOUR: read error");
      EXIT;
    }
  vnlj = t4bunpack (mas);
  PRINTF (("ADM.ini: VNLJ=%d\n", (i4_t)vnlj));

  /*vvvvvvvvvvvvvvv start LJ  vvvvvvvvvvvvvvvvvvvvv*/

  INITARG;
  args[0] = LJ;
  sprintf (args[1], "%d", fdlj);
  sprintf (args[2], "%d", (i4_t)rjzl);
  sprintf (args[3], "%d", (i4_t)mpagel);
  sprintf (args[4], "%d", (i4_t)keylj);
  sprintf (args[5], "%d", (i4_t)keybf);
  sprintf (args[6], "%d", (i4_t)keyadm);
  sprintf (args[7], "%d", (i4_t)keymj);
  args[8] = NULL;
  pidlj = run_chld(-1,args,NULL,NULL);

  /*^^^^^^^^^^^^^^^ start LJ  ^^^^^^^^^^^^^^^^^^^^^*/

  MSG_INIT (msgidl, keylj, "LJ");

  /* LogJournal is the special (with number NUM_LJ) transaction */
  TO_TABTR (pidlj, msgidl);

  PRINTF (("ADM.ini: INI LOGJ done\n"));  
 
  
  if (repmj == MJ_PPS)
    {
      /* Memory crash, invoke Memory Crash Recovery Utility */
      PRINTF (("ADM.ini: load MCR MCR_name=%s, MJ_name = %s\n", MCR, MJ));
      
      /*vvvvvvvvvvvvvvv start MCR  vvvvvvvvvvvvvvvvvvvvv*/

      INITARG;
      args[0] = MCR;
      args[1] = MJFILE;
      args[2] = LJFILE;
      sprintf (args[3], "%d", (i4_t)keymj);
      sprintf (args[4], "%d", (i4_t)keylj);
      sprintf (args[5], "%d", (i4_t)keybf);
      sprintf (args[6], "%d", (i4_t)keymcr);
      sprintf (args[7], "%d", sizesc);
      args[8] = NULL;
      pidmcr = run_chld(-1,args,NULL,NULL);
      
      /*^^^^^^^^^^^^^^^ start MCR ^^^^^^^^^^^^^^^^^^^^^*/

      MSG_INIT (msgidc, keymcr, "MCR");
      if (msgrcv (msgidc, (MSGBUFP)&rbuf, 1, ANSADM, 0) < 0)
        perror ("ADM.msgrcv: Answer from MCR");
      repmj = *rbuf.mtext;
    }
  PRINTF (("ADM.ini: log-fix\n"));
  logj_fix ();

  /*---------------INI SYNCH-------------------------------*/
  PRINTF (("ADM.ini: INI SYN\n"));

  /*vvvvvvvvvvvvvvv start SYN vvvvvvvvvvvvvvvvvvvvv*/

  INITARG;
  args[0] = SYN;
  ARG (1, keysn);
  ARG (2, keybf);
  args[3] = NULL;
  pidsn = run_chld(-1,args,NULL,NULL);

  /*^^^^^^^^^^^^^^^ start SYN ^^^^^^^^^^^^^^^^^^^^^*/

  MSG_INIT (msgids, keysn, "SYN");

  transaction_number = 0;
  return (0);
}

static int
rpc_check(i4_t tr_num, rpc_svc_t type,int need_debug)
{
  static struct timeval tv = { 25, 0 };
  gss_client_t *cl = NULL;
  
  int     rc = 0,i=10;
  char   *hostname;
#if 0
  struct  utsname unm;
  uname(&unm);
  hostname = unm.nodename;
#else
  hostname = "localhost";
#endif

  while (i>0)
    {
      if (cl == NULL)
        {
          cl = get_gss_handle(hostname, DEFAULT_TRN + tr_num, 1, 1);
        }
      if ( cl != NULL )
        if ( gss_client_call(cl, IS_RPC_READY,
                             (xdrproc_t) xdr_void, (caddr_t) NULL,
                             (xdrproc_t) xdr_long, (caddr_t) &rc,
                             tv)
             == RPC_SUCCESS)
	break;
      if (tabtr[tr_num].idprtr == 0)
	break;
      fprintf(stderr,"%s:%s\n",hostname,clnt_error_msg());
      rc = 0;
      sleep(1);
      
      if(!need_debug)
        i--;
    }
  
  if (i>10)
    printf("%s:%s\n",hostname,clnt_error_msg());
  if (cl)
    drop_gss_handle(cl);
  if(rc>0)
    {
      assert( tabtr[tr_num].idprtr == (pid_t)rc);
    }
  return (i<=10);
}

/* New transaction with interpretator or compiler creating. *
 * Returns number of it in TABTR or error (< 0).            */
     
i4_t 
creatr (init_arg *arg)
{
  static char *svcs[] = SVC_FILES;
  int    i, j, trnum;
  pid_t idpr;
  char *args[20];
  char  argarea[20][20];

  PRINTF (("ADM.creatr.b: FIR_TRN_NUM = %d\n", FIR_TRN_NUM));
  for (i=0; i<20; i++)
    args[i] = argarea[i];
  
  /* tabtr cleaning & finding free place in it */
  for (trnum = 0, j = FIR_TRN_NUM; j <= maxusetran; j++)
    if (!(tabtr[j].cretime) && (!tabtr[j].idprtr) )
      {
        trnum = j;
        break;
      }
  
  if (!trnum) /* there is no free place in tabtr[1..MAXUSETRAN] */
    {
      trnum = ++maxusetran;
      if (numtr == maxtrans)
	{
	  keytrn = (key_t *) xrealloc (keytrn,
		   2 * maxtrans * sizeof (key_t));
	  for (i = maxtrans; i < 2 * maxtrans; i++)
	    keytrn[i] = 0;
      
	  maxtrans *= 2;
	  tabtr = (struct des_trn *) xrealloc (tabtr, 
		  maxtrans * sizeof (struct des_trn));
	}
    }
  {
    char *debug = 
      "break compiler_disconnect\n"
      "set   under_dbg=1\n";
    
    keytrn[trnum] = get_msg_key();
    assert(arg->type >=0);
    assert(arg->type < sizeof(svcs)/sizeof(char*));
    args[0]  = svcs[arg->type];
    ARG (1,    keytrn[trnum]);
    ARG (2,    keysn);
    ARG (3,    keylj);
    ARG (4,    keymj);
    ARG (5,    keybf);
    ARG (6,    trnum);
    ARG (7,    minid);
    ARG (8,    sizext);
    ARG (9,    sizesc);
    ARG (10,   arg->wait_time);
    ARG (11,   DEFAULT_TRN + trnum);
    args[12] = arg->user_name;
    ARG (13,   adm_pid);
    ARG (14,   keyadm);
    args[15] = NULL;
    
    idpr = run_chld(arg->need_gdb,args,debug,arg->x_server);
    if (idpr< 0)
      return -TRN_INIT;
  }
  
  numtr++;
  tabtr[trnum].uidtr = 0;
  tabtr[trnum].idprtr = idpr;
  tabtr[trnum].res_ready = 0;
  tabtr[trnum].cretime = time(NULL);
  MSG_INIT (tabtr[trnum].msgidtrn, keytrn[trnum], "TRN");
  if (!rpc_check(trnum, arg->type,arg->need_gdb))
    {
      if (tabtr[trnum].idprtr == idpr) /* if process still not initialized */
        {                              /* and still exist - kill it        */
          tabtr[trnum].idprtr = -idpr;
          kill(idpr,SIGKILL);          
          keytrn[trnum] = 0;
        }
      printf ("ADM.creatr: TRAN creation failed\n");
      return -TRN_INIT;
    }
  PRINTF (("ADM.creatr.e: trn number = %d\n", trnum));
  transaction_number++;
  return (trnum);
}

#undef ARG


void 
copylj (void)
{
  char file_name[1024];
  char arr[32];
  i4_t n, fd;
  char buf[1024];
  
  /*------------------- suspend LJ --------------------------*/
  
  n = logj_fix ();
  /** printf ("ADM.COPYLJ: after LOGJ.FIX fdlj=%d\n", fdlj); */

  /*---------------copying of Logic Log----------------------*/
 
  if (fix_path != NULL)
    {
      if (lseek (fdlj, 0, SEEK_SET) < 0)
	{
	  perror ("ADM.lseek: lseek ljfile");
	  return;
	}
      if ((n = read (fdlj, buf, 1024)) == 0)
        {
          perror ("LJOUR: read error");
          EXIT;
        }
      vnlj = t4bunpack (buf);
      sprintf (arr, "%d", (i4_t)vnlj);
      strcpy (file_name, ARCHIVE);
      strcat (file_name, "/ljour.");
      strcat (file_name, arr);
      if ((fd = creat (file_name, 0640)) < 0)
	{
	  perror (file_name);
	  return;
	}

      PRINTF (("ADM.COPYLJ: FIX_PATH=%s\n", file_name));
      write (fd, buf, n);
      while ((n = read (fdlj, buf, 1024)) != 0)
        {
          write (fd, buf, n);
        }
      close (fd);
    }

  /*----------------refreshment of Logic Log---------------------------*/
  sbuf.mtype = RENEW;
  t4bpack ((i4_t)fdlj, sbuf.mtext);
  t4bpack ((i4_t)rjzl, sbuf.mtext + sizeof(i4_t));
  if (msgsnd (msgidl, (MSGBUFP)&sbuf, 2 * sizeof (i4_t), 0) < 0)
    {
      perror ("ADM.msgsnd: LOGJ->RENEW");
      return;
    }
  if (msgrcv (msgidl, (MSGBUFP)&sbuf, 1, ANSLJ, 0) < 0)
    {
      perror ("ADM.msgrcv: Answer LOGJ->RENEW");
      return;
    }
  PRINTF (("ADM.COPYLJ.e: fdlj=%d\n", fdlj));
  fix_lj_fl = 0;
}

void 
kill_all (i4_t err)
{
  i4_t i;
  i4_t error;
  struct msqid_ds msqidds;
  static char entered = 0;
  static i4_t  err_code = 0;
  
  if (entered==0)
    {
      entered++;
      err_code = err;
      
      if (tabtr)
        {
          for (i = FIR_TRN_NUM; i <= maxusetran; i++)
            if (tabtr[i].idprtr)
              {
                msgctl (tabtr[i].msgidtrn, IPC_RMID, &msqidds);
                kill (tabtr[i].idprtr, SIGKILL);
                tabtr[i].idprtr = -tabtr[i].idprtr;
              }
        }
      
#define RMMSG(id) if (id!=-1){msgctl(id,IPC_RMID,&msqidds);id = -1;}
      RMMSG(msgidl);   /* lj  queue */
      RMMSG(msgidm);   /* mj  queue */
      RMMSG(msgidb);   /* buf queue */
      RMMSG(msgids);   /* syn queue ?*/
      RMMSG(msgida);   /* adm queue */
#undef RMMSG
      
#define KILLP(id) if (id){ error = kill(id, SIGKILL); id = -id; }
      KILLP(pidlj);
      KILLP(pidmj);
      KILLP(pidbf);
      KILLP(pidsn);
#undef KILLP
    }

  if (children)
    return; /* let's wait a bit more */
  
  err |= err_code;
#define CLEAR(ptr)  if(ptr) { xfree(ptr); ptr = NULL; }

  CLEAR(keytrn);
  CLEAR(trn_ids);
  CLEAR(tabtr);
  
#undef CLEAR

  printf ("ADM is stopped\n");
  if(err!=-1)
    {
      /* svc_exit(); */
      exit (err);
    }
  entered = finit_done = err_code = 0;
  ini();
} /* kill_all */

void
finit (i4_t err)
{
  i4_t rep, cd, i;
  
  /* This function can't use EXIT */
  
  if (finit_done)
    return;
  
  if (err < 0)
    kill_all(-1);

  finit_done = 1;

  
  if (tabtr)
    {
      time_t tm;
      i4_t all_done;
      
      for (i = FIR_TRN_NUM; i <= maxusetran; i++)
	if (tabtr[i].idprtr)
	  {
	    TRN_SEND (FINIT, 0);
	  }      
      tm = time (NULL);
      do {
	all_done = 1;
	for (i = FIR_TRN_NUM; i <= maxusetran; i++)
	  if (tabtr[i].idprtr)
	    {
	      all_done--;
	      break;
	    }
	if (all_done)
	  break;
	sleep (1);
      } while (time (NULL) - tm < KILL_WAIT);
    }
  
  PRINTF (("ADM.finit.b: UNNAME=%d \n", (i4_t)unname));
  p2.unname = unname + 1;
  if (lseek (fdaf, 0, SEEK_SET) < 0)
    perror ("ADM.lseek: lseek admfile");
  if (write (fdaf, (char *) &p2, adfsize) != adfsize)
    perror ("ADMFILE");
  rep = close (fdaf);
  fdaf = 0;
  
  if (pidlj)
    logj_fix ();
  
  sbuf.mtype = FINIT;
#if 0
  if(msgsnd(msgidl,&sbuf,0,0)<0) 
    { 
      perror("ADM.msgsnd: LOGJ->FINIT");
      exit(1); 
    } 
  if(msgrcv(msgidl,&rbuf,1,ANSLJ,0)<0) 
    { 
      perror("ADM.msgrcv: FINIT LJ"); 
      exit(1); 
    } 
#endif
  cd = close (fdlj);
  fdlj = 0;
  PRINTF (("ADM.finit: after LJ finish\n"));

  if (pidmj)
    {
      if (msgsnd (msgidm, (MSGBUFP)&sbuf, 0, 0) < 0)
	perror ("ADM.msgsnd: MJ->FINIT");
      if (msgrcv (msgidm, (MSGBUFP)&rbuf, 1, ANSMJ, 0) < 0)
	perror ("ADM.msgrcv: FINIT MJ");
      PRINTF (("ADM.finit: after MJ finish\n"));
    }

  if (pidbf)
    {
      if (msgsnd (msgidb, (MSGBUFP)&sbuf, 0, 0) < 0)
	perror ("ADM.msgsnd: BUF->FINIT");
      if (msgrcv (msgidb, (MSGBUFP)&rbuf, 1, ANSBUF, 0) < 0)
	perror ("ADM.msgrcv: FINIT BUF");
      PRINTF (("ADM.finit: after BUF finish\n"));
    }
  
  if (pidsn)
    {
      if (msgsnd (msgids, (MSGBUFP)&sbuf, 0, 0) < 0)
	perror ("ADM.msgsnd: SYN->FINIT");
      PRINTF (("ADM.finit: after SYN finish\n"));
    }
  
  sleep(3); /* let's give to subprocess a chance to stop by itself */
  printf ("ADM.finit.e:\n");
  kill_all (err); /* kill rest of them */
} /* finit */

void
dyn_change_parameters(void)
{
  i4_t fddp, length;
  struct DGSPARAM dyn_par;
  char *p;
  i4_t optbufnum, maxtact;  

  if ( (fddp = open (DYNPARS, O_RDWR, DEFAULT_ACCESS_RIGHTS)) <0)
    {
      perror ("DYNPARS: open error");
      exit (1);
    }
  if (lseek (fddp, 0, SEEK_SET) < 0)
    { 
      perror ("DYNPARS.lseek: lseek dynpars");
      exit (1);
    }
  length = sizeof (struct DGSPARAM);      
  if (read (fddp, (char *)&dyn_par, length) != length) {
    perror ("DYNPARSE: read error");
    exit (1);
  }      
  close (fddp);
  unlink (DYNPARS);
  
  rjzl = dyn_par.d_lj_red_boundary;
  optbufnum = dyn_par.d_opt_buf_num;
  maxtact = dyn_par.d_max_tact_num;

  sbuf.mtype = OPTNUM;
  p = sbuf.mtext;
  t4bpack (optbufnum, p);
  t4bpack (maxtact, p + sizeof(i4_t));
  if (msgsnd (msgidb, (MSGBUFP)&sbuf, 2 * sizeof(i4_t), 0) < 0)
    perror ("ADM.msgsnd: BUF->OPTNUM");
  printf ("ADM.dyn_change_parameters: new parameters has set\n");  
}
