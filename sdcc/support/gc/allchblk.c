/* 
 * Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers
 * Copyright (c) 1991-1994 by Xerox Corporation.  All rights reserved.
 * Copyright (c) 1998-1999 by Silicon Graphics.  All rights reserved.
 *
 * THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
 * OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
 *
 * Permission is hereby granted to use or copy this program
 * for any purpose,  provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 */

#define DEBUG
#undef DEBUG
#include <stdio.h>
#include "gc_priv.h"


/*
 * Free heap blocks are kept on one of several free lists,
 * depending on the size of the block.  Each free list is doubly linked.
 * Adjacent free blocks are coalesced.
 */

 
# define MAX_BLACK_LIST_ALLOC (2*HBLKSIZE)
		/* largest block we will allocate starting on a black   */
		/* listed block.  Must be >= HBLKSIZE.			*/


# define UNIQUE_THRESHOLD 32
	/* Sizes up to this many HBLKs each have their own free list    */
# define HUGE_THRESHOLD 256
	/* Sizes of at least this many heap blocks are mapped to a	*/
	/* single free list.						*/
# define FL_COMPRESSION 8
	/* In between sizes map this many distinct sizes to a single	*/
	/* bin.								*/

# define N_HBLK_FLS (HUGE_THRESHOLD - UNIQUE_THRESHOLD)/FL_COMPRESSION \
				 + UNIQUE_THRESHOLD

struct hblk * GC_hblkfreelist[N_HBLK_FLS+1] = { 0 };

/* Map a number of blocks to the appropriate large block free list index. */
int GC_hblk_fl_from_blocks(blocks_needed)
word blocks_needed;
{
    if (blocks_needed <= UNIQUE_THRESHOLD) return blocks_needed;
    if (blocks_needed >= HUGE_THRESHOLD) return N_HBLK_FLS;
    return (blocks_needed - UNIQUE_THRESHOLD)/FL_COMPRESSION
					+ UNIQUE_THRESHOLD;
    
}

# define HBLK_IS_FREE(hdr) ((hdr) -> hb_map == GC_invalid_map)
# define PHDR(hhdr) HDR(hhdr -> hb_prev)
# define NHDR(hhdr) HDR(hhdr -> hb_next)

# ifdef USE_MUNMAP
#   define IS_MAPPED(hhdr) (((hhdr) -> hb_flags & WAS_UNMAPPED) == 0)
# else  /* !USE_MMAP */
#   define IS_MAPPED(hhdr) 1
# endif /* USE_MUNMAP */

# if !defined(NO_DEBUGGING)
void GC_print_hblkfreelist()
{
    struct hblk * h;
    word total_free = 0;
    hdr * hhdr;
    word sz;
    int i;
    
    for (i = 0; i <= N_HBLK_FLS; ++i) {
      h = GC_hblkfreelist[i];
      if (0 != h) GC_printf1("Free list %ld:\n", (unsigned long)i);
      while (h != 0) {
        hhdr = HDR(h);
        sz = hhdr -> hb_sz;
    	GC_printf2("\t0x%lx size %lu ", (unsigned long)h, (unsigned long)sz);
    	total_free += sz;
        if (GC_is_black_listed(h, HBLKSIZE) != 0) {
             GC_printf0("start black listed\n");
        } else if (GC_is_black_listed(h, hhdr -> hb_sz) != 0) {
             GC_printf0("partially black listed\n");
        } else {
             GC_printf0("not black listed\n");
        }
        h = hhdr -> hb_next;
      }
    }
    if (total_free != GC_large_free_bytes) {
	GC_printf1("GC_large_free_bytes = %lu (INCONSISTENT!!)\n",
		   (unsigned long) GC_large_free_bytes);
    }
    GC_printf1("Total of %lu bytes on free list\n", (unsigned long)total_free);
}

