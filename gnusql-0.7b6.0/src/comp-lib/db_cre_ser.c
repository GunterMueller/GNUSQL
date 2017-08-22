/*
 *  db_cre_ser.c  -  creating DB catalogs (server part)
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
 *  This file is written by Konstantin Dyshlevoi
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

/* $Id: db_cre_ser.c,v 1.247 1998/05/20 05:43:19 kml Exp $ */

#include "gsqltrn.h" /* c/s protocol defnitions */
#include "global.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "pupsi.h"
#include "tptrn.h"
#include "expop.h"
#include "const.h"
#include "fieldtp.h"
#include "exti.h"
#include "type_lib.h"


#define   FWR(tabid)    fwrite(&tabid,sizeof(Tabid),1,fb)
#define   FWRID(indid)  fwrite(&indid,sizeof(Indid),1,fb)
#define   FRD(tabid)    fread (&tabid,sizeof(Tabid),1,fb)
#define   FRDID(indid)  fread (&indid,sizeof(Indid),1,fb)

#define SIX  (strlen(SYSADM))
#define PUBLIC "PUBLIC"

#define KILL_PROC(cod)				\
{						\
  if (cod > 0)					\
    rollback (cod);				\
  else						\
    commit ();					\
  return (-1);					\
}

extern i4_t initializing_base;

static void *colval[MAX_COLNO];
static u2_t lencols[MAX_COLNO];
static i4_t   cnlist[8];
static char str1[MAX_COLNO], str2[MAX_COLNO], str3[MAX_COLNO];
static char char1[1], char2[1];
static i2_t short1[1], short2[1], short3[1], short4[1];
static i4_t int1[1], int2[1], int3[1], int4[1];

                       /*   code       prec  len   */
#define TYPE_INT       { SQLType_Int,   0,  SZ_INT }
#define TYPE_SHORT     { SQLType_Short, 0,  SZ_SHT }
#define TYPE_STRING(l) { SQLType_Char,  0,    l    }

sql_type_t TABLES_COLTYPES [TABLES_COLNO] = 
{ TYPE_STRING ( MAX_USER_NAME_LNG ),/* tabname    */
  TYPE_STRING ( MAX_USER_NAME_LNG ),/* owner      */
  TYPE_INT,                         /* untabid    */
  TYPE_SHORT,                       /* segid      */
  TYPE_INT,                         /* tabd       */
  TYPE_INT,                         /* clasindid  */
  TYPE_INT,                         /* primindid  */
  TYPE_STRING ( 1 ),                /* tabtype    */
  TYPE_STRING ( 8 ),                /* credate    */
  TYPE_STRING ( 8 ),                /* cretime    */
  TYPE_SHORT,                       /* ncols      */
  TYPE_INT,                         /* nrows      */
  TYPE_INT                          /* nnulcolnum */
};

sql_type_t COLUMNS_COLTYPES [COLUMNS_COLNO] =
/*   code prec len */
{ TYPE_STRING ( MAX_USER_NAME_LNG), /* colname    */
  TYPE_INT,                         /* untabid    */
  TYPE_SHORT,                       /* colno      */
  TYPE_STRING ( 1 ),                /* coltype    */
  TYPE_INT,                         /* coltype1   */
  TYPE_INT,                         /* coltype2   */
  TYPE_STRING ( MAX_STR_LNG ),      /* defval     */
  TYPE_STRING ( 1 ),                /* defnull    */
  TYPE_INT,                         /* valno      */
  TYPE_STRING ( MAX_STR_LNG ),      /*  minval    */
  TYPE_STRING ( MAX_STR_LNG )       /*  maxval    */
};

sql_type_t INDEXES_COLTYPES [INDEXES_COLNO] =
/*   code prec len */
{ TYPE_INT,                         /* untabid */
  TYPE_STRING ( MAX_USER_NAME_LNG), /* inxname */
  TYPE_STRING ( MAX_USER_NAME_LNG ),/* owner   */
  TYPE_STRING ( 2 ),                /* inxtype */
  TYPE_SHORT,                       /* ncol    */
  TYPE_SHORT,                       /* colno1  */
  TYPE_SHORT,                       /* colno2  */
  TYPE_SHORT,                       /* colno3  */
  TYPE_SHORT,                       /* colno4  */
  TYPE_SHORT,                       /* colno5  */
  TYPE_SHORT,                       /* colno6  */
  TYPE_SHORT,                       /* colno7  */
  TYPE_SHORT,                       /* colno8  */
  TYPE_STRING ( 8 ),                /* credate */
  TYPE_STRING ( 8 ),                /* cretime */
  TYPE_INT                          /* unindid */
};

