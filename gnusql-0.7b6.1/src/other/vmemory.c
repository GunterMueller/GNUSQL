/*
 *  vmemory.c  - simulated virtual memory manager of GNU SQL compiler
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

/* $Id: vmemory.c,v 1.255 1998/09/29 22:23:45 kimelman Exp $ */

/*=====================================================================
  
                    Simulated virtual memory 
                                
        1. User interface

        Simululated virtual memory  looks from the  user point of
view as a   set  of VM  segments, each  of  them associated  with
independently compiled or  used piece  of  interpretator code  or
compiler tree (just like object modules in regular OS). User can
create, link, unlink  and  store out segments during  the program
execution.  Virtual space  is shared by  all  linked segments and
each   of  them  assotiated   with  it's   own   range of virtual
addresses. To allow intersegment references each segment contains
table  of  relocable virtual  addresses  and table of names which
segment exports. Relocation  table used by  VM manager to correct
listed addresses when the segment linked.
        VM  manager support    special  conception of   "current"
segment, allowing local references inside  segments.  The segment
marked as a  current by 'create', 'link' or 'set_current_segment'
calls. 'vmemory.h'  included   some  specific  comment   to  part
of VMinterface calls.


        2. Virtual Memory Inside

        Virtual  memory      is  realized  like    "segment-page"
adressation in computer. But in   this case it was impossible  to
use fixed    size   pages.  So   memory  divided  to  "segments",
associated  with independently   compiled or used   piece of code
(like object modules).  Each segment  represented by small header
and one-directional list  of pages of different size,  associated
to pieces  of virtual memory. Each virtual  memory page  (vmp) in
turn  has  page descriptor contained pointer  to  page data area,
offset of the page in segment (so there can be holes in vm space)
and other page realated  information. Space on the page allocated
by  a virtual memory  blocks (vmb). Each vmb descriptor specifies
the vmb size (while  consequece of free  vmbs appears it's joined
to one) and mode (alignment of  data or 0  for free vmb). Because
each  free vmb has  to contain  at  least  4 bytes of  free space
(otherwise it doesn't worth to consider it as a free space), this
space used for storing pointers needed  for bidirectional link of
free   blocks.  Using relative   offsets  as a  pointers for this
purpose allows to join pages'  areas (when it's needed) without a
problem.
        The only one available by  user element of VM is segment.
VM manager is  responsible for  creating, storing, unlinking  and
linking  segments and  holds for  this  purpose internal  segment
table (segment_table). Virtual address (i4_t) of any data contain
segment identifier (index  of segment  in segment table),   which
occupied high bits of address, and offset  in segment.  Number of
bits used for offset defined by  SEGMENT_OFFSET_SIZE and equal 20
now.   Segment identifier 0 is used  for reference inside current
segment (there  is a  special  identifier of  "current" segment).
Size of page limited now by 64K  (16bit offset). For intersegment
addressation we use relocation table (contain relocable addresses
inside segment), and  exported name table (contain exported names
and assotiated with  them relocable addresses of exported objects
- structures of data, routines   and so on). The information   of
exported data can  be got by  direct  'resolve_reference' call as
well  as by using special form  of  address used for dynamic name
resolution    (I   mean  'vpointer'    resolves   these cases  by
itself).    This    kind    of    addresses   is     created   by
'external_reference'   call.  Actually  it's  almost  ordinal  VM
pointer to locally saved imported name, but segment identifier of
it is bit inverted (~seg_id).

==================================================================*/
     
#include "setup_os.h"
#include "xmem.h"
#include "vmemory.h"
#include "sql_decl.h"
#include <assert.h>


/*------------------------------------------------------*/

#define SEGMENT_OFFSET_SIZE      20
/* Maximum size of the virtual segment */
#define SEGMENT_LIMIT           (1<<SEGMENT_OFFSET_SIZE)

#define SEG_ID(adr)    ((adr)>>SEGMENT_OFFSET_SIZE)
#define SEG_BASE(s_id) ((s_id)<<SEGMENT_OFFSET_SIZE)
#define SEG_IND(adr)   vm_segment_ind(adr)
#define SEG_OFF(adr)   (adr&(SEGMENT_LIMIT-1))
#define SEG_PTR(adr)   (&(segment_table.entries[SEG_IND(adr)]))

#if HAVE_LONG_DOUBLE
#  define HUGEST_MACHINE_TYPE     long double
#else
#  define HUGEST_MACHINE_TYPE     double
#endif
#  define PAGE_ALIGN              sizeof(HUGEST_MACHINE_TYPE)

int debug_vmemory = 0;
#define DV(mask) (debug_vmemory & (mask))

typedef i2_t Offset;           /* offset on the page, computed in bytes */

/* virtual memory block descriptor -- "VMB descr"  */

typedef struct area
{
  i2_t size;
  i2_t mode;                   /* 0-free; other 'mode' bytes alignment */
} *pdesc;

#define DSC_SZ sizeof(struct area)

/* VMB descr for unused space */

typedef struct f_area
{
  struct area desc;
  Offset      next,             /* double linked list of free blocks */
              prev;
} *pdescf;

#define FDS_SZ sizeof(struct f_area)
#define NEXT_VMB_DSC(ds)  ((pdescf)((byte*)ds+ds->next))
#define PREV_VMB_DSC(ds)  ((pdescf)((byte*)ds+ds->prev))

/********* Page **********************/

typedef struct virtual_memory_page *P_vmpage;

struct virtual_memory_page/* VMP  ===========================================*/
{                         /*                                                 */
  P_vmpage  next;         /* points to the next page descriptor              */
  i4_t      base;         /* offset of the page in  segment                  */
  i4_t      size;         /* size of the current page                        */
  i4_t      used;         /* size of used space on the page                  */
  i4_t      free_ptr;     /* points to free space - just after descriptor of */
                          /* the first free vmb on the page                  */
  i4_t      lfree_ptr;    /* points to free space - just after descriptor of */
                          /* the last free vmb on the page                   */
  i4_t      flags;        /* nonzero to avoiding freeing memory-the same     */
                          /* meaning as 'dont_remove' below                  */
  byte     *page;         /* have to be aligned to "i4_t double"             */
};                        /*=================================================*/

/********* Segment *******************/
typedef struct            /* Relocation table================================*/
{                         /*                                                 */
  i4_t  used_ent;         /* number of used entries                          */
  i4_t  ent_nmb;          /* number of entries                               */
  i4_t  dont_remove;      /* nonzero if the memory, used by table, will be   */
                          /* fried later                                     */
  VADR *entries;          /* scalable array with local addresses of          */
                          /* relocable pointers                              */
} reloc_tbl;              /*=================================================*/

typedef struct            /* Name table - this table contains names and =====*/
{                         /* adresses of entry points: routines amd variables*/
  i4_t    ent_nmb;        /* number of entries                               */
  i4_t    dont_remove;    /* nonzero if the memory, used by table, will be   */
                          /* fried later                                     */
  struct                  /*                                                 */
  {                       /*                                                 */
    VADR   name;          /* address in the segment of the exported name     */
    VADR   adr;           /* relocable address in the segment of the object  */
  }      *entries;        /* scalable array of exported entries              */
} export_tbl;             /*=================================================*/

