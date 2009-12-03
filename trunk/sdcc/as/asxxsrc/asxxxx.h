/* asxxxx.h

   Copyright (C) 1989-1995 Alan R. Baldwin
   721 Berkeley St., Kent, Ohio 44240

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>. */

/*
 * 10-Nov-07 borutr:
 *	- change a_id from [NCPS] to pointer
 * 28-Oct-97 JLH:
 *	- add proto for strsto
 *	- change s_id from [NCPS] to pointer
 *	- change m_id from [NCPS] to pointer
 *	- change NCPS to 80
 *	- case sensitive
 *	- add R_J11 for 8051 assembler
 *	- add outr11 prototype for 8051 assembler
 *	- always define "ccase"
 *  2-Nov-97 JLH:
 *	- add jflag for debug control
 *	- prototypes for DefineNoICE_Line
 * 30-Jan-98 JLH:
 *	- add memory space flags to a_flag for 8051
 *
 *  3-Feb-00 KV:
 *	- add DS80C390 flat mode support.
 */

/*
 * System Include Files
 */

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>

#define VERSION "V02.00 + NoICE + SDCC mods + Flat24 Feb-1999"

#if !defined(__BORLANDC__) && !defined(_MSC_VER)
#include <unistd.h>
#endif

/*)Module	asxxxx.h
 *
 *	The module asxxxx.h contains the definitions for constants,
 *	structures, global variables, and ASxxxx functions
 *	contained in the ASxxxx.c files.  The two functions
 *	and three global variables from the machine dependent
 *	files are also defined.
 */

/*
 *	 compiler/operating system specific definitions
 */

/* DECUS C void definition */
/* File/extension seperator */

#ifdef	decus
#define	VOID	char
#define	FSEPX	'.'
#endif

/* PDOS C void definition */
/* File/extension seperator */

#ifdef	PDOS
#define	VOID	char
#define	FSEPX	':'
#endif

/* Default void definition */
/* File/extension seperator */

#ifndef	VOID
#define	VOID	void
#define	FSEPX	'.'
#define	OTHERSYSTEM
#endif

/*
 * PATH_MAX
 */
#include <limits.h>
#ifndef PATH_MAX				/* POSIX, but not required	*/
#if defined(_MSC_VER) || defined(__BORLANDC__)	/* Microsoft C or Borland C*/
#include <stdlib.h>
#define PATH_MAX	_MAX_PATH
#else
#define PATH_MAX				/* define a reasonable value */
#endif
#endif

#ifdef _WIN32		/* WIN32 native */

#  define NATIVE_WIN32		1
#  ifdef __MINGW32__	/* GCC MINGW32 depends on configure */
#    include "../../sdccconf.h"
#  else
#    include "../../sdcc_vc.h"
#    define PATH_MAX	_MAX_PATH
#  endif

#else			/* Assume Un*x style system */
#  include "../../sdccconf.h"
#endif

/*
 * Error definitions
 */
#define	ER_NONE		0	/* No error */
#define	ER_WARNING	1	/* Warning */
#define	ER_ERROR	2	/* Assembly error */
#define	ER_FATAL	3	/* Fatal error */

/*
 * Assembler definitions.
 */
#define LFTERM	'('		/* Left expression delimeter */
#define RTTERM	')'		/* Right expression delimeter */

#define NCPS	80		/* Characters per symbol */
#define ASXXXX_HUGE 1000	/* A huge number */
#define NERR	3		/* Errors per line */
#define NINPUT	1024	/* Input buffer size */
#define NCODE	128		/* Listing code buffer size */
#define NTITL	80		/* Title buffer size */
#define NSBTL	80		/* SubTitle buffer size */
#define NHASH	64		/* Buckets in hash table */
#define HMASK	077		/* Hash mask */
#define NLPP	60		/* Lines per page */
#define MAXFIL	6		/* Maximum command line input files */
#define MAXINC	6		/* Maximum nesting of include files */
#define MAXIF	10		/* Maximum nesting of if/else/endif */
#define FILSPC	256		/* Chars. in filespec */

#define NLIST	0		/* No listing */
#define SLIST	1		/* Source only */
#define ALIST	2		/* Address only */
#define BLIST	3		/* Address only with allocation */
#define CLIST	4		/* Code */
#define ELIST	5		/* Equate only */