sql_type_t TABAUTH_COLTYPES [TABAUTH_COLNO] =
/*   code prec len */
{ TYPE_INT,                         /* untabid */
  TYPE_STRING ( MAX_USER_NAME_LNG ),/* grantee */
  TYPE_STRING ( MAX_USER_NAME_LNG ),/* grantor */
  TYPE_STRING ( 7 )                 /* tabauth */
};

sql_type_t COLAUTH_COLTYPES [COLAUTH_COLNO] =
/*   code prec len */
{ TYPE_INT,                         /* untabid */
  TYPE_SHORT,                       /* colno   */
  TYPE_STRING ( MAX_USER_NAME_LNG ),/* grantee */
  TYPE_STRING ( MAX_USER_NAME_LNG ),/* grantor */
  TYPE_STRING ( 2 )                 /* colauth */
};

sql_type_t REFCONSTR_COLTYPES [REFCONSTR_COLNO] =
/*   code prec len */
{ TYPE_INT,                         /* tabfrom  */
  TYPE_INT,                         /* tabto    */
  TYPE_INT,                         /* indto    */
  TYPE_SHORT,                       /* ncols    */
  TYPE_SHORT,                       /* colnofr1 */
  TYPE_SHORT,                       /* colnofr2 */
  TYPE_SHORT,                       /* colnofr3 */
  TYPE_SHORT,                       /* colnofr4 */
  TYPE_SHORT,                       /* colnofr5 */
  TYPE_SHORT,                       /* colnofr6 */
  TYPE_SHORT,                       /* colnofr7 */
  TYPE_SHORT,                       /* colnofr8 */
  TYPE_SHORT,                       /* colnoto1 */
  TYPE_SHORT,                       /* colnoto2 */
  TYPE_SHORT,                       /* colnoto3 */
  TYPE_SHORT,                       /* colnoto4 */
  TYPE_SHORT,                       /* colnoto5 */
  TYPE_SHORT,                       /* colnoto6 */
  TYPE_SHORT,                       /* colnoto7 */
  TYPE_SHORT                        /* colnoto8 */
};

sql_type_t CHCONSTR_COLTYPES [CHCONSTR_COLNO] =
/*   code prec len */
{ TYPE_INT,                         /* untabid */
  TYPE_INT,                         /* chconid */
  TYPE_INT,                         /* consize */
  TYPE_SHORT,                       /* ncols   */
  TYPE_SHORT,                       /* colno1  */
  TYPE_SHORT,                       /* colno2  */
  TYPE_SHORT,                       /* colno3  */
  TYPE_SHORT,                       /* colno4  */
  TYPE_SHORT,                       /* colno5  */
  TYPE_SHORT,                       /* colno6  */
  TYPE_SHORT,                       /* colno7  */
  TYPE_SHORT                        /* colno8  */
};

sql_type_t CHCONSTRTWO_COLTYPES [CHCONSTRTWO_COLNO] =
/* code prec len */
{ TYPE_INT,                         /* chconid  */
  TYPE_SHORT,                       /* fragno   */
  TYPE_INT,                         /* fragsize */
  TYPE_STRING ( MAX_STR_LNG )       /* frag     */
};

sql_type_t VIEWS_COLTYPES [VIEWS_COLNO] =
/* code prec len */
{ TYPE_INT,                         /* untabid  */
  TYPE_SHORT,                       /* fragno   */
  TYPE_INT,                         /* fragsize */
  TYPE_STRING ( MAX_STR_LNG )       /* frag     */
};