/* Return the free list index on which the block described by the header */
/* appears, or -1 if it appears nowhere.				 */
int free_list_index_of(wanted)
hdr * wanted;
{
    struct hblk * h;
    hdr * hhdr;
    int i;
    
    for (i = 0; i <= N_HBLK_FLS; ++i) {
      h = GC_hblkfreelist[i];
      while (h != 0) {
        hhdr = HDR(h);
	if (hhdr == wanted) return i;
        h = hhdr -> hb_next;
      }
    }
    return -1;
}

void GC_dump_regions()
{
    int i;
    ptr_t start, end;
    ptr_t p;
    size_t bytes;
    hdr *hhdr;
    for (i = 0; i < GC_n_heap_sects; ++i) {
	start = GC_heap_sects[i].hs_start;
	bytes = GC_heap_sects[i].hs_bytes;
	end = start + bytes;
	/* Merge in contiguous sections.	*/
	  while (i+1 < GC_n_heap_sects && GC_heap_sects[i+1].hs_start == end) {
	    ++i;
	    end = GC_heap_sects[i].hs_start + GC_heap_sects[i].hs_bytes;
	  }
	GC_printf2("***Section from 0x%lx to 0x%lx\n", start, end);
	for (p = start; p < end;) {
	    hhdr = HDR(p);
	    GC_printf1("\t0x%lx ", (unsigned long)p);
	    if (IS_FORWARDING_ADDR_OR_NIL(hhdr)) {
		GC_printf1("Missing header!!\n", hhdr);
		p += HBLKSIZE;
		continue;
	    }
	    if (HBLK_IS_FREE(hhdr)) {
                int correct_index = GC_hblk_fl_from_blocks(
					divHBLKSZ(hhdr -> hb_sz));
	        int actual_index;
		
		GC_printf1("\tfree block of size 0x%lx bytes",
			   (unsigned long)(hhdr -> hb_sz));
	 	if (IS_MAPPED(hhdr)) {
		    GC_printf0("\n");
		} else {
		    GC_printf0("(unmapped)\n");
		}
		actual_index = free_list_index_of(hhdr);
		if (-1 == actual_index) {
		    GC_printf1("\t\tBlock not on free list %ld!!\n",
				correct_index);
		} else if (correct_index != actual_index) {
		    GC_printf2("\t\tBlock on list %ld, should be on %ld!!\n",
			       actual_index, correct_index);
		}
		p += hhdr -> hb_sz;
	    } else {
		GC_printf1("\tused for blocks of size 0x%lx bytes\n",
			   (unsigned long)WORDS_TO_BYTES(hhdr -> hb_sz));
		p += HBLKSIZE * OBJ_SZ_TO_BLOCKS(hhdr -> hb_sz);
	    }
	}
    }
}

# endif /* NO_DEBUGGING */

/* Initialize hdr for a block containing the indicated size and 	*/
/* kind of objects.							*/
/* Return FALSE on failure.						*/
static GC_bool setup_header(hhdr, sz, kind, flags)
register hdr * hhdr;
word sz;	/* object size in words */
int kind;
unsigned char flags;
{
    register word descr;
    
    /* Add description of valid object pointers */
      if (!GC_add_map_entry(sz)) return(FALSE);
      hhdr -> hb_map = GC_obj_map[sz > MAXOBJSZ? 0 : sz];
      
    /* Set size, kind and mark proc fields */
      hhdr -> hb_sz = sz;
      hhdr -> hb_obj_kind = kind;
      hhdr -> hb_flags = flags;
      descr = GC_obj_kinds[kind].ok_descriptor;
      if (GC_obj_kinds[kind].ok_relocate_descr) descr += WORDS_TO_BYTES(sz);
      hhdr -> hb_descr = descr;
      
    /* Clear mark bits */
      GC_clear_hdr_marks(hhdr);
      
    hhdr -> hb_last_reclaimed = (unsigned short)GC_gc_no;
    return(TRUE);
}

#define FL_UNKNOWN -1
/*
 * Remove hhdr from the appropriate free list.
 * We assume it is on the nth free list, or on the size
 * appropriate free list if n is FL_UNKNOWN.
 */
