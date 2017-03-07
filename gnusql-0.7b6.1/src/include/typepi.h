/* 
 *  typepi.h -  file with data types for synthesator and interpretator
 *              of GNU SQL compiler
 *                
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Konstantin Dyshlevoj 
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
 *  Contacts: gss@ispras.ru
 *
 */

/* $Id: typepi.h,v 1.247 1998/09/29 21:26:01 kimelman Exp $ */

#ifndef __TYPEPI_H__
#define __TYPEPI_H__

#include <sql_type.h>
#include <vmemory.h>
#include <type_lib.h>

#define V_PTR(adr,typ) ((typ *)vpointer(adr))
#define ADR(ptr,elem)  V_PTR( ((ptr)->elem).off, char )
#define TP_ADR(ptr,elem,typ) V_PTR( ((ptr)->elem).off, typ )
#define V_ADR(ptr,elem,typ)  V_PTR( (ptr)->elem,typ )
#define VMALLOC(n,typ) vmalloc( (n)*sizeof(typ) )
#define P_VMALLOC(var,n,typ) \
        var = V_PTR (V_##var = vmalloc ((n) * sizeof (typ)), typ)

#define PUT_STRING(to, str) {                                         \
                              VADR V_adr; char *adr;                  \
                              P_VMALLOC(adr, strlen (str) + 1, char); \
                              strcpy (adr, str);                      \
                              to = V_adr;                             \
			    }


#define CH_ALL     'A'
#define CH_UNIC    'U'

#define CH_INC     'A'
#define CH_DEC     'D'

#define CH_TRUE    0
#define CH_FLS     1

#define CH_STNDR   0

#define CH_TAB     't'
#define CH_INDX    'i'
#define CH_FLTR    'f'

#define SEC_VW     CH_TAB
#define IND_VW     CH_INDX
#define FLT_VW     CH_FLTR
#define JON_VW     'j'


enum CmdCode {OPSCAN, TMPSCN, TMPCRE, SORTTBL, READROW, FINDROW, INSROW,
	      INSLIST, MODROW, DELROW, CLOSE, DROPTTAB, MKGROUP, FUNC,
	      until, RetPar, SavePar, GoTo, end, SetHandle, ERROR,
	      CursorHeader, MKUNION, Procedure, COND_EXIT, CRETAB,
	      DROPBASE, CREIND, PRIVLG, Drop_Reg, INSTAB, next_vm_piece};

/* All fields(-pointers) of structures from module have type *
 * PSECT_PTR . In field out of such union - virtual address. */


typedef struct {
  /* this structure is used for calling *
   * procedure for SubQuery handling    */
  VADR ProcAdr;       /* Procedure beginning address               */
  data_unit_t ToDtStack; /* This element must be putted to data stack */
  char GoUpNeed;      /* = 1 if there is needed to go up in tree   */
		      /* after procedure calling, = 0 - stand here */
} PrepSQ;

#define NL_FL(x)    ((x)->nl_fl   )
#define MAX_LEN(x)  ((x)->max_len )
#define VL(x)       ((x)->val)
#define STR_PTR(x)  ((x)->val.StrPtr.adr)
#define STR_VPTR(x) ((x)->val.StrPtr.off)
#define SRT_VL(x)   ((x)->val.Srt )
#define INT_VL(x)   ((x)->val.Iint)
#define LNG_VL(x)   ((x)->val.Lng )
#define FLT_VL(x)   ((x)->val.Flt )
#define DBL_VL(x)   ((x)->val.Dbl )

/***************************************************************
 *      some structures for interpretator work management      *
 ***************************************************************/

struct S_CursorHeader {  /* information for work with cursor         */
  
      VADR CursName;     /* Name of cursor ( for external references)*/ 
      VADR DelCurConstr; /* pointer to structure S_ConstrHeader for  *
		          * DELETE CURRENT operation                 */
      char *Cur;         /* address of work with cursor continuing   */
      char OpFl;         /* flag of cursor state                     */
   } ;

struct S_Drop_Reg {      /* addresses for scans & temporary tables'  *
			  * identifiers registration ( for possible  *
			  * EXIT from current segment ).             */
  
