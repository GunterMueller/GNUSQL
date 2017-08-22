/*  dessench.h  - internal structures of synchronizer
 *                Kernel of GNU SQL-server. Synchronizer    
 *
 * This file is a part of GNU SQL Server
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

/* $Id: dessnch.h,v 1.247 1998/08/21 00:26:51 kimelman Exp $ */

        /* Internal structures of synchronizer */

#include "destrn.h"
#include <stdio.h>

#define TRNUM   100     /* the number of transactions*/
#define LBSIZE   13     /* initial size of locks area*/

struct des_tran {               /* transaction descriptor */
        char    *ptlb;              /* locks area pointer */
        u2_t   idtr;                /* transaction identifier */
        struct  des_cp *plcp;       /* the last checkpoint pointer */
        struct  des_tran *pbltr;    /* blocking transaction identifier pointer */
        struct  des_lock *pwlock;   /* the first waiting lock pointer */
        char    *firstfree;         /* the first free byte pointer of locks area */
        i4_t    freelb;             /* free bytes quantity of locks area */
        COST    trcost;             /* current transaction cost */
};
struct des_rel {                /* relation descriptor */
        struct  id_ob     idrel;     /* relation identifier */
        struct  des_rel   *frellist; /* forward reference on relations list */
        struct  des_rel   *brellist; /* back reference on relations list */
        struct  des_lock  *rof;      /* pointer to beginning of locks list to a relation */
        struct  des_lock  *rob;      /* pointer to end of locks list to a relation */
        u2_t   rfn;                 /* the number of relation fields */
};
struct des_cp {                 /* checkpoint descriptor */
        u2_t   dls;                 /* checkpoint descriptor size */
        CPNM    cpnum;               /* a checkpoint number */
        struct  des_cp    *pdcp;     /* previous checkpoint descriptor pointer */
        COST    cpcost;              /* transaction cost up to this checkpoint */
};
struct des_lock {               /* lock descriptor */
        u2_t   dls;                 /* lock descriptor size */
        char    lockin;              /* lock type */
        struct  des_tran *tran;      /* transaction descriptor pointer */
        struct  des_rel  *rel;       /* relation descriptor pointer */
        struct  des_lock  *of;       /* forward reference on locks list to a relation */
        struct  des_lock  *ob;       /* back reference on locks list to a relation */
        struct  des_wlock *Ddown;    /* down reference on blocking tree */
};
struct des_wlock {              /* the first waiting lock descriptor */
        struct  des_lock  l;
        struct  des_lock  *Dup;      /* up reference on blocking tree */
        struct  des_wlock *Dqueue;   /* reference on blocker queue */
        COST    newcost;             /* new arrival cost */
};
#define  bhsize sizeof(struct block_head)
#define  rfsize sizeof(struct des_field)
#define  relsize sizeof(struct des_rel)
#define  cpsize ((sizeof(struct des_cp)+15)/16*16)
#define  locksize sizeof(struct des_lock)
#define  wlocksize sizeof(struct des_wlock)
#define  size1b sizeof(i1_t)
#define  size2b sizeof(i2_t)
#define  size4b sizeof(i4_t)
#define  wlsize wlocksize-locksize