void GC_remove_from_fl(hhdr, n)
hdr * hhdr;
int n;
{
    GC_ASSERT(((hhdr -> hb_sz) & (HBLKSIZE-1)) == 0);
    if (hhdr -> hb_prev == 0) {
        int index;
	if (FL_UNKNOWN == n) {
            index = GC_hblk_fl_from_blocks(divHBLKSZ(hhdr -> hb_sz));
	} else {
	    index = n;
	}
	GC_ASSERT(HDR(GC_hblkfreelist[index]) == hhdr);
	GC_hblkfreelist[index] = hhdr -> hb_next;
    } else {
	PHDR(hhdr) -> hb_next = hhdr -> hb_next;
    }
    if (0 != hhdr -> hb_next) {
	GC_ASSERT(!IS_FORWARDING_ADDR_OR_NIL(NHDR(hhdr)));
	NHDR(hhdr) -> hb_prev = hhdr -> hb_prev;
    }
}

/*
 * Return a pointer to the free block ending just before h, if any.
 */
struct hblk * GC_free_block_ending_at(h)
struct hblk *h;
{
    struct hblk * p = h - 1;
    hdr * phdr = HDR(p);

    while (0 != phdr && IS_FORWARDING_ADDR_OR_NIL(phdr)) {
	p = FORWARDED_ADDR(p,phdr);
	phdr = HDR(p);
    }
    if (0 != phdr && HBLK_IS_FREE(phdr)) return p;
    p = GC_prev_block(h - 1);
    if (0 != p) {
      phdr = HDR(p);
      if (HBLK_IS_FREE(phdr) && (ptr_t)p + phdr -> hb_sz == (ptr_t)h) {
	return p;
      }
    }
    return 0;
}

/*
 * Add hhdr to the appropriate free list.
 * We maintain individual free lists sorted by address.
 */
void GC_add_to_fl(h, hhdr)
struct hblk *h;
hdr * hhdr;
{
    int index = GC_hblk_fl_from_blocks(divHBLKSZ(hhdr -> hb_sz));
    struct hblk *second = GC_hblkfreelist[index];
#   ifdef GC_ASSERTIONS
      struct hblk *next = (struct hblk *)((word)h + hhdr -> hb_sz);
      hdr * nexthdr = HDR(next);
      struct hblk *prev = GC_free_block_ending_at(h);
      hdr * prevhdr = HDR(prev);
      GC_ASSERT(nexthdr == 0 || !HBLK_IS_FREE(nexthdr) || !IS_MAPPED(nexthdr));
      GC_ASSERT(prev == 0 || !HBLK_IS_FREE(prevhdr) || !IS_MAPPED(prevhdr));
#   endif
    GC_ASSERT(((hhdr -> hb_sz) & (HBLKSIZE-1)) == 0);
    GC_hblkfreelist[index] = h;
    hhdr -> hb_next = second;
    hhdr -> hb_prev = 0;
    if (0 != second) HDR(second) -> hb_prev = h;
    GC_invalidate_map(hhdr);
}

#ifdef USE_MUNMAP

/* Unmap blocks that haven't been recently touched.  This is the only way */
/* way blocks are ever unmapped.					  */
void GC_unmap_old(void)
{
    struct hblk * h;
    hdr * hhdr;
    word sz;
    unsigned short last_rec, threshold;
    int i;
#   define UNMAP_THRESHOLD 6
    
    for (i = 0; i <= N_HBLK_FLS; ++i) {
      for (h = GC_hblkfreelist[i]; 0 != h; h = hhdr -> hb_next) {
        hhdr = HDR(h);
	if (!IS_MAPPED(hhdr)) continue;
	threshold = (unsigned short)(GC_gc_no - UNMAP_THRESHOLD);
	last_rec = hhdr -> hb_last_reclaimed;
	if (last_rec > GC_gc_no
	    || last_rec < threshold && threshold < GC_gc_no
				       /* not recently wrapped */) {
          sz = hhdr -> hb_sz;
	  GC_unmap((ptr_t)h, sz);
	  hhdr -> hb_flags |= WAS_UNMAPPED;
    	}
      }
    }  
}