#define dot	sym[0]		/* Dot, current loc */
#define dca	area[0]		/* Dca, default code area */


/* NB: for Flat24 extentions to work, a_uint must be at least 24
 * bits. This is checked at runtime when the .flat24 directive
 * is processed.
 */
typedef unsigned int a_uint;

/*
 *	The area structure contains the parameter values for a
 *	specific program or data section.  The area structure
 *	is a linked list of areas.  The initial default area
 *	is "_CODE" defined in asdata.c, the next area structure
 *	will be linked to this structure through the structure
 *	element 'struct area *a_ap'.  The structure contains the
 *	area name, area reference number ("_CODE" is 0) determined
 *	by the order of .area directives, area size determined
 *	from the total code and/or data in an area, area fuzz is
 *	a variable used to track pass to pass changes in the
 *	area size caused by variable length instruction formats,
 *	and area flags which specify the area's relocation type.
 */
struct	area
{
	struct	area *a_ap;	/* Area link */
	char *	a_id;		/* Area Name */
	int	a_ref;			/* Ref. number */
	a_uint	a_size;		/* Area size */
	a_uint	a_fuzz;		/* Area fuzz */
	int	a_flag;			/* Area flags */
/* sdas specific */
	a_uint	a_addr;		/* Area address */
/* ebd sdas specific */
};

/*
 *	The "A_" area constants define values used in
 *	generating the assembler area output data.
 *
 * Area flags
 *
 *	   7     6     5     4     3     2     1     0
 *	+-----+-----+-----+-----+-----+-----+-----+-----+
 *	| BIT |XDATA|DATA | PAG | ABS | OVR |     |     |
 *	+-----+-----+-----+-----+-----+-----+-----+-----+
 */

#define A_CON	0000		/* Concatenating */
#define A_OVR	0004		/* Overlaying */
#define A_REL	0000		/* Relocatable */
#define A_ABS	0010		/* absolute */
#define A_NOPAG 0000		/* Non-Paged */
#define A_PAG	0020		/* Paged */

/* sdas specific */
/* Additional flags for 8051 address spaces */
#define A_DATA	0000		/* data space (default)*/
#define A_CODE	0040		/* code space */
#define A_XDATA 0100		/* external data space */
#define A_BIT	0200		/* bit addressable space */

#define A_NOLOAD  0400		/* nonloadable */
#define A_LOAD	0000		/* loadable (default) */
/* end sdas specific */

/*
 *	The "R_" relocation constants define values used in
 *	generating the assembler relocation output data for
 *	areas, symbols, and code.
 *
 * Relocation flags
 *
 *	   7	 6     5     4	   3	 2     1     0
 *	+-----+-----+-----+-----+-----+-----+-----+-----+
 *	| MSB | PAGn| PAG0| USGN| BYT2| PCR | SYM | BYT |
 *	+-----+-----+-----+-----+-----+-----+-----+-----+
 */

#define R_WORD	0x00		/* 16 bit */
#define R_BYTE	0x01		/*  8 bit */

#define R_AREA	0x00		/* Base type */
#define R_SYM	0x02

#define R_NORM	0x00		/* PC adjust */
#define R_PCR	0x04

#define R_BYT1	0x00		/* Byte count for R_BYTE = 1 */
#define R_BYT2	0x08		/* Byte count for R_BYTE = 2 */

#define R_SGND	0x00		/* Signed Byte */
#define R_USGN	0x10		/* Unsigned Byte */

#define R_NOPAG 0x00		/* Page Mode */
#define R_PAG0	0x20		/* Page '0' */
#define R_PAG	0x40		/* Page 'nnn' */

#define R_LSB	0x00		/* low byte */
#define R_MSB	0x80		/* high byte */

#define R_BYT3	0x100		/* if R_BYTE is set, this is a
							 * 3 byte address, of which
							 * the linker must select one byte.
							 */
#define R_HIB	0x200			/* If R_BYTE & R_BYT3 are set, linker
					 * will select byte 3 of the relocated
					 * 24 bit address.
					 */

#define R_BIT	0x400		/* Linker will convert from byte-addressable
							 * space to bit-addressable space.
							 */

