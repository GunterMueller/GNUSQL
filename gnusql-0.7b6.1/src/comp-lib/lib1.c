/*
 *  lib1.c  -  DB engine interface (part2)
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

/* $Id: lib1.c,v 1.249 1998/09/29 21:24:51 kimelman Exp $ */

#include "global.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "engine/destrn.h"
#include "engine/strml.h"
#include "engine/expop.h"
#include "exti.h"
#include "engine/fdcltrn.h"
#include "type_lib.h"
/*---------------------------------------------*/
EXTERNAL char buffer[SOC_BUF_SIZE];
EXTERNAL char res_buf[SOC_BUF_SIZE];
EXTERNAL char *pointbuf;
/*---------------------------------------------*/
#define RTN(s)        if( s < 0 ) return (s )
#define INITBUF       pointbuf = buffer
/*---------------------------------------------*/
#define TRANIND(pidind,indid);  \
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
#define TRANTAB(pidrel,tabid); \
{\
u2_t page, index;\
     page=(u2_t)((tabid->tabd)>>16);\
     index=(u2_t)((tabid->tabd)&0xFFFF);\
     Copy(&(pidrel.urn.segnum),&(tabid->segid ),sizeof(Segid) );\
     Copy(&(pidrel.urn.obnum ),&(tabid->untabid),sizeof(Unid)  );\
     Copy(&(pidrel.pagenum),&page,sizeof(u2_t) );\
     Copy(&(pidrel.index),&index,sizeof(u2_t) );\
}
     /*-------------------------------------------------------*/
u2_t *mk_short_arr(i4_t num, i4_t *arr);

i4_t 
uidnsttab (Tabid * i1tabid, Tabid * i2tabid, Tabid * otabid, i4_t code)
#if 0
     /* Theory-set uniting or intersection or difference
	of two temporal	relations sorted in the same order
	with duplicate removing and locating results of this
	in the new relation sorted in the same columns and order
     */
 
     Tabid *i1tabid;	/* identifier of the first original
			   temporal relation                    */ 	

     Tabid *i2tabid;	/* identifier of the second original
			   temporal relation                    */ 	
	

     Tabid *otabid;	/* identefier of the result
			   temporal relation                    */
#endif
{
  i4_t stat;
  struct ans_ctob answer;
  struct id_ob pit1, pit2;
  
  if (cl_debug)
    {
      if( code==(i4_t) BLDUN )
	fprintf (STDOUT, "BASE.unsttab \n");
      if ( code==(i4_t) INTRSC )
	fprintf (STDOUT, "BASE.intsttab \n");
      if ( code== (i4_t)DFFRNC )
	fprintf (STDOUT, "BASE.difsttab \n");
    }
  Copy (&(pit1.segnum), &(i1tabid->segid), sizeof (Segid));
  Copy (&(pit1.obnum), &(i1tabid->untabid), sizeof (Unid));
  Copy (&(pit2.segnum), &(i2tabid->segid), sizeof (Segid));
  Copy (&(pit2.obnum), &(i2tabid->untabid), sizeof (Unid));
  
  switch (code)
    {
    case BLDUN :  answer = bdunion   (&pit1, &pit2);
                  break;
    case INTRSC : answer = intersctn (&pit1, &pit2);
                  break;
    case DFFRNC : answer = differnc  (&pit1, &pit2);
                  break;
    }
  
  stat = answer.cpncob;
  Copy (&(otabid->segid), &(answer.idob.segnum), sizeof (Segid));
  Copy (&(otabid->untabid), &(answer.idob.obnum), sizeof (Unid));
  if (cl_debug)
    {
      if( code==(i4_t) BLDUN )
	fprintf (STDOUT, "BASE.unsttab: segid=%d,untabid=%d\n",
		 otabid->segid, (i4_t)otabid->untabid);
      if ( code==(i4_t) INTRSC )
	fprintf (STDOUT, "BASE.insttab: segid=%d,untabid=%d\n",
		 otabid->segid, (i4_t)otabid->untabid);
      if ( code==(i4_t)DFFRNC )
	fprintf (STDOUT, "BASE.difsttab: segid=%d,untabid=%d\n",
		 otabid->segid, (i4_t)otabid->untabid);
    }
  return (stat);
}

/**********************************************************************/