/* Merge all unmapped blocks that are adjacent to other free		*/
/* blocks.  This may involve remapping, since all blocks are either	*/
/* fully mapped or fully unmapped.					*/
void GC_merge_unmapped(void)
{
    struct hblk * h, *next;
    hdr * hhdr, *nexthdr;
    word size, nextsize;
    int i;
    
    for (i = 0; i <= N_HBLK_FLS; ++i) {
      h = GC_hblkfreelist[i];
      while (h != 0) {
	hhdr = HDR(h);
	size = hhdr->hb_sz;
	next = (struct hblk *)((word)h + size);
	nexthdr = HDR(next);
	/* Coalesce with successor, if possible */
	  if (0 != nexthdr && HBLK_IS_FREE(nexthdr)) {
	    nextsize = nexthdr -> hb_sz;
	    if (IS_MAPPED(hhdr)) {
	      GC_ASSERT(!IS_MAPPED(nexthdr));
	      /* make both consistent, so that we can merge */
	        if (size > nextsize) {
		  GC_remap((ptr_t)next, nextsize);
		} else {
		  GC_unmap((ptr_t)h, size);
		  hhdr -> hb_flags |= WAS_UNMAPPED;
		}
	    } else if (IS_MAPPED(nexthdr)) {
	      GC_ASSERT(!IS_MAPPED(hhdr));
	      if (size > nextsize) {
		GC_unmap((ptr_t)next, nextsize);
	      } else {
		GC_remap((ptr_t)h, size);
		hhdr -> hb_flags &= ~WAS_UNMAPPED;
	      }
	    } else {
	      /* Unmap any gap in the middle */
		GC_unmap_gap((ptr_t)h, size, (ptr_t)next, nexthdr -> hb_sz);
	    }
	    /* If they are both unmapped, we merge, but leave unmapped. */
	    GC_remove_from_fl(hhdr, i);
	    GC_remove_from_fl(nexthdr, FL_UNKNOWN);
	    hhdr -> hb_sz += nexthdr -> hb_sz; 
	    GC_remove_header(next);
	    GC_add_to_fl(h, hhdr); 
	    /* Start over at beginning of list */
	    h = GC_hblkfreelist[i];
	  } else /* not mergable with successor */ {
	    h = hhdr -> hb_next;
	  }
      } /* while (h != 0) ... */
    } /* for ... */
}

#endif /* USE_MUNMAP */

/*
 * Return a pointer to a block starting at h of length bytes.
 * Memory for the block is mapped.
 * Remove the block from its free list, and return the remainder (if any)
 * to its appropriate free list.
 * May fail by returning 0.
 * The header for the returned block must be set up by the caller.
 * If the return value is not 0, then hhdr is the header for it.
 */
struct hblk * GC_get_first_part(h, hhdr, bytes, index)
struct hblk *h;
hdr * hhdr;
word bytes;
int index;
{
    word total_size = hhdr -> hb_sz;
    struct hblk * rest;
    hdr * rest_hdr;

    GC_ASSERT((total_size & (HBLKSIZE-1)) == 0);
    GC_remove_from_fl(hhdr, index);
    if (total_size == bytes) return h;
    rest = (struct hblk *)((word)h + bytes);
    if (!GC_install_header(rest)) return(0);
    rest_hdr = HDR(rest);
    rest_hdr -> hb_sz = total_size - bytes;
    rest_hdr -> hb_flags = 0;
#   ifdef GC_ASSERTIONS
      // Mark h not free, to avoid assertion about adjacent free blocks.
        hhdr -> hb_map = 0;
#   endif
    GC_add_to_fl(rest, rest_hdr);
    return h;
}