#define R_J11	(R_WORD|R_BYT2)	/* JLH: 11 bit JMP and CALL (8051) */
#define R_J19	(R_WORD|R_BYT2|R_MSB)	/* 19 bit JMP/CALL (DS80C390)	   */
#define R_C24	(R_WORD|R_BYT1|R_MSB)	/* 24 bit address (DS80C390)	   */
#define R_J19_MASK	(R_BYTE|R_BYT2|R_MSB)

#define IS_R_J19(x)	(((x) & R_J19_MASK) == R_J19)
#define IS_R_J11(x)	(((x) & R_J19_MASK) == R_J11)
#define IS_C24(x)	(((x) & R_J19_MASK) == R_C24)

#define R_ESCAPE_MASK	0xf0	/* Used to escape relocation modes
								 * greater than 0xff in the .rel
								 * file.
								 */

/*
 * Listing Control Flags
 */

#define R_HIGH	0040000		/* High Byte */
#define R_RELOC 0100000		/* Relocation */

#define R_DEF	00		/* Global def. */
#define R_REF	01		/* Global ref. */
#define R_REL	00		/* Relocatable */
#define R_ABS	02		/* Absolute */
#define R_GBL	00		/* Global */
#define R_LCL	04		/* Local */

/*
 *	The mne structure is a linked list of the assembler
 *	mnemonics and directives.  The list of mnemonics and
 *	directives contained in the device dependent file
 *	xxxpst.c are hashed and linked into NHASH lists in
 *	module assym.c by syminit().  The structure contains
 *	the mnemonic/directive name, a subtype which directs
 *	the evaluation of this mnemonic/directive, a flag which
 *	is used to detect the end of the mnemonic/directive
 *	list in xxxpst.c, and a value which is normally
 *	associated with the assembler mnemonic base instruction
 *	value.
 */
struct	mne
{
	struct	mne *m_mp;	/* Hash link */
	char	*m_id;		/* Mnemonic (JLH) */
	char	m_type;		/* Mnemonic subtype */
	char	m_flag;		/* Mnemonic flags */
	a_uint	m_valu;		/* Value */
};

/*
 *	The sym structure is a linked list of symbols defined
 *	in the assembler source files.	The first symbol is "."
 *	defined in asdata.c.  The entry 'struct tsym *s_tsym'
 *	links any temporary symbols following this symbol and
 *	preceeding the next normal symbol.  The structure also
 *	contains the symbol's name, type (USER or NEW), flag
 *	(global, assigned, and multiply defined), a pointer
 *	to the area structure defining where the symbol is
 *	located, a reference number assigned by outgsd() in
 *	asout.c, and the symbols address relative to the base
 *	address of the area where the symbol is located.
 */
struct	sym
{
	struct	sym  *s_sp;	/* Hash link */
	struct	tsym *s_tsym;	/* Temporary symbol link */
	char	*s_id;		/* Symbol (JLH) */
	char	s_type;		/* Symbol subtype */
	char	s_flag;		/* Symbol flags */
	struct	area *s_area;	/* Area line, 0 if absolute */
	int	s_ref;			/* Ref. number */
	a_uint	s_addr;		/* Address */
/* sdas specific */
	a_uint	s_org;		/* Start Address if absolute */
/* end sdas specific */
};

#define S_GBL		01	/* Global */
#define S_ASG		02	/* Assigned */
#define S_MDF		04	/* Mult. def */
#define	S_END		010	/* End mark for ___pst files */

#define S_NEW		0	/* New name */
#define S_USER		1	/* User name */
				/* unused slot */
				/* unused slot */
				/* unused slot */

#define S_BYTE		5	/* .byte */
#define S_WORD		6	/* .word */
#define S_ASCII		7	/* .ascii */
#define S_ASCIZ		8	/* .asciz */
#define S_BLK		9	/* .blkb or .blkw */
#define S_INCL		10	/* .include */
#define S_DAREA		11	/* .area */
#define S_ATYP		12	/* .area type */
#define S_AREA		13	/* .area name */
#define S_GLOBL		14	/* .globl */
#define S_PAGE		15	/* .page */
#define S_TITLE		16	/* .title */
#define S_SBTL		17	/* .sbttl */
#define S_IF		18	/* .if */
#define S_ELSE		19	/* .else */
#define S_ENDIF		20	/* .endif */
#define S_EVEN		21	/* .even */
#define S_ODD		22	/* .odd */
#define S_RADIX		23	/* .radix */
#define S_ORG		24	/* .org */
#define S_MODUL		25	/* .module */
#define S_ASCIS		26	/* .ascis */
/* sdas specific */
#define S_FLAT24	27	/* .flat24 */
#define S_FLOAT		28	/* .df */
#define S_ULEB128	29	/* .uleb128 */
#define S_SLEB128	30	/* .sleb128 */
#define S_OPTSDCC	31	/* .optsdcc */
/* end sdas specific */