/**********************************************************************

i4_t 
bldttfil (Tabid * tabid, Filid * filid, i4_t nc, cond_buf_t cond, i4_t *spn)
#if 0
     * Building the sorted filter on the basis of scaning
	given constant table without index using  or filter.
	The function result is >= 0 in the case of successful
	finish and negative one containing the error code
	otherwise
     *
 
     Tabid *tabid;		 * Basic table identifier           *

     Filid *filid;		 * Result parameter -
				   filter identifier                *
              
     i4_t nc;			 *            *
     Cond *cond;		 * condition of current string selecting
				   NULL means absence of any conditions *

     i4_t *spn;			 * number of control point to which the
				   rolback has to be done in the case of
				   fail in the execution of operation
				   because of deadlock                  *
#endif
{
  i4_t stat;
  struct ans_ctob answer;
  struct id_rel pidrel;
  
  if (cl_debug)
    fprintf (STDOUT, "BASE.bldtftab \n");
  TRANTAB (pidrel, tabid);
  
  answer = blflrl (&pidrel, nc, cond);
  stat = answer.cpncob;
  Copy (filid, &(answer.idob), sizeof (filid));
  if (stat > 0)
    *spn = stat;
  return (stat);
}

**********************************************************************
i4_t 
bldtifil (Indid * indid, Filid * filid, i4_t nc, cond_buf_t cond, i4_t nra,
	  cond_buf_t range, i4_t *spn)
#if 0
      * Building the sorted filter on the basis of scaning
	given constant table with index using.
	The function result is >= 0 in the case of successful
	finish and negative one containing the error code
	otherwise
     *
 
     Indid *indid;		 * Basic table identifier           * 
     Filid *filid;		 * Result parameter -
				   filter identifier                * 
     i4_t nc;			 *             * 
     Cond *cond;		 * condition of current string selecting
				   NULL means absence of any conditions * 

     i4_t nra;			 *             * 
     Cond *range;		 * scan range is restricted form
				   of column condition put on the
				   columns included into index
				   NULL means undefined range         * 

     i4_t *spn;			 * number of control point to which the
				   rolback has to be done in the case of
				   fail in the execution of operation
				   because of deadlock                  *  
#endif
{
  i4_t stat;
  struct ans_ctob answer;
  struct id_ind pidind;
  
  if (cl_debug)
    fprintf (STDOUT, "BASE.bldtifil \n");
  TRANIND (pidind, indid);
  
  answer = blflin (&pidind, nc, cond, nra, range);
  stat = answer.cpncob;
  Copy (filid, &(answer.idob), sizeof (filid));
  if (stat > 0)
    *spn = stat;
  return (stat);
}

 ********************************************************************** 
i4_t 
bldtffil (Filid * ifilid, Filid * ofilid, i4_t nc, cond_buf_t cond)
#if 0 
 * Building the sorted filter on the basis of scaning
	given basic table with filter using.
	The function result is >= 0 in the case of successful
	finish and negative one containing the error code
	otherwise
     * 
  
     Filid *ifilid;		 * filter identifier * 
     Filid *ofilid;		 * Result parameter -
				   filter identifier                * 
     i4_t nc;			 *                 * 
     Cond *cond;		 * condition of current string selecting
				   NULL means absence of any conditions * 
#endif
{
  struct ans_ctob answer;
  
  answer = blflfl (&ifilid, nc, cond);
  Copy (ofilid, &(answer.idob), sizeof (answer.idob));
  return answer.cpncob;
}

 **********************************************************************/
i4_t 
bldtttab (Tabid * itabid, Tabid * otabid, i4_t nr, i4_t *rlist, i4_t nc,
	  cond_buf_t cond, i4_t *spn)
#if 0
     /* Building the sorted filter on the basis of scaning
	given table without index using  or filter.
	The function result is >= 0 in the case of successful
	finish and negative one containing the error code
	otherwise
     */

     Tabid *itabid;		/* Basic table identifier           */
     Tabid *otabid;		/* Result parameter -
				   temporal table identifier        */

     i4_t nr;			/* the number of columns which values
				   will be selected from basic table
				   and inserted in the temporal one  */

     i4_t rlist[];		/* arrays of column number which
				   values will be selected           */

     i4_t nc;			/*               */
     Cond *cond;		/* condition of current string selecting
				   NULL means absence of any conditions */

     i4_t *spn;			/* number of control point to which the
				   rolback has to be done in the case of
				   fail in the execution of operation
				   because of deadlock                  */
#endif
{
  i4_t stat;
  struct ans_ctob answer;
  struct id_rel pidrel;
  
  if (cl_debug)
    fprintf (STDOUT, "BASE. bldtttab\n");
  INITBUF;
  TRANTAB (pidrel, itabid);
  
  answer = rflrel (&pidrel, nr, mk_short_arr (nr, rlist), nc, cond);
  stat = answer.cpncob;
  Copy (&(otabid->segid), &(answer.idob.segnum), sizeof (Segid));
  Copy (&(otabid->untabid), &(answer.idob.obnum), sizeof (Unid));
  if (cl_debug)
    fprintf (STDOUT, "BASE.bldtttab: segid=%d,untabid=%d\n",
	     otabid->segid, (i4_t)otabid->untabid);
  if (stat > 0 && spn)
    *spn = stat;
  return (stat);
} /* bldtttab */

