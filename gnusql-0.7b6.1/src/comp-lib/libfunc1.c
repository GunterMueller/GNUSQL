/*
 *  libfunc1.c  -  engine interface library (part 1)
 *                 GNU SQL server
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
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

/* $Id: libfunc1.c,v 1.250 1998/09/29 21:24:51 kimelman Exp $ */

#include "setup_os.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "engine/pupsi.h"

#include "sql_decl.h"
#include "type_lib.h"
#include "pr_glob.h"
#include "typeif.h"
#include "engine/tptrn.h"
#include "engine/destrn.h"
#include "engine/strml.h"
#include "engine/fdcltrn.h"
#include "exti.h"
#include "assert.h"
#include "funall.h"

/*---------------------------------------------*/
EXTERNAL char res_buf[SOC_BUF_SIZE];
EXTERNAL char buffer[SOC_BUF_SIZE];
EXTERNAL char *pointbuf;

/*---------------------------------------------*/
#define RTN(s)        if( s < 0 ) return (s )
#define INITBUF       pointbuf = buffer
/*--------------------------------------------------*/
#define TRANIND(pidind,indid)\
{\
     u2_t page, index;\
     page=(u2_t)(((*indid).tabid.tabd)>>16);\
     index=(u2_t)(((*indid).tabid.tabd)&0xFFFF); \
     Copy(&(pidind.irii.urn.segnum),\
                   &((*indid).tabid.segid ),sizeof(Segid) );\
     Copy(&(pidind.irii.urn.obnum ),\
                   &((*indid).tabid.untabid),sizeof(Unid)  );\
     Copy(&(pidind.irii.pagenum),&page,sizeof(u2_t) );\
     Copy(&(pidind.irii.index),&index,sizeof(u2_t) );\
     Copy(&(pidind.inii),&((*indid).unindid),sizeof(Unid) );\
}
/*--------------------------------------------------------*/
#define TRANTAB(pidrel,tabid)\
{\
     u2_t page, index;\
     page=(u2_t)((tabid->tabd)>>16);\
     index=(u2_t)((tabid->tabd)&0xFFFF);\
     Copy(&(pidrel.urn.segnum),&(tabid->segid ),sizeof(Segid) );\
     Copy(&(pidrel.urn.obnum ),&(tabid->untabid),sizeof(Unid)  );\
     Copy(&(pidrel.pagenum),&page,sizeof(u2_t) );\
     Copy(&(pidrel.index),&index,sizeof(u2_t) );\
}
/*---------------------------------------------*/
#define PUTIND(indid,answer);     Copy(&((*indid).tabid.segid),\
                &(answer.idinci.irii.urn.segnum),sizeof(Segid) );\
     Copy(&((*indid).tabid.untabid),\
                &(answer.idinci.irii.urn.obnum),sizeof(Unid) );\
tid = ((i4_t)(answer.idinci.irii.pagenum)<<16)|(i4_t)(answer.idinci.irii.index);\
     Copy(&((*indid).tabid.tabd),&(tid),sizeof(Tid) );\
     Copy(&((*indid).unindid),&(answer.idinci.inii),sizeof(Unid) );
/*------------------------------------------------*/
#define PUTTAB(tabid,answer);     Copy(&(tabid->segid),\
                            &(answer.idracr.urn.segnum),sizeof(Segid) );\
     Copy(&(tabid->untabid),&(answer.idracr.urn.obnum),sizeof(Unid) );\
     tid = ((i4_t)(answer.idracr.pagenum)<<16)|(i4_t)(answer.idracr.index);\
     Copy(&(tabid->tabd),&(tid),sizeof(Tid) );

/*================================================================*/

u2_t *
mk_short_arr(i4_t num, i4_t *arr)
{
  i4_t i, align;
  u2_t *res;
  
  if ((align = (pointbuf - buffer)%SZ_LNG))
    pointbuf+= SZ_LNG - align;
  res = (u2_t *)pointbuf;
  pointbuf += sizeof(u2_t) * num;
  for(i=0; i<num; i++)
    {
      assert(arr[i]>=0); 
      assert(arr[i]<0xffffL); 
      res[i] = (u2_t)arr[i];
    }
  return res;
} /* mk_short_arr */