i4_t 
ToCol(i4_t untabid, char *name, i4_t colno, i4_t nulno, sql_type_t *type_arr)
{
  i4_t lcod;
  
  lencols[0] = strlen (name);
  strcpy (str1, name);                   /* colname  */
  *short1 = colno;                       /* colno    */
  *char1  = type_arr[colno].code;        /* coltype  */
  *int1   = untabid;                     /* untabid  */
  *int2   = type_arr[colno].len;         /* coltype1 */
  *int3   = type_arr[colno].prec;        /* coltype2 */
  *char2  = (nulno > colno) ? '1' : '0'; /* defnull  */
  
  lcod = insrow (&syscoltabid, lencols, colval);
  if (lcod)
    {
      fprintf (stderr, "BASE: cant't make insrow for SYSCOLUMNS,cod=%d\n ", lcod);
      KILL_PROC (lcod);
    }
  return 0;
}

i4_t 
ToAuth ()
{
  i4_t lcod;
  lcod = insrow (&tabauthtabid, lencols, colval);
  if (lcod)
    {
      fprintf (stderr, "BASE: cant't make insrow for SYSTABAUTH,cod=%d\n ", lcod);
      KILL_PROC (lcod);
    }
  return 0;
}

i4_t 
TOCOLAuth ()
{
  i4_t lcod;
  lcod = insrow (&colauthtabid, lencols, colval);
  if (lcod)
    {
      fprintf (stderr, "BASE: cant't make insrow for SYSCOLAUTH,cod=%d\n ", lcod);
      KILL_PROC (lcod);
    }
  return 0;
}

i4_t 
InsSysTab ()
{
  i4_t lcod;
  lcod = insrow (&systabtabid, lencols, colval);
  if (lcod)
    {
      fprintf (stderr, "BASE: cant't make insrow in systables,cod=%d\n ", lcod);
      KILL_PROC (lcod);
    }
  return 0;
}

int
IndCre (char *inm,Indid *indid, Tabid *tabid, 
	i4_t cnum, i4_t n1, i4_t n2, i4_t n3, i4_t un_fl)
     /* Index creation : inm - index name; indid - pointer to *
      * index identifier (result); tabid - pointer to table   *
      * identifier; cnum - index columns number; n1, n2, n3 - *
      * numbers of these columns ( -1 if is not used );       *
      * un_fl = 1 if UNIQUE index, 0 -else.                   */
{
  i4_t cod;
  
  if (cl_debug)
    fprintf (STDOUT, "BASE:  making index %s \n", inm);
  
  cnlist[0] = n1;
  cnlist[1] = n2;
  cnlist[2] = n3;
  
  cod = creind (indid, tabid, un_fl, 0, cnum, cnlist);
  if (cod < 0)
    {
      fprintf (stderr, "BASE: cant't make it : cod = %d\n ", cod);
      commit ();
      return (-1);
    }
  
  lencols[1] = strlen (inm);
  lencols[3] = un_fl;
  
  *int1 = tabid->untabid;                     /* untabid */
  strcpy (str1, inm);                         /* inxname */
  colval[3] = (un_fl) ? char1 : NULL;         /* inxtype */
  *short4 = cnum;                             /* ncol    */
  *short1 = n1;                               /* colno1  */
  
  if (n2 == -1)
    colval[6] = NULL;
  else
    {
      colval[6] = short2;                     /* colno2  */
      *short2 = n2;
    }
  
  if (n3 == -1)
    colval[7] = NULL;
  else
    {
      colval[7] = short3;                     /* colno3  */
      *short3 = n3;
    }
  
  *int2 = indid->unindid;                     /* unindid */
  
  cod = insrow (&sysindtabid, lencols, colval);
  if (cod)
    {
      fprintf (stderr, "BASE: cant't make index : cod=%d\n ", cod);
      KILL_PROC (cod);
    }
  return 0; /* O'K */
} /* IndCre */

int
CreateTable (Tabid * tabid, char *str_name, i4_t colno,
	     i4_t nulno, sql_type_t *coltypes, i4_t nrows)
{
  static sql_type_t tp[MAX_COLNO];
  i4_t cod, i;
  
  for (i = 0; i < colno; i++)
    type_to_bd( coltypes + i, tp + i);
    
  cod = creptab (tabid, 1 /* segid */, colno, nulno, tp);
  if (cod)
    KILL_PROC (cod);
  lencols[0] = strlen (str_name);
  strcpy (str1, str_name);  /* tabname    */
  *int1 = tabid->untabid;   /* untabid    */
  *short1 = tabid->segid;   /* segid      */
  *int2 = tabid->tabd;      /* tabd       */
  *short2 = colno;          /* ncols      */
  *int4 = nrows;            /* nrows      */
  *int3 = nulno;            /* nnulcolnum */
  if (InsSysTab ())
    return (-1);
  
  return 0;
} /* CreateTable */