typedef struct virtual_memory_segment /* Virtual memory segment descriptor ==*/
{                                 /*                                         */
  P_vmpage    base;               /* pointer to the chain of segment's pages */
  i4_t        used;               /*                                         */
  reloc_tbl   relocation_table;   /* relocation table of the segment         */
  export_tbl  export_table;       /* exported names table                    */
  void       *loaded_block_address; /* address of the segment - used for     */
                                  /* latter freeing memory if it was loaded  */
                                  /* from outside as a whole piece           */
} vmsegment;                      /*=========================================*/

/********** Global manipulation tables *********/

typedef struct            /* Local table of loaded segments  ================*/
{                         /*                                                 */
  i4_t       ent_nmb;     /* number of segments                              */
  i4_t       dont_remove; /* used just for compatibility in macro            */
  i4_t       current;     /* number (identifier) of the current segment -    */
                          /* used if segment id in VADR==0                   */
  vmsegment *entries;     /* scalable array of segment's descriptors -       */
                          /* Attention! entries[0] never used                */
} SEGM_TBL;               /*=================================================*/

static SEGM_TBL segment_table =
{0, 0, 0, NULL};


#define REALLOC(p,sz) l_realloc(p,sz)
#define FREE(p)       l_free(p)

#ifndef USE_XVM

#define MALLOC(size)   xmalloc(size)
#define AREALLOC(p,sz) xrealloc(p,sz)
#define AFREE(p)       xfree(p)

#else

static void *lmalloc(i4_t size);
static void *lrealloc(void *p,i4_t size);
static void  lfree __P((void *p));
static VADR xmemory_segment = VNULL;

#define MALLOC(size)  lmalloc(size)
#define AREALLOC(p,sz) lrealloc(p,sz)
#define AFREE(p)       lfree(p)

#endif

#define TBL_PORTION 32

#define TBL_ELEM_SZ(tbl) sizeof(*(tbl.entries))
#define TBL_SIZE(tbl)  \
((TBL_PORTION+(tbl.ent_nmb))*TBL_ELEM_SZ(tbl))

#define ADD_ENTRY(tbl) 				\
{						\
  if(tbl.ent_nmb%TBL_PORTION==0)		\
    {						\
      register void *tmp_ptr=MALLOC(TBL_SIZE(tbl)); \
      if (tbl.ent_nmb >0)			\
        {					\
          bcopy(tbl.entries,tmp_ptr,TBL_ELEM_SZ(tbl)*tbl.ent_nmb); \
          if(tbl.dont_remove)tbl.dont_remove=0;	\
          else               FREE((void*)(tbl.entries)); \
        }					\
      tbl.entries=tmp_ptr;			\
    }						\
  tbl.ent_nmb++; 				\
}

/************************************************************************\
************  CACHE  *****************************************************
\************************************************************************/
static void *
cache_proc(VADR n, void *p,i4_t action)
{
#define CACHE_SIZE 5
  struct c_el {
    VADR  v;
    void *p;
  };
  static   struct c_el cache[CACHE_SIZE];
  register i4_t         i;

  switch(action)
    {
    case -1: /* flush */
#define  flush_cache() cache_proc(VNULL, NULL, -1)
      for (i = 0; i < CACHE_SIZE; i ++ )
        cache[i].v = 0;
      return NULL;
    case 0: /* check */
#define  check_cache(n) cache_proc(n, NULL, 0)
      assert(n);
      i = (i4_t)(( n >> 4 ) % CACHE_SIZE);
      return ( n == cache[i].v ? cache[i].p : NULL );
    case 1: /* update */
#define  update_cache(n,p) cache_proc(n,p, 1)
      i = (i4_t)(( n >> 4 ) % CACHE_SIZE);
      cache[i].v = n;
      cache[i].p = p;
      return NULL;
    default:
      assert("vm cache_proc unrecognized action");
    }
  return NULL;
#undef CACHE_SIZE
}

static void *
l_realloc(void *p,i4_t size)
{
  flush_cache();
  return AREALLOC(p,size);
}

static void
l_free(void *p)
{
  flush_cache();
  AFREE(p);
}

/************************************************************************\
**************************************************************************
\************************************************************************/
static i4_t enable_auto_create = 0;
static i4_t force_free         = 0;


static long
vm_segment_ind (register VADR ptr)
{
  register i4_t p = SEG_ID (ptr);
  if (p == 0)
    {
      if (segment_table.current > 0)
	return segment_table.current;
      else if (enable_auto_create)
	{
	  fprintf (STDERR, "Virtual memory handler: warning :"
		   "auto-creating new segment\n");
	  create_segment ();
	  p = segment_table.current;
	}
      else
	return p;
    }
  else if (p < 0)
    p = ~p;
  if (p == 0 || p >= segment_table.ent_nmb || !segment_table.entries[p].used)
    yyfatal ("Virtual memory handler: incorrect segment identifier");
  return p;
}

static P_vmpage 
check_for_null_page (register P_vmpage vmp)
{
  if (vmp && vmp->size == 0)
    {
      register P_vmpage vmp1 = vmp;
      vmp = vmp->next;
      vmp1->next = 0;
      if (vmp1->flags == 0)     /* in other case it'll be freed later */
        {
          FREE (vmp1->page);
          FREE (vmp1);
        }
    }
  return vmp;
}

static i4_t 
check_free_memory_on_page (register P_vmpage vmp)
{
  register pdescf a;
  register i4_t    sz = 0;
  assert(!vmp->free_ptr == !vmp->lfree_ptr);
  assert(vmp->free_ptr>=0 && vmp->free_ptr<vmp->size);
  assert(vmp->lfree_ptr>=0 && vmp->lfree_ptr<vmp->size);
  if (DV(0xf) <= 2 )
    return vmp->size - vmp->used;
  if (vmp->free_ptr )
    for (a = (pdescf) (vmp->page + vmp->free_ptr - DSC_SZ);
         a;
         a = a->next ? NEXT_VMB_DSC (a) : 0)
      {
        if (!(a->desc.size > 0))
          yyfatal ("Virtual memory handler: memory block size <=0 ");
        if (a->desc.mode > 0)
          yyfatal ("Virtual memory handler: VMB mode !=0 in free list");
        sz += a->desc.size + DSC_SZ;
      }
  assert (vmp->used + sz == vmp->size);
  return sz;
}