static i4_t
DU_to_colval(i4_t nv, data_unit_t *ins_from, sql_type_t *dt_types,
             void **colval, u2_t *lenval)
{
  i4_t i, err;
  TpSet *Place;
  data_unit_t *dt_from;
  sql_type_t *to_type;
  
  for (i = 0; i < nv; i++)
    if (ins_from[i].dat.nl_fl == REGULAR_VALUE)
      {
        dt_from = ins_from + i;
        to_type = dt_types + i;
        if (dt_from->type.code == T_STR)
          {
            lenval[i] = dt_from->type.len;
            colval[i] = STR_PTR (&(dt_from->dat));
          }
        else
          {
            if (to_type)
              {
                lenval[i] = sizeof_sqltype (*to_type);
                if (to_type->code != dt_from->type.code)
                  {
                    Place = (TpSet *) pointbuf;
                    err = put_dat (&(dt_from->dat), 0, dt_from->type.code, REGULAR_VALUE,
                                   Place, 0, to_type->code, NULL);
                    if (err < 0)
                      return err;
                    colval[i] = pointbuf;
                    pointbuf += sizeof (TpSet);
                  }
                else
                  colval[i] = &(dt_from->dat);
              }
            else
              {
                lenval[i] = dt_from->type.len;
                colval[i] = &(dt_from->dat);
              }
          }
      }
    else
      colval[i] = NULL;
  return 0;
}

i4_t 
opentscan (Tabid * tabid, i4_t *spn, char mode, i4_t nr, i4_t *rlist,
	   i4_t nc, cond_buf_t cond, i4_t nm, i4_t *mlist)
#if 0
     /* table scan opening without using of index or filter,
      *	function value if scan identifier or negative number
      *	containing error code
     */
     Tabid *tabid;	/* relation identifier    */
     i4_t *spn;		/* the number or control point till wich
			 * rollback has to be done when breaking of 
			 * scan open was coused by deadlock           */
     char mode;		/* open mode:string containing wether one of
			 * symbols R-reading, D-deleting, M-updating
			 * or combination of D and M                  */
     i4_t nr;	        /* the number of columns which values will
			 * be selected                                */
     i4_t rlist[];       /* array of column numbers which values will
			 * be selected                                */
     i4_t nc;            /* size of select condition                   */
     cond_buf_t cond;   /* condition of current string selection
                         * NULL - condition is absent                 */
     i4_t nm;	        /* the number of column which values will
			 * be updated                 		      */
     i4_t mlist[];	/* array of column numbers which values will
			 * be updated                                 */
#endif
{
  i4_t stat;
  struct ans_opsc answer;
  struct id_rel pidrel;
  
  if (cl_debug)
    fprintf (STDOUT, "BASE.opentscan \n");
  TRANTAB (pidrel, tabid);
  INITBUF;
  
  answer = opscrel (&pidrel, mode, nr, mk_short_arr (nr, rlist),
		    nc, cond, nm, mk_short_arr (nm, mlist));
  stat = answer.cpnops;
  if (stat < 0)
    return (stat);
  if (stat > 0)
    *spn = stat;
  if (cl_debug)
    fprintf (STDOUT, "BASE.opentscan  answer=%d\n", stat);
  return (answer.scnum);
} /* opentscan */

/*--------------------------------------------------------------*/

i4_t 
openiscan (Indid * indid, i4_t *spn, char mode, i4_t nr, i4_t *rlist,
     i4_t nc, cond_buf_t cond, i4_t nra, cond_buf_t range, i4_t nm, i4_t *mlist)
#if 0
     /* table scan opening with using of index or filter,
      *	function value is scan identifier or negative number
      *	comtaining error code
     */
   
     Indid *indid;	/* index identifier */
     i4_t *spn;	
                	/* the number or control point till wich      *
			 * rollback has to be done when breaking of   *
			 * scan open was coused by deadlock           */
		              
     char mode;	       /* open mode: string containig wether one      *
			* symbol from set: R - reading, D - removing, *
			* M - updating or pair of D and M             */
                           
     i4_t nr;	       /* The number of columns which values will be  *
                        * selected                                    */
     i4_t rlist[];      /* array of columns' numbers                   */
     i4_t    nc;        /* size of select condition                    */
     Cond *cond;       /* select condition of current string          *
	                * NULL - means absence of condition           */
     i4_t nra;	       /*                                             */
     Cond *range;      /* scan range - restricted form of condition   *
                        * applied to columns locating in idex.        *
                        * NULL - condition is absent                  */
     i4_t nm;	       /* the number of columns which values will be  */
                        * modified                                    */
     i4_t mlist[];      /* array of columns' numbers which values will *
                        * be modified                                 */