/**********************************************************************/
i4_t 
bldtitab (Indid * indid, Tabid * otabid, i4_t nr, i4_t *rlist, i4_t nc,
	  cond_buf_t cond, i4_t nra, cond_buf_t range, i4_t *spn)
#if 0
     /* Building of temporal table on basic of scaning given constant
	table with index using. 
	The function result is >= 0 in the case of successful finish
	and negative one containing the error code otherwise
	*/
   
     Indid *indid;		/* index identifier             */
     Tabid *otabid;		/* Result parameter -
				   temporal table identifier        */
          
     i4_t nr;			/* the number of columns which values
				   will be selected from basic table
				   and inserted in the temporal one  */

     i4_t rlist[];		/* arrays of column number which
				   values will be selected           */

     i4_t nc;			/*         */
     Cond *cond;		/* condition of current string selecting
				   NULL means absence of any conditions */

     i4_t nra;			/* range size                         */
     Cond *range;		/* scan range is restricted form
				   of column condition put on the
				   columns included into index
				   NULL means undefined range         */

     i4_t *spn;			/* number of control point to which the
				   rolback has to be done in the case of
				   fail in the execution of operation
				   because of deadlock                  */ 
#endif
{
  i4_t stat;
  struct ans_ctob answer;
  struct id_ind pidind;

  if (cl_debug)
    fprintf (STDOUT, "BASE.bldtitab \n");
  INITBUF;
  TRANIND (pidind, indid);
       
  answer = rflind (&pidind, nr, mk_short_arr (nr, rlist), nc, cond, nra, range);
  stat = answer.cpncob;
  Copy (&(otabid->segid), &(answer.idob.segnum), sizeof (Segid));
  Copy (&(otabid->untabid), &(answer.idob.obnum), sizeof (Unid));
  if (cl_debug)
    fprintf (STDOUT, "BASE.bldtitab: segid=%d,untabid=%d\n",
	     otabid->segid, (i4_t)otabid->untabid);
  if (stat > 0 && spn)
    *spn = stat;
  return (stat);
} /* bldtitab */

/**********************************************************************/
i4_t 
bldtftab (Filid * filid, Tabid * otabid, i4_t nr, i4_t *rlist, i4_t nc,
	  cond_buf_t cond)
#if 0
     /* Building of temporal table on basic of scaning given constant
	table with filter using. 
	The function result is >= 0 in the case of successful finish
	and negative one containing the error code otherwise
	*/
     
     Filid *filid;		/* filter identifier */
     Tabid *otabid;		/* Result parameter -
				   temporal table identifier        */

     i4_t nr;			/* the number of columns which values
				   will be selected from basic table
				   and inserted in the temporal one  */

     i4_t rlist[];		/* arrays of column number which
				   values will be selected           */

     i4_t nc;
     Cond *cond;		/* condition of current string selecting
				   NULL means absence of any conditions */
#endif
{
  i4_t stat;
  struct ans_ctob answer;
  
  INITBUF;
  answer = rflflt (*filid, nr, mk_short_arr (nr, rlist), nc, cond);
  stat = answer.cpncob;
  Copy (&(otabid->segid), &(answer.idob.segnum), sizeof (Segid));
  Copy (&(otabid->untabid), &(answer.idob.obnum), sizeof (Unid));
  if (cl_debug)
    fprintf (STDOUT, "BASE.bldtftab: segid=%d,untabid=%d\n",
	     otabid->segid, (i4_t)otabid->untabid);
  return (stat);
}

/**********************************************************************/
i4_t 
delttab (Tabid * tabid, i4_t nc, cond_buf_t cond, i4_t *spn)
#if 0
     /* string removing from given table which satisfy condition
	om basic of table scaning without index or filter using.
	The function result is >= 0 in the case of successful finish
	and negative one containing the error code otherwise
	*/

     Tabid *tabid;		/* table identifier                   */
     i4_t nc;			/* condition size ( in byte )          */
     Cond *cond;		/* condition of current string selecting
				   NULL means absence of any conditions */

     i4_t *spn;			/* number of control point to which the
				   rolback has to be done in the case of
				   fail in the execution of operation
				   because of deadlock                  */ 
#endif
{
  i4_t stat;
  struct id_rel pidrel;
  
  if (cl_debug)
    fprintf (STDOUT, "BASE. delttab\n");
  TRANTAB (pidrel, tabid);
  
  stat = delcrl (&pidrel, nc, cond);
  if (stat > 0)
    *spn = stat;
  return (stat);
}