static int
check_integrity (register P_vmpage vmp)
{
  register pdesc a;
  register i4_t   sz = 0;
  if (vmp == 0)
    return 0;
  vmp->next = check_for_null_page (vmp->next);
  if (vmp->page == 0)
    {
      assert (vmp->used == 0 && vmp->size == 0);
      return 0;
    }
  
  sz = vmp->size - vmp->used;
  
  if (DV(0xf) > 1)
    {
      for (sz = 0,a = (pdesc) (vmp->page);
           (byte *) a < (vmp->page + vmp->size);
           a = (pdesc) ((byte *) a + DSC_SZ + a->size))
        {
          if (!(a->size > 0))
            yyfatal ("Virtual memory handler: memory block size <=0 ");
          if(a->mode)
            {
              register i4_t mode;
              for (mode=1; mode <= PAGE_ALIGN; mode *= 2)
                if( mode == a->mode)
                  break;
              if (mode > PAGE_ALIGN)
                yyfatal ("Virtual memory handler: integrity checking failed"
                         " (alignment info destroyed)");
            }
          else /* a->mode==0 */ if (DV(0xf) >=4)
            {
              i4_t *p, *e;
              
              p = (i4_t*)a + FDS_SZ/sizeof(i4_t);
              e = p + (a->size + DSC_SZ - FDS_SZ)/sizeof(i4_t);
              while(p<e)
                if(*p++)
                  yyfatal ("Virtual memory handler: free memory not 0");
            }
          sz += a->size + DSC_SZ;
          if (force_free == 2 && a->mode)
            {
              register byte *ptr /* ,*endptr */;
              register FILE *f = stderr;
              register i4_t  ss;
              ptr = (byte*)a + DSC_SZ;
              if ( ((i4_t)ptr) % a->mode )
                {
                  ptr += a->mode;
                  ptr -= ((i4_t)ptr) % a->mode ;
                }
              ss = (i4_t)ptr - (i4_t)vmp->page + vmp->base;
              fprintf(f,"/* vmblock:%x (%d)*/ %d,\n",ss,a->size,ss);
#if 0
              for ( ptr = (byte*)a + DSC_SZ , endptr = ptr + a->size ; 
                    ptr < endptr;
                    ptr++
                    )
                fprintf(f,"%c",*ptr);
              fputs(" --- ",f);
              for ( ptr = (byte*)a + DSC_SZ, endptr = ptr + a->size ; 
                    ptr < endptr;
                    ptr++
                    )
                fprintf(f," %x",(i4_t)(*ptr));
              fputs("\n -4b- ",f);
              for ( ptr = (byte*)a + DSC_SZ, endptr = ptr + a->size ; 
                    ptr < endptr;
                    ptr += 4
                    )
                fprintf(f,"%lx ",*(i4_t*)(ptr));
              fputs("\n",f);
#endif
            }
        }
      assert (sz == vmp->size);
    }
  if (DV(0xf) > 0)
    sz = check_free_memory_on_page (vmp);
  return sz;
}

static void 
join_fvmb (register pdescf vmb_a, register P_vmpage vmp)
{
  register pdescf vmb_b;
  register i2_t off;
  check_integrity (vmp);
  if (vmb_a->next == 0)
    return;
  assert (vmb_a->next > 0);
  vmb_b = NEXT_VMB_DSC (vmb_a);
  off = vmb_a->desc.size + DSC_SZ;
  if ((off + (byte *) vmb_a) != (byte *) (vmb_b))
    return;
  /* join with next area */
  vmb_a->desc.size += DSC_SZ + vmb_b->desc.size;
  assert (vmb_a->desc.size > 0);
  if (vmb_b->next)
    {
      vmb_a->next = vmb_b->next + off;
      NEXT_VMB_DSC (vmb_a)->prev -= off;
    }
  else
    {
      vmb_a->next = 0;
      vmp->lfree_ptr = (byte *) vmb_a - (byte *) (vmp->page) + DSC_SZ;
    }
  bzero(vmb_b,FDS_SZ);
  join_fvmb (vmb_a, vmp);
}

static void 
insert_fvmb (register pdescf vmb_a, register P_vmpage vmp)
{
  register i4_t off;
  flush_cache();
  check_integrity (vmp);
  assert (vmb_a->desc.size > 0);
  bzero((char*)vmb_a + DSC_SZ, vmb_a->desc.size);
  if (vmp->free_ptr == 0)
    {
      vmp->used -= vmb_a->desc.size + DSC_SZ;
      vmp->free_ptr = vmp->lfree_ptr = 
        (byte *) vmb_a - (byte *) vmp->page + DSC_SZ;
      vmb_a->desc.mode = 0;
      vmb_a->next = vmb_a->prev = 0;
    }
  else
    {                           /* update free areas list */
      register pdescf vmb_b = (pdescf) (vmp->page + vmp->free_ptr - DSC_SZ);
      while (((byte *) vmb_b < (byte *) vmb_a)
             && (vmb_b->next > 0))
        vmb_b = NEXT_VMB_DSC (vmb_b);
      assert (vmb_b->desc.size > 0);
      assert (vmb_b->desc.mode == 0);
      check_integrity (vmp);
      off = (byte *) vmb_b - (byte *) vmb_a;
      if ((off == 0) || (off % DSC_SZ != 0))
        yyfatal ("Virtual memory handler: incorrect offset of memory block");
      vmb_a->desc.mode = 0;
      vmp->used -= vmb_a->desc.size + DSC_SZ;
      if (off < 0)
        {                       /* b < a */
          vmb_a->prev = off;
          if (vmb_b->next > 0)
            {
              vmb_a->next = vmb_b->next + off;
              NEXT_VMB_DSC (vmb_a)->prev -= off;
            }
          else
            {
              assert (vmb_b->next == 0);
              vmb_a->next = 0;
              vmp->lfree_ptr = (byte *) vmb_a - (byte *) (vmp->page) + DSC_SZ;
            }
          vmb_b->next = -off;
          join_fvmb (vmb_b, vmp);
        }
      else
        /*  a < b */
        {
          vmb_a->next = off;
          if (vmb_b->prev < 0)
            {
              vmb_a->prev = vmb_b->prev + off;
              PREV_VMB_DSC (vmb_a)->next -= off;
            }
          else
            {
              assert (vmb_b->prev == 0);
              vmb_a->prev = 0;
              vmp->free_ptr = (byte *) vmb_a - (byte *) (vmp->page) + DSC_SZ;
            }
          vmb_b->prev = -off;
          if (vmb_a->prev)
            join_fvmb (PREV_VMB_DSC (vmb_a), vmp);
          else
            join_fvmb (vmb_a, vmp);
        }
    }
  check_integrity (vmp);
}

static void 
delete_fvmb (pdescf vmb_a, P_vmpage vmp)
{
  /* let be paranoid about caching */
  flush_cache(); 
  if ( ((DV(0xf00)>>8) > 1) && (DV(0xf) > 2))
    fprintf (STDERR, "(%d free bef del-ing)...", check_integrity (vmp));
  
  if (vmb_a->next)
    {
      if (vmb_a->prev)
        {
          NEXT_VMB_DSC (vmb_a)->prev += vmb_a->prev;
          PREV_VMB_DSC (vmb_a)->next += vmb_a->next;
        }
      else
        {
          NEXT_VMB_DSC (vmb_a)->prev = 0;
          vmp->free_ptr += vmb_a->next;
        }
    }
  else
    {
      if (vmb_a->prev)
        {
          vmp->lfree_ptr += vmb_a->prev;
          PREV_VMB_DSC (vmb_a)->next = 0;
        }
      else
        {
          vmp->free_ptr = 0;
          vmp->lfree_ptr = 0;
        }
    }
  vmb_a->desc.mode = 1;
  vmp->used += vmb_a->desc.size + DSC_SZ;
}

static void 
split_vmb (register pdescf vmb_a,
           register P_vmpage vmp,
           register i4_t off)
/* "off" - offset on the page - place to split current vmb. Data stored
 * in the vmb ended here.
 */
{
  ALIGN (off, DSC_SZ);
  if (off + FDS_SZ < 
      (i4_t) ((byte *) vmb_a - vmp->page) + vmb_a->desc.size + DSC_SZ)
    {                           /* create new free vmb */
      register pdescf vmb_b = (pdescf) (vmp->page + off);

      if ( ((DV(0xf00)>>8) > 1) && (DV(0xf) > 2))
        fprintf (STDERR, "(%d free bef rep-ing)...",
                 check_integrity (vmp));
      
      off -= (i4_t) ((byte *) vmb_a - vmp->page);
      vmb_b->desc.size = vmb_a->desc.size - off;
      assert (vmb_b->desc.size > 0);
      vmb_a->desc.size = off - DSC_SZ;
      assert (vmb_a->desc.size > 0);
      vmb_b->desc.mode = 0;
      insert_fvmb (vmb_b, vmp);
    }
}