/*
 * H is a free block.  N points at an address inside it.
 * A new header for n has already been set up.  Fix up h's header
 * to reflect the fact that it is being split, move it to the
 * appropriate free list.
 * N replaces h in the original free list.
 *
 * Nhdr is not completely filled in, since it is about to allocated.
 * It may in fact end up on the wrong free list for its size.
 * (Hence adding it to a free list is silly.  But this path is hopefully
 * rare enough that it doesn't matter.  The code is cleaner this way.)
 */
void GC_split_block(h, hhdr, n, nhdr, index)
struct hblk *h;
hdr * hhdr;
struct hblk *n;
hdr * nhdr;
int index;	/* Index of free list */
{
    word total_size = hhdr -> hb_sz;
    word h_size = (word)n - (word)h;
    struct hblk *prev = hhdr -> hb_prev;
    struct hblk *next = hhdr -> hb_next;

    /* Replace h with n on its freelist */
      nhdr -> hb_prev = prev;
      nhdr -> hb_next = next;
      nhdr -> hb_sz = total_size - h_size;
      nhdr -> hb_flags = 0;
      if (0 != prev) {
	HDR(prev) -> hb_next = n;
      } else {
        GC_hblkfreelist[index] = n;
      }
      if (0 != next) {
	HDR(next) -> hb_prev = n;
      }
#     ifdef GC_ASSERTIONS
	nhdr -> hb_map = 0;	/* Don't fail test for consecutive	*/
				/* free blocks in GC_add_to_fl.		*/
#     endif
#   ifdef USE_MUNMAP
      hhdr -> hb_last_reclaimed = GC_gc_no;
#   endif
    hhdr -> hb_sz = h_size;
    GC_add_to_fl(h, hhdr);
    GC_invalidate_map(nhdr);
}
	
struct hblk * GC_allochblk_nth();

/*
 * Allocate (and return pointer to) a heap block
 *   for objects of size sz words, searching the nth free list.
 *
 * NOTE: We set obj_map field in header correctly.
 *       Caller is responsible for building an object freelist in block.
 *
 * We clear the block if it is destined for large objects, and if
 * kind requires that newly allocated objects be cleared.
 */
struct hblk *
GC_allochblk(sz, kind, flags)
word sz;
int kind;
unsigned char flags;  /* IGNORE_OFF_PAGE or 0 */
{
    int start_list = GC_hblk_fl_from_blocks(OBJ_SZ_TO_BLOCKS(sz));
    int i;
    for (i = start_list; i <= N_HBLK_FLS; ++i) {
	struct hblk * result = GC_allochblk_nth(sz, kind, flags, i);
	if (0 != result) return result;
    }
    return 0;
}
/*
 * The same, but with search restricted to nth free list.
 */