/**********************************************************************/
i4_t 
delitab (Indid * indid, i4_t nc, cond_buf_t cond, i4_t nra, cond_buf_t range,
	 i4_t *spn)
#if 0
     /* string removing from given table which satisfy condition
	om basic of table scaning with index using.
	The function result is >= 0 in the case of successful finish
	and negative one containing the error code otherwise
	*/
 
     Indid *indid;		/* table identifier                   */
     i4_t nc;			/* condition size ( in byte )         */
     Cond *cond;		/* condition of current string selecting
				   NULL means absence of any conditions */

     i4_t nra;			/* range size                         */
     Cond *range;		/* scan range is restricted form
				   of column condition put on the
				   columns included into index
				   NULL means undefined range         */

     i4_t *spn;			/* number of control point to which the
				   rolback has to be done in the case of
				   fail in the execution of operation
				   because of deadlock                  */ 
#endif
{
  i4_t stat;
  struct id_ind pidind;
  
  TRANIND (pidind, indid);
  
  stat = delcin (&pidind, nc, cond, nra, range);
  if (stat > 0)
    *spn = stat;
  return (stat);
}

/**********************************************************************/
i4_t 
delftab (Filid * filid, i4_t nc, cond_buf_t cond, i4_t *spn)
#if 0
     /* string removing from given table which satisfy condition
	om basic of table scaning with filter using.
	The function result is >= 0 in the case of successful finish
	and negative one containing the error code otherwise
	*/
 
     Filid *filid;		/* filter identifier                  */
     i4_t nc;
     Cond *cond;		/* condition of current string selecting
				   NULL means absence of any conditions */

     i4_t *spn;			/* number of control point to which the
				   rolback has to be done in the case of
				   fail in the execution of operation
				   because of deadlock                  */ 
#endif
{
  i4_t stat;
  
  stat = delcfl (*filid, nc, cond);
  if (stat > 0)
    *spn = stat;
  return (stat);
}

/**********************************************************************/
i4_t 
modttab (Tabid * tabid, i4_t nc, cond_buf_t cond, i4_t nm, i4_t *mlist,
	 u2_t *lenval, Colval colval)
#if 0
     /* function updates strings in given table if they
	satisfies condition on the basic of table scaning 
	without index or filter using.
	The function result is == 0 in the case of successful finish,
	negative one containing the error code otherwise,
        or a number of control point to which the
        rolback has to be done in the case of
        fail in the execution of operation
        because of deadlock
	*/
 
     Tabid *tabid;		/* table identifier                   */
     i4_t nc;
     Cond *cond;		/* condition of current string selecting
				   NULL means absence of any conditions */

     i4_t nm;			/* the number of columns which values
				   will be modified                     */

     i4_t mlist[];		/* array  of columns which values
				   will be modified                     */

     u2_t *lenval;		/* pointer of lenght array              */
     Colval colval;		/* pointer of new values of current
				   string array; the number of pointers 
				   in array has to be equal the value of
				   "nm" parameter, NULL pointer is NULL
				   corresponds undefined value of 
				   responding column                    */

#endif
{
  struct id_rel pidrel;
  
  INITBUF;
  TRANTAB (pidrel, tabid);

  return modcrl (&pidrel, nc, cond, nm, mk_short_arr (nm, mlist), colval, lenval);
}

/**********************************************************************/
i4_t 
moditab (Indid * indid, i4_t nc, cond_buf_t cond, i4_t nr, cond_buf_t range,
	 i4_t nm, i4_t *mlist, u2_t * lenval, Colval colval)
#if 0
     /* function updates strings in given table if they
	satisfies condition on the basic of table scaning 
	with index using.
	The function result is == 0 in the case of successful finish
	and negative one containing the error code otherwise,
        or a number of control point to which the
        rolback has to be done in the case of
        fail in the execution of operation
        because of deadlock
	*/

     Indid *indid;		/* index identifier                   */
     i4_t nc;
     Cond *cond;		/* condition of current string selecting
				   NULL means absence of any conditions */

     i4_t nr;
     Cond *range;		/* scan range is restricted form
				   of column condition put on the
				   columns included into index
				   NULL means undefined range         */

     i4_t nm;			/* the number of columns which values
				   will be modified                     */

     i4_t mlist[];		/* array  of columns which values
				   will be modified                     */

     Colval lenval;		/* pointer of lenght array              */

     Colval colval;		/* pointer of new values of current
				   string array; the number of pointers 
				   in array has to be equal the value of
				   "nm" parameter, NULL pointer is NULL
				   corresponds undefined value of 
				   responding column                    */

     