static void 
find_vmb (P_vmpage * pvmp, register VADR n, pdescf * pvmb_a)
{
  register byte *p;
  register P_vmpage vmp;
  register pdescf vmb_a;
  
  vmp = SEG_PTR (n)->base;
  n = SEG_OFF (n);
  while (vmp
         && (check_integrity (vmp) >= 0)
         && (n > vmp->base + vmp->size)
    )
    vmp = vmp->next;
  if (!vmp)
    yyfatal ("Virtual memory handler: required page not found");
  *pvmp = vmp;
  if (pvmb_a==NULL)
    return;
  /* looking for pvmb_a */
  p = vmp->page + (n - vmp->base);
  for (vmb_a = (pdescf) vmp->page;
       (byte *) vmb_a < vmp->page + vmp->size;
       vmb_a = (pdescf) ((byte *) vmb_a + vmb_a->desc.size + DSC_SZ))
    {
      if (!(vmb_a->desc.size > 0))
        yyfatal ("Virtual memory handler: size <=0 on level 1");
      if ((p >= (byte *) vmb_a + DSC_SZ) &&
          (p < (byte *) vmb_a + DSC_SZ + vmb_a->desc.size) &&
          (vmb_a->desc.mode > 0)
        )
        {
          *pvmb_a = vmb_a;
          return;
        }
    }
  yyfatal ("Virtual memory handler: can\'t find required block on page");
}

static P_vmpage 
new_vmpage (i4_t base, i4_t size)
{
  register P_vmpage ptr;
  register pdescf vmb_a;

  ALIGN (size, PAGE_ALIGN);
  {
    register Offset s = size-DSC_SZ;
    if (size-DSC_SZ != s)
      yyfatal ("Too large virtual memory segment required");
  }
  ptr = MALLOC (sizeof (*ptr));
  ptr->page = MALLOC (size);
  ptr->size = size;
  ptr->base = base;
  vmb_a = (pdescf) (ptr->page);
  vmb_a->desc.mode = 0;
  vmb_a->desc.size = size - DSC_SZ;
  vmb_a->next = vmb_a->prev = 0;
  ptr->used = 0;
  ptr->lfree_ptr = ptr->free_ptr = DSC_SZ;
  check_integrity (ptr);
  return ptr;
}

static P_vmpage 
resize_vmpage (P_vmpage vmp, i4_t size)
{
#define ret(v)  { debug_vmemory -=1; return (v); }

  debug_vmemory +=1;
  check_integrity (vmp);
  
  ALIGN (size, PAGE_ALIGN);
  
  {
    register Offset s = size - DSC_SZ;
    if (size-DSC_SZ != s)
      {
        lperror("Too large virtual memory segment required (%d > 32k)",
                size);
	yyfatal ("");
      }
  }
  if (size > vmp->size)
    {                                           /* increasing page size      */
      register pdescf vmb_a;
      if (vmp->flags==0)
        vmp->page = REALLOC (vmp->page, size); /* may be explicitly copy? ?!! */
      else
        {
          void *p = MALLOC(size);
          bcopy(vmp->page,p,vmp->size);
          vmp->page = p;
        }
      check_integrity (vmp); /* let's check if it was copied correctly       */
      vmb_a = (pdescf) (vmp->page + vmp->size);
      vmb_a->desc.size = size - vmp->size - DSC_SZ;
      vmb_a->desc.mode = DSC_SZ;
      vmp->used += size - vmp->size;
      vmp->size = size;
      check_integrity (vmp);  /* a lot of pseudo-used memory added to page   */
      insert_fvmb (vmb_a, vmp);
    }
  else if (size < vmp->size)  /* request to decrease page size               */
    {                         /* size must be equal "0" or "lfree_ptr-DSC_SZ"*/
      register byte *p;
      register pdescf vmb_a;
      if ( ! vmp->lfree_ptr)              /* if there is no free space       */
        ret (vmp);                        /* this request has to be rejected */
      {
        register i4_t sz = vmp->lfree_ptr - DSC_SZ;
        ALIGN(sz, PAGE_ALIGN);
        if (vmp->size == sz)       /* if there is only a tiny piece of space */
          ret (vmp);               /* it doen't worth to do anything         */
      }
      vmb_a = (pdescf) (vmp->page + vmp->lfree_ptr - DSC_SZ);
      p = (byte *) vmb_a + DSC_SZ + vmb_a->desc.size - vmp->size;
      if (p < vmp->page)           /* if free space is only in the middle of */
        ret (vmp);                 /* page we reject this request too        */
      if (p > vmp->page)
        yyfatal ("Virtual memory handler: segmentation fault on level 1");
      assert (p == vmp->page);
      
      if ((vmb_a->desc.size + DSC_SZ < sizeof (*vmp)) &&/*IF rest of page is tiny*/
          vmp->next &&                                 /*and there is a next page*/
          (vmp->next->base > vmp->base + vmp->size) && /* just after this one    */
          vmp->used                                    /*and there are some data */
        )                                              /*on this page            */
        ret (vmp);                        /* THEN it doesn't worth to strip page */
      
      delete_fvmb (vmb_a, vmp);
      
      {                                        /* test for page size "alignment" */
        register i4_t sz1,sz = vmp->size - vmb_a->desc.size - DSC_SZ;
        sz1 = sz;
        ALIGN (sz, PAGE_ALIGN);
        if (sz != sz1)
          while (sz < sz1 + FDS_SZ)
            sz += PAGE_ALIGN;
        if (size && size != sz)
          yyerror ("Virtual memory handler: "
                   "incorrect size in request for resizing page");
        size = vmp->size;       /* just save previous size of page           */
        vmp->used -= vmp->size - sz;
        vmp->size = sz;         /*                                           */
        if ( sz != sz1 )        /* if page size alignment requires to have a */
          {                     /* bit more space than it really need        */
            vmb_a->desc.size = sz - sz1 - DSC_SZ;  /* add tiny free block at */
            insert_fvmb(vmb_a,vmp);                /* the end of page        */
          }
      }
      check_integrity (vmp);
      if (vmp->size)
        {
          if(!vmp->flags)
            vmp->page = REALLOC (vmp->page, vmp->size);
        }
      else
        /* if no data on the page */
        {
          register P_vmpage vmp1 = vmp->next;
          if(!vmp->flags)
            FREE (vmp->page);
          vmp->page = NULL;
          if (vmp1)
            {
              bcopy ((void *) vmp1, (void *) vmp, sizeof (*vmp));
              FREE ((void *) vmp1);
            }
        }
    }
  check_integrity (vmp);
  ret (vmp);
#undef ret
}

static P_vmpage 
free_vmpage (P_vmpage ptr)
{
  if (ptr)
    {
      check_integrity (ptr);
      ptr->next = free_vmpage (ptr->next);
      if (ptr->next)
        return ptr;
      if ((ptr->used && (force_free == 0)))
        return ptr;
      if (ptr->flags == 0)      /* in other case it'll be freed later */
        {
          FREE (ptr->page);
          FREE (ptr);
        }
    }
  return NULL;
}

