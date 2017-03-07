/*
 *  vmemory.h  - support tree library for SQL precompiler
 *               Virtual memory manager interface
 *
 *  This file is a part of GNU SQL Server
 *
 *  Copyright (c) 1996-1998, Free Software Foundation, Inc
 *  Developed at the Institute of System Programming, Russia
 *  This file is written by Michael Kimelman
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
 *  Contact:  gss@ispras.ru
 *
 */

/* $Id: vmemory.h,v 1.247 1998/09/29 21:26:01 kimelman Exp $ */

/*=====================================================================
  
            Simulated virtual memory (user interface)

        Simululated virtual memory  looks from the  user point of
view as a   set  of VM  segments, each  of  them assotiated  with
independently compiled or  used piece  of  interpretator code  or
compiler tree (just like object modules in ordinary OS). User can
create, link, unlink  and  store out segments during  the program
execution.  Virtual space  is shared by  all  linked segments and
each   of  them  assotiated   with  it's   own   range of virtual
addresses. To allow intersegment references each segment contains
table  of  relocable virtual  addresses  and table of names which
segment exports. Relocation  table used by  VM manager to correct
listed addresses when the segment linked.
        VM  manager support    special  conception of   "current"
segment, allowing local inside  segments references.  The segment
marked as a  current by 'create', 'link' or 'set_current_segment'
calls.  

=====================================================================*/

#ifndef __VMEMORY_H__
#define __VMEMORY_H__

#include "setup_os.h"

typedef i4_t VADR;
#define VNULL (VADR)0l

VADR create_segment __P((void));
/* Function returns   relocable   virtual  address   NULL of */
/* created segment.  Actually  it's just  segment identifier */
/* but  in form of virtual address.                          */

VADR link_segment __P((void *buffer,i4_t size));
/* buffer contains loaded from outside packed segment, which */
/* has to  be  linked to   others (set segment's  interpages */
/* references to physical address as well as correct virtual */
/* relocation addresses).                                    */

void *export_segment __P((VADR s_id,i4_t *buf_size, i4_t unlink_seg));
/* create buffer and    put packed  segment  in it.   Before  */
/* packing routine changed relocation adresses and interpage  */
/* references  to   base   them   on   the    beginning   of  */
/* buffer. Returns  buf_size  and  address of  buffer.  s_id  */
/* should point inside   segment (You can  use VADR received  */
/* from create_segment or load_segment as  well as any other  */
/* relocable addresses pointed into the segment.)             */

int  unlink_segment __P((VADR s_id,i4_t enforce_free));
VADR switch_to_segment __P((VADR segment_id));
VADR get_vm_segment_id __P((VADR ptr_in_segment));

#define GET_CURRENT_SEGMENT_ID  get_vm_segment_id(0)

VADR vmalloc __P((i4_t size));

#define vmalloc(size) vm_ob_alloc(size,0)

VADR vm_ob_alloc __P((i4_t size,i4_t align));
VADR vmrealloc __P((VADR old_ptr,i4_t new_size));
/*      vmrealloc(NULL,new_size) => vmalloc(new_size));        */

void vmfree __P((VADR ptr));
void *vpointer __P((VADR ptr));
VADR ptr2vadr __P((void *p));

void register_relocation_address __P((VADR ptr));
/* add  new entry to relocation  table  of the segment where  */
/* this  address is. "ptr"  points to address data which has  */
/* to be relocable.                                           */

void register_export_name __P((VADR object_ptr,VADR name_ptr));
/* add  new entry to export  list  and register reference to */
/* object as relocation  address. (both  address have to  be */
/* placed on the same segment) */

void register_export_address __P((VADR registered_adr,char *name));
VADR resolve_reference __P((char *name));
VADR resolve_local_reference __P((char *name));

VADR resolve_import_name __P((VADR name_ptr));
#define resolve_import_name(n) resolve_reference((char*)vpointer(n))

VADR external_reference __P((char *name));
/* make  pointer (!local) to object which  is required to be */
/* imported.    When   vpointer   read    such  pointer,  it */
/* automatically  try   to  find  required  name and resolve */
/* reference, returning the   phisical address of  desirable */
/* data. */

#endif