#endif
{
  i4_t stat;
  struct ans_opsc answer;
  struct id_ind pidind;

  if (cl_debug)
    fprintf (STDOUT, "BASE.openiscan nm=%d nr=%d unindid=%d\n", nm, nr,
	     (i4_t)((*indid).unindid));

  TRANIND (pidind, indid);
  INITBUF;
  
  answer = opscin (&pidind, mode, nr, mk_short_arr (nr, rlist),
		   nc, cond, nra, range, nm, mk_short_arr (nm, mlist));
  
  stat = answer.cpnops;

  if (stat < 0)
    return (stat);
  if (stat > 0)
    *spn = stat;
  return (answer.scnum);
} /* openiscan */

/**********************************************************************/
i4_t 
openfscan (Filid * filid, char *mode, i4_t nr, i4_t *rlist,
	   i4_t nc, cond_buf_t cond, i4_t nm, i4_t *mlist)
#if 0
     /* table scan opening with using of filter,
      *	function value is scan identifier or negative number
      *	comtaining error code
     */
    
     Filid *filid;      /* filter identifier                          */
     char *mode;        /* open mode: string containig wether one     *
		         * symbol from set: R - reading, D - removing,*
		         * M - updating or pair of D and M            */
      i4_t nr;	        /* the number of columns which values will
			 * be selected                                */
     i4_t rlist[];       /* array of column numbers which values will
			 * be selected                                */
     i4_t nc;            /* size of select condition                   */
     cond_buf_t cond;   /* condition of current string selection
                         * NULL - condition is absent                 */
     i4_t nm;	        /* the number of column which values will
			 * be updated                 		      */
     i4_t mlist[];	/* array of column numbers which values will
			 * be updated                                 */
   
#endif
{
  INITBUF;
  if (cl_debug)
    fprintf (STDOUT, "BASE.openfscan nm=%d nr=%d filid=%d\n", nm, nr, *filid);

  return opscfl (*filid, *mode, nr, mk_short_arr (nr, rlist),
		 nc, cond, nm, mk_short_arr (nm, mlist));
} /* openfscan */

/**********************************************************************/
i4_t 
findrow (Scanid scanid, data_unit_t **colval)
#if 0
     /* scan pointer locating on next table string satisfying
      *	the select condition and selecting values from this
      *	string;
      *	in the case of correct finish of function
      *	the function value is equal value of colval's parameter,
      *	NULL means the the finish of scaning;
      *	in the case of error the function value is equal
      *	neither colval nor NULL.                          */
     
     Scanid scanid;	      /* scan identifier */
     data_unit_t **colval;    /* array of pointers used for location values
			       * of current string; the number of pointers
			       * in the array has to be equal the value of
			       * parameter nr defined during the scan opening
                	       */
#endif
{
  return next (scanid, colval);
} /* findrow */

/**********************************************************************/
i4_t 
read_row (Scanid scanid, i4_t nr, i4_t *rlist, data_unit_t **colval)
#if 0
     /* reading the values of given column set of current
      * scan string
      *	in the case of correct finish of function
      *	the function value is equal value of colval's parameter,
      *	NULL means the the finish of scaning;
      *	in the case of error the function value is equal
      *	neither colval nor NULL.                          */
     Scanid scanid;    /* scan identifier                   */
     i4_t nr;	       /* the number of columns which values will    *
		        * be selected                                */
     i4_t rlist[];     /* array of column numbers which values will  *
		        * be selected                                */
     data_unit_t **colval;    /* array of pointers for current string*
                        * location; the number of pointer in the     *
                        * array has to be equal "nr"                 */
#endif
{
  INITBUF;
  return readrow (scanid, nr, mk_short_arr (nr, rlist), colval);
} /* read_row */

/**********************************************************************/
i4_t 
saposit (Scanid scanid)
#if 0
/*  mempos(scnum)  */
/* stores the current position of pointed scan;
   function returns value >=0 in the case of correct
   finish and <0 in the case of error                     */
     Scanid scanid;
#endif
{
  if (cl_debug)
    fprintf (STDOUT, "BASE: saposit\n");
  return mempos (scanid);
} /* saposit */ 

/**********************************************************************/
i4_t 
reposit (Scanid scanid)
#if 0
/*  curpos(scnum)  */
/* recovers stored position of pointed scan;
   returns values which >=0 in the case of correct
   finish and <0 in the case of error                     */
     Scanid scanid;