i4_t
create_base ()
/* create system catalogs for data base  */
/* return 0 if success */
{
  char cur_dt[9], cur_tm[9];
  
  FILE *fb;
  i4_t cod, i, cur_untabid, cur_nulno, cur_colno;
  sql_type_t *cur_type_arr;
  
  curdate (cur_dt);
  curtime (cur_tm); 

/*---------- creation of SYSTEM CATALOGUES & SYSTABLES filling  -----------*/

  lencols[1] = SIX;
  lencols[2] = sizeof (i4_t);
  lencols[3] = sizeof (i2_t);
  lencols[4] = sizeof (i4_t);
  lencols[5] = 0;
  lencols[6] = 0;
  lencols[7] = 1;
  lencols[8] = 8;
  lencols[9] = 8;
  lencols[10] = sizeof (i2_t);
  lencols[11] = sizeof (i4_t);
  lencols[12] = sizeof (i4_t);
  
  colval[0] = str1;              /* tabname    */
  colval[1] = str2;              /* owner      */
  colval[2] = int1;              /* untabid    */
  colval[3] = short1;            /* segid      */
  colval[4] = int2;              /* tabd       */
  colval[5] = NULL;              /* clasindid  */
  colval[6] = NULL;              /* primindid  */
  colval[7] = str3;              /* tabtype    */  
  colval[8] = cur_dt;            /* credate    */
  colval[9] = cur_tm;            /* cretime    */
  colval[10] = short2;           /* ncols      */
  colval[11] = int4;             /* nrows      */
  colval[12] = int3;             /* nnulcolnum */

  strcpy (str2, SYSADM);       /* owner      */
  strcpy (str3, "B");            /* tabtype    */  
  
#define CRE_SYS_TABLE(tabid, name, nrows)		    \
  cod = CreateTable (&tabid, "SYS" #name, name##_COLNO,     \
		     name##_NULNO, name##_COLTYPES, nrows); \
  if (cod < 0)					            \
    return (cod);
							    
		/* tabid          name	       nrows */				    
  CRE_SYS_TABLE (systabtabid,    TABLES,         9);
  CRE_SYS_TABLE (syscoltabid,    COLUMNS,       89);
  CRE_SYS_TABLE (sysindtabid,    INDEXES,       11);
  CRE_SYS_TABLE (tabauthtabid,   TABAUTH,        8);
  CRE_SYS_TABLE (colauthtabid,   COLAUTH,       10);
  CRE_SYS_TABLE (refconstrtabid, REFCONSTR,      0);
  CRE_SYS_TABLE (chconstrtabid,  CHCONSTR,       0);
  CRE_SYS_TABLE (chcontwotabid,  CHCONSTRTWO,    0);
  CRE_SYS_TABLE (viewstabid,     VIEWS,          0);
  
/*-------------------------- SYSCOLUMNS filling ------------------------*/

  lencols[1] = sizeof (i4_t);
  lencols[2] = sizeof (u2_t);
  lencols[3] = 1;
  lencols[4] = sizeof (i4_t);
  lencols[5] = sizeof (i4_t);
  lencols[6] = 0;
  lencols[7] = 1;
  lencols[8] = 0;
  lencols[9] = 0;
  lencols[10] = 0;

  colval[0] = str1;              /* colname   */
  colval[1] = int1;              /* untabid   */
  colval[2] = short1;            /* colno     */
  colval[3] = char1;             /* coltype   */
  colval[4] = int2;              /* coltype1  */
  colval[5] = int3;              /* coltype2  */
  colval[6] = NULL;              /* defval    */
  colval[7] = char2;             /* defnull   */
  colval[8] = NULL;              /* valno     */
  colval[9] = NULL;              /* minval    */
  colval[10] = NULL;             /* maxval    */
  
/*------------ columns of SYSTABLES to SYSCOLUMNS ---------------*/
  
#define ABOUT_TABLE(name, tabid)		\
  cur_untabid = tabid.untabid;			\
  cur_nulno = name##_NULNO;			\
  cur_type_arr = name##_COLTYPES;		\
  cur_colno = 0;
  
#define TO_SYSCOL(name) if ((cod = ToCol (cur_untabid, name, cur_colno++, \
                            cur_nulno, cur_type_arr)) < 0) return (cod)

  ABOUT_TABLE (TABLES, systabtabid);
  TO_SYSCOL("TABNAME");
  TO_SYSCOL("OWNER");
  TO_SYSCOL("UNTABID");
  TO_SYSCOL("SEGID");
  TO_SYSCOL("TABD");
  TO_SYSCOL("CLASINDID");
  TO_SYSCOL("PRIMINDID");
  TO_SYSCOL("TABTYPE");
  TO_SYSCOL("CREDATE");
  TO_SYSCOL("CRETIME");
  TO_SYSCOL("NCOLS");
  TO_SYSCOL("NROWS");
  TO_SYSCOL("NNULCOLNUM");
  