#endif
{
  struct id_ind pidind;
  
  INITBUF;
  TRANIND (pidind, indid);
  
  return modcin (&pidind, nc, cond, nr, range, nm,
                 mk_short_arr (nm, mlist), colval, lenval);
}

/**********************************************************************/

i4_t 
modftab (Filid * filid, i4_t nc, cond_buf_t cond, i4_t nm, i4_t *mlist,
	 u2_t *lenval, Colval colval)
#if 0
     /* function updates strings in given table if they
	satisfies condition on the basic of table scaning 
	with filter using.
	The function result is == 0 in the case of successful finish
	and negative one containing the error code otherwise,
        or a number of control point to which the
        rolback has to be done in the case of
        fail in the execution of operation
        because of deadlock
	*/
 
     Filid *filid;		/* filter identifier                  */
     i4_t nc;			/*        */
     Cond *cond;		/* condition of current string selecting
				   NULL means absence of any conditions */

     i4_t nm;			/* the number of column number  which
				   values will be modified              */

     i4_t mlist[];		/* array  of columns which values
				   will be modified                     */

     Colval lenval;             /* pointer of lenght array              */
     Colval colval;		/* pointer of new values of current
				   string array; the number of pointers 
				   in array has to be equal the value of
				   "nm" parameter, NULL pointer is NULL
				   corresponds undefined value of 
				   responding column                    */


#endif
{
  INITBUF;
  
  return modcfl (*filid, nc, cond, nm, mk_short_arr (nm, mlist), colval, lenval);
}

/**********************************************************************/
i4_t 
instttab (Tabid * itabid, Tabid * otabid, i4_t nr, i4_t *rlist,
	  i4_t nc, cond_buf_t cond, i4_t *spn)
#if 0
     /* inserts set of strings received by scaning another
	table with given condition without index or filter
	using. 
	The function result is >= 0 in the case of successful 
	finish and negative one containing the error code otherwise
	*/
 
    
     Tabid *itabid;		/* scaned table identifier           */
     Tabid *otabid;		/* identifier of the table in which
				   strings are inserted              */
				
     i4_t nr;			/* the number of columns which will be
				   selected from basic table and
				   inserted in temporal one          */

     i4_t rlist[];		/* array  of column number  which
				   values  will be selected          */

     i4_t nc;                    /* condition size                    */
     Cond *cond;		/* condition of current string selecting
				   NULL means absence of any conditions */

     i4_t *spn;			/* number of control point to which the
				   rolback has to be done in the case of
				   fail in the execution of operation
				   because of deadlock                  */ 
#endif
{
  i4_t stat;
  struct id_rel ipidrel, pidrel;
  
  if (cl_debug)
    fprintf (STDOUT, "BASE.instttab \n");
  INITBUF;
  TRANTAB (pidrel, itabid);
  TRANTAB (ipidrel, otabid);
  
  stat = inscrl (&pidrel, &ipidrel, nr, mk_short_arr (nr, rlist), nc, cond);
  if (stat > 0)
    *spn = stat;
  return (stat);
}

/**********************************************************************/
i4_t 
institab (Indid * indid, Tabid * otabid, i4_t nr, i4_t *rlist, i4_t nc,
	  cond_buf_t cond, i4_t nra, cond_buf_t range, i4_t *spn)
#if 0
     /* inserts set of strings received by scaning another
	table with given condition with index using. 
	The function result is >= 0 in the case of successful 
	finish and negative one containing the error code otherwise
	*/
  
     Indid *indid;		/* scaned table identifier           */

     Tabid *otabid;		/* identifier of the table in which
				   strings are inserted              */
				

     i4_t nr;			/* the number of columns which will be
				   selected from basic table and
				   inserted in temporal one          */


     i4_t rlist[];		/* array  of column number  which
				   values  will be selected          */

     i4_t nc;			/* condition size                    */

     Cond *cond;		/* condition of current string selecting
				   NULL means absence of any conditions */

     i4_t nra;			/* scan range size                    */
     Cond *range;		/* scan range is restricted form
				   of column condition put on the
				   columns included into index
				   NULL means undefined range         */

     i4_t *spn;			/* number of control point to which the
				   rolback has to be done in the case of
				   fail in the execution of operation
				   because of deadlock                  */ 

#endif
{
  i4_t stat;
  struct id_ind pidind;
  struct id_rel pidrel;
  
  INITBUF;
  TRANIND (pidind, indid);
  if (cl_debug)
    fprintf (STDOUT, "BASE.institab \n");
  TRANTAB (pidrel, otabid);
  
  stat = inscin (&pidind, &pidrel, nr,
                 mk_short_arr (nr, rlist), nc, cond, nra, range);
  if (stat > 0)
    *spn = stat;
  return (stat);
}