#endif
{
  if (cl_debug)
    fprintf (STDOUT, "BASE: reposit\n");
  return curpos (scanid);
} /* reposit */

/**********************************************************************/
i4_t 
delrow (Scanid scanid)
#if 0
/*  dltn(scnum)  */
/* removing of current string of pointed scan;
 * function returns 0 in the case of correct finish and
 * negative value in the case of error; positive result
 * corresponds number of control point to which
 * transaction has to rollback                          */
     Scanid scanid;    /* scan identifier                 */
#endif
{
  if (cl_debug)
    fprintf (STDOUT, "BASE: delrow\n");
  
  return dltn (scanid);
} /* delrow */

/**********************************************************************/
i4_t 
mod_data (Scanid scanid, i4_t nm, data_unit_t *mod_from,
	  sql_type_t *dt_types, i4_t *mlist)
#if 0
     /* updating of current string of pointed scan;       *
      * function returns "0" in the case of correct finish*
      * , negative value in the case of error, positive   *
      * value corresponds number of control point to wich *
      * given transaction is needed to rollback           */
     Scanid scanid;      /* scan identifier                                */
     i4_t nm;	         /* the number of modification elements            */
     data_unit_t *mod_from; /* array of data for modification : needed element*
			    number i is on mlist[i] place in this array    */
     sql_type_t *dt_types;     /* array of types of data for modification        */
     i4_t mlist[];        /* array of numbers of updated columns            */

#endif
{
  i4_t stat;
  u2_t lenval_stat[256];
  void *colval_stat[256];
  u2_t *lenval;
  void **colval;
  
  if (nm <= 256)
    {
      lenval = lenval_stat;
      colval = colval_stat;
    }
  else
    {
      lenval = xmalloc (sizeof(u2_t)*nm);
      colval = xmalloc (sizeof(*colval)*nm);
    }
  
  INITBUF;
  if (cl_debug)
    fprintf (STDOUT, "BASE: modrow\n");
  stat = DU_to_colval(nm, mod_from, dt_types, colval, lenval);
  if (stat >= 0)
    stat = mod_spec_flds (scanid, nm, mk_short_arr (nm, mlist), colval, lenval);
  if (nm > 256)
    {
      xfree(lenval);
      xfree(colval);
    }
  return (stat);
} /* mod_data */

/**********************************************************************/
i4_t 
insrow (Tabid * tabid, u2_t * lenval, Colval colval)
#if 0
     /* inserting of current string of pointed scan;      *
      * function returns "0" in the case of correct finish*
      * , negative value in the case of error, positive   *
      * value corresponds number of control point to wich *
      * given transaction is needed to rollback           */
     Tabid *tabid;	/* table identifier                         */
     u2_t *lenval;	/* array of lenghts of inserted elements    */
     Colval colval;	/* array of pointers to columns' values of  *
                         * inserted string; order of pointers has to*
                         * corresponds table definition; NULL       *
                         * corresponds undefined value of given     *
                         * column; "nv" could be less than the      *
                         * number of columns in the table, undefined*
                         * values for all other are supposed        */
#endif
{
  struct id_rel pidrel;
  
  TRANTAB (pidrel, tabid);

  return insrtn (&pidrel, lenval, colval);

} /* insrow */

/**********************************************************************/
i4_t 
ins_data (Tabid *tabid, i4_t nv, data_unit_t *ins_from, sql_type_t *dt_types)
#if 0
     /* inserting of current string of pointed scan;      *
      * function returns "0" in the case of correct finish*
      * , negative value in the case of error, positive   *
      * value corresponds number of control point to wich *
      * given transaction is needed to rollback           */
     Tabid *tabid;	       /* table identifier                     */
     i4_t nv;		       /* the number of insertion elements     */
     data_unit_t *ins_from;    /* array of data for insertion          */
     sql_type_t *dt_types;     /* array of types of data for insertion */
#endif
{
  i4_t err;
  struct id_rel pidrel;
  u2_t lenval_stat[256];
  void *colval_stat[256];
  u2_t *lenval;
  void **colval;
  
  if (nv <= 256)
    {
      lenval = lenval_stat;
      colval = colval_stat;
    }
  else
    {
      lenval = xmalloc (sizeof(u2_t)*nv);
      colval = xmalloc (sizeof(*colval)*nv);
    }
  
  INITBUF;
  TRANTAB (pidrel, tabid);

  err = DU_to_colval(nv, ins_from, dt_types, colval, lenval);
  if (err >= 0)
    err = insrtn (&pidrel, lenval, colval); 
  if (nv > 256)
    {
      xfree(lenval);
      xfree(colval);
    }
  return err;
} /* ins_data */

