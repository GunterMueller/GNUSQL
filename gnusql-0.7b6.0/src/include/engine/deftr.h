/*
 *  deftr.h  - definitions of Transaction
 *              Kernel of GNU SQL-server  
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
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

#ifndef __deftr_h__
#define __deftr_h__

/* $Id: deftr.h,v 1.247 1997/11/05 16:00:31 kml Exp $ */

/*#ifndef __PR_GLOB_H__*/

enum {
  NRSNUM  =0,
  RDRNUM  =0L,
  SEGSIZE =512,
  SZSNBF  =BD_PAGESIZE,
  MAXCL   =2,		/* max number of narrow locks */
  MIN_TUPLE_LENGTH =5,


  /* record codes in BD page */
  IND     =0,		/* record - indirect reference */
  IDTR    =01,		/* record - transaction identifier */
  CORT    =02,		/* record - ordinary tuple */
  CREM    =03,		/* record - removed tuple */

  /* scan types */
  SCR     =1,		/* relation scan */
  SCTR    =2,		/* temporary relation scan */
  SCI     =3,		/* index scan */
  SCF     =4,		/* filter scan */

  /* scan modes */
  FASTSCAN   =1,
  SLOWSCAN   =2,

  /* record types in Logical Journal */
  MODLJ       =1,	/* modification */
  INSLJ       =3,	/* insertion */
  DELLJ       =5,	/* deletion */
  RLBLJ       =7,	/* rollback */
  CRILJ       =9,	/* index creation */
  DLILJ       =11,	/* index deletion */
  ADFLJ       =13,	/* fields addition */
  RLBLJ_AS_OP =15,	/* rollback as an operation*/
  EOTLJ       =0,	/* end of transaction */
  CPRLJ       =2,	/* record about c.p. */
  GRLBLJ      =4,	/* record about global rollback */

  /* record types in Microjournal */
  OLD     =1,		/* about old value */
  SHF     =2,		/* about shift */
  COMBR   =3,		/* about old value and rigth shift */
  COMBL   =4,		/* about old value and left shift */

  /* masks */
  MSKIDTR =07777777777L,	/* for transaction identifier */
  EOSC    =0200,		/* end of tuple scale */
  MSKCORT   =03,
  MSKIND  =037777,
  MSK21B   =0377,
  MSKCRT   =0376,
  MSKCREM  =0377,
  MSKB4B   =017,
  MSKS4B   =0360,

  DELRD    =8,		/* for relation descriptor */
  DTSCAN   =8,		/* for scan table */
  DEXTD    =10,		/* for extent descriptor table */
  TOBPTD   =10,		/* for temporary object pointer table */
  DSEGSIZE =64,		/* for dynamic segment */

  /* temporary object characteristics */
  TREL    =1,		/* temporary relation */
  FLTR    =0,		/* filter */
  SORT    =1,
  NSORT   =0,


  /* Index Control Program answer cods */
  EOI      =-1,
  NO_KEY2  =-2,

  UNIQ    =0200,
  PRUN    =1,
  PRCL    =1,

  IROOT   =1, /* ROOT is defined in compiler */
  LEAF    =0,
  BTWN    =2,
  THREAD =64
};

/* #endif */

#define size1b          sizeof(i1_t)
#define size2b          sizeof(i2_t)
#define size4b          sizeof(i4_t)
#define drbdsize        sizeof(struct d_r_bd)
#define drtsize         sizeof(struct d_r_t)
#define dtrsize         sizeof(struct des_trel)
#define phsize          sizeof(struct page_head)
#define phtrsize        sizeof(struct p_h_tr)
#define phfsize         sizeof(struct p_h_f)
#define rfsize          sizeof(struct des_field)
#define tidsize         sizeof(struct des_tid)
#define ldisize         sizeof(struct ldesind)
#define dinsize         sizeof(struct des_index)
#define dexsize         sizeof(struct des_exns)
#define scrsize         sizeof(struct d_sc_r)
#define scisize         sizeof(struct d_sc_i)
#define scfsize         sizeof(struct d_sc_f)
#define chpsize         sizeof(char *)
#define cpnsize         sizeof(CPNM)
#define adjsize         sizeof(struct ADBL)
#define dflsize         sizeof(struct des_fltr)
#define indphsize       sizeof(struct ind_page)

#define CHECK_PG_ENTRY(ent)  \
    (assert(ent && (!*ent || ((*ent>0) && (*ent <= BD_PAGESIZE)))),1)

#define mod_nels(newnels,loc)  \
    { u2_t mnels;             \
	mnels=t2bunpack(loc);  \
	mnels+=(newnels);      \
	t2bpack(mnels,loc);    \
    }

#define GET_IND_REF()\
{\
  u2_t *afi, *ai;\
  unsigned char t;\
  afi = (u2_t *) (asp + phsize);\
  ai = afi + tid->tindex;\
  if (*ai == 0)\
    {\
      putpg (&pg, 'n');\
      return (0);\
    }\
  tuple = asp + *ai;\
  t = *tuple & MSKCORT;\
  if (t == CREM || t == IDTR)\
    return (0);\
  if (t == IND)\
    {\
      u2_t pn2, ind2;\
      ref_tid->tindex = ind2 = t2bunpack (tuple + 1);\
      ref_tid->tpn = pn2 = t2bunpack (tuple + 1 + size2b);\
      putpg (&pg, 'n');\
      while ((asp = getpg (&pg, sn, pn2, 's')) == NULL);\
      afi = (u2_t *) (asp + phsize);\
      ai = afi + ind2;\
      tuple = asp + *ai;\
      assert ((*tuple & CREM) != 0 && *ai != 0);\
    }\
  else\
    ref_tid->tpn = (u2_t) ~ 0;\
  corsize = calsc (afi, ai);\
}

#define IND_REF(page2,sn,tuple)\
{\
      u2_t *ai;\
      char *asp;\
      ind = t2bunpack (tuple + 1);\
      pn = t2bunpack (tuple + 1 + size2b); \
      while ((asp= getpg (&page2, sn, pn, 's')) == NULL);\
      ai = (u2_t *) (asp + phsize) + ind;\
      assert (*ai != 0);\
      tuple = asp + *ai;\
}

#endif