      i4_t ScanNum;       /* Number of elements in array Scan         */
      VADR Scan;         /* array of pointers (VADR) to Scanid       */
			 /* for  DELROW & CLOSE operations           */
      i4_t TabdNum;       /* Number of elements in array Tabd         */
      VADR Tabd;         /* array os pointers (VADR) to TabId        */
			 /* for DROPTTAB making in CLOSE operation   */
   } ;

typedef union {
  Tabid t;
  Indid i;
  Filid f;
} UnId;

/* user's program calls main procedures by their numbers.
   In interpreter calls of auxiliary procedures are made by
   procedures' address pointing                                */

/***************************************************************
 *       interpretator commands for work with DB - server      *
 ***************************************************************/


struct S_OPSCAN {
  VADR Scan;            /* table scan                            */
  VADR Tid;             /* Tabid,Indid or Filid                  */
  char OperType;        /* operation type of System of Memory    */
                        /* and  Synchronization Managment        */
  char mode;            /* open mode                             */
  i4_t  nr;              /* the number of columns which values    */
                        /* will be selected                      */
  VADR rlist;           /* array of column numbers which values  */
                        /* will be selected                      */
  i4_t  nl;
  VADR list;            /* array of VADR for trees of SP         */
  i4_t  ic;
  VADR range;           /* scan range - restricted form of       */
                        /* conditions for columns included in    */
                        /* idexes; array of VADR                 */
  i4_t  nm;              /* the number of columns which values    */
                        /* will be modified                      */
  VADR mlist;           /* array of column number which values   */
                        /* will be modified                      */
  VADR Exit;            /* exit in the case of                   */
                        /* incompatibility of  SP                */
};

struct S_TMPSCN {
                           /* for creating temporal relation only   */
			   /* using by base table scanning          */
     char OperType;        /* method of base table scanning         */
			   /* = 't' , 'i' , 'f'                     */   
     VADR TidFrom;         /* TabId of source table                 */
     VADR TidTo;           /* TabId of result table                 */
     i4_t  nr;              /* the number of columns which values    */
                           /* will be selected                      */
     VADR rlist;           /* array of column numbers which values  */
                           /* will be selected                      */
     i4_t  nl;   
     VADR list;            /* array of VADR for trees of SP         */
     i4_t  ic;
     VADR range;           /* scan range - restricted form of       */
                           /* conditions for columns included in    */
                           /* idexes; array of VADR                 */
     VADR Exit;            /* exit in the case of                   */
                           /* incompatibility of  SP                */
};

struct S_TMPCRE {
      VADR  Tid;           /* address for resulting TabId           */
      i4_t   colnum;        /* the number of columns in the table    */
      i4_t   nnulnum;       /* the number of the first columns       */
                           /* which  may not contain                */
                           /* unidentified values                   */
      VADR coldescr;       /* array of column descriptions          */
			   /* TREE_ND_TYPE array                    */
} ;

struct S_SORTTBL  {
      VADR TidIn;          /* TabId of source Temporal Relation     */
      VADR TidOut;         /* TabId of result Temporal Relation     */
      i4_t  ns;             /* the number of columns which values are*/
                           /* used in sorting                       */ 
      VADR rlist;          /* array of numbers of columns which     */
                           /* values are used in sorting            */ 
      char fl;             /* CH_ALL-all, CH_UNIC -without          */
                           /* duplicates                            */
      char order;          /* sort order indicator : 'A'            */
                           /* - by increas, 'D' (or any symbol      */
			   /* exept 'A', - by decrease.             */
			   /* look  CH_INC, CH_DEC                  */
} ;

struct S_MKUNION  {
      VADR TidIn1;         /* TabId of source Sorted Temporal Relation 1 */
      VADR TidIn2;         /* TabId of source Sorted Temporal Relation 2 */
      VADR TidOut;         /* TabId of result Temporal Relation          */
   } ;


struct S_READROW {
  char      flag;         /* this flag='n' till first execution  */
                          /* of 'readrow' after flah='y'. Last   */
                          /* means that in all fields with type  */
                          /* PSECT_PTR and in link 'lst'         */
                          /* addresses  but not offsets are      */
                          /* stored                              */
    PSECT_PTR Scan;       /* table scan for value selection      */
                          /* (for removed table used by dynamic     */
			  /* SQL : Scan= -1:off-offset of           */
			  /* cursor's name                          */
    i4_t       nr;         /* the number of selection columns        */
    PSECT_PTR ColPtr;     /* addresses of locations of              */
                          /* (PSECT_PTR) results                    */
    PSECT_PTR rlist;      /* array of column number which values    */
                          /* will be selected                       */
   } ;