/**********************************************************************/
i4_t 
instid (Scanid scanid, Filid filid)
#if 0
     /* inserting of identifier of current scan string    *
      * in existing filter; function returns value >=0 in *
      * the case of succesful finish and negative value   *
      * in the case of error                              */
     Scanid scanid;		/* scan identifier        */
     Filid filid;		/* filter identifier      */
#endif
{
  if (cl_debug)
    fprintf (STDOUT, "BASE: instid\n");
  
  return insfltr (scanid, filid);
} /* instid */
/**********************************************************************/
i4_t 
closescan (Scanid scanid)
#if 0
     /* closes pointed scan;                       *
      * function returns value >=0 in  the case of *
      * succesful finish and negative value        *
      * in the case of error                       */ 
     Scanid scanid;		/* scan identifier */

#endif
{
  return closesc (scanid);
} /* closescan */

/**********************************************************************/
i4_t 
crepview (Tabid * tabid, Segid segid, i4_t colnum, sql_type_t * types)
#if 0
     /* view creation                              * 
      * function returns value >=0 in  the case of *
      * correct finish and negative value          *
      * in the case of error                       */
     Tabid *tabid;    /* output parameter - relation identifier  */
     Segid segid;     /* segment identifier                      */
     i4_t colnum;            /* the number of columns in the view      */
     sql_type_t types[];   /* array of columns' descriptions         */

#endif
{
  i4_t stat, i;
  struct ans_ctob answer;
  struct des_field descol_stat[256];
  struct des_field *descol;

  if (colnum <= 256)
    descol = descol_stat;
  else
    descol = xmalloc (sizeof(*descol)*colnum);
  

  if (cl_debug)
    fprintf (STDOUT, "BASE: create view colnum=%d\n", colnum);

  for (i = 0; i < colnum; i++)
    {
      descol[i].field_type = types[i].code;
      descol[i].field_size = types[i].len;
    }
  
  answer = crview (segid, colnum, descol);

  if (cl_debug)
    fprintf (STDOUT, "BASE.after crview: answer.cpncob=%d\n", answer.cpncob);

  stat = answer.cpncob;
  if (stat >= 0)
    {
      Copy (&(tabid->segid), &(answer.idob.segnum), sizeof (Segid));
      Copy (&(tabid->untabid), &(answer.idob.obnum), sizeof (Unid));
      if (cl_debug)
        fprintf (STDOUT, "BASE.crepview: segid=%d,untabid=%d\n", tabid->segid,
                 (i4_t)tabid->untabid);
    }
  if (colnum > 256)
    xfree(descol);
  
  return (stat);
} /* crepview */

/**********************************************************************/
i4_t 
creptab (Tabid * tabid, Segid segid, i4_t colnum,
	 i4_t nnulnum, sql_type_t * types)
#if 0
     /* creation of relation (non temporal)        *
      * function returns value ==0 in  the case of *
      * correct finish and negative value          *
      * in the case of error                       */
     /* or number of control point to which            *
      * rollback would be needed to do if operation    *
      * were failed because of synchronization deadlock*/
     Tabid *tabid;   /* output parameter - relation identifier */
     Segid segid;    /* segment identifier                     */
     i4_t colnum;     /* the number of columns in the table            */
     i4_t nnulnum;    /* the number of first columns which have not to *
                      * contain undefined values                       */
     sql_type_t types[];    /* array of columns' descriptions          */
#endif
{
  i4_t stat, tid, i;
  struct ans_cr answer;
  struct des_field descol_stat[256];
  struct des_field *descol;

  if (colnum <= 256)
    descol = descol_stat;
  else
    descol = xmalloc (sizeof(*descol)*colnum);
  

  if (cl_debug)
    fprintf (STDOUT, "BASE: create rel CRR colnum=%d,nnulnum=%d\n",
	     colnum, nnulnum);
  for (i = 0; i < colnum; i++)
    {
      descol[i].field_type = types[i].code;
      descol[i].field_size = types[i].len;
    }
  answer = crrel (segid, colnum, nnulnum, descol);
  stat = answer.cpnacr;
  if (stat == 0)
    {
      Copy (&(tabid->segid), &(answer.idracr.urn.segnum), sizeof (Segid));
      Copy (&(tabid->untabid), &(answer.idracr.urn.obnum), sizeof (Unid));
      tid = ((i4_t) (answer.idracr.pagenum) << 16) | (i4_t) (answer.idracr.index);
      Copy (&(tabid->tabd), &(tid), sizeof (Tid));
    }
  if (colnum > 256)
    xfree(descol);
  
  return (stat);
} /* creptab */