/*------------ columns of SYSCOLUMNS to SYSCOLUMNS --------------*/
  
  ABOUT_TABLE (COLUMNS, syscoltabid);
  TO_SYSCOL("COLNAME");
  TO_SYSCOL("UNTABID");
  TO_SYSCOL("COLNO");
  TO_SYSCOL("COLTYPE");
  TO_SYSCOL("COLTYPE1");
  TO_SYSCOL("COLTYPE2");
  TO_SYSCOL("DEFVAL");
  TO_SYSCOL("DEFNULL");
  TO_SYSCOL("VALNO");
  TO_SYSCOL("MINVAL");
  TO_SYSCOL("MAXVAL");
  
/*------------ columns of SYSINDEXES to SYSCOLUMNS --------------*/
  
  ABOUT_TABLE (INDEXES, sysindtabid);
  TO_SYSCOL("UNTABID");
  TO_SYSCOL("INXNAME");
  TO_SYSCOL("OWNER");
  TO_SYSCOL("INXTYPE");
  TO_SYSCOL("NCOL");
  TO_SYSCOL("COLNO1");
  TO_SYSCOL("COLNO2");
  TO_SYSCOL("COLNO3");
  TO_SYSCOL("COLNO4");
  TO_SYSCOL("COLNO5");
  TO_SYSCOL("COLNO6");
  TO_SYSCOL("COLNO7");
  TO_SYSCOL("COLNO8");
  TO_SYSCOL("CREDATE");
  TO_SYSCOL("CRETIME");
  TO_SYSCOL("UNINDID");
  
/*------------ columns of SYSTABAUTH to SYSCOLUMNS --------------*/
  
  ABOUT_TABLE (TABAUTH, tabauthtabid);
  TO_SYSCOL("UNTABID");
  TO_SYSCOL("GRANTEE");
  TO_SYSCOL("GRANTOR");
  TO_SYSCOL("TABAUTH");
  
/*------------ columns of SYSCOLAUTH to SYSCOLUMNS --------------*/
  
  ABOUT_TABLE (COLAUTH, colauthtabid);
  TO_SYSCOL("UNTABID");
  TO_SYSCOL("COLNO");
  TO_SYSCOL("GRANTEE");
  TO_SYSCOL("GRANTOR");
  TO_SYSCOL("COLAUTH");
  
/*------------ columns of SYSREFCONSTR to SYSCOLUMNS ------------*/
  
  ABOUT_TABLE (REFCONSTR, refconstrtabid);
  TO_SYSCOL("TABFROM");
  TO_SYSCOL("TABTO");
  TO_SYSCOL("INDTO");
  TO_SYSCOL("NCOLS");
  TO_SYSCOL("COLNOFR1");
  TO_SYSCOL("COLNOFR2");
  TO_SYSCOL("COLNOFR3");
  TO_SYSCOL("COLNOFR4");
  TO_SYSCOL("COLNOFR5");
  TO_SYSCOL("COLNOFR6");
  TO_SYSCOL("COLNOFR7");
  TO_SYSCOL("COLNOFR8");
  TO_SYSCOL("COLNOTO1");
  TO_SYSCOL("COLNOTO2");
  TO_SYSCOL("COLNOTO3");
  TO_SYSCOL("COLNOTO4");
  TO_SYSCOL("COLNOTO5");
  TO_SYSCOL("COLNOTO6");
  TO_SYSCOL("COLNOTO7");
  TO_SYSCOL("COLNOTO8");
  
