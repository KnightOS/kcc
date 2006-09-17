/* lkarea.c */

/*
 * (C) Copyright 1989-1995
 * All Rights Reserved
 *
 * Alan R. Baldwin
 * 721 Berkeley St.
 * Kent, Ohio  44240
 */

#include <stdio.h>
#include <string.h>
#include "aslink.h"

/*)Module	lkarea.c
 *
 *	The module lkarea.c contains the functions which
 *	create and link together all area definitions read
 *	from the .rel file(s).
 *
 *	lkarea.c contains the following functions:
 *		VOID	lnkarea()
 *		VOID	lnksect()
 *		VOID	lkparea()
 *		VOID	newarea()
 *
 *	lkarea.c contains no global variables.
 */

/*)Function	VOID	newarea()
 * 
 *	The function newarea() creates and/or modifies area
 *	and areax structures for each A directive read from
 *	the .rel file(s).  The function lkparea() is called
 *	to find the area structure associated with this name.
 *	If the area does not yet exist then a new area
 *	structure is created and linked to any existing
 *	linked area structures. The area flags are copied
 *	into the area flag variable.  For each occurence of
 *	an A directive an areax structure is created and
 *	linked to the areax structures associated with this
 *	area.  The size of this area section is placed into
 *	the areax structure.  The flag value for all subsequent
 *	area definitions for the same area are compared and
 *	flagged as an error if they are not identical.
 *	The areax structure created for every occurence of
 *	an A directive is loaded with a pointer to the base
 *	area structure and a pointer to the associated
 *	head structure.  And finally, a pointer to this
 *	areax structure is loaded into the list of areax
 *	structures in the head structure.  Refer to lkdata.c
 *	for details of the structures and their linkage.
 *
 *	local variables:
 *		areax **halp		pointer to an array of pointers
 *		int	i		counter, loop variable, value
 *		char	id[]		id string
 *		int	narea		number of areas in this head structure
 *		areax *	taxp		pointer to an areax structure
 *					to areax structures
 *
 *	global variables:
 *		area	*ap		Pointer to the current
 *				 	area structure
 *		areax	*axp		Pointer to the current
 *				 	areax structure
 *		head	*hp		Pointer to the current
 *				 	head structure
 *		int	lkerr		error flag
 *
 *	functions called:
 *		Addr_T	eval()		lkeval.c
 *		VOID	exit()		c_library
 *		int	fprintf()	c_library
 *		VOID	getid()		lklex.c
 *		VOID	lkparea()	lkarea.c
 *		VOID	skip()		lklex.c
 *
 *	side effects:
 *		The area and areax structures are created and
 *		linked with the appropriate head structures.
 *		Failure to allocate area or areax structure
 *		space will terminate the linker.  Other internal
 *		errors most likely caused by corrupted .rel
 *		files will also terminate the linker.
 */

/*
 * Create an area entry.
 *
 * A xxxxxx size nnnn flags mm
 *   |           |          |
 *   |           |          `--  ap->a_flag
 *   |           `------------- axp->a_size
 *   `-------------------------  ap->a_id
 *
 */
VOID
newarea()
{
	register int i, narea;
	struct areax *taxp;
	struct areax **halp;
	char id[NCPS];

	/*
	 * Create Area entry
	 */
	getid(id, -1);
	lkparea(id);
	/*
	 * Evaluate area size
	 */
	skip(-1);
	axp->a_size = eval();
	/*
	 * Evaluate flags
	 */
	skip(-1);
	i = 0;
	taxp = ap->a_axp;
	while (taxp->a_axp) {
		++i;
		taxp = taxp->a_axp;
	}
	if (i == 0) {
		ap->a_flag = eval();
	} else {
		i = eval();
		if (i && (ap->a_flag != i)) {
		    fprintf(stderr, "Conflicting flags in area %.8s\n", id);
		    lkerr++;
		}
	}
	/*
	 * Place pointer in header area list
	 */
	if (headp == NULL) {
		fprintf(stderr, "No header defined\n");
		lkexit(1);
	}
	narea = hp->h_narea;
	halp = hp->a_list;
	for (i=0; i < narea ;++i) {
		if (halp[i] == NULL) {
			halp[i] = taxp;
			return;
		}
	}
	fprintf(stderr, "Header area list overflow\n");
	lkexit(1);
}

