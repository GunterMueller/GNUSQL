/* 
 *  typepi.h -  External Transaction Types of GNU SQL server
 *                
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996, 1997, Free Software Foundation, Inc
 *  Developed at the Institute of[ ]System Programming
 *  This file is written by Vera Ponomarenko
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


#ifndef __TPTRN_H__
#define __TPTRN_H__

/* $Id: tptrn.h,v 1.245 1997/03/31 03:46:38 kml Exp $ */

#include "engine/rnmtp.h"

struct  id_ob {
        u2_t segnum;
        i4_t  obnum;
};
struct  id_rel {
        struct id_ob urn;
        u2_t pagenum;
        u2_t index;
};
struct  id_ind {
        struct  id_rel irii;
        i4_t    inii;
};
struct ans_next {
        i2_t  cotnxt;
        u2_t csznxt;
        char  cadnxt[BD_PAGESIZE];
};
struct  des_field {             /* ןניףבפולר נןלס ןפמןûומיס */
        u2_t field_type;          /* פינ */
        u2_t field_size;          /* עבתםוע */
};
struct ans_cr {
        i4_t cpnacr;
        struct  id_rel idracr;
};
struct  ans_ctob {
        i4_t cpncob;
        struct  id_ob idob;
};
struct ans_cind {
        i4_t cpnci;
        struct  id_ind idinci;
};
struct ans_opsc {
        i4_t cpnops;
        i4_t scnum;
};
struct ans_cnt {
        i4_t cpncnt;
        i4_t cntn;
};
struct ans_avg {
        i4_t cotavg;
        double agr_avg;
};

#endif