/*------------ columns of SYSCHCONSTR to SYSCOLUMNS -------------*/

  ABOUT_TABLE (CHCONSTR, chconstrtabid);
  TO_SYSCOL("UNTABID");
  TO_SYSCOL("CHCONID");
  TO_SYSCOL("CONSIZE");
  TO_SYSCOL("NCOLS");
  TO_SYSCOL("COLNO1");
  TO_SYSCOL("COLNO2");
  TO_SYSCOL("COLNO3");
  TO_SYSCOL("COLNO4");
  TO_SYSCOL("COLNO5");
  TO_SYSCOL("COLNO6");
  TO_SYSCOL("COLNO7");
  TO_SYSCOL("COLNO8");
  
/*---------- columns of SYSCHCONSTRTWO to SYSCOLUMNS ------------*/
  
  ABOUT_TABLE (CHCONSTRTWO, chcontwotabid);
  TO_SYSCOL("CHCONID");
  TO_SYSCOL("FRAGNO");
  TO_SYSCOL("FRAGSIZE");
  TO_SYSCOL("FRAG");
  
/*------------ columns of SYSVIEWS to SYSCOLUMNS ----------------*/
  
  ABOUT_TABLE (VIEWS, viewstabid);
  TO_SYSCOL("CHCONID");
  TO_SYSCOL("FRAGNO");
  TO_SYSCOL("FRAGSIZE");
  TO_SYSCOL("FRAG");
  
#undef TO_SYSCOL
  
/*------------------------ SYSTABAUTH filling -------------------------*/
  
#define TO_SYSTABAUTH(tid)    *int1 = tid.untabid; \
                              if (ToAuth ())       \
                                return (-1)

  lencols[0] = sizeof (i4_t);
  lencols[1] = strlen(PUBLIC);
  lencols[2] = SIX;
  lencols[3] = 1;

  colval[0] = int1;		/* untabid */
  colval[1] = str1;		/* grantee */
  colval[2] = str2;		/* grantor */
  colval[3] = str3;		/* tabauth */
  
  strcpy (str1, PUBLIC);        /* grantee */
  strcpy (str2, SYSADM);      	/* grantor */
  strcpy (str3, "S"); 		/* tabauth */
  
/*                   tabid  */  
  
  TO_SYSTABAUTH ( syscoltabid    );
  TO_SYSTABAUTH ( sysindtabid    );
  TO_SYSTABAUTH ( viewstabid     );
  TO_SYSTABAUTH ( tabauthtabid   );
  TO_SYSTABAUTH ( colauthtabid   );
  TO_SYSTABAUTH ( refconstrtabid );
  TO_SYSTABAUTH ( chconstrtabid  );
  TO_SYSTABAUTH ( chcontwotabid  );

  lencols[3] = 2;
  strcpy (str3, "Su"); 		/* tabauth */
  TO_SYSTABAUTH(systabtabid);
  
#undef TO_SYSTABAUTH
  
/*------------------------ SYSCOLAUTH filling -------------------------*/

#define   TOCOLAUTH  {if (TOCOLAuth ()) return (-1);}

  lencols[0] = sizeof (i4_t);
  lencols[1] = sizeof (u2_t);
  lencols[2] = strlen("kml");
  lencols[3] = SIX;
  lencols[4] = strlen("U");

  colval[0] = int1;
  colval[1] = short1;
  colval[2] = str1;
  colval[3] = str2;
  colval[4] = str3;

  *int1 = systabtabid.untabid;  /* untabid */
  strcpy (str1, "kml");         /* grantee */
  strcpy (str2, SYSADM);       	/* grantor */
  strcpy (str3, "U"); 		/* tabauth */

  for (i = 5; i< 14; i++)
    {
      *short1 = i;              /* colno   */
      TOCOLAUTH;	
    }
  
#undef TOCOLAUTH

/*------------------------ INDEXES CREATION -------------------------*/