struct S_FINDROW {
      char      flag;
      PSECT_PTR Scan;     /* table's scan                           */
      i4_t       nr;       /* the number of selected columns         */
      PSECT_PTR ColPtr;   /* addresses of locations of              */
                          /* (PSECT_PTR) results                    */
      PSECT_PTR Exit;     /* exit when the scanning cycle finished*/
   } ;

struct S_INSROW {
      char      flag;
      PSECT_PTR Tid;      /* pointer to table's Tabid             */
      i4_t       nv;       /* the number of inserted columns          */
      PSECT_PTR InsList;  /* array of pointers (PSECT_PTR) to     */
                          /* the roots of trees of insertion      */
      PSECT_PTR ClmTp;    /* array of insertion columns types     */
      PSECT_PTR Constr;   /* pointer to structure S_ConstrHeader  */
   } ;

struct S_INSLIST {
      char      flag;
      PSECT_PTR Tid;          /* pointer to table's Tabid           */
      i4_t       nv;           /* the number of inserted columns        */
      PSECT_PTR len;          /* array of lengths of inserted columns  */
      PSECT_PTR col_types;    /* array of insertion columns types      */
      PSECT_PTR col_ptr;      /* array of address of inserted columns  */
      PSECT_PTR credat_adr;   /* if != NULL : date should be placed */
			      /* to this place                      */
      PSECT_PTR cretime_adr;  /* if != NULL : time should be placed */
			      /* to this place                      */
   } ;

struct S_INSTAB {
      VADR TidFrom;          /* pointer to source table's Tabid     */
      VADR TidTo;            /* pointer to receiver table's Tabid   */
   } ;

struct S_MODROW    {
      char      flag;
      PSECT_PTR Scan;    /* table's scan(for removed table)     */
                         /* in dynamic SQL : Scan= -1:off-offset*/
                         /* of cursor's string-name             */
      i4_t       nl;      /* the number of columns which values will*/
		      	 /* be modified                         */
      PSECT_PTR mlist;   /* array of nubmbers of modification columns*/
      PSECT_PTR OutList; /* array of pointers (PSECT_PTR) to    */
                         /* tree's roots                        */
      PSECT_PTR ClmTp;   /* array of modification columns types */
      PSECT_PTR Constr;  /* pointer to structure S_ConstrHeader */
   } ;

struct S_DELROW    {
      VADR Scan;         /* table's scan                        */
      VADR Constr;       /* pointer to structure S_ConstrHeader */
   } ;

struct S_CLOSE    {
      VADR Scan;         /* table's scan                        */
   } ;

struct S_DROPTTAB {      /* dropping of temporal relation       */
      VADR Tid;          /* table's Tabid                       */
      };

struct S_DROPBASE {      /* dropping of basic relation or view  */
      i4_t untabid;
      };

struct S_MKGROUP {
      VADR TidIn;      /* Tabid Scan of source temporal relation*/
      VADR TidOut;     /* Tabid of result temporal relation     */
                       /* if functions are there then the last  */
                       /* nf columns correspond flist[]            */
      i4_t  ng;         /* the number of columns belonging to group */
      VADR glist;      /* array of numbers of columns belonging to */
                       /* group                                 */
      char order;      /* indicator of sorting order: 'A'   -   */
		       /* - increase, 'D' (or any other symbol  */
		       /* exept  'A', - decrease                */
      i4_t  nf;         /* the number of columns for which functions*/
		       /* have to be computed                   */
      VADR flist;      /* array of numbers  of columns for which   */
		       /* functions have to be computed         */
      VADR fl;         /* char fl[nf-1] - string of flags       */
		       /* corresponding for each element from   */
		       /*  flist[].                             */
		       /* fl[i]==1  COUNT(flist[i])             */
		       /* fl[i]==2    AVG(flist[i])             */
		       /* fl[i]==3    MAX(flist[i])             */
		       /* fl[i]==4    MIN(flist[i])             */
		       /* fl[i]==5   SUMM(flist[i])             */
      };