struct hblk *
GC_allochblk_nth(sz, kind, flags, n)
word sz;
int kind;
unsigned char flags;  /* IGNORE_OFF_PAGE or 0 */
int n;
{
    register struct hblk *hbp;
    register hdr * hhdr;		/* Header corr. to hbp */
    register struct hblk *thishbp;
    register hdr * thishdr;		/* Header corr. to hbp */
    signed_word size_needed;    /* number of bytes in requested objects */
    signed_word size_avail;	/* bytes available in this block	*/

    size_needed = HBLKSIZE * OBJ_SZ_TO_BLOCKS(sz);

    /* search for a big enough block in free list */
	hbp = GC_hblkfreelist[n];
	hhdr = HDR(hbp);
	for(; 0 != hbp; hbp = hhdr -> hb_next, hhdr = HDR(hbp)) {
	    size_avail = hhdr->hb_sz;
	    if (size_avail < size_needed) continue;
#	    ifdef PRESERVE_LAST
		if (size_avail != size_needed
		    && !GC_incremental && GC_should_collect()) {
		    continue;
		} 
#	    endif
	    /* If the next heap block is obviously better, go on.	*/
	    /* This prevents us from disassembling a single large block */
	    /* to get tiny blocks.					*/
	    {
	      signed_word next_size;
	      
	      thishbp = hhdr -> hb_next;
	      if (thishbp != 0) {
	        thishdr = HDR(thishbp);
	        next_size = (signed_word)(thishdr -> hb_sz);
	        if (next_size < size_avail
	          && next_size >= size_needed
	          && !GC_is_black_listed(thishbp, (word)size_needed)) {
	          continue;
	        }
	      }
	    }
	    if ( !IS_UNCOLLECTABLE(kind) &&
	         (kind != PTRFREE || size_needed > MAX_BLACK_LIST_ALLOC)) {
	      struct hblk * lasthbp = hbp;
	      ptr_t search_end = (ptr_t)hbp + size_avail - size_needed;
	      signed_word orig_avail = size_avail;
	      signed_word eff_size_needed = ((flags & IGNORE_OFF_PAGE)?
	      					HBLKSIZE
	      					: size_needed);
	      
	      
	      while ((ptr_t)lasthbp <= search_end
	             && (thishbp = GC_is_black_listed(lasthbp,
	             				      (word)eff_size_needed))) {
	        lasthbp = thishbp;
	      }
	      size_avail -= (ptr_t)lasthbp - (ptr_t)hbp;
	      thishbp = lasthbp;
	      if (size_avail >= size_needed) {
	        if (thishbp != hbp && GC_install_header(thishbp)) {
		  /* Make sure it's mapped before we mangle it. */
#		    ifdef USE_MUNMAP
		      if (!IS_MAPPED(hhdr)) {
		        GC_remap((ptr_t)hbp, size_avail);
		        hhdr -> hb_flags &= ~WAS_UNMAPPED;
		      }
#		    endif
	          /* Split the block at thishbp */
	              thishdr = HDR(thishbp);
		      GC_split_block(hbp, hhdr, thishbp, thishdr, n);
		  /* Advance to thishbp */
		      hbp = thishbp;
		      hhdr = thishdr;
		      /* We must now allocate thishbp, since it may	*/
		      /* be on the wrong free list.			*/
		}
	      } else if (size_needed > (signed_word)BL_LIMIT
	                 && orig_avail - size_needed
			    > (signed_word)BL_LIMIT) {
	        /* Punt, since anything else risks unreasonable heap growth. */
	        WARN("Needed to allocate blacklisted block at 0x%lx\n",
		     (word)hbp);
	        size_avail = orig_avail;
	      } else if (size_avail == 0 && size_needed == HBLKSIZE
			 && IS_MAPPED(hhdr)) {
		if (!GC_find_leak) {
	      	  static unsigned count = 0;
	      	  
	      	  /* The block is completely blacklisted.  We need 	*/
	      	  /* to drop some such blocks, since otherwise we spend */
	      	  /* all our time traversing them if pointerfree	*/
	      	  /* blocks are unpopular.				*/
	          /* A dropped block will be reconsidered at next GC.	*/
	          if ((++count & 3) == 0) {
	            /* Allocate and drop the block in small chunks, to	*/
	            /* maximize the chance that we will recover some	*/
	            /* later.						*/
		      word total_size = hhdr -> hb_sz;
	              struct hblk * limit = hbp + divHBLKSZ(total_size);
	              struct hblk * h;
		      struct hblk * prev = hhdr -> hb_prev;
	              
		      GC_words_wasted += total_size;
		      GC_large_free_bytes -= total_size;
		      GC_remove_from_fl(hhdr, n);
	              for (h = hbp; h < limit; h++) {
	                if (h == hbp || GC_install_header(h)) {
	                  hhdr = HDR(h);
	                  (void) setup_header(
	                	  hhdr,
	              		  BYTES_TO_WORDS(HBLKSIZE - HDR_BYTES),
	              		  PTRFREE, 0); /* Cant fail */
	              	  if (GC_debugging_started) {
	              	    BZERO(h + HDR_BYTES, HBLKSIZE - HDR_BYTES);
	              	  }
	                }
	              }
	            /* Restore hbp to point at free block */
		      hbp = prev;
		      if (0 == hbp) {
			return GC_allochblk_nth(sz, kind, flags, n);
		      }
	   	      hhdr = HDR(hbp);
	          }
		}
	      }
	    }
	    if( size_avail >= size_needed ) {
#		ifdef USE_MUNMAP
		  if (!IS_MAPPED(hhdr)) {
		    GC_remap((ptr_t)hbp, size_avail);
		    hhdr -> hb_flags &= ~WAS_UNMAPPED;
		  }
#	        endif
		/* hbp may be on the wrong freelist; the parameter n	*/
		/* is important.					*/
		hbp = GC_get_first_part(hbp, hhdr, size_needed, n);
		break;
	    }
	}

    if (0 == hbp) return 0;
	
    /* Notify virtual dirty bit implementation that we are about to write. */
    	GC_write_hint(hbp);
    
    /* Add it to map of valid blocks */
    	if (!GC_install_counts(hbp, (word)size_needed)) return(0);
    	/* This leaks memory under very rare conditions. */
    		
    /* Set up header */
        if (!setup_header(hhdr, sz, kind, flags)) {
            GC_remove_counts(hbp, (word)size_needed);
            return(0); /* ditto */
        }
        
    /* Clear block if necessary */
	if (GC_debugging_started
	    || sz > MAXOBJSZ && GC_obj_kinds[kind].ok_init) {
	    BZERO(hbp + HDR_BYTES,  size_needed - HDR_BYTES);
	}

    /* We just successfully allocated a block.  Restart count of	*/
    /* consecutive failures.						*/
    {
	extern unsigned GC_fail_count;
	
	GC_fail_count = 0;
    }

    GC_large_free_bytes -= size_needed;
    
    GC_ASSERT(IS_MAPPED(hhdr));
    return( hbp );
}
 