static P_vmpage 
join_memory (register P_vmpage vmp)
{
  register P_vmpage vmp1;
  register i4_t size;
    
  check_integrity (vmp);
  if (vmp == NULL)
    return NULL;
  if (vmp->next)
    vmp->next = join_memory (vmp->next);
  vmp = resize_vmpage (vmp, 0);
  if (vmp->next == 0)
    return vmp;
  if (vmp->next->base > vmp->base + vmp->size)
    return vmp;
  assert (vmp->next->base == vmp->base + vmp->size);
  vmp1 = vmp->next;
  size = vmp->size + vmp1->size;
  {
    register Offset s = size;
    if (s != size)
      return vmp;
  }
  /* We CAN join the page with follow one !!!*/

  vmp->next = vmp1->next;
  size=vmp->size;
  vmp = resize_vmpage (vmp, vmp->size+vmp1->size);
  /* correct last free_vmb */
  {
    register pdescf vmb_a, vmb_b;
    vmb_a = (pdescf) (vmp->page + vmp->lfree_ptr - DSC_SZ);
    delete_fvmb (vmb_a, vmp);
    if ((byte *) vmb_a < vmp->page + size)
      {
	vmb_a->desc.size = vmp->page + size - (byte *) vmb_a - DSC_SZ;
	insert_fvmb (vmb_a, vmp);
      }
    bcopy (vmp1->page, vmp->page + size, vmp1->size);
    assert(size + vmp1->size == vmp->size);
    vmp->used += vmp1->used - vmp1->size;
    if (vmp1->free_ptr) /* Increasing space is not expected */ 
      {
	if (vmp->lfree_ptr)
	  {
	    vmb_a = (pdescf) (vmp->page + vmp->lfree_ptr - DSC_SZ);
	    vmb_b = (pdescf) (vmp->page + size + vmp1->free_ptr-DSC_SZ);
	    vmb_b->prev = vmb_a->next = (byte *) vmb_b - (byte *) vmb_a;
	    vmp->lfree_ptr = size + vmp1->lfree_ptr;
	  }
	else
	  {
	    vmp->free_ptr = size + vmp1->free_ptr;
	    vmp->lfree_ptr = size + vmp1->lfree_ptr;
	  }
      }
  }
  vmp1->next = 0;
  {
    i4_t ff = force_free;
    force_free=1;
    assert (free_vmpage (vmp1) == 0);
    force_free=ff;
  }
  debug_vmemory +=1;
  check_integrity (vmp);
  debug_vmemory -=1;
  return vmp;
}

static void 
compress_data (register vmsegment * seg)
{
  register P_vmpage vmp;
  flush_cache();
  vmp = seg->base = check_for_null_page (free_vmpage (seg->base));
  if (vmp == NULL)
    return;
  seg->base = check_for_null_page (join_memory (seg->base));
}

static VADR 
ob_alloc (i4_t size, i4_t align, vmsegment * seg)
{
  register i4_t ptr;
  register P_vmpage vmp = seg->base;
  register pdescf vmb_a;
  i4_t pg_size = 0x1000;         /* 32k */
  i4_t os_needs = 0x40;
  if (size == 0)
    return (VADR)NULL;

  if (align != sizeof (i4_t) && align != PAGE_ALIGN )
    {
      if ((align > 0) && (align < sizeof (i4_t)))
        align = sizeof (i4_t);
      else
        {
          align = sizeof (i4_t);
          while ((align < size) && (align<PAGE_ALIGN) )
            align <<= 1;
          if (align > PAGE_ALIGN)
            align >>= 1;
        }
    }

  while ((size + DSC_SZ + align) >= (pg_size - os_needs))
    pg_size <<= 1;
  if (!vmp)
    vmp = seg->base = new_vmpage (0, pg_size);
  ptr = 0;
  for (;;)                   /* check free space for new adding new data and */
    {                        /*  add additional pages if it's needed         */
      check_integrity (vmp);
      if (vmp->free_ptr)     /* if there are some free vmb on page           */
        {
          register Offset p = vmp->free_ptr;
          for (;;)           /* check each of them                           */
            {
              vmb_a = (pdescf) (vmp->page + p - DSC_SZ);
              ptr = p % align;
              if (ptr)
                ptr = align - ptr;
              if (vmb_a->desc.size >= (size + ptr))
                {
                  ptr += p;
                  break;
                }
              ptr = 0;
              if (vmb_a->next == 0)
                break;
              p += vmb_a->next;
            }
          if (ptr)
            break;
        }
      if (vmp->next)
        vmp = vmp->next;
      else
        {
          vmb_a = (pdescf) (vmp->page + vmp->lfree_ptr - DSC_SZ);
          if (                  /* there is no large free tail on the page */
               vmp->lfree_ptr == 0
               || (vmp->lfree_ptr > vmp->size * 3 / 4)
               || (vmp->lfree_ptr + vmb_a->desc.size < vmp->size)
               || (pg_size > vmp->size)
#ifdef USE_XVM
               || (SEG_PTR(xmemory_segment) == seg) /* don't move xmalloc pages */
#endif
            )
            vmp = vmp->next = new_vmpage (vmp->base + vmp->size,
                                          pg_size-os_needs);
          else                  /* realloc page for large vmb */
            resize_vmpage (vmp, vmp->size + pg_size);
        }
    }
  /* now "vmb_a" points to free vmb descriptor
   * and "ptr"   is offset on page to aligned position of data inside this vmb.
   */
  delete_fvmb (vmb_a, vmp);
  split_vmb (vmb_a, vmp, ptr + size);

  vmb_a->desc.mode = align;
  bzero ((byte *) vmb_a + DSC_SZ, vmb_a->desc.size);
  ptr = vmp->base + ptr;
  {
    register i4_t sz = check_integrity (vmp);
    if ( ((DV(0xf00)>>8) > 1) && (DV(0xf) > 2))
      fprintf (STDERR,
               "Allocated %3X(%3X),aligned %2X at %5x (used %5X/free %5X)\n",
               size, vmb_a->desc.size, align, ptr, vmp->used, sz);
  }
#if 0
  {
    VADR critical_regions[] = {0};
    register i4_t i = sizeof(critical_regions)/sizeof(VADR);
    while ( i--)
      if ( critical_regions[i]==(VADR)ptr )
	{
	  yyerror("? ?");
	}
  }
#endif
  return (VADR) ptr;
}

static void
check_segment_integrity(register i4_t s_id)
{
  i4_t     i;
  i4_t     dv;
  P_vmpage vmp;
  pdescf   vmb_a;

  assert (s_id >= 0 && s_id < segment_table.ent_nmb);
  
  if( DV(0xf0) == 0)
    return;
  dv = debug_vmemory;
  debug_vmemory &= 0xf0f ;
  /*
   * looking for entries in relocation table which have to be deleted 
   */
#define TBL segment_table.entries[s_id]
  for(vmp= TBL.base;vmp;vmp = vmp->next)
    check_integrity(vmp);
#undef TBL

  debug_vmemory = 0;

#define TBL segment_table.entries[s_id].relocation_table
  for (i = 0; i < TBL.ent_nmb; i++ )
    if(TBL.entries[i])
      find_vmb (&vmp, SEG_BASE (s_id) + TBL.entries[i], &vmb_a);
#undef TBL
  
  /*
   * looking for entries in export name table which have to be deleted 
   */
#define TBL segment_table.entries[s_id].export_table
  for (i = 0; i < TBL.ent_nmb; i++ )
    if(TBL.entries[i].adr)
      {
        find_vmb (&vmp, SEG_BASE (s_id) + TBL.entries[i].adr, &vmb_a);
        find_vmb (&vmp, SEG_BASE (s_id) + TBL.entries[i].name, &vmb_a);
      }
#undef TBL
  debug_vmemory = dv;
}