/**********************************************************************/
i4_t 
crettab (Tabid * tabid, i4_t colnum, i4_t nnulnum, sql_type_t * types)
#if 0
     /* creation of temporal relation              *
      * function returns value >=0 in  the case of *
      * correct finish and negative value          *
      * in the case of error                       */
     Tabid *tabid;    /* output parameter - relation identifier        */
     i4_t nnulnum;     /* the number of first columns which have not to *
                       * contain undefined values                      */
     i4_t colnum;		/* the number of columns in the table  */
     sql_type_t types[];	/* array of columns' descriptions      */
#endif
{
  i4_t i, stat;
  struct ans_ctob answer;
  struct des_field descol_stat[256];
  struct des_field *descol;

  if (colnum <= 256)
    descol = descol_stat;
  else
    descol = xmalloc (sizeof(*descol)*colnum);
  
  for (i = 0; i < colnum; i++)
    {
      descol[i].field_type = types[i].code;
      descol[i].field_size = types[i].len;
    }
  
  answer = crtrel (colnum, nnulnum, descol);
  
  stat = answer.cpncob;
  if (stat >= 0)
    {
      Copy (&(tabid->segid), &(answer.idob.segnum), sizeof (Segid));
      Copy (&(tabid->untabid), &(answer.idob.obnum), sizeof (Unid));
      if (cl_debug)
        fprintf (STDOUT, "BASE.crettab: segid=%d,untabid=%d\n", tabid->segid,
                 (i4_t)tabid->untabid);
    }
  if (colnum > 256)
    xfree(descol);
  return (stat);
} /* crettab */

/**********************************************************************/
i4_t 
crefil (Filid * filid, Tabid * tabid)
#if 0
     /* filter creation                            *
      * function returns value >=0 in  the case of *
      * correct finish and negative value          *
      * in the case of error                       */
     Filid *filid;	/* output parameter - filter identifier      */
     Tabid *tabid;	/* relation identifier                       */
#endif
{
  i4_t stat;
  struct ans_ctob answer;
  struct id_rel pidrel;
  
  if (cl_debug)
    fprintf (STDOUT, "BASE.crefil \n");
  TRANTAB (pidrel, tabid);
  
  answer = crfltr (&pidrel);
  stat = answer.cpncob;
  if (stat >= 0)
    Copy (filid, &(answer.idob), sizeof (answer.idob));
   return (stat);
} /* crefil */

/**********************************************************************/
i4_t 
creind (Indid * indid, Tabid * tabid, char unique,
	char clust, i4_t ncn, i4_t *cnlist)
#if 0
     /* index creation                             *
      * function returns value ==0 in  the case of *
      * correct finish and negative value          *
      * in the case of error                       */
     /* or number of control point to which            *
      * rollback would be needed to do if operation    *
      * were failed because of synchronization deadlock*/
     Indid *indid;   /* output parameter - index identifier */
     Tabid *tabid;   /* relation identifier                 */
     char unique;    /* sign of unique key: 'U'                        */
     char clust;     /*                      */
     i4_t ncn;	     /* the number of columns for key of index         */
     i4_t cnlist[];   /* array of numbers of columns forming index's key*/

#endif
{
  i4_t tid, stat;
  struct ans_cind answer;
  struct id_rel pidrel;
  
  if (cl_debug)
    fprintf (STDOUT, "BASE.creind \n");
  INITBUF;
  TRANTAB (pidrel, tabid);
  
  answer = crind (&pidrel, unique, clust, ncn, mk_short_arr (ncn, cnlist));
  stat = answer.cpnci;
  if (stat < 0)
    return (stat);
  PUTIND (indid, answer);
  if (cl_debug)
    fprintf (STDOUT, "BASE.creind: segid=%d,untabid=%d,tid=%d,unindid=%d\n",
	     (*indid).tabid.segid, (i4_t)(*indid).tabid.untabid,
	     (i4_t)(*indid).tabid.tabd, (i4_t)(*indid).unindid);
  return (stat);
} /* creind */