/*)Function	VOID	lkparea(id)
 *
 *		char *	id		pointer to the area name string
 *
 *	The function lkparea() searches the linked area structures
 *	for a name match.  If the name is not found then an area
 *	structure is created.  An areax structure is created and
 *	appended to the areax structures linked to the area structure.
 *	The associated base area and head structure pointers are
 *	loaded into the areax structure.
 *
 *	local variables:
 *		area *	tap		pointer to an area structure
 *		areax *	taxp		pointer to an areax structure
 *
 *	global variables:
 *		area	*ap		Pointer to the current
 *				 	area structure
 *		area	*areap		The pointer to the first
 *				 	area structure of a linked list
 *		areax	*axp		Pointer to the current
 *				 	areax structure
 *
 *	functions called:
 *		VOID *	new()		lksym()
 *		char *	strcpy()	c_library
 *		int	symeq()		lksym.c
 *
 *	side effects:
 *		Area and/or areax structures are created.
 *		Failure to allocate space for created structures
 *		will terminate the linker.
 */

VOID
lkparea(char *id)
{
	register struct area *tap;
	register struct areax *taxp;

	ap = areap;
	axp = (struct areax *) new (sizeof(struct areax));
	while (ap) {
		if (symeq(id, ap->a_id)) {
			taxp = ap->a_axp;
			while (taxp->a_axp)
				taxp = taxp->a_axp;
			taxp->a_axp = axp;
			axp->a_bap = ap;
			axp->a_bhp = hp;
			return;
		}
		ap = ap->a_ap;
	}
	ap = (struct area *) new (sizeof(struct area));
	if (areap == NULL) {
		areap = ap;
	} else {
		tap = areap;
		while (tap->a_ap)
			tap = tap->a_ap;
		tap->a_ap = ap;
	}
	ap->a_axp = axp;
	axp->a_bap = ap;
	axp->a_bhp = hp;
	strncpy(ap->a_id, id, NCPS);
        ap->a_addr = 0;
}

/*)Function	VOID	lnkarea()
 *
 *	The function lnkarea() resolves all area addresses.
 *	The function evaluates each area structure (and all
 *	the associated areax structures) in sequence.  The
 *	linking process supports four (4) possible area types:
 *
 *	ABS/OVR	-	All sections (each individual areax
 *			section) starts at the identical base
 *			area address overlaying all other
 *			areax sections for this area.  The
 *			size of the area is largest of the area
 *			sections.
 *
 *	ABS/CON -	All sections (each individual areax
 *			section) are concatenated with the
 *			first section starting at the base
 *			area address.  The size of the area
 *			is the sum of the section sizes.
 *
 *		NOTE:	Multiple absolute (ABS) areas are
 *			never concatenated with each other,
 *			thus absolute area A and absolute area
 *			B will overlay each other if they begin
 *			at the same location (the default is
 *			always address 0 for absolute areas).
 *
 *	REL/OVR	-	All sections (each individual areax
 *			section) starts at the identical base
 *			area address overlaying all other
 *			areax sections for this area.  The
 *			size of the area is largest of the area
 *			sections.
 *
 *	REL/CON -	All sections (each individual areax
 *			section) are concatenated with the
 *			first section starting at the base
 *			area address.  The size of the area
 *			is the sum of the section sizes.
 *
 *		NOTE:	Relocatable (REL) areas are always concatenated
 *			with each other, thus relocatable area B
 *			(defined after area A) will follow
 *			relocatable area A independent of the
 *			starting address of area A.  Within a
 *			specific area each areax section may be
 *			overlayed or concatenated with other
 *			areax sections.
 *
 *
 *	If a base address for an area is specified then the
 *	area will start at that address.  Any relocatable
 *	areas defined subsequently will be concatenated to the
 *	previous relocatable area if it does not have a base
 *	address specified.
 *
 *	The names s_<areaname> and l_<areaname> are created to
 *	define the starting address and length of each area.
 *
 *	local variables:
 *		Addr_T	rloc		;current relocation address
 *		char	temp[]		;temporary string
 *		struct symbol	*sp	;symbol structure
 *
 *	global variables:
 *		area	*ap		Pointer to the current
 *				 	area structure
 *		area	*areap		The pointer to the first
 *				 	area structure of a linked list
 *
 *	functions called:
 *		int	fprintf()	c_library
 *		VOID	lnksect()	lkarea.c
 *		symbol *lkpsym()	lksym.c
 *		char *	strncpy()	c_library
 *		int	symeq()		lksym.c
 *
 *	side effects:
 *		All area and areax addresses and sizes are
 *		determined and saved in their respective
 *		structures.
 */