/**********************************************************************/
i4_t 
instftab (Filid * filid, Tabid * otabid, i4_t nr, i4_t *rlist, i4_t nc,
	  cond_buf_t cond, i4_t *spn)
#if 0
     /* inserts set of strings received by scaning another
	table with given condition with filter using. 
	The function result is >= 0 in the case of successful 
	finish and negative one containing the error code otherwise
	*/
 
     Filid *filid;		/* identifier of scaned table filter   */

     Tabid *otabid;		/* identifier of the table in which
				   strings are inserted              */

     i4_t nr;			/* the number of columns which will be
				   selected from basic table and
				   inserted in temporal one          */

     i4_t rlist[];		/* array  of column number  which
				   values  will be selected          */

     i4_t nc;			/*  condition size                   */
     Cond *cond;		/* condition of current string selecting
				   NULL means absence of any conditions */

     i4_t *spn;			/* number of control point to which the
				   rolback has to be done in the case of
				   fail in the execution of operation
				   because of deadlock                  */ 
#endif
{
  i4_t stat;
  struct id_rel pidrel;

  if (cl_debug)
    fprintf (STDOUT, "BASE.instftab \n");
  INITBUF;
  TRANTAB (pidrel, otabid);
  
  stat = inscfl (*filid, &pidrel, nr, mk_short_arr (nr, rlist), nc, cond);
  if (stat > 0)
    *spn = stat;
  return (stat);
}

/**********************************************************************/
i4_t 
jnsttab (Tabid * i1tabid, Tabid * i2tabid, Tabid * otabid, i4_t ns,
	 i4_t *r1list, i4_t *r2list)
#if 0
     /* natural joining of two sorted in the same order
	temporal relations and locating result in new
	sorted in the same order and columns temporal
	relation
	*/
 
     Tabid *i1tabid;	        /* identifier of the first original
			           temporal relation                    */

     Tabid *i2tabid;		/* identifier of the second original
			           temporal relation                    */

     Tabid *otabid;		/* identefier of the result
			           temporal relation                    */

     i4_t ns;			/* the number of columns on which 
			           values the natural joining 
			           is produced                          */

     i4_t r1list[];		/* array of column numbers on which
			           values  the natural joining 
				   is produced                          */

     i4_t r2list[];		/* array of column numbers of the second
				   relation by which values the
				   natural joining is produced          */
#endif
{
  i4_t stat;
  struct ans_ctob answer;
  struct id_rel pit1, pit2;
  
  TRANTAB (pit1, i1tabid);
  TRANTAB (pit2, i2tabid);

  if (cl_debug)
    fprintf (STDOUT, "BASE.jnsttab \n");
  INITBUF;

  answer = join (&pit1, ns, mk_short_arr (ns, r1list),
                 &pit2, ns, mk_short_arr (ns, r2list));
  stat = answer.cpncob;
  Copy (&(otabid->segid), &(answer.idob.segnum), sizeof (Segid));
  Copy (&(otabid->untabid), &(answer.idob.obnum), sizeof (Unid));
  if (cl_debug)
    fprintf (STDOUT, "BASE.jnsttab: segid=%d,untabid=%d\n",
	     otabid->segid, (i4_t)otabid->untabid);
  return (stat);
}

/*---------------------------------------------*/
/*            aggregate functions              */
/*---------- only  "SUM" is absent !!! --------*/
i4_t 
cnt_ttab (Tabid * tabid, i4_t nc, cond_buf_t cond)
#if 0
     /* counting the number of the strings on basic of scaning
	basic table without index or filter using;
	The function result is >= 0 in the case of successful 
	finish and negative one containing the error code otherwise
	*/
    
     Tabid *tabid;		/* table identifier                   */
     i4_t nc;			/* condition size ( in bytes )        */
     Cond *cond;		/* condition of current string selecting
				   NULL means absence of any conditions */

#endif
{
  i4_t stat;
  struct ans_cnt *answer;
  struct id_rel pidrel;

  answer = (struct ans_cnt *)res_buf;
  if (cl_debug)
    fprintf (STDOUT, "BASE.cntttab \n");
  TRANTAB (pidrel, tabid);
  
  cntttab (answer, &pidrel, nc, cond);
  stat = answer->cpncnt;
  if (stat < 0)
    return (stat);
/*   if(stat > 0 )  *spn = stat;   */
  return (answer->cntn);
}