/**********************************************************************/
i4_t 
dropptab (Tabid * tabid)
#if 0
     Tabid *tabid;    /* relation identifier */
     
     /* dropping of non temporal relation;            * 
      * function returns 0 in  the case of correct    *
      * finish, negative value in the case of error   *
      * and positive value, whiich denotes the number *
      * of control point to which rollback            *
      *	has to be done when execution of operation    *
      *	was broken because of deadlock                */
#endif
{
  struct id_rel pidrel;
  
  if (cl_debug)
    fprintf (STDOUT, "BASE.droptab \n");
  TRANTAB (pidrel, tabid);
  
  return delrel (&pidrel);
} /* dropptab */

/**********************************************************************/
i4_t 
dropttab (Tabid * tabid)
#if 0
     /* dropping of temporal relation;             * 
      * function returns value >=0 in  the case of *
      * correct finish and negative value          *
      * in the case of error                       */
     Tabid *tabid;		/* relation identifier */
#endif
{
  struct id_ob pidtob;
  
  if (cl_debug)
    fprintf (STDOUT, "BASE.dropttab \n");
  Copy (&(pidtob.segnum), &(tabid->segid), sizeof (Segid));
  Copy (&(pidtob.obnum), &(tabid->untabid), sizeof (Unid));
  
  return deltob (&pidtob);
} /* dropttab */

/**********************************************************************/
i4_t 
dropind (Indid * indid)
#if 0
     /* index dropping;
      * function returns 0 in  the case of correct        *
      * finish, negative value in the case                *
      * of error and positive value denotes the point to  *
      * which given tranzaction has to rollback           */
     /* or number of control point to which rollback      *
      *	has to be done when execution of operation        *
      *	was broken because of deadlock                    */
     Indid *indid;   /* index identifier                  */
#endif
{
  struct id_ind pidind;
  
  if (cl_debug)
    fprintf (STDOUT, "BASE.dropind \n");
  TRANIND (pidind, indid);
  
  return delind (&pidind);
} /* dropind */

/**********************************************************************/
i4_t 
addcol (Tabid * tabid, i4_t ncn, sql_type_t * types)
#if 0
/* addflds(pidrel,fn,afn)     */
     /* column adding to the pointed relation;  function      *
      * returns 0 in the case of correct finish, negative     *
      * value in the case of error and positive value denotes *
      * the point to which given tranzaction has to rollback  */
     /* number of control point to which rollback  *
      * has to be done when execution of operation *
      * was broken because of deadlock             */
     Tabid *tabid;	/* relation identifier                        */
     i4_t ncn;		/* size of array of columns' descriptions     */
     sql_type_t types[];	/* array of columns' descriptions     */
#endif
{
  i4_t i;
  struct id_rel pidrel;
  struct des_field descol_stat[256];
  struct des_field *descol;

  if (ncn <= 256)
    descol = descol_stat;
  else
    descol = xmalloc (sizeof(*descol)*ncn);
  
  if (cl_debug)
    fprintf (STDOUT, "BASE.addcol \n");
  TRANTAB (pidrel, tabid);
  
  for (i = 0; i < ncn; i++)
    {
      descol[i].field_type = types[i].code;
      descol[i].field_size = types[i].len;
    }
  i = addflds (&pidrel, ncn, descol);

  if (ncn > 256)
    xfree(descol);

  return i;
} /* addcol */

/**********************************************************************/
i4_t 
savepoint (void)
     /* setting of control(save) point in the current tranzaction *
      * function returns number of control(save) point            */
{
  if (cl_debug)
    fprintf (STDOUT, "BASE.savepoint\n");
  return svpnt ();
} /* savepoint */

/**********************************************************************/

/**********************************************************************/

i4_t 
sorttab (Tabid * itabid, Tabid * otabid, i4_t ns, i4_t *slist, char order, char fl)
#if 0
     /* sorting of temporal table in the order of decrease *
      *  or increase of values of pointed columns; locates *
      * result in the new temporal relation                */
     Tabid *itabid;	 /* identifier of source temporal relation */
     Tabid *otabid;	 /* identifier of a new temporal relation  */
     i4_t ns;		 /* the number of columns which values are *
			  * used in sorting                        */
     i4_t slist[];	 /* array of columns' numbers which values *
			  * are used in sorting                    */
     char order;	 /* indicator of sort order: 'A' - increase*
			  *, 'D' (or any other symbol exept 'A') - *
			  * decrease                               */
     char fl;		 /* = CH_ALL | CH_UNIC - unique flag       */