static void
check_vm_integrity(void)
{
  i4_t  i;
  i4_t dv;

  if( DV(0xf0) == 0x10)
    return;
  
  dv = debug_vmemory;
  debug_vmemory &= 0xf1f ;
  
  for( i = 1; i < segment_table.ent_nmb; i++)
    check_segment_integrity(i);
  
  debug_vmemory = dv;
}

VADR 
vmrealloc (VADR old_ptr, i4_t new_size)
{
  P_vmpage vmp;
  pdescf vmb_a;
  register i4_t old_size;

  check_vm_integrity();
  if (SEG_OFF (old_ptr) == 0)
    return vmalloc (new_size);
  find_vmb (&vmp, old_ptr, &vmb_a);
  old_size = (i4_t) ((byte *) vmb_a - vmp->page) /*   vmb offset on page     */
    + DSC_SZ + vmb_a->desc.size                  /* + block size             */
    - (SEG_OFF (old_ptr) - vmp->base);           /* - offset of data on page */
  if (new_size + FDS_SZ <= old_size)
    /* decrease memory block size and create new free vmb */
    split_vmb (vmb_a, vmp, SEG_OFF (old_ptr) - vmp->base + new_size);
  else if (new_size > old_size)
    {
      register pdescf vmb_b = (pdescf) ((byte *) vmb_a + DSC_SZ +
                                        vmb_a->desc.size);
      if (((byte *)vmb_b < vmp->page + vmp->size ) &&
          (vmb_b->desc.mode == 0) &&
          (new_size <= old_size+DSC_SZ + vmb_b->desc.size))
        {                      /* append following free block to current one */
          delete_fvmb (vmb_b, vmp);
          vmb_a->desc.size += vmb_b->desc.size + DSC_SZ;
        }
      else
        {                     /* allocate another block with required length */
          register VADR n = ob_alloc (new_size, vmb_a->desc.mode,
                                      SEG_PTR (old_ptr));
          bcopy (vpointer (old_ptr), vpointer (n), old_size);
          insert_fvmb (vmb_a, vmp);
          return n;
        }
    }
  /* in other case we don't need to do anything */
  check_segment_integrity(SEG_IND(old_ptr));
  check_vm_integrity();
  return old_ptr;
}

void 
vmfree (VADR ptr)
{
  P_vmpage vmp;
  pdescf   vmb_a;
  i4_t      s_id;
  
  check_vm_integrity();
  if (SEG_OFF (ptr) == 0)
    return;
  find_vmb (&vmp, ptr, &vmb_a);
  s_id = SEG_IND(ptr);

  if ( ((DV(0xf00)>>8) > 2) && (DV(0xf) > 2))
    fprintf (STDERR, "Deallocated %X at %X (used %X/free %X)... ",
             vmb_a->desc.size + DSC_SZ, SEG_OFF(ptr), vmp->used, check_integrity (vmp));
  
  /*
   * looking for entries in relocation table which have to be deleted 
   */
#define TBL SEG_PTR(ptr)->relocation_table
  { 
    register i4_t i;
    register i4_t lo,hi,p;
    lo = vmp->base + ((char*)vmb_a - (char*)vmp->page) + DSC_SZ;
    hi = lo + vmb_a->desc.size;
    for (i = 0; i < TBL.ent_nmb; i++ )
      {
        p = TBL.entries[i];
        if ( lo <= p  && p <= hi )
          {
            TBL.entries[i] = 0; /* mark the entry dummy */
            TBL.used_ent --;
          }
      }
  }
#undef TBL
  
  /*
   * looking for entries in export name table which have to be deleted 
   */
#define TBL SEG_PTR(ptr)->export_table
  { 
    register i4_t i;
    register i4_t lo,hi,p;
    lo = vmp->base + ((char*)vmb_a - (char*)vmp->page) + DSC_SZ;
    hi = lo + vmb_a->desc.size;
    for (i = 0; i < TBL.ent_nmb; i++ )
      {
        p = TBL.entries[i].adr;
        if ( lo <= p  && p <= hi )
	  {
	    TBL.entries[i].adr = 0;     /* mark the entry dummy */
	    vmfree(TBL.entries[i].name);
	  }
      }
  }
#undef TBL


  check_segment_integrity(s_id);
  insert_fvmb (vmb_a, vmp);
  check_segment_integrity(s_id);

  if ( ((DV(0xf00)>>8) > 2) && (DV(0xf) > 2))
    fprintf (STDERR, "Done (used %X/free %X)\n",
             vmp->used, check_integrity (vmp));
  
  if (vmp == SEG_PTR (ptr)->base)
    SEG_PTR (ptr)->base = free_vmpage (vmp);
  else
    vmp->next = free_vmpage (vmp->next);
  check_vm_integrity();
  return;
}

void *
vpointer (VADR n)
{
  register byte *ret;
  VADR           n1 = n;
  P_vmpage vmp;

  if (SEG_OFF (n) == 0)
    return NULL;
  ret = check_cache(n);
  if (ret)
    return ret;
  check_vm_integrity();
  {                             /* import name processing */
    register i4_t s_id;
    s_id = SEG_ID (n);
    if (s_id < 0)
      n = resolve_reference (vpointer (SEG_BASE (~s_id) + SEG_OFF (n)));
  }
  find_vmb (&vmp, n, NULL);
  ret = vmp->page + (SEG_OFF (n) - vmp->base);
  
  check_vm_integrity();
  update_cache(n1, ret);
  return (void *) ret;
}

VADR 
vm_ob_alloc (i4_t size, i4_t align)
{
  register VADR obj;
  
  check_vm_integrity();
  enable_auto_create = 1;
  obj = ob_alloc (size, align, SEG_PTR (0));
  enable_auto_create = 0;
  
  check_segment_integrity(SEG_IND(obj));
  check_vm_integrity();
  return obj;
}

VADR 
get_vm_segment_id (VADR ptr)
{
  return SEG_BASE (SEG_IND (ptr));
}

VADR 
switch_to_segment (VADR ptr)
{
  segment_table.current = SEG_IND (ptr);
  return SEG_BASE (segment_table.current);
}

VADR 
create_segment (void)
{
  register i4_t p;
  for (p = segment_table.ent_nmb; --p > 0;)
    if (segment_table.entries[p].used == 0)
      break;
  if (p < 0)
    p = 0;
  if (!p)
    {
      ADD_ENTRY (segment_table);
      p = segment_table.ent_nmb - 1;
    }
  if (!p)                       /* index 0 is used for "current" segment. */
    {
      ADD_ENTRY (segment_table);
      p = segment_table.ent_nmb - 1;
    }
  segment_table.current = p;
  segment_table.entries[p].used = 1;
  check_segment_integrity(p);

  return (VADR) SEG_BASE (p);
}