struct hblk * GC_freehblk_ptr = 0;  /* Search position hint for GC_freehblk */

/*
 * Free a heap block.
 *
 * Coalesce the block with its neighbors if possible.
 *
 * All mark words are assumed to be cleared.
 */
void
GC_freehblk(hbp)
struct hblk *hbp;
{
struct hblk *next, *prev;
hdr *hhdr, *prevhdr, *nexthdr;
signed_word size;


    hhdr = HDR(hbp);
    size = hhdr->hb_sz;
    size = HBLKSIZE * OBJ_SZ_TO_BLOCKS(size);
    GC_remove_counts(hbp, (word)size);
    hhdr->hb_sz = size;
    
    /* Check for duplicate deallocation in the easy case */
      if (HBLK_IS_FREE(hhdr)) {
        GC_printf1("Duplicate large block deallocation of 0x%lx\n",
        	   (unsigned long) hbp);
      }

    GC_ASSERT(IS_MAPPED(hhdr));
    GC_invalidate_map(hhdr);
    next = (struct hblk *)((word)hbp + size);
    nexthdr = HDR(next);
    prev = GC_free_block_ending_at(hbp);
    /* Coalesce with successor, if possible */
      if(0 != nexthdr && HBLK_IS_FREE(nexthdr) && IS_MAPPED(nexthdr)) {
	GC_remove_from_fl(nexthdr, FL_UNKNOWN);
	hhdr -> hb_sz += nexthdr -> hb_sz; 
	GC_remove_header(next);
      }
    /* Coalesce with predecessor, if possible. */
      if (0 != prev) {
	prevhdr = HDR(prev);
	if (IS_MAPPED(prevhdr)) {
	  GC_remove_from_fl(prevhdr, FL_UNKNOWN);
	  prevhdr -> hb_sz += hhdr -> hb_sz;
	  GC_remove_header(hbp);
	  hbp = prev;
	  hhdr = prevhdr;
	}
      }

    GC_large_free_bytes += size;
    GC_add_to_fl(hbp, hhdr);    
}