/*    char  order[]; array of indicator of sort order              */
#endif
{
  i4_t stat, i;
  struct ans_ctob answer;
  struct id_rel pidrel;
  char ord;
  char  order_arr_stat[256];
  char *order_arr;
  
  if (ns <= 256)
    order_arr = order_arr_stat;
  else
    order_arr = xmalloc(sizeof(*order_arr)*ns);
  
  if (cl_debug)
    fprintf (STDOUT, "BASE.sorttab \n");
  INITBUF;
  if (order == 'A')
    ord = GROW;
  else
    ord = DECR;
  for (i = 0; i < ns; i++)
    order_arr[i] = ord;
  Copy (&(pidrel.urn.segnum), &(itabid->segid), sizeof (Segid));
  Copy (&(pidrel.urn.obnum), &(itabid->untabid), sizeof (Unid));
  
  answer = trsort (&pidrel, ns, mk_short_arr (ns, slist),
		   order_arr, (fl == CH_UNIC) ? NODBL : DBL);
  stat = answer.cpncob;
  if (stat >= 0)
    {
      Copy (&(otabid->segid), &(answer.idob.segnum), sizeof (Segid));
      Copy (&(otabid->untabid), &(answer.idob.obnum), sizeof (Unid));
      
      if (cl_debug)
        fprintf (STDOUT, "BASE.sorttab: segid=%d,untabid=%d\n",
                 otabid->segid, (i4_t)otabid->untabid);
    }
  if(ns>256)
    xfree(order_arr);
  return (stat);
} /* sorttab */


/*--------------------------------------------------------------*/
i4_t 
make_group (Tabid * itabid, Tabid * otabid, i4_t ng, i4_t *glist,
	    char order, i4_t nf, i4_t *flist, char *fl)
#if 0
     /* group separation in temporal table; function     *
      * locates result in a new temporal relation and    *
      * counts agregate functions; new temporal relation *
      * is sorted by the group's columns in the given    *
      * order ("order")                                  */
     Tabid *itabid;	/* identifier of source temporal relation  */
     Tabid *otabid;	/* identifier of new temporal relation; if *
			 * agregate functions are there then last  *
			 * "nf" columns correspond "flist[]"       */
     i4_t ng;		/* the number of columns which are included*
			 * in the group                            */
     i4_t glist[];	/* array of columns which are included in  *
			 * the group                               */	
     char order;	/* indicator of sort order: 'A' - increase *
			 *, 'D' (or any other symbol exept 'A') -  *
			 * decrease                                */
     i4_t nf;		/* the number of columns for which         *
			 * functions have to be counted            */
     i4_t flist[];	/* array of columns' numbers for which     *
			 * functions have to be counted            */
     char *fl;		/* char fl[nf-1] - string of flags         *
	                 * corresponding every element of из flist[]. */
#endif
{
  i4_t stat, i;
  struct ans_ctob answer;
  struct id_rel pidrel;
  char ord;
  char  order_arr_stat[256];
  char *order_arr;
   
  if (ng <= 256)
    order_arr = order_arr_stat;
  else
    order_arr = xmalloc(sizeof(*order_arr)*ng);
  
  INITBUF;
  if (order == 'A')
    ord = GROW;
  else
    ord = DECR;
  for (i = 0; i < ng; i++)
    order_arr[i] = ord;
  if (cl_debug)
    fprintf (STDOUT, "BASE.makegroup \n");
  Copy (&(pidrel.urn.segnum), &(itabid->segid), sizeof (Segid));
  Copy (&(pidrel.urn.obnum), &(itabid->untabid), sizeof (Unid));

  answer = makegroup (&pidrel, ng, mk_short_arr (ng, glist),
		      nf, mk_short_arr (nf, flist), fl, order_arr);
  stat = answer.cpncob;
  if (stat >= 0)
    {
      Copy (&(otabid->segid), &(answer.idob.segnum), sizeof (Segid));
      Copy (&(otabid->untabid), &(answer.idob.obnum), sizeof (Unid));
      if (cl_debug)
        fprintf (STDOUT, "BASE.makegroup: segid=%d,untabid=%d\n",
                 otabid->segid, (i4_t)otabid->untabid);
    }
  if(ng>256)
    xfree(order_arr);
  return (stat);
} /* make_group */

/*------------------ E N D   P A R T 1 ----------------------*/
