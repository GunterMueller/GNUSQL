/*
 *  inpop.h  - Internal operation names of Kernel of GNU SQL-server 
 *
 * This file is a part of GNU SQL Server
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

#ifndef __inpop_h__
#define __inpop_h__

#include "pupsi.h"

#define     SZMSGBUF    (2*BD_PAGESIZE)

enum {
  /* Administrator */
  PUTEXT    =1,    /* TRN->ADM */
  IAMMOD    =2,    /* TRN->ADM */
  WLJET     =3,    /* TRN->ADM */
  ENDOTR    =4,    /* TRN->ADM */
  COPY      =5,    /* LJ->ADM */
  NOBUF     =6,    /* BUF->ADM */
  ERRFU     =7,    /* all->ADM */
  UNIQNM    =8,    /* TRN->ADM */
  RES_READY =9,    /* TRN->ADM */
  GETEXT   =10,    /* TRN->ADM */

  /* Answers for Administrator */
  ANSADM  =9,    /* ADM->all */
  READY  =11,    /* TRN ready for message receiving */

  /* Transaction */
  GETMIT  =1,    /* ADM->TRN: Get min transaction identifier */

  /* LJ */
  RENEW   =1,    /* ADM->LJ: Update */
  FIX     =2,    /* ADM->LJ: Fixation */
  PUTOUT  =3,    /* ADM->LJ: Record with push */
  STATE   =4,    /* ADM->LJ: Information about state LJ */
  PUTREC  =5,    /* TRN->LJ: Record */
  PUTHREC =6,    /* TRN->LJ: Record */
  GETREC  =7,    /* TRN->LJ: Get record */
  BEGFIX  =8,    /* ADM->LJ: Do fixation */
  INILJ   =9,    /* MCR->LJ: Do initialization */
  OVFLMJ  =10,   /* MJ->LJ: Get next record */
  /* Answers for LJ */
  ANSLJ   =15,    /* LJ->all */

  /* MJ */
  STATEMJ =1,    /* ADM->MJ: Information about state MJ */
  PUTBL   =2,    /* TRN->MJ */
  GETBL   =3,    /* TRN->MJ */
  OUTDISK =4,    /* BUF->MJ */
  DOFIX   =5,    /* BUF->MJ */
  INIMJ   =6,    /* ADM->MJ: Do initialisation */
  /* Answres for MJ */
  ANSMJ   =15,    /* MJ->all */

  /* Buffer */
  BEGTACT  =1,   /* ADM->BUF */
  INIFIXB  =2,   /* LJ->BUF: Start fixation */
  GETPAGE  =3,   /* TRN->BUF */
  PUTPAGE  =4,   /* TRN->BUF */
  UNLKPG   =5,   /* TRN->BUF */
  LOCKPAGE =6,   /* TRN->BUF */
  ENFORCE  =7,   /* TRN->BUF */
  ENDOP    =8,   /* TRN->BUF */
  OPTNUM   =9,   /* ADM->BUF */
  NEWGET  =10,   /* TRN->BUF */
  LOCKGET =11,   /* TRN->BUF */
  PUTUNL  =12,   /* TRN->BUF */
  /* Answers for BUF */
  ANSBUF  =15,   /* BUF->all: Answer message from BUF */

  /* Synchronizator */
  START  =1,  /* TRN->SYN */
  LOCK   =2,  /* TRN->SYN */
  SVPNT  =3,  /* TRN->SYN */
  UNLTSP =4,  /* TRN->SYN */
  COMMIT =5,  /* TRN->SYN */
  ANSSYN =15,  /* SYN->all: Answer message from SYN */

  /* Sorter */
  TRSORT  =1, /* TRN->SRT */
  FLSORT  =2, /* TRN->SRT */
  TIDSRT  =3, /* TRN->SRT */
  CRTRN   =4, /* ADM->SRT */
  ANSSRT =15, /* SRT->all: Answer message from SRT */

  FINIT   =14,   /* ADM->all */
  CONSTTR =51,   /* Constant for transaction number within SSTMS */

  NO_LOCK =0,
  WEAK    =1,     /* page lock types */            
  STRONG  =2,
  
  BACKUP  = -1,   /* page has locked */
  PRMOD  =1,
  PRNMOD =0
};


struct msg_buf
{
  long mtype;
  char mtext[SZMSGBUF];
};

/* For administrator */
enum {
  
NUM_LJ       =1,
FIR_TRN_NUM  =2
};

#endif
