/*
 *  totdecl.h - Function declaration of pack functions
 *              Kernel of GNU SQL-server
 *                
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming
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
 *  $Id: totdecl.h,v 1.246 1998/09/29 21:26:10 kimelman Exp $
 */

#ifndef __TOTFTN_H__
#define __TOTFTN_H__

#include "setup_os.h"

#if 1

void t4bpack __P((i4_t n, char *pnt));
i4_t t4bunpack __P((char *pnt));
void t2bpack __P((u2_t n, char *pnt));
u2_t t2bunpack __P((char *pnt));

#define PROC_PACK

#elif 1
# error should be here
#define p_byte(i4,p,i)      ((unsigned char*)(p))[i] = ((((i4_t)(i4))>>(8*i)) & 0xff)
#define up_byte(p,i)        ((i4_t)(((unsigned char*)(p))[i]) << (8*i))

#define t4bpack(n,p)        { p_byte(n,p,0);p_byte(n,p,1);p_byte(n,p,2);p_byte(n,p,3);}
#define t4bunpack(p)        (up_byte(p,0) + up_byte(p,1) + up_byte(p,2)+ up_byte(p,3))

#define t2bpack(n,p)        { p_byte(n,p,0); p_byte(n,p,1);}
#define t2bunpack(p)        ((u2_t)(up_byte(p,0) + up_byte(p,1)))

#else
# error should be here too

#define t4bpack(n,p)        { *(i4_t*)(p) = (i4_t)(n); }
#define t4bunpack(p)        ( *(i4_t*)(p))

#define t2bpack(n,p)        { *(i2_t*)(p) = (i2_t)(n); }
#define t2bunpack(p)         ((u2_t)*(i2_t*)(p))

#endif


#ifdef PROC_PACK

#include "xmem.h"
#include <assert.h>

#define TPACK(i,p)      bcopy(&i,p,sizeof(i))
#define TUPACK(p,i)     bcopy(p,&i,sizeof(i))
#define BUFPACK(i,p)    { TPACK(i,p) ; *((char**)&p) += sizeof(i); }
#define BUFUPACK(p,i)   { TUPACK(p,i); *((char**)&p) += sizeof(i); }

#endif

#endif