struct S_FUNC          /* computing of aggregate functions      */
  {
  char      flag;
  PSECT_PTR Tid;       /* Tabid of original table               */
  char      OperType;  /* type of SMSM's operation: CH_TAB,CH_INDX */
  i4_t       nl;        /* condition of current string selection */
  PSECT_PTR list;      /* array of VADR for trees of SP         */
  i4_t       ic;
  PSECT_PTR range;     /* scan range - restricted form of       */
		       /* condition on columns included in idex    */
		       /* array of VADR                         */
  PSECT_PTR colval;    /* PSECT_PTR points to array of PSECT_PTR*/
		       /* - array of pointers for functions'    */
		       /* values location ; the number of       */
		       /* pointers in array has to be equal to  */
		       /* value of 'nf'                         */
  i4_t       nf;        /* the number of columns for which functions*/
		       /* have to be computed                   */
  PSECT_PTR flist;     /* array of numbers  of columns for which   */
		       /* functions have to be computed         */
  PSECT_PTR fl;        /* char fl[nf-1] - string of flags       */
		       /* corresponding for each element from   */
		       /* flist[].                              */
  PSECT_PTR Exit;      /* exit in the case of                   */
                       /* incompatibility of  SP                */
};

/***************************************************************
 *        commands for management of interpreter work        *
 *           ( withoit connection with DB - server)            *
 ***************************************************************/

struct S_until {
       char      flag;
       PSECT_PTR cond;   /* checked condition - rest            */
       PSECT_PTR ret;    /* begining of cycle repeat            */
   };

struct S_RetPar {
      char      flag;
      char      ExitFlag; /* FALSE if is used in SELECT stmt.   */
      i4_t       OutNum;  /* the number of input parameters      */
      PSECT_PTR OutList; /* array of pointers(PSECT_PTR) to     */
                         /* trees' roots from  SELECT           */
   } ;

struct S_SavePar {
  char      flag;
  i4_t       ParamNum; /* the number of saved parameters */
  PSECT_PTR PlaceOff; /* array of elmements (DataUnion) *
		       * where to put the parameters    */
                  } ;

struct S_GoTo {
      char      flag;
      PSECT_PTR Branch;  /* point of branch                     */
   } ;

struct S_SetHandle {
      char      flag;
      MASK      Bit;
      i4_t       TrCnt;  /* the number of trees in which Handle  */
                        /* should be checked accoding to  Bit:  */
                        /* Bit itself if(Bit != 0) else         */
                        /* - input parameter 'HandleSQ'         */
      PSECT_PTR Trees;  /* array of address(PSECT_PTR) of these trees*/
    } ;

struct S_Procedure {
  VADR ProcBeg;    /* address of called procedure */
} ;

struct S_COND_EXIT {
  char      flag;
  PSECT_PTR Tree; /* if the result of calculating of this tree is TRUE => *
	           * interpretator finishes work with current procedure   */
} ;

struct S_ERROR {
  i4_t  Err; /* error code.  */
} ;

struct S_CRETAB {
     VADR tabname;         /* string - name of the table         */
     VADR owner;           /* string - authorization             */
     Segid segid;          /*                                    */
     i4_t  colnum;          /* column's number                    */
     i4_t  nncolnum;        /* number of first columns : NOT NULL */
     VADR coldescr;        /* array of column's descriptions     */
     VADR tabid;           /* pointer where to put Tabid         */
   } ;

struct S_CREIND {
     VADR tabid;           /* identifier of table in DB          */
     VADR indid;           /* identifier of index in DB (result) */
     char uniq_fl;         /* ='U' if UNIQUE, 0 else             */
     i4_t  colnum;          /* index column's number              */
     VADR clmlist;         /* list of index columns numbers(i4_t) */
   } ;

struct S_PRIVLG {          /* privileges checking */
  VADR untabid;            /* privilegies on table with this untabid  */
  VADR owner;              /* table's owner name                      */ 
  VADR user;               /* this user name must be equal to current *
			    * user name                               */
  VADR privcodes;          /* string of priveleges codes              */
  i4_t  un;                 /* columns number in UPDATE list           */
  VADR ulist;              /* list of columns in UPDATE (i4_t)         */
  i4_t  rn;                 /* columns number in REFERENCES list       */
  VADR rlist;              /* list of columns in REFERENCES  (i4_t)    */
} ;
#endif