unsigned long codemap[2048];
Addr_T lnksect(register struct area *tap);
/*
 * Resolve all area addresses.
 */
VOID
lnkarea()
{
        register Addr_T rloc = 0;
        Addr_T gs_size = 0;
	char temp[NCPS];
	struct sym *sp;
        struct area *abs_ap = NULL;
        struct area *gs0_ap = NULL;

        memset(codemap, 0, sizeof(codemap));

        /* first sort all absolute areas to the front */
        ap = areap;
        /* no need to check first area, it's in front anyway */
        while (ap && ap->a_ap)
        {
                if (ap->a_ap->a_flag & A_ABS)
                {/* next area is absolute, move it to front,
                    reversed sequence is no problem for absolutes */
                        abs_ap = ap->a_ap;
                        ap->a_ap = abs_ap->a_ap;
                        abs_ap->a_ap = areap;
                        areap = abs_ap;
                }
                else
                {
                        ap = ap->a_ap;
                }
        }

        /* next accumulate all GSINITx/GSFINAL area sizes
           into GSINIT so they stay together */
        ap = areap;
        while (ap)
        {
                if (!strncmp(ap->a_id, "GS", 2))
                {/* GSxxxxx area */
                        if (ap->a_size == 0)
                        {
                                axp = ap->a_axp;
                                while (axp)
                                {
                                        ap->a_size += axp->a_size;
                                        axp = axp->a_axp;
                                }
                        }
                        gs_size += ap->a_size;
                        if (!strcmp(ap->a_id, "GSINIT0"))
                        {/* GSINIT0 area */
                                gs0_ap = ap;
                        }
                }
                ap = ap->a_ap;
        }
        if (gs0_ap)
                gs0_ap->a_size = gs_size;

	ap = areap;
        while (ap)
        {
                if (ap->a_flag & A_ABS) {
			/*
			 * Absolute sections
			 */
			lnksect(ap);
		} else {
			/*
			 * Relocatable sections
			 */
			if (ap->a_addr == 0)
				ap->a_addr = rloc;
                        rloc = lnksect(ap);
		}

		/*
		 * Create symbols called:
		 *	s_<areaname>	the start address of the area
		 *	l_<areaname>	the length of the area
		 */

                if (! symeq(ap->a_id, _abs_))
                {
			strncpy(temp+2,ap->a_id,NCPS-2);
			*(temp+1) = '_';

			*temp = 's';
			sp = lkpsym(temp, 1);
			sp->s_addr = ap->a_addr;
			sp->s_axp = NULL;
			sp->s_type |= S_DEF;

			*temp = 'l';
			sp = lkpsym(temp, 1);
			sp->s_addr = ap->a_size;
			sp->s_axp = NULL;
			sp->s_type |= S_DEF;
		}
		ap = ap->a_ap;
	}
}