/*--------------------------------------------------------*/
i4_t 
cnt_itab (Indid * indid, i4_t nc, cond_buf_t cond, i4_t nr, cond_buf_t range)
#if 0
     /* counting the number of the strings on basic of scaning
	basic table with index using;
	The function result is >= 0 in the case of successful 
	finish and negative one containing the error code otherwise
	*/
    

     Indid *indid;		/* index identifier                   */
     i4_t nc;			/* condition size ( in bytes )        */

     Cond *cond;		/* condition of current string selecting
				   NULL means absence of any conditions */

     i4_t nr;			/* range size ( in bytes )            */
     Cond *range;		/* scan range is restricted form
				   of column condition put on the
				   columns included into index
				   NULL means undefined range         */

#endif
{
  i4_t stat;
  struct ans_cnt *answer = (struct ans_cnt *)res_buf;
  struct id_ind pidind;

  if (cl_debug)
    fprintf (STDOUT, "BASE.cntitab \n");
  TRANIND(pidind,indid);

  cntitab (answer, &pidind, nc, cond, nr, range);
  stat = answer->cpncnt;
  if (stat < 0)
    return (stat);
/*     if(stat > 0 )  *spn = stat;     */
  return (answer->cntn);
}

/*--------------------------------------------------------*/
i4_t 
cnt_ftab (Filid * filid, i4_t nc, cond_buf_t cond)
#if 0
     /* counting the number of the strings on basic of scaning
	basic table with filter using;
	The function result is >= 0 in the case of successful 
	finish and negative one containing the error code otherwise
	*/


     Filid *filid;		/* filter identifier                  */
     i4_t nc;			/* condition size ( in bytes )        */

     Cond *cond;		/* condition of current string selecting
				   NULL means absence of any conditions */

#endif
{
  return cntftab (*filid, nc, cond);
}

/*--------------------------------------------------------*/

/*--------------------------------------------------------*/
i4_t 
maxminavgitab (Indid * indid, i4_t maxminavg, i4_t nc, cond_buf_t cond,
	       i4_t nr, cond_buf_t range, i4_t code)
#if 0
     /* counting the maximum or minimum or average of values of
	the first field key of index on basic of given table scaning
	with index using.
	The function result is >= 0 in the case of successful 
	finish and negative one containing the error code otherwise
	*/

     Indid *indid;		/* index identifier                  */
     i4_t maxminavg;		/* result                            */
     i4_t nc;			/* condition size ( in bytes )       */
     Cond *cond;		/* condition of current string selecting
				   NULL means absence of any conditions */

     i4_t nr;			/* range size ( in byte )             */
     Cond *range;		/* scan range is restricted form
				   of column condition put on the
				   columns included into index
				   NULL means undefined range         */


#endif
{
  i4_t stat;
  struct ans_next *answer;
  struct id_ind pidind;
  
  answer = (struct ans_next *)res_buf;
  if (cl_debug)
    {
      if( code==(i4_t) MAXITAB)
	fprintf (STDOUT, "BASE.maxitab \n"); 
      if ( code==(i4_t) MINITAB )
	fprintf (STDOUT, "BASE.minitab \n");
      if ( code==(i4_t)AVGITAB )
	fprintf (STDOUT, "BASE.avgitab \n");
    }
  TRANIND (pidind, indid);
  
  answer->csznxt = 0;
  switch (code)
    {
    case MAXITAB : maxitab (answer, &pidind, nc, cond, nr, range);
                   break;
    case MINITAB : minitab (answer, &pidind, nc, cond, nr, range);
      break;
		   /*
    case AVGITAB : avgitab (answer, &pidind, nc, cond, nr, range);
    break;
    */
    }
  stat = answer->cotnxt;
  if (stat < 0)
    return (stat);
  return ((i4_t) (answer->cadnxt));

}

/*--------------------------------------------------------*/
i4_t 
minmaxavgstab (Tabid * tabid, i4_t minmaxavg, i4_t code)
#if 0
     /* counting the maximum or minimum or average of values of
	the first field of value of sorting for sorted temporal
	table.
	The function result is >= 0 in the case of successful 
	finish and negative one containing the error code otherwise
	*/
 
     Tabid *tabid;		/* temporal relation identifier       */
     i4_t minmaxavg;      	/* result                             */
#endif
{
  i4_t stat, res;
  struct id_rel pidrel;

  if (cl_debug)
    {
      if( code==(i4_t)MINSTAB )
	fprintf (STDOUT, "BASE.minstab \n");
      if( code==(i4_t)MAXSTAB )
	fprintf (STDOUT, "BASE.maxstab \n");
      if( code==(i4_t)AVGSTAB )
	fprintf (STDOUT, "BASE.avgstab \n");
    }
  TRANTAB(pidrel,tabid);

  switch (code)
    {
    case MINSTAB :
      stat = minstab (&pidrel, (char *)(&res));
      break;
    case MAXSTAB :
      stat = maxstab (&pidrel, (char *)(&res));
      break;
    default:
      stat = -1;
		   /* This function can't return the result of this operation
    case AVGSTAB : answer = minstab (&pidrel, p);
    break;
    */
    }
  if (stat < 0)
    return (stat);
/*   if(stat > 0 )  *spn = stat;   */
  return (res);
}