int
unlink_segment (VADR s_id, i4_t enforce_free)
{
  register vmsegment *p = SEG_PTR (s_id);
  force_free = enforce_free;
  p->base = free_vmpage (p->base);
  force_free = 0;
  if (p->base)
    {
      if (debug_vmemory)
	{
	  i4_t u = 0;
	  register P_vmpage vmp;
	  for (vmp = p->base; vmp ; vmp = vmp->next )
	    u += vmp->used;
	  fprintf (STDERR,"There are %d bytes used in given segment\n",u);
	  force_free = 2;
	  p->base = free_vmpage (p->base);
	  force_free = 0;
	}
      return 0;
    }
  if (segment_table.current == SEG_IND (s_id))
    segment_table.current = 0;
  if (!p->relocation_table.dont_remove)
    FREE (p->relocation_table.entries);
  if (!p->export_table.dont_remove)
    FREE (p->export_table.entries);
  if (p->loaded_block_address)
    FREE (p->loaded_block_address);
  bzero ((void *) p, sizeof (vmsegment));
  return 1;
}

void *
export_segment (VADR s_id, i4_t *len, i4_t unlink_seg)
{
  register i4_t sz,seg_ind = SEG_IND(s_id);
  register P_vmpage vmp;
  register void *barr = NULL;
  register vmsegment *s1, *seg = SEG_PTR (s_id);
  i4_t      r_en, e_en;
  i4_t     dv = debug_vmemory;
  
  debug_vmemory |= 0x33;
  
  *len = 0;
  check_segment_integrity(seg_ind);
  compress_data (seg);
  check_segment_integrity(seg_ind);
  sz = sizeof (vmsegment);
  r_en = ( 1 + seg->relocation_table.ent_nmb / TBL_PORTION) * TBL_PORTION;
  if (seg->relocation_table.ent_nmb % TBL_PORTION ==0)
    r_en -= TBL_PORTION;
  sz += r_en * sizeof (*(seg->relocation_table.entries));
  
  e_en = ( 1 + seg->export_table.ent_nmb / TBL_PORTION) * TBL_PORTION;
  if (seg->export_table.ent_nmb % TBL_PORTION ==0)
    e_en -= TBL_PORTION;
  sz += e_en * sizeof (*(seg->export_table.entries));
  
  {
    register i4_t pgsp;
    for (pgsp = 0, vmp = seg->base; vmp; vmp = vmp->next)
      {
        pgsp += vmp->size;
        sz += sizeof (*vmp);
        check_integrity (vmp);
      }
    ALIGN (sz, PAGE_ALIGN);
    *len = sz + pgsp;
  }
  barr = MALLOC (*len);
  sz = 0;

#define ADD_BLK(p,l) {bcopy((p),((byte*)barr+sz),l);sz+=l;}

  ADD_BLK (seg, sizeof (vmsegment));
  s1 = (vmsegment *) barr;
  /* since that moment all of the 's1' reference and derivatives
     changed to offsets in allocated area instead of real address
  */
  s1->relocation_table.entries = (void *) sz;
  ADD_BLK (seg->relocation_table.entries,
           s1->relocation_table.ent_nmb *
           sizeof (*(s1->relocation_table.entries)));
  sz = (i4_t) s1->relocation_table.entries  +
    r_en * sizeof (*(s1->relocation_table.entries));
  s1->export_table.entries = (void *) sz;
  ADD_BLK (seg->export_table.entries,
           s1->export_table.ent_nmb *
           sizeof (*(s1->export_table.entries)));
  sz = (i4_t) s1->export_table.entries  +
    e_en * sizeof (*(s1->export_table.entries));
  s1->base = (void *) sz;

  for (vmp = seg->base; vmp; vmp = vmp->next)
    {
      register P_vmpage pvmp = (P_vmpage) ((byte *) barr + sz);
      ADD_BLK (vmp, sizeof (*vmp));
      if (pvmp->next)
        pvmp->next = (void *) sz;
      pvmp->flags = 1;
    }
  ALIGN (sz, PAGE_ALIGN);

  for (vmp = s1->base; vmp;
       vmp = ((P_vmpage) ((byte *) barr + (i4_t) vmp))->next)
    {
      register P_vmpage pvmp = (P_vmpage) ((byte *) barr + (i4_t) vmp);
      register i4_t page_offset = sz;
      ADD_BLK (pvmp->page, pvmp->size);
      pvmp->page = (void *) page_offset;
    }
#undef ADD_BLK
  assert (sz == *len);
  check_segment_integrity(seg_ind);
  if (unlink_seg)
    unlink_segment (s_id, 1);
  
  debug_vmemory = dv;
  return barr;
}

VADR 
link_segment (void *buffer, i4_t size)
{
  register i4_t sz;
  register VADR s_id;
  register P_vmpage vmp;
  register vmsegment *s;
  i4_t     dv = debug_vmemory;

  debug_vmemory |= 0x33;

  s_id = create_segment ();
  s = SEG_PTR (0);

#define CORRECT_ADR(p) p=(void*)((byte*)buffer+(i4_t)(p))
  bcopy (buffer, s, sizeof (vmsegment));
  CORRECT_ADR (s->base);
  CORRECT_ADR (s->relocation_table.entries);
  if (s->relocation_table.ent_nmb==0)
    s->relocation_table.entries = NULL;
  s->relocation_table.dont_remove = 1;
  CORRECT_ADR (s->export_table.entries);
  if (s->export_table.ent_nmb==0)
    s->export_table.entries = NULL;
  s->export_table.dont_remove = 1;
  for (vmp = s->base; vmp; vmp = vmp->next)
    {
      if (vmp->page)
        CORRECT_ADR (vmp->page);
      if (vmp->next)
        CORRECT_ADR (vmp->next);
      check_integrity (vmp);
    }
#undef CORRECT_ADR
  
  for (sz = 0; sz < s->relocation_table.ent_nmb; sz++)
    if (s->relocation_table.entries[sz])
      {
        VADR *vp = (VADR *) vpointer (s->relocation_table.entries[sz]);
        if (*vp)
          *vp = SEG_OFF(*vp) +
            (SEG_ID(*vp)>=0 ? s_id : SEG_BASE(~SEG_ID(s_id)));
      }
  s->loaded_block_address = buffer;
  check_segment_integrity(SEG_IND(s_id));
  debug_vmemory = dv;
  return s_id;
}

#define CORRECT_ADR(ptr) 			\
{						\
  VADR *vp = &(ptr);				\
  if(*vp)					\
    *vp = SEG_OFF(*vp)+				\
      SEG_BASE(get_vm_segment_id(*vp)^(SEG_ID(*vp)>=0?0:~0)); \
}

void 
register_relocation_address (VADR ptr)
{
  register i4_t      p;
  register reloc_tbl *rt  = & (SEG_PTR(ptr)->relocation_table);
#define TBL (*rt)
  
  assert(SEG_OFF(ptr) != 0);

  
  /* self checking --- fix to be more smart */ 
  for (p = 0; p < TBL.ent_nmb; p++ )
    if ( TBL.entries[p] == SEG_OFF(ptr) )
      {
        yyerror("Internal error: relocation address registered twice");
        return;
      }
  /*---------------*/
  if ( TBL.used_ent == TBL.ent_nmb )
    p = TBL.ent_nmb;
  else
    for (p = TBL.ent_nmb; p > 0 ;)
      if ( TBL.entries[--p] == 0 ) /* IF there are holes in the table      */
        break;                     /* just use it instead of adding new entries */
  if ( p >= TBL.ent_nmb)
    {
      ADD_ENTRY (TBL);
      p = TBL.ent_nmb - 1;
      TBL.used_ent = TBL.ent_nmb;
    }
  TBL.entries[p] = SEG_OFF(ptr);
  CORRECT_ADR (*((VADR *) vpointer (ptr)));
  check_segment_integrity(SEG_IND(ptr));
#undef TBL
}