/*
 *	The tsym structure is a linked list of temporary
 *	symbols defined in the assembler source files following
 *	a normal symbol.  The structure contains the temporary
 *	symbols number, a flag (multiply defined), a pointer to the
 *	area structure defining where the temporary structure
 *	is located, and the temporary symbol's address relative
 *	to the base address of the area where the symbol
 *	is located.
 */
struct	tsym
{
	struct	tsym *t_lnk;	/* Link to next */
/* sandeep changed to 'int' from 'char' */
/* this will increase the number temp symbols
   that can be defined from 255 to INT_MAX */
	int t_num;		/* 0-INT_MAX$ */
	int t_flg;		/* flags */

	struct	area *t_area;	/* Area */
	a_uint	t_addr;		/* Address */
};

/*
 *	External Definitions for all Global Variables
 */

extern	int	aserr;		/*	ASxxxx error counter
				 */
extern	jmp_buf jump_env;	/*	compiler dependent structure
				 *	used by setjmp() and longjmp()
				 */
extern	int	inpfil;		/*	count of assembler
				 *	input files specified
				 */
extern	int	incfil;		/*	current file handle index
				 *	for include files
				 */
extern	int	cfile;		/*	current file handle index
				 *	of input assembly files
				 */
extern	int	flevel;		/*	IF-ELSE-ENDIF flag will be non
				 *	zero for false conditional case
				 */
extern	int	tlevel;		/*	current conditional level
				 */
extern	int	ifcnd[MAXIF+1];	/*	array of IF statement condition
				 *	values (0 = FALSE) indexed by tlevel
				 */
extern	int	iflvl[MAXIF+1];	/*	array of IF-ELSE-ENDIF flevel
				 *	values indexed by tlevel
				 */
extern	char
	afn[FILSPC];		/*	afile() temporary filespec
				 */
extern	char
	srcfn[MAXFIL][FILSPC];	/*	array of source file names
				 */
extern	int
	srcline[MAXFIL];	/*	current source file line
				 */
extern	char
	incfn[MAXINC][FILSPC];	/*	array of include file names
				 */
extern	int
	incline[MAXINC];	/*	current include file line
				 */
extern	int	radix;		/*	current number conversion radix:
				 *	2 (binary), 8 (octal), 10 (decimal),
				 *	16 (hexadecimal)
				 */
extern	int	line;		/*	current assembler source
				 *	line number
				 */
extern	int	page;		/*	current page number
				 */
extern	int	lop;		/*	current line number on page
				 */
extern	int	pass;		/*	assembler pass number
				 */
extern	int	lflag;		/*	-l, generate listing flag
				 */
extern	int	gflag;		/*	-g, make undefined symbols global flag
				 */
extern	int	aflag;		/*	-a, make all symbols global flag
				 */
extern	int	oflag;		/*	-o, generate relocatable output flag
				 */
extern	int	sflag;		/*	-s, generate symbol table flag
				 */
extern	int	pflag;		/*	-p, enable listing pagination
				 */
extern	int	wflag;		/*	-w, enable wide format listing
				 */
extern	int	zflag;		/*	-z, enable symbol case sensitivity
				 */
extern	int	xflag;		/*	-x, listing radix flag
				 */
extern	int	fflag;		/*	-f(f), relocations flagged flag
				 */
extern	a_uint	laddr;		/*	address of current assembler line
				 *	or value of .if argument
				 */
extern	a_uint	fuzz;		/*	tracks pass to pass changes in the
				 *	address of symbols caused by
				 *	variable length instruction formats
				 */
extern	int	lmode;		/*	listing mode
				 */
extern	struct	area	area[];	/*	array of 1 area
				 */
extern	struct	area *areap;	/*	pointer to an area structure
				 */
extern	struct	sym	sym[];	/*	array of 1 symbol
				 */
extern	struct	sym *symp;	/*	pointer to a symbol structure
				 */