static
Addr_T find_empty_space(Addr_T start, Addr_T size, unsigned long *map)
{
    int i, j, k;
    unsigned long mask, b;

    while (1)
    {
        Addr_T a = start;
        i = start >> 5;
        j = (start + size) >> 5;
        mask = -(1 << (start & 0x1F));

        while (i < j)
        {
            if (map[i] & mask)
            {
                k = 32;
                for (b=0x80000000; b!=0; b>>=1, k--)
                {
                    if (map[i] & b)
                      break;
                }
                start = a + k;
                break;
            }
            i++;
            mask = 0xFFFFFFFF;
            a += 32;
        }
        if (start > a)
          continue;

        mask &= (1 << ((start + size) & 0x1F)) - 1;
        if (map[i] & mask)
        {
            k = 32;
            for (b=0x80000000; b!=0; b>>=1, k--)
            {
                if (map[i] & b)
                  break;
            }
            start = (a & ~0x1F) + k;
        }
        if (start <= a)
          break;
    }
    return start;
}

static
Addr_T allocate_space(Addr_T start, Addr_T size, char* id, unsigned long *map)
{
    int i, j;
    unsigned long mask;
    Addr_T a = start;
    i = start >> 5;
    j = (start + size) >> 5;
    mask = -(1 << (start & 0x1F));

    while (i < j)
    {
        if (map[i] & mask)
        {
            fprintf(stderr, "memory overlap near 0x%X for %s\n", a, id);
        }
        map[i++] |= mask;
        mask = 0xFFFFFFFF;
        a += 32;
    }
    mask &= (1 << ((start + size) & 0x1F)) - 1;
    if (map[i] & mask)
    {
        fprintf(stderr, "memory overlap near 0x%X for %s\n", a, id);
    }
    map[i] |= mask;
    return start;
}

/*)Function	VOID	lnksect()
 *
 *		area *	tap		pointer to an area structure
 *
 *	The function lnksect() is the function called by
 *	lnkarea() to resolve the areax addresses.  Refer
 *	to the function lnkarea() for more detail. Pageing
 *	boundary and length errors will be reported by this
 *	function.
 *
 *	local variables:
 *		Addr_T	size		size of area
 *		Addr_T	addr		address of area
 *		areax *	taxp		pointer to an areax structure
 *
 *	global variables:
 *		int	lkerr		error flag
 *
 *	functions called:
 *		none
 *
 *	side effects:
 *		All area and areax addresses and sizes area determined
 *		and linked into the structures.
 */

Addr_T lnksect(register struct area *tap)
{
	register Addr_T size, addr;
	register struct areax *taxp;

	size = 0;
	addr = tap->a_addr;
	if ((tap->a_flag&A_PAG) && (addr & 0xFF)) {
	    fprintf(stderr,
	    "\n?ASlink-Warning-Paged Area %.8s Boundary Error\n", tap->a_id);
	    lkerr++;
	}

	taxp = tap->a_axp;
        if (tap->a_flag & A_OVR) {
		/*
		 * Overlayed sections
		 */
		while (taxp) {
			taxp->a_addr = addr;
			if (taxp->a_size > size)
				size = taxp->a_size;
			taxp = taxp->a_axp;
		}
        }
        else if (tap->a_flag & A_ABS)
        {
                /*
                 * Absolute sections
                 */
                while (taxp)
                {
                        allocate_space(taxp->a_addr, taxp->a_size, tap->a_id, codemap);
                        taxp->a_addr = 0; /* reset to zero so relative addresses become absolute */
                        size += taxp->a_size;
                        taxp = taxp->a_axp;
                }
        }
        else
        {
		/*
		 * Concatenated sections
		 */
                if (tap->a_size) {
                        addr = find_empty_space(addr, tap->a_size, codemap);
                }
		while (taxp) {
                        //find next unused address now
                        if (taxp->a_size)
                        {
                                addr = find_empty_space(addr, taxp->a_size, codemap);
                                allocate_space(addr, taxp->a_size, tap->a_id, codemap);
                        }
			taxp->a_addr = addr;
			addr += taxp->a_size;
			size += taxp->a_size;
			taxp = taxp->a_axp;
		}
	}
	tap->a_size = size;

        if ((tap->a_flag & A_PAG) && (size > 256))
        {
	    fprintf(stderr,
	    "\n?ASlink-Warning-Paged Area %.8s Length Error\n", tap->a_id);
	    lkerr++;
	}
        return addr;
}