void 
register_export_name (VADR object_ptr, VADR name_ptr)
{
#define TBL seg->export_table
  register i4_t       p,p1 = -1;
  register char      *name;
  register vmsegment *seg;

  assert (SEG_IND (object_ptr) == SEG_IND (name_ptr));
  seg = SEG_PTR (object_ptr);
  name = vpointer (name_ptr);
  for (p = 0; p < TBL.ent_nmb; p++)
    if (TBL.entries[p].adr == 0)
      {
	p1 = p;
	continue;
      }
    else if (0 == strcmp (vpointer (TBL.entries[p].name), name))
      { /* IF given name has already registered */
        if (SEG_OFF (object_ptr) != SEG_OFF (TBL.entries[p].adr))
	  {
	    /* but the exported object isn't the same */
	    yyerror ("Attempt to register different relocable"
		     " objects with the same name");
	  }
        return;
      }
  if ( p1 >= 0 )              /* IF there are holes in the table           */
    p = p1;                   /* just use it instead of adding new entries */
  else                        /* IF there is no holes in the table         */
    {                         /* add new entry there                       */
      ADD_ENTRY (TBL);
      p = TBL.ent_nmb - 1;
    }
  TBL.entries[p].adr  = SEG_OFF(object_ptr);
  TBL.entries[p].name = SEG_OFF(name_ptr);
  check_segment_integrity(SEG_IND(object_ptr));
#undef TBL
}

void 
register_export_address (VADR registered_adr, char *name)
{
  VADR name_ptr;
  register i4_t len;
  len = strlen (name) + 1;
  name_ptr = ob_alloc (len, 1, SEG_PTR (registered_adr));
  name_ptr += get_vm_segment_id (registered_adr);
  bcopy (name, vpointer (name_ptr), len);
  register_export_name (registered_adr, name_ptr);
}

static VADR 
resolve_reference_internal (char *name,i4_t local)
{
#define TBL  segment_table.entries[segment_table.current].export_table
  register i4_t p, s;
  register VADR obj = (VADR)NULL, cs = segment_table.current;

  for (s = 0; s < segment_table.ent_nmb; s++)
    {
      segment_table.current = s ? s : cs ;
      for (p = 0; p < TBL.ent_nmb; p++)
	if (TBL.entries[p].adr == 0 )
	  continue;
	else if (0 == strcmp (vpointer (TBL.entries[p].name), name))
          {
            register VADR o;
	    o = TBL.entries[p].adr + SEG_BASE (segment_table.current);
            if (!obj)
              obj = o;
            else if (obj != o)
              {
                fprintf (STDERR, "Ambigous name reference '%s'\n", 
                         (char *) vpointer (TBL.entries[p].name));
                obj = TNULL;
		goto EXIT;
              }
          }
      if (s == 0 && (obj || local))
	goto EXIT;
    }
EXIT:
  segment_table.current = cs;
  return obj;
#undef TBL
}

VADR 
resolve_reference (char *name)
{
  return resolve_reference_internal (name, 0);
}

VADR 
resolve_local_reference (char *name)
{
  return resolve_reference_internal (name, 1);
}

VADR 
external_reference (char *name) 
{ /* creating external reference to 'name' from 'current' segment */
  register VADR ptr;
  register i4_t len;
  len = strlen (name) + 1;
  ptr = vm_ob_alloc (len, 0); 
  bcopy (name, vpointer (ptr), len);
  /*
   * !!! It's a local reference - the name resolution has to be required  
   *     only while this segment marked as a 'current'
   */
  return SEG_OFF (ptr) + SEG_BASE (~SEG_ID (ptr));
}

#undef CORRECT_ADR

VADR
ptr2vadr(void *p)
{
  register i4_t i;
  register P_vmpage vmp;
  register vmsegment *s;

  for(i=0; i<segment_table.ent_nmb; i++)
    {
      s = &(segment_table.entries[i?i:segment_table.current]);
      for(vmp=s->base; vmp; vmp = vmp = vmp->next)
        if (vmp->page < (byte*)p && (byte*)p < vmp->page+vmp->size)
          return SEG_BASE(i?i:segment_table.current) +
            vmp->base + (i4_t)((byte*)p - vmp->page);
    }
  return VNULL;
}


#ifdef USE_XVM
/*******************************************************/
/*********$$$$$$$$$$$$$$$$$$$$$$$$$*********************/
/*******************************************************/

#define HPLIM 50
static void *huge_pieces[HPLIM];
static i4_t   hp_used = 0;

static void *
h1(void *p,i4_t size)
{
  i4_t i;
  for(i=hp_used;i--;)
    if(huge_pieces[i]==p)
      break;
  if(i<0)
    i=hp_used++;
  
  assert(i<HPLIM);
  if(size)
    huge_pieces[i] = (p?lrealloc(p,size):lmalloc(size));
  else
    {
      lfree(p);
      huge_pieces[i] = NULL;
    }
  return huge_pieces[i];
}

void *
xmalloc(i4_t size)
{
  VADR hold_seg = GET_CURRENT_SEGMENT_ID;
  void *ptr;
  if(size>=0x8000) /* 32k*/
    return h1(NULL,size);
  if(xmemory_segment==VNULL)
    xmemory_segment = create_segment();
  switch_to_segment(xmemory_segment);
  ptr = vpointer(vmalloc(size));
  switch_to_segment( hold_seg );
  return ptr;
}

void *
xcalloc (i4_t number, i4_t size)
{
  return xmalloc(number*size);
}

void *
xrealloc(void *p,i4_t size)
{
  VADR hold_seg = GET_CURRENT_SEGMENT_ID;
  void *ptr;
  if(size>=0x8000) /* 32k*/
    return h1(p,size);
  if(xmemory_segment==VNULL)
    xmemory_segment = create_segment();
  switch_to_segment(xmemory_segment);
  ptr = vpointer(vmrealloc(ptr2vadr(p),size));
  switch_to_segment( hold_seg );
  return ptr;
}

void
xfree(void *p)
{
  VADR hold_seg = GET_CURRENT_SEGMENT_ID;
  VADR v;
  if(xmemory_segment==VNULL)
    xmemory_segment = create_segment();
  switch_to_segment(xmemory_segment);
  v = ptr2vadr(p);
  if (v)
    vmfree(v);
  else
    h1(p,0);
  switch_to_segment( hold_seg );
}

#define L_DLT 0x100

static void *
lmalloc(i4_t size)
{
  void *p = malloc(2*L_DLT+size);
  if (p==NULL)
    yyfatal("Memory Full");
  p = (char*)p+L_DLT; 
  bzero (p, size);
  return p;
}

static void *
lrealloc(void *p,i4_t size)
{
  if(p)
    p = (char*)p-L_DLT;
  p = realloc(p,size + 2*L_DLT);
  if (p==NULL)
    yyfatal("Memory Full");
  /*  bzero (p, size); */
  return (char*)p + L_DLT;
}

static void
lfree(void *p)
{
  if(p)
    free((char*)p - L_DLT);
}

#endif