extern	struct	sym *symhash[NHASH]; /*	array of pointers to NHASH
				      *	linked symbol lists
				      */
extern	struct	mne *mnehash[NHASH]; /*	array of pointers to NHASH
				      *	linked mnemonic/directive lists
				      */
extern	char	*ep;		/*	pointer into error list
				 *	array eb[NERR]
				 */
extern	char	eb[NERR];	/*	array of generated error codes
				 */
extern	const char *ip;	/*	pointer into the assembler-source
				 *	text line in ib[]
				 */
extern	const char *ib;	/*	assembler-source text line
				 */
extern	char	*cp;		/*	pointer to assembler output
				 *	array cb[]
				 */
extern	char	cb[NCODE];	/*	array of assembler output values
				 */
extern	int	*cpt;		/*	pointer to assembler relocation type
				 *	output array cbt[]
				 */
extern	int	cbt[NCODE];	/*	array of assembler relocation types
				 *	describing the data in cb[]
				 */
extern	char	tb[NTITL];	/*	Title string buffer
				 */
extern	char	stb[NSBTL];	/*	Subtitle string buffer
				 */
extern	char	symtbl[];	/*	string "Symbol Table"
				 */
extern	char	aretbl[];	/*	string "Area Table"
				 */
extern	char	module[NCPS];	/*	module name string
				 */
extern	FILE	*lfp;		/*	list output file handle
				 */
extern	FILE	*ofp;		/*	relocation output file handle
				 */
extern	FILE	*tfp;		/*	symbol table output file handle
				 */
extern	FILE	*sfp[MAXFIL];	/*	array of assembler-source file handles
				 */
extern	FILE	*ifp[MAXINC];	/*	array of include-file file handles
				 */
extern	unsigned char	ctype[128];	/*	array of character types, one per
				 *	ASCII character
				 */
extern	char	ccase[128];	/* an array of characters which
				 *	perform the case translation function
				 */
/*sdas specific */
extern	int	asfatal;	/*	ASxxxx fatal error counter
				 */
extern	int	org_cnt;	/*	.org directive counter
				 */
extern	int	cflag;		/*	-c, generate sdcdb debug information
				 */
extern	int	jflag;		/*	-j, generate debug information flag
				 */
extern	char	optsdcc[NINPUT];	/*	sdcc compile options
				 */
extern	int	flat24Mode;	/*	non-zero if we are using DS390 24 bit
				 *	flat mode (via .flat24 directive).
				 */
/*end sdas specific */

/*
 * Definitions for Character Types
 */
#define SPACE	0000
#define ETC	0000
#define LETTER	0001
#define DIGIT	0002
#define BINOP	0004
#define RAD2	0010
#define RAD8	0020
#define RAD10	0040
#define RAD16	0100
#define ILL	0200

#define DGT2	DIGIT|RAD16|RAD10|RAD8|RAD2
#define DGT8	DIGIT|RAD16|RAD10|RAD8
#define DGT10	DIGIT|RAD16|RAD10
#define LTR16	LETTER|RAD16

/*
 *	The exp structure is used to return the evaluation
 *	of an expression.  The structure supports three valid
 *	cases:
 *	(1)	The expression evaluates to a constant,
 *		mode = S_USER, flag = 0, addr contains the
 *		constant, and base = NULL.
 *	(2)	The expression evaluates to a defined symbol
 *		plus or minus a constant, mode = S_USER,
 *		flag = 0, addr contains the constant, and
 *		base = pointer to area symbol.
 *	(3)	The expression evaluates to a external
 *		global symbol plus or minus a constant,
 *		mode = S_NEW, flag = 1, addr contains the
 *		constant, and base = pointer to symbol.
 */
struct	expr
{
	char	e_mode;		/* Address mode */
	char	e_flag;		/* Symbol flag */
	a_uint	e_addr;		/* Address */
	union	{
		struct area *e_ap;
		struct sym  *e_sp;
	} e_base;		/* Rel. base */
	int	e_rlcf;		/* Rel. flags */
};

/* C Library functions */
/* for reference only
extern	VOID		exit();
extern	int		fclose();
extern	char *		fgets();
extern	FILE *		fopen();
extern	int		fprintf();
extern	VOID		longjmp();
extern	VOID *		malloc();
extern	int		printf();
extern	char		putc();
extern	int		rewind();
extern	int		setjmp();
extern	int		strcmp();
extern	char *		strcpy();
extern	int		strlen();
extern	char *		strncpy();
*/