/*----------------------------------------------------------*/
i4_t 
funci (Indid * indid, i4_t *spn, i4_t nc, cond_buf_t cond,
       i4_t nr, cond_buf_t range, data_unit_t **colval, i4_t nf, 
       i4_t *flist, char *fl)
#if 0
     /* computing of aggregate functions on basic of table scaning
	with index using
	*/
 
     Indid *indid;		/* index identifier                   */
     i4_t *spn;			/* number of control point to which the
				   rolback has to be done in the case of
				   fail in the execution of operation
				   because of deadlock                  */ 

     i4_t nc;			/* condition size ( in bytes )       */

     char *cond;		/* condition of current string selecting
				   NULL means absence of any conditions */

     i4_t nr;			/* range size                         */
     Cond *range;		/* scan range is restricted form
				   of column condition put on the
				   columns included into index
				   NULL means undefined range         */

     data_unit_t **colval;		/* array of pointer for location of 
				   function values; the number of pointers
				   in the array has to be equal to value
				   of parameter "nf"                  */

     i4_t nf;                    /* the number of columns for which 
				   functions have be used           */	
	                       
     i4_t flist[];		/* array of column values for which 
				   functions have be used */

     char *fl;			/* char fl[nf-1] - string of flags,  *
	                         * corresponding every elements      *
	                         * from flist[].                     */
#endif
{
  i4_t stat;
  struct id_ind pidind;

  INITBUF;
  if (cl_debug)
    fprintf (STDOUT, "BASE.funci \n");
  TRANIND(pidind,indid);

  stat = agrfind (colval, &pidind, nf, mk_short_arr (nf, flist), nc, cond, nr, range, fl);
/*    analasing of answer and writing it in colval       */
  if (stat < 0)
    return (stat);
  
  if (cl_debug)
    fprintf (STDOUT, "BASE:end funci \n");
  if (stat > 0 && spn)
    *spn = stat;
  return (stat);
} /* funci */

/*--------------------------------------------------------*/
i4_t 
func (Tabid * tabid, i4_t *spn, i4_t nc, cond_buf_t cond,
      data_unit_t **colval, i4_t nf, i4_t *flist, char *fl)
#if 0
     /* computing of aggregate functions on basic of table scaning
	without index or filter using
	*/
   
     Tabid *tabid;		/* table identifier                   */
     i4_t *spn;			/* number of control point to which the
				   rolback has to be done in the case of
				   fail in the execution of operation
				   because of deadlock                  */ 

     i4_t nc;                    /* condition size ( in bytes )       */
     char *cond;		/* condition of current string selecting
				   NULL means absence of any conditions */

     data_unit_t **colval;		/* array of pointer for location of 
				   function values; the number of pointers
				   in the array has to be equal to value
				   of parameter "nf"                    */

     i4_t nf;			/* the number of columns for which 
				   functions have be used           */
     i4_t flist[];		/* array of column values for which 
				   functions have be used */

     char *fl;			/* char fl[nf-1] - string of flags  *
	                         * corresponding every elements     *
	                         * from flist[].                    */
#endif
{
  i4_t stat, i, m, err;
  struct ans_next *answer;
  struct id_rel pidrel;
  
  answer = (struct ans_next *)res_buf;
  if (cl_debug)
    fprintf (STDOUT, "BASE.func \n");
  INITBUF;
  TRANTAB(pidrel,tabid);

  agrfrel (answer, &pidrel, nf, mk_short_arr (nf, flist), nc, cond, fl);
/*   analising of answer and writing it in colval         */
  stat = answer->cotnxt;
  if (stat < 0)
    return (stat);
  m = answer->csznxt;
  if (cl_debug)
    fprintf (STDOUT, "BASE:func csznxt=%d\n", m);
  for (i = 0; i < m; i++)
    if (cl_debug)
      fprintf (STDOUT, "%X ", answer->cadnxt[i]);
  
  err = res_row_save (answer->cadnxt, nf, colval);
  if (err < 0)
    return err;
  
  if (cl_debug)
    fprintf (STDOUT, "BASE:end func \n");
  if (stat > 0 && spn)
    *spn = stat;
  return (stat);
} /* func */

/*--------------------- E N D  P A R T 2 ------------------ */
