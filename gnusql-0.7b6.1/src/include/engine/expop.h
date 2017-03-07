/*
 *  expop.h  - External operations of Kernel of GNU SQL-server  
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

#ifndef __expop_h__
#define __expop_h__

/* $Id: expop.h,v 1.246 1998/09/29 21:26:05 kimelman Exp $ */


enum {
  /* From */
  CREATR  =1,    /* USER->ADM */
  COPYLJ  =2,    /* OUT->ADM */
  ASK     =3,    /* OUT->ADM */
  FINITM  =4,    /* OUT->ADM */
  FINITH  =5,    /* OUT->ADM */

  BLDUN  =27,   /* USER->TRN: Build a union */
  INTRSC =28,   /* USER->TRN: Build a intersection */
  DFFRNC =29,   /* USER->TRN: Build a difference */
  MINITAB =53,  /* USER->TRN: Count minimum of the 1st field values 
                   of a key by an index scan */
  MAXITAB =54,  /* USER->TRN: Count maximum of the 1st field values 
                   of a key by an index scan */
  AVGITAB =55,  /* USER->TRN: Count average of the 1st field values 
                   of a key by an index scan */
  MINSTAB =56,  /* USER->TRN: Count minimum of the 1st field values 
                   of a sort of a sort temporary relation */
  MAXSTAB =57,  /* USER->TRN: Count maximum of the 1st field values 
                   of a sort of a sort temporary relation */
  AVGSTAB =58,  /* USER->TRN: Count average of the 1st field values 
                   of a sort of a sort temporary relation */

  /* To */
  NCOPY    =1,    /* ADM->OUT */
  PUPERR   =2     /* ADM->OUT */

};

#endif