/* Machine independent functions */

#ifdef	OTHERSYSTEM
/* aslex.c */
extern	int		comma(int flag);

/* assym.c */
extern	char *		strsto(char *str);

/* assubr.c */
/* sdas specific */
extern	VOID		warnBanner(void);
/* end sdas specific */

/* asout.c */
extern	VOID		outrw(struct expr *, int);
/* sdas specific */
extern	int		byte3(int);
extern	VOID		outr24(struct expr *, int);
extern	VOID		out_l24(int, int);
extern	VOID		out_t24(int);
extern	VOID		outr19(struct expr *, int, int);
extern	VOID		outdp(struct area *, struct expr *);

/* asnoice.c */
extern void DefineNoICE_Line(void);
extern void DefineCDB_Line(void);
/* end sdas specific */

/* Machine dependent functions */

extern	VOID		minit(void);
extern	VOID		machine(struct mne *);

/* sdas specific */
/* strcmpi.c */
extern	int as_strcmpi(const char *s1, const char *s2);
extern	int as_strncmpi(const char *s1, const char *s2, size_t n);
/* end sdas specific */
#else
/* aslex.c */
extern	int		comma();

/* assym.c */
extern	char *		strsto();

/* assubr.c */
/* sdas specific */
extern	VOID		warnBanner();
/* end sdas specific */

/* asout.c */
extern	VOID		outrw();
/* sdas specific */
extern	int		byte3();
extern	VOID		outr24();
extern	VOID		out_l24();
extern	VOID		out_t24();
extern	VOID		outr19(t);
extern	VOID		outdp();

/* asnoice.c */
extern void DefineNoICE_Line();
extern void DefineCDB_Line();
/* end sdas specific */

/* Machine dependent functions */

extern	VOID		minit();
extern	VOID		machine();

/* sdas specific */
/* strcmpi.c */
extern	int as_strcmpi();
extern	int as_strncmpi();
/* end sdas specific */
#endif

/* asmain.c */
extern	FILE *		afile();
extern	VOID		asexit();
extern	VOID		asmbl();
extern	int		main();
extern	VOID		newdot();
extern	VOID		phase();
extern	VOID		usage();

/* aslex.c */
extern	char		endline();
extern	char		get();
extern	VOID		getid();
extern	int		as_getline();
extern	int		getmap();
extern	char		getnb();
extern	VOID		getst();
extern	int		more();
extern	VOID		unget();
extern	VOID		chop_crlf();

/* assym.c */
extern	struct	area *	alookup();
extern	struct	mne *	mlookup();
extern	int		hash();
extern	struct	sym *	lookup();
extern	VOID *		new();
extern	int		symeq();
extern	VOID		syminit();
extern	VOID		symglob();
extern	VOID		allglob();

/* assubr.c */
extern	VOID		aerr();
extern	VOID		diag();
extern	VOID		err();
extern	char *		geterr();
extern	VOID		qerr();
extern	VOID		rerr();

/* asexpr.c */
extern	VOID		abscheck();
extern	a_uint		absexpr();
extern	VOID		clrexpr();
extern	int		digit();
extern	int		is_abs();
extern	VOID		expr();
extern	int		oprio();
extern	VOID		term();

/* aslist.c */
extern	VOID		list();
extern	VOID		list1();
extern	VOID		list2();
extern	VOID		lstsym();
extern	VOID		slew();

/* asout.c */
extern	int		hibyte();
extern	int		lobyte();
extern	VOID		out();
extern	VOID		outab();
extern	VOID		outarea();
extern	VOID		outaw();
extern	VOID		outall();
extern	VOID		outdot();
extern	VOID		outbuf();
extern	VOID		outchk();
extern	VOID		outgsd();
extern	VOID		outrb();
extern	VOID		outr11();	/* JLH */
extern	VOID		outsym();
extern	VOID		out_lb();
extern	VOID		out_lw();
extern	VOID		out_rw();
extern	VOID		out_tw();

/* Machine dependent variables */

extern	char *		cpu;
extern	char *		dsft;
extern	int		hilo;
extern	struct	mne	mne[];