#define CREIND(inm, indid, tabid, cnum, n1, n2, n3, un_fl)               \
              if (IndCre (inm, &indid, &tabid, cnum, n1, n2, n3, un_fl)) \
                return -1;
  
  lencols[0] = sizeof (i4_t);
  lencols[2] = SIX;
  lencols[4] = sizeof (u2_t);
  lencols[5] = sizeof (u2_t);
  lencols[6] = sizeof (u2_t);
  lencols[7] = sizeof (u2_t);
  for (i = 8; i < 13; i++)
    lencols[i] = 0;
  lencols[13] = 8;
  lencols[14] = 8;
  lencols[15] = sizeof (i4_t);
  
  colval[0] = int1;        /* untabid */
  colval[1] = str1;        /* inxname */
  colval[2] = str2;        /* owner   */
  colval[4] = short4;      /* ncol    */
  colval[5] = short1;      /* colno1  */
  for (i = 8; i < 13; i++)
    colval[i] = NULL;      /* colno with number i */
  colval[13] = cur_dt;     /* credate */
  colval[14] = cur_tm;     /* cretime */
  colval[15] = int2;       /* unindid */
  
  strcpy (str2, SYSADM);   /* owner   */
  *char1 = 'U';            /* inxtype */
  
  /*        index_name        indid_ptr         tabid       clm_num      columns unique */
  
  CREIND("SYSTABLESIND1ID",   indid1,           systabtabid,    2,      0,  1, -1,  1)
  CREIND("SYSTABLESIND2ID",   indid2,           systabtabid,    1,      2, -1, -1,  1)
  CREIND("SYSCOLUMNSIND1ID",  indidcol,         syscoltabid,    2,      0,  1, -1,  1)
  CREIND("SYSCOLUMNSIND2ID",  indidcol2,        syscoltabid,    2,      1,  2, -1,  1)
  CREIND("SYSINDEXIND",       sysindexind,      sysindtabid,    1,      0, -1, -1,  0)
  CREIND("SYSTABAUTHIND1ID",  sysauthindid,     tabauthtabid,   2,      0,  1, -1,  1)
  CREIND("SYSCOLAUTHIND1ID",  syscolauthindid,  colauthtabid,   3,      0,  1,  2,  1)
  CREIND("SYSREFCONSTRINDEX", sysrefindid,      refconstrtabid, 1,      0, -1, -1,  0)
  CREIND("SYSREFCONSTRIND1",  sysrefindid1,     refconstrtabid, 1,      1, -1, -1,  0)
  CREIND("SYSCHCONSTRINDEX",  chconstrindid,    chconstrtabid,  1,      0, -1, -1,  0)
  CREIND("SYSCHCONSTRTWOIND", chconstrtwoind,   chcontwotabid,  2,      0,  1, -1,  1)
  CREIND("SYSVIEWSIND",       viewsind,         viewstabid,     2,      0,  1, -1,  1)
    
#undef CREIND
  
/*------------- tables & indexes identifiers saving -------------*/

  fb = fopen (BASE_DAT, "w+");
  if (fb == NULL)
    perror ("can't open base.dat \n");
  fseek (fb, 0, SEEK_SET);
  FWR (systabtabid);
  FWR (syscoltabid);
  FWR (sysindtabid);
  FWR (viewstabid);
  FWR (tabauthtabid);
  FWR (colauthtabid);
  FWR (refconstrtabid);
  FWR (chconstrtabid);
  FWR (chcontwotabid);

  FWRID (indid1);
  FWRID (indid2);
  FWRID (indidcol);
  FWRID (indidcol2);
  FWRID (sysindexind);
  FWRID (sysauthindid);
  FWRID (syscolauthindid);
  FWRID (sysrefindid);
  FWRID (sysrefindid1);
  FWRID (chconstrindid);
  FWRID (chconstrtwoind);
  FWRID (viewsind);

  fclose (fb);
  
  if (cl_debug)
    fprintf (STDOUT, "BASE: end of creatbas \n");
  initializing_base = 1;
  return (0);
} /* create_base */

result_t*
db_create(void *in, struct svc_req *rqstp)
{
  gsqltrn_rc.sqlcode = create_base ();
  if (!gsqltrn_rc.sqlcode)
    commit ();
  return &gsqltrn_rc;
}

