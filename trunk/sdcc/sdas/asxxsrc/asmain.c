/* asmain.c

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
 * 29-Oct-97 JLH pass ";!" comments to output file
 */

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <math.h>
#include "sdas.h"
#include "asxxxx.h"

/*)Module       asmain.c
 *
 *      The module asmain.c includes the command argument parser,
 *      the three pass sequencer, and the machine independent
 *      assembler parsing code.
 *
 *      asmain.c contains the following functions:
 *              int             main(argc, argv)
 *              VOID    asexit(n)
 *              VOID    asmbl()
 *              FILE *  afile(fn, ft, wf)
 *              VOID    newdot(nap)
 *              VOID    phase(ap, a)
 *              VOID    usage()
 *
 *      asmain.c contains the array char *usetxt[] which
 *      references the usage text strings printed by usage().
 */

/* sdas specific */
static const char *search_path[100];
static int search_path_length;

/**
 * The search_path_append is used to append another directory to the end
 * of the include file search path.
 *
 * @param dir
 *              The directory to be added to the path.
 */
void
search_path_append(const char *dir)
{
        if (search_path_length < sizeof(search_path)/sizeof(char*))
        {
                search_path[search_path_length++] = dir;
        }
}

/**
 * The search_path_fopen function is used to open the named file.  If
 * the file isn't in the current directory, the search path is then used
 * to build a series of possible file names, and attempts to open them.
 * The first found is used.
 *
 * @param filename
 *              The name of the file to be opened.
 * @param mode
 *              The mode of the file to be opened.
 * @returns
 *              what the fopen function would return on success, or NULL if the
 *              file is not anywhere in the search path.
 */
static FILE *
search_path_fopen(const char *filename, const char *mode)
{
        FILE *fp;
        int j;

        fp = fopen(filename, mode);
        if (fp != NULL || filename[0] == '/' || filename[0] == '\\')
                return fp;
        for (j = 0; j < search_path_length; ++j) {
                char path[2000];

                strncpy(path, search_path[j], sizeof(path));
                if ((path[strlen(path) - 1] != '/') &&
                        (path[strlen(path) - 1] != DIR_SEPARATOR_CHAR)) {
                        strncat(path, DIR_SEPARATOR_STRING, sizeof(path) - strlen(path));
                }
                strncat(path, filename, sizeof(path) - strlen(path));
                fp = fopen(path, mode);
                if (fp != NULL)
                        return fp;
        }
        errno = ENOENT;
        return NULL;
}
/* end sdas specific */

/*)Function     int     main(argc, argv)
 *
 *              int     argc            argument count
 *              char *  argv            array of pointers to argument strings
 *
 *      The function main() is the entry point to the assembler.
 *      The purpose of main() is to (1) parse the command line
 *      arguments for options and source file specifications and
 *      (2) to process the source files through the 3 pass assembler.
 *      Before each assembler pass various variables are initialized
 *      and source files are rewound to their beginning.  During each
 *      assembler pass each assembler-source text line is processed.
 *      After each assembler pass the assembler information is flushed
 *      to any opened output files and the if-else-endif processing
 *      is checked for proper termination.
 *
 *      The function main() is also responsible for opening all
 *      output files (REL, LST, and SYM), sequencing the global (-g)
 *      and all-global (-a) variable definitions, and dumping the
 *      REL file header information.
 *
 *      local variables:
 *              char *  p               pointer to argument string
 *              int     c               character from argument string
 *              int     i               argument loop counter
 *              area *  ap              pointer to area structure
 *
 *      global variables:
 *              int     aflag           -a, make all symbols global flag
 *              char    afn[]           afile() constructed filespec
 *              area *  areap           pointer to an area structure
 *              int     aserr           assembler error counter
 *              int     cb[]            array of assembler output values
 *              int     cbt[]           array of assembler relocation types
 *                                      describing the data in cb[]
 *              int     cfile           current file handle index
 *                                      of input assembly files
 *              int *   cp              pointer to assembler output array cb[]
 *              int *   cpt             pointer to assembler relocation type
 *                                      output array cbt[]
 *              char    eb[]            array of generated error codes
 *              char *  ep              pointer into error list array eb[]
 *              int     fflag           -f(f), relocations flagged flag
 *              int     flevel          IF-ELSE-ENDIF flag will be non
 *                                      zero for false conditional case
 *              a_uint  fuzz            tracks pass to pass changes in the
 *                                      address of symbols caused by
 *                                      variable length instruction formats
 *              int     gflag           -g, make undefined symbols global flag
 *              char    ib[]            assembler-source text line
 *              int     inpfil          count of assembler
 *                                      input files specified
 *              int     ifcnd[]         array of IF statement condition
 *                                      values (0 = FALSE) indexed by tlevel
 *              int     iflvl[]         array of IF-ELSE-ENDIF flevel
 *                                      values indexed by tlevel
 *              int     incfil          current file handle index
 *                                      for include files
 *              char *  ip              pointer into the assembler-source
 *                                      text line in ib[]
 *              jmp_buf jump_env        compiler dependent structure
 *                                      used by setjmp() and longjmp()
 *              int     lflag           -l, generate listing flag
 *              int     line            current assembler source
 *                                      line number
 *              int     lop             current line number on page
 *              int     oflag           -o, generate relocatable output flag
 * sdas specific
 *              int jflag               -j, generate debug info flag
 * end sdas specific
 *              int     page            current page number
 *              int     pflag           enable listing pagination
 *              int     pass            assembler pass number
 *              int     radix           current number conversion radix:
 *                                      2 (binary), 8 (octal), 10 (decimal),
 *                                      16 (hexadecimal)
 *              int     sflag           -s, generate symbol table flag
 *              char    srcfn[][]       array of source file names
 *              int     srcline[]       current source file line
 *              char    stb[]           Subtitle string buffer
 *              sym *   symp            pointer to a symbol structure
 *              int     tlevel          current conditional level
 *              int     wflag           -w, enable wide listing format
 *              int     xflag           -x, listing radix flag
 *              int     zflag           -z, enable symbol case sensitivity
 *              FILE *  lfp             list output file handle
 *              FILE *  ofp             relocation output file handle
 *              FILE *  tfp             symbol table output file handle
 *              FILE *  sfp[]           array of assembler-source file handles
 *
 *      called functions:
 *              FILE *  afile()         asmain.c
 *              VOID    allglob()       assym.c
 *              VOID    asexit()        asmain.c
 *              VOID    diag()          assubr.c
 *              VOID    err()           assubr.c
 *              int     fprintf()       c-library
 *              int     as_getline()    aslex.c
 *              VOID    list()          aslist.c
 *              VOID    lstsym()        aslist.c
 *              VOID    minit()         ___mch.c
 *              VOID    newdot()        asmain.c
 *              VOID    outbuf()        asout.c
 *              VOID    outchk()        asout.c
 *              VOID    outgsd()        asout.c
 *              int     rewind()        c-library
 *              int     setjmp()        c-library
 *              VOID    symglob()       assym.c
 *              VOID    syminit()       assym.c
 *              VOID    usage()         asmain.c
 *
 *      side effects:
 *              Completion of main() completes the assembly process.
 *              REL, LST, and/or SYM files may be generated.
 */

/* sdas specific */
char relFile[128];
/* end sdas specific */

int
main(argc, argv)
int argc;
char *argv[];
{
        char *p, *q = NULL;
        int c, i;
        struct area *ap;

        /* sdas specific */
        /* sdas initialization */
        sdas_init(argv[0]);
        /* end sdas specific */

        if (!is_sdas())
                fprintf(stdout, "\n");
        inpfil = -1;
        pflag = 1;
        for (i=1; i<argc; ++i) {
                p = argv[i];
                if (*p == '-') {
                        if (inpfil >= 0)
                                usage(ER_FATAL);
                        ++p;
                        while ((c = *p++) != 0)
                                switch(c) {

                                case 'a':
                                case 'A':
                                        ++aflag;
                                        break;

                                case 'c':
                                case 'C':
                                        ++cflag;
                                        break;

                                case 'g':
                                case 'G':
                                        ++gflag;
                                        break;

                                case 'i':
                                case 'I':
                                        search_path_append(p);
                                        while (*p)
                                                ++p;
                                        break;

                                case 'j':                       /* JLH: debug info */
                                case 'J':
                                        ++jflag;
                                        ++oflag;                /* force object */
                                        break;

                                case 'l':
                                case 'L':
                                        ++lflag;
                                        break;

                                case 'o':
                                case 'O':
                                        ++oflag;
                                        break;

                                case 's':
                                case 'S':
                                        ++sflag;
                                        break;

                                case 'p':
                                case 'P':
                                        pflag = 0;
                                        break;

                                case 'w':
                                case 'W':
                                        ++wflag;
                                        break;

                                case 'z':
                                case 'Z':
                                        ++zflag;
                                        break;

                                case 'x':
                                case 'X':
                                        xflag = 0;
                                        break;

                                case 'q':
                                case 'Q':
                                        xflag = 1;
                                        break;

                                case 'd':
                                case 'D':
                                        xflag = 2;
                                        break;

                                case 'f':
                                case 'F':
                                        ++fflag;
                                        break;

                                default:
                                        usage(ER_FATAL);
                                }
                } else {
                        if (++inpfil == MAXFIL) {
                                fprintf(stderr, "too many input files\n");
                                asexit(ER_FATAL);
                        }
                        if (inpfil == 0) {
                                q = p;
                                if (++i < argc) {
                                        p = argv[i];
                                        if (*p == '-')
                                                usage(ER_FATAL);
                                }
                        }
                        sfp[inpfil] = afile(p, "", 0);
                        strcpy(srcfn[inpfil],afn);
                }
        }
        if (inpfil < 0)
                usage(ER_WARNING);
        if (lflag)
                lfp = afile(q, "lst", 1);
        if (oflag) {
                ofp = afile(q, is_sdas() ? "" : "rel", 1);
                /* sdas specific */
                // save the file name if we have to delete it on error
                strcpy(relFile,afn);
                /* end sdas specific */
        }
        if (sflag)
                tfp = afile(q, "sym", 1);
        syminit();
        for (pass=0; pass<3; ++pass) {
                aserr = 0;
                if (gflag && pass == 1)
                        symglob();
                if (aflag && pass == 1)
                        allglob();
                if (oflag && pass == 2)
                        outgsd();
                flevel = 0;
                tlevel = 0;
                ifcnd[0] = 0;
                iflvl[0] = 0;
                radix = 10;
                srcline[0] = 0;
                page = 0;
                /* sdas specific */
                org_cnt = 0;
                /* end sdas specific */
                stb[0] = 0;
                lop  = NLPP;
                cfile = 0;
                incfil = -1;
                for (i = 0; i <= inpfil; i++)
                        rewind(sfp[i]);
                ap = areap;
                while (ap) {
                        ap->a_fuzz = 0;
                        ap->a_size = 0;
                        ap = ap->a_ap;
                }
                fuzz = 0;
                dot.s_addr = 0;
                dot.s_area = &dca;
                outbuf("I");
                outchk(0,0);
                symp = &dot;
                minit();
                while (as_getline()) {
                        cp = cb;
                        cpt = cbt;
                        ep = eb;
                        ip = ib;

                        /* sdas specific */
                        /* JLH: if line begins with ";!", then
                         * pass this comment on to the output file
                         */
                        if (oflag && (pass == 1) &&
                            (ip[0] == ';') && (ip[1] == '!'))
                        {
                                fprintf(ofp, "%s\n", ip );
                        }
                        /* end sdas specific */

                        if (setjmp(jump_env) == 0)
                                asmbl();
                        if (pass == 2) {
                                diag();
                                list();
                        }
                }
                newdot(dot.s_area); /* Flush area info */
                if (flevel || tlevel)
                        err('i');
        }
        if (oflag)
                outchk(ASXXXX_HUGE, ASXXXX_HUGE);  /* Flush */
        if (sflag) {
                lstsym(tfp);
        } else
        if (lflag) {
                lstsym(lfp);
        }
        asexit(aserr ? ER_ERROR : ER_NONE);
        return(0);
}

/*)Function     VOID    asexit(i)
 *
 *                      int     i       exit code
 *
 *      The function asexit() explicitly closes all open
 *      files and then terminates the program.
 *
 *      local variables:
 *              int     j               loop counter
 *
 *      global variables:
 *              FILE *  ifp[]           array of include-file file handles
 *              FILE *  lfp             list output file handle
 *              FILE *  ofp             relocation output file handle
 *              FILE *  tfp             symbol table output file handle
 *              FILE *  sfp[]           array of assembler-source file handles
 *
 *      functions called:
 *              int     fclose()        c-library
 *              VOID    exit()          c-library
 *
 *      side effects:
 *              All files closed. Program terminates.
 */

VOID
asexit(i)
int i;
{
        int j;

        if (lfp != NULL) fclose(lfp);
        if (ofp != NULL) fclose(ofp);
        if (tfp != NULL) fclose(tfp);

        for (j=0; j<MAXFIL && sfp[j] != NULL; j++) {
                fclose(sfp[j]);
        }

        for (j=0; j<MAXINC && ifp[j] != NULL; j++) {
                fclose(ifp[j]);
        }
        /* sdas specific */
        if (i) {
                /* remove output file */
                printf ("removing %s\n", relFile);
                remove(relFile);
        }
        /* end sdas specific */
        exit(i);
}

/*)Function     VOID    asmbl()
 *
 *      The function asmbl() scans the assembler-source text for
 *      (1) labels, global labels, equates, global main
equates, and local
 *      symbols, (2) .if, .else, .endif, and .page directives,
 *      (3) machine independent assembler directives, and (4) machine
 *      dependent mnemonics.
 *
 *      local variables:
 *              mne *   mp              pointer to a mne structure
 *              sym *   sp              pointer to a sym structure
 *              tsym *  tp              pointer to a tsym structure
 *              int     c               character from assembler-source
 *                                      text line
 *              area *  ap              pointer to an area structure
 *              expr    e1              expression structure
 *              char    id[]            id string
 *              char    opt[]           options string
 *              char    fn[]            filename string
 *              char *  p               pointer into a string
 *              int     d               temporary value
 *              int     n               temporary value
 *              int     uaf             user area options flag
 *              int     uf              area options
 *
 *      global variables:
 *              area *  areap           pointer to an area structure
 *              char    ctype[]         array of character types, one per
 *                                      ASCII character
 *              int     flevel          IF-ELSE-ENDIF flag will be non
 *                                      zero for false conditional case
 *              a_uint  fuzz            tracks pass to pass changes in the
 *                                      address of symbols caused by
 *                                      variable length instruction formats
 *              int     ifcnd[]         array of IF statement condition
 *                                      values (0 = FALSE) indexed by tlevel
 *              int     iflvl[]         array of IF-ELSE-ENDIF flevel
 *                                      values indexed by tlevel
 *              FILE *  ifp[]           array of include-file file handles
 *              char    incfn[][]       array of include file names
 *              int     incline[]       current include file line
 *              int     incfil          current file handle index
 *                                      for include files
 *              a_uint  laddr           address of current assembler line
 *                                      or value of .if argument
 *              int     lmode           listing mode
 *              int     lop             current line number on page
 *              char    module[]        module name string
 *              int     pass            assembler pass number
 *              int     radix           current number conversion radix:
 *                                      2 (binary), 8 (octal), 10 (decimal),
 *                                      16 (hexadecimal)
 *              char    stb[]           Subtitle string buffer
 *              sym *   symp            pointer to a symbol structure
 *              char    tb[]            Title string buffer
 *              int     tlevel          current conditional level
 *
 *      functions called:
 *              a_uint  absexpr()       asexpr.c
 *              area *  alookup()       assym.c
 *              VOID    clrexpr()       asexpr.c
 *              int     digit()         asexpr.c
 *              char    endline()       aslex.c
 *              VOID    err()           assubr.c
 *              VOID    expr()          asexpr.c
 *              FILE *  fopen()         c-library
 *              int     get()           aslex.c
 *              VOID    getid()         aslex.c
 *              int     getmap()        aslex.c
 *              int     getnb()         aslex.c
 *              VOID    getst()         aslex.c
 *              sym *   lookup()        assym.c
 *              VOID    machin()        ___mch.c
 *              mne *   mlookup()       assym.c
 *              int     more()          aslex.c
 *              VOID *  new()           assym.c
 *              VOID    newdot()        asmain.c
 *              VOID    outall()        asout.c
 *              VOID    outab()         asout.c
 *              VOID    outchk()        asout.c
 *              VOID    outrb()         asout.c
 *              VOID    outrw()         asout.c
 *              VOID    phase()         asmain.c
 *              VOID    qerr()          assubr.c
 *              char *  strcpy()        c-library
 *              char *  strncpy()       c-library
 *              char *  strsto()        assym.c
 *              VOID    unget()         aslex.c
 */

VOID
asmbl()
{
        struct mne *mp;
        struct sym *sp;
        struct tsym *tp;
        int c;
        struct area  *ap;
        struct expr e1;
        char id[NCPS];
        char opt[NCPS];
        char fn[FILSPC+FILSPC];
        char *p;
        int d, n, uaf, uf;
        /* sdas specific */
        static struct area *abs_ap; /* pointer to current absolute area structure */
        /* end sdas specific */

        laddr = dot.s_addr;
        lmode = SLIST;
loop:
        if ((c=endline()) == 0) { return; }
        /*
         * If the first character is a digit then assume
         * a local symbol is being specified.  The symbol
         * must end with $: to be valid.
         *      pass 0:
         *              Construct a tsym structure at the first
         *              occurance of the symbol.  Flag the symbol
         *              as multiply defined if not the first occurance.
         *      pass 1:
         *              Load area, address, and fuzz values
         *              into structure tsym.
         *      pass 2:
         *              Check for assembler phase error and
         *              multiply defined error.
         */
        if (ctype[c] & DIGIT) {
                if (flevel)
                        return;
                n = 0;
                while ((d = digit(c, 10)) >= 0) {
                        n = 10*n + d;
                        c = get();
                }
                if (c != '$' || get() != ':')
                        qerr();
                tp = symp->s_tsym;
                if (pass == 0) {
                        while (tp) {
                                if (n == tp->t_num) {
                                        tp->t_flg |= S_MDF;
                                        break;
                                }
                                tp = tp->t_lnk;
                        }
                        if (tp == NULL) {
                                tp=(struct tsym *) new (sizeof(struct tsym));
                                tp->t_lnk = symp->s_tsym;
                                tp->t_num = n;
                                tp->t_flg = 0;
                                tp->t_area = dot.s_area;
                                tp->t_addr = dot.s_addr;
                                symp->s_tsym = tp;
                        }
                } else {
                        while (tp) {
                                if (n == tp->t_num) {
                                        break;
                                }
                                tp = tp->t_lnk;
                        }
                        if (tp) {
                                if (pass == 1) {
                                        fuzz = tp->t_addr - dot.s_addr;
                                        tp->t_area = dot.s_area;
                                        tp->t_addr = dot.s_addr;
                                } else {
                                        phase(tp->t_area, tp->t_addr);
                                        if (tp->t_flg & S_MDF)
                                                err('m');
                                }
                        } else {
                                err('u');
                        }
                }
                lmode = ALIST;
                goto loop;
        }
        /*
         * If the first character is a letter then assume a label,
         * symbol, assembler directive, or assembler mnemonic is
         * being processed.
         */
        if ((ctype[c] & LETTER) == 0) {
                if (flevel) {
                        return;
                } else {
                        qerr();
                }
        }
        getid(id, c);
        c = getnb();
        /*
         * If the next character is a : then a label is being processed.
         * A double :: defines a global label.  If this is new label
         * then create a symbol structure.
         *      pass 0:
         *              Flag multiply defined labels.
         *      pass 1:
         *              Load area, address, and fuzz values
         *              into structure symp.
         *      pass 2:
         *              Check for assembler phase error and
         *              multiply defined error.
         */
        if (c == ':') {
                if (flevel)
                        return;
                if ((c = get()) != ':') {
                        unget(c);
                        c = 0;
                }
                symp = lookup(id);
                if (symp == &dot)
                        err('.');
                if (pass == 0)
                        if ((symp->s_type != S_NEW) &&
                           ((symp->s_flag & S_ASG) == 0))
                                symp->s_flag |= S_MDF;
                if (pass != 2) {
                        fuzz = symp->s_addr - dot.s_addr;
                        symp->s_type = S_USER;
                        symp->s_area = dot.s_area;
                        symp->s_addr = dot.s_addr;
                } else {
                        if (symp->s_flag & S_MDF)
                                err('m');
                        phase(symp->s_area, symp->s_addr);
                }
                if (c) {
                        symp->s_flag |= S_GBL;
                }
                lmode = ALIST;
                goto loop;
        }
        /*
         * If the next character is a = then an equate is being processed.
         * A double == defines a global equate.  If this is new variable
         * then create a symbol structure.
         */
        if (c == '=') {
                if (flevel)
                        return;
                if ((c = get()) != '=') {
                        unget(c);
                        c = 0;
                }
                clrexpr(&e1);
                expr(&e1, 0);
                sp = lookup(id);
                if (sp == &dot) {
                        outall();
                        if (e1.e_flag || e1.e_base.e_ap != dot.s_area) {
                                err('.');
                        } else {
                                sp->s_area = e1.e_base.e_ap;
                                sp->s_addr = laddr = e1.e_addr;
                        }
                } else {
                        if (sp->s_type != S_NEW && (sp->s_flag & S_ASG) == 0) {
                                err('m');
                        }
                        sp->s_area = e1.e_base.e_ap;
                        sp->s_addr = e1.e_addr;
                }
                sp->s_type = S_USER;
                sp->s_flag |= S_ASG;
                if (c) {
                        sp->s_flag |= S_GBL;
                }
                laddr = sp->s_addr;
                lmode = ELIST;
                goto loop;
        }
        unget(c);
        lmode = flevel ? SLIST : CLIST;
        if ((mp = mlookup(id)) == NULL) {
                if (!flevel)
                        err('o');
                return;
        }
        /*
         * If we have gotten this far then we have found an
         * assembler directive or an assembler mnemonic.
         *
         * Check for .if, .else, .endif, and .page directives
         * which are not controlled by the conditional flags
         */
        switch (mp->m_type) {

        case S_IF:
                n = absexpr();
                if (tlevel < MAXIF) {
                        ++tlevel;
                        ifcnd[tlevel] = n;
                        iflvl[tlevel] = flevel;
                        if (n == 0) {
                                ++flevel;
                        }
                } else {
                        err('i');
                }
                lmode = ELIST;
                laddr = n;
                return;

        case S_ELSE:
                if (ifcnd[tlevel]) {
                        if (++flevel > (iflvl[tlevel]+1)) {
                                err('i');
                        }
                } else {
                        if (--flevel < iflvl[tlevel]) {
                                err('i');
                        }
                }
                lmode = SLIST;
                return;

        case S_ENDIF:
                if (tlevel) {
                        flevel = iflvl[tlevel--];
                } else {
                        err('i');
                }
                lmode = SLIST;
                return;

        case S_PAGE:
                lop = NLPP;
                lmode = NLIST;
                return;

        default:
                break;
        }
        if (flevel)
                return;
        /*
         * If we are not in a false state for .if/.else then
         * process the assembler directives here.
         */
        switch (mp->m_type) {

        case S_EVEN:
                outall();
                laddr = dot.s_addr = (dot.s_addr + 1) & ~1;
                lmode = ALIST;
                break;

        case S_ODD:
                outall();
                laddr = dot.s_addr |= 1;
                lmode = ALIST;
                break;

        case S_BYTE:
        case S_WORD:
                do {
                        clrexpr(&e1);
                        expr(&e1, 0);
                        if (mp->m_type == S_BYTE) {
                                outrb(&e1, R_NORM);
                        } else {
                                outrw(&e1, R_NORM);
                        }
                } while ((c = getnb()) == ',');
                unget(c);
                break;

        /* sdas z80 specific */
        case S_FLOAT:
                do {
                        double f1, f2;
                        unsigned int mantissa, exponent;
                        const char readbuffer[80];

                        getid(readbuffer, ' '); /* Hack :) */
                        if ((c = getnb()) == '.') {
                                getid(&readbuffer[strlen(readbuffer)], '.');
                        }
                        else
                                unget(c);

                        f1 = strtod(readbuffer, (char **)NULL);
                        /* Convert f1 to a gb-lib type fp
                         * 24 bit mantissa followed by 7 bit exp and 1 bit sign
                         */

                        if (f1 != 0) {
                                f2 = floor(log(fabs(f1)) / log(2)) + 1;
                                mantissa = (unsigned int) ((0x1000000 * fabs(f1)) / exp(f2 * log(2)));
                                mantissa &= 0xffffff;
                                exponent = (unsigned int) (f2 + 0x40) ;
                                if (f1 < 0)
                                        exponent |=0x80;
                        }
                        else {
                                mantissa = 0;
                                exponent = 0;
                        }

                        outab(mantissa & 0xff);
                        outab((mantissa >> 8) & 0xff);
                        outab((mantissa >> 16) & 0xff);
                        outab(exponent & 0xff);

                } while ((c = getnb()) == ',');
                unget(c);
                break;
        /* end sdas z80 specific */

        /* sdas hc08 specific */
        case S_ULEB128:
        case S_SLEB128:
                do {
                        a_uint val = absexpr();
                        int bit = sizeof(val)*8 - 1;
                        int impliedBit;

                        if (mp->m_type == S_ULEB128) {
                                impliedBit = 0;
                        } else {
                                impliedBit = (val & (1 << bit)) ? 1 : 0;
                        }
                        while ((bit>0) && (((val & (1 << bit)) ? 1 : 0) == impliedBit)) {
                                bit--;
                        }
                        if (mp->m_type == S_SLEB128) {
                                bit++;
                        }
                        while (bit>=0) {
                                if (bit<7) {
                                        outab(val & 0x7f);
                                } else {
                                        outab(0x80 | (val & 0x7f));
                                }
                                bit -= 7;
                                val >>= 7;
                        }
                } while ((c = getnb()) == ',');
                unget(c);
                break;
        /* end sdas hc08 specific */

        case S_ASCII:
        case S_ASCIZ:
                if ((d = getnb()) == '\0')
                        qerr();
                while ((c = getmap(d)) >= 0)
                        outab(c);
                if (mp->m_type == S_ASCIZ)
                        outab(0);
                break;

        case S_ASCIS:
                if ((d = getnb()) == '\0')
                        qerr();
                c = getmap(d);
                while (c >= 0) {
                        if ((n = getmap(d)) >= 0) {
                                outab(c);
                        } else {
                                outab(c | 0x80);
                        }
                        c = n;
                }
                break;

        case S_BLK:
                clrexpr(&e1);
                expr(&e1, 0);
                outchk(ASXXXX_HUGE,ASXXXX_HUGE);
                dot.s_addr += e1.e_addr*mp->m_valu;
                lmode = BLIST;
                break;

        case S_TITLE:
                p = tb;
                if ((c = getnb()) != 0) {
                        do {
                                if (p < &tb[NTITL-1])
                                        *p++ = c;
                        } while ((c = get()) != 0);
                }
                *p = 0;
                unget(c);
                lmode = SLIST;
                break;

        case S_SBTL:
                p = stb;
                if ((c = getnb()) != 0) {
                        do {
                                if (p < &stb[NSBTL-1])
                                        *p++ = c;
                        } while ((c = get()) != 0);
                }
                *p = 0;
                unget(c);
                lmode = SLIST;
                break;

        case S_MODUL:
                getst(id, getnb()); // a module can start with a digit
                if (pass == 0) {
                        if (module[0]) {
                                err('m');
                        } else {
                                strncpy(module, id, NCPS);
                        }
                }
                lmode = SLIST;
                break;

        /* sdas specific */
        case S_OPTSDCC:
                optsdcc = strsto(ip);
                lmode = SLIST;
                return; /* line consumed */
        /* end sdas specific */

        case S_GLOBL:
                do {
                        getid(id, -1);
                        sp = lookup(id);
                        sp->s_flag |= S_GBL;
                } while ((c = getnb()) == ',');
                unget(c);
                lmode = SLIST;
                break;

        case S_DAREA:
                getid(id, -1);
                uaf = 0;
                uf  = A_CON|A_REL;
                if ((c = getnb()) == '(') {
                        do {
                                getid(opt, -1);
                                mp = mlookup(opt);
                                if (mp && mp->m_type == S_ATYP) {
                                        ++uaf;
                                        uf |= mp->m_valu;
                                } else {
                                        err('u');
                                }
                        } while ((c = getnb()) == ',');
                        if (c != ')')
                                qerr();
                } else {
                        unget(c);
                }
                if ((ap = alookup(id)) != NULL) {
                        if (uaf && uf != ap->a_flag)
                                err('m');
                } else {
                        ap = (struct area *) new (sizeof(struct area));
                        ap->a_ap = areap;
                        ap->a_id = strsto(id);
                        ap->a_ref = areap->a_ref + 1;
                        /* sdas specific */
                        ap->a_addr = 0;
                        /* end sdas specific */
                        ap->a_size = 0;
                        ap->a_fuzz = 0;
                        ap->a_flag = uaf ? uf : (A_CON|A_REL);
                        areap = ap;
                }
                newdot(ap);
                lmode = SLIST;
                if (dot.s_area->a_flag & A_ABS)
                        abs_ap = ap;
                break;

        case S_ORG:
                if (dot.s_area->a_flag & A_ABS) {
                        char buf[NCPS];

                        laddr = absexpr();
                        sprintf(buf, "%s%x", abs_ap->a_id, org_cnt++);
                        if ((ap = alookup(buf)) == NULL) {
                                ap = (struct area *) new (sizeof(struct area));
                                *ap = *areap;
                                ap->a_ap = areap;
                                ap->a_id = strsto(buf);
                                ap->a_ref = areap->a_ref + 1;
                                ap->a_size = 0;
                                ap->a_fuzz = 0;
                                areap = ap;
                        }
                        newdot(ap);
                        lmode = ALIST;
                        dot.s_addr = dot.s_org = laddr;
                } else {
                        err('o');
                }
                break;

        case S_RADIX:
                if (more()) {
                        switch (getnb()) {
                        case 'b':
                        case 'B':
                                radix = 2;
                                break;
                        case '@':
                        case 'o':
                        case 'O':
                        case 'q':
                        case 'Q':
                                radix = 8;
                                break;
                        case 'd':
                        case 'D':
                                radix = 10;
                                break;
                        case 'h':
                        case 'H':
                        case 'x':
                        case 'X':
                                radix = 16;
                                break;
                        default:
                                radix = 10;
                                qerr();
                                break;
                        }
                } else {
                        radix = 10;
                }
                lmode = SLIST;
                break;

        case S_INCL:
                d = getnb();
                p = fn;
                while ((c = get()) != d) {
                        if (p < &fn[FILSPC-1]) {
                                *p++ = c;
                        } else {
                                break;
                        }
                }
                *p = 0;
                if (++incfil == MAXINC ||
                   (ifp[incfil] = search_path_fopen(fn, "r")) == NULL) {
                        --incfil;
                        err('i');
                } else {
                        lop = NLPP;
                        incline[incfil] = 0;
                        strcpy(incfn[incfil],fn);
                }
                lmode = SLIST;
                break;

        /* sdas hc08 specific */
        case S_FLAT24:
                if (more()) {
                        getst(id, -1);

                        if (!as_strcmpi(id, "on")) {
                                /* Quick sanity check: size of
                                * a_uint must be at least 24 bits.
                                */
                                if (sizeof(a_uint) < 3) {
                                        warnBanner();
                                        fprintf(stderr,
                                                "Cannot enable Flat24 mode: "
                                                "host system must have 24 bit "
                                                "or greater integers.\n");
                                }
                                else {
                                        flat24Mode = 1;
                                }
                        }
                        else if (!as_strcmpi(id, "off")) {
                                flat24Mode = 0;
                        }
                        else {
                                qerr();
                        }
                }
                else {
                        qerr();
                }
                lmode = SLIST;
                #if 0
                printf("as8051: ds390 flat mode %sabled.\n",
                        flat24Mode ? "en" : "dis");
                #endif
                break;
        /* end sdas hc08 specific */

        /*
         * If not an assembler directive then go to
         * the machine dependent function which handles
         * all the assembler mnemonics.
         */
        default:
                machine(mp);
                /* sdas hc08 specific */
                /* if cdb information then generate the line info */
                if (cflag && (pass == 1))
                        DefineSDCC_Line();

                /* JLH: if -j, generate a line number symbol */
                if (jflag && (pass == 1))
                        DefineNoICE_Line();
                /* end sdas hc08 specific */
        }

        if (is_sdas()) {
                if ((c = endline()) != 0) {
                        err('q');
                }
        }
        else {
                goto loop;
        }
}

/*)Function     FILE *  afile(fn, ft, wf)
 *
 *              char *  fn              file specification string
 *              char *  ft              file type string
 *              int     wf              read(0)/write(1) flag
 *
 *      The function afile() opens a file for reading or writing.
 *              (1)     If the file type specification string ft
 *                      is not NULL then a file specification is
 *                      constructed with the file path\name in fn
 *                      and the extension in ft.
 *              (2)     If the file type specification string ft
 *                      is NULL then the file specification is
 *                      constructed from fn.  If fn does not have
 *                      a file type then the default source file
 *                      type dsft is appended to the file specification.
 *
 *      afile() returns a file handle for the opened file or aborts
 *      the assembler on an open error.
 *
 *      local variables:
 *              int     c               character value
 *              FILE *  fp              filehandle for opened file
 *              char *  p1              pointer to filespec string fn
 *              char *  p2              pointer to filespec string fb
 *              char *  p3              pointer to filetype string ft
 *
 *      global variables:
 *              char    afn[]           afile() constructed filespec
 *              char    dsft[]          default assembler file type string
 *              char    afn[]           constructed file specification string
 *
 *      functions called:
 *              VOID    asexit()        asmain.c
 *              FILE *  fopen()         c_library
 *              int     fprintf()       c_library
 *
 *      side effects:
 *              File is opened for read or write.
 */

FILE *
afile(fn, ft, wf)
char *fn;
char *ft;
int wf;
{
        char *p2, *p3;
        int c;
        FILE *fp;

        p2 = afn;
        p3 = ft;

        strcpy (afn, fn);
        p2 = strrchr (afn, FSEPX);              // search last '.'
        if (!p2)
                p2 = afn + strlen (afn);
        if (p2 > &afn[PATH_MAX-4])              // truncate filename, if it's too long
                p2 = &afn[PATH_MAX-4];
        *p2++ = FSEPX;

        // choose a file-extension
        if (*p3 == 0) {                         // extension supplied?
                p3 = strrchr (fn, FSEPX);       // no: extension in fn?
                if (p3)
                        ++p3;
                else
                        p3 = dsft;              // no: default extension
        }

        while ((c = *p3++) != 0) {              // strncpy
                if (p2 < &afn[PATH_MAX-1])
                        *p2++ = c;
        }
        *p2++ = 0;

        if ((fp = fopen(afn, wf?"w":"r")) == NULL) {
                fprintf(stderr, "%s: cannot %s.\n", afn, wf?"create":"open");
                asexit(1);
        }
        return (fp);
}

/*)Function     VOID    newdot(nap)
 *
 *              area *  nap             pointer to the new area structure
 *
 *      The function newdot():
 *              (1)     copies the current values of fuzz and the last
 *                      address into the current area referenced by dot
 *              (2)     loads dot with the pointer to the new area and
 *                      loads the fuzz and last address parameters
 *              (3)     outall() is called to flush any remaining
 *                      bufferred code from the old area to the output
 *
 *      local variables:
 *              area *  oap             pointer to old area
 *
 *      global variables:
 *              sym     dot             defined as sym[0]
 *              a_uint  fuzz            tracks pass to pass changes in the
 *                                      address of symbols caused by
 *                                      variable length instruction formats
 *
 *      functions called:
 *              none
 *
 *      side effects:
 *              Current area saved, new area loaded, buffers flushed.
 */

VOID
newdot(nap)
struct area *nap;
{
        struct area *oap;

        oap = dot.s_area;
        /* fprintf (stderr, "%s dot.s_area->a_size: %d dot.s_addr: %d\n",
                oap->a_id, dot.s_area->a_size, dot.s_addr); */
        oap->a_fuzz = fuzz;
        if (oap->a_flag & A_OVR) {
                // the size of an overlay is the biggest size encountered
                if (oap->a_size < dot.s_addr) {
                        oap->a_size = dot.s_addr;
                }
        }
        else if (oap->a_flag & A_ABS) {
                oap->a_addr = dot.s_org;
                oap->a_size += dot.s_addr - dot.s_org;
                dot.s_addr = dot.s_org = 0;
        }
        else {
                oap->a_addr = 0;
                oap->a_size = dot.s_addr;
        }
        if (nap->a_flag & A_OVR) {
                // a new overlay starts at 0, no fuzz
                dot.s_addr = 0;
                fuzz = 0;
        }
        else if (nap->a_flag & A_ABS) {
                // a new absolute starts at org, no fuzz
                dot.s_addr = dot.s_org;
                fuzz = 0;
        }
        else {
                dot.s_addr = nap->a_size;
                fuzz = nap->a_fuzz;
        }
        dot.s_area = nap;
        outall();
}

/*)Function     VOID    phase(ap, a)
 *
 *              area *  ap              pointer to area
 *              a_uint  a               address in area
 *
 *      Function phase() compares the area ap and address a
 *      with the current area dot.s_area and address dot.s_addr
 *      to determine if the position of the symbol has changed
 *      between assembler passes.
 *
 *      local variables:
 *              none
 *
 *      global varaibles:
 *              sym *   dot             defined as sym[0]
 *
 *      functions called:
 *              none
 *
 *      side effects:
 *              The p error is invoked if the area and/or address
 *              has changed.
 */

VOID
phase(ap, a)
struct area *ap;
a_uint a;
{
        if (ap != dot.s_area || a != dot.s_addr)
                err('p');
}

char *usetxt[] = {
        "Usage: [-Options] file",
        "Usage: [-Options] outfile file1 [file2 file3 ...]",
        "  -d   Decimal listing",
        "  -q   Octal   listing",
        "  -x   Hex     listing (default)",
        "  -j   Add line number and debug information to file", /* JLH */
        "  -g   Undefined symbols made global",
        "  -a   All user symbols made global",
        "  -l   Create list   output file1[lst]",
        "  -o   Create object output file1[rel]",
        "  -s   Create symbol output file1[sym]",
        "  -c   Generate sdcdb debug information",
        "  -p   Disable listing pagination",
        "  -w   Wide listing format for symbol table",
        "  -z   Enable case sensitivity for symbols",
        "  -f   Flag relocatable references by  `   in listing file",
        "  -ff  Flag relocatable references by mode in listing file",
        "  -I   Add the named directory to the include file",
        "       search path.  This option may be used more than once.",
        "       Directories are searched in the order given.",
        "",
        0
};

/*)Function     VOID    usage()
 *
 *      The function usage() outputs to the stderr device the
 *      assembler name and version and a list of valid assembler options.
 *
 *      local variables:
 *              char ** dp              pointer to an array of
 *                                      text string pointers.
 *
 *      global variables:
 *              char    cpu[]           assembler type string
 *              char *  usetxt[]        array of string pointers
 *
 *      functions called:
 *              VOID    asexit()        asmain.c
 *              int     fprintf()       c_library
 *
 *      side effects:
 *              program is terminated
 */

VOID
usage(n)
{
        char **dp;

        /* sdas specific */
        fprintf(stderr, "\n%s Assembler %s  (%s)\n\n", is_sdas() ? "sdas" : "ASxxxx", VERSION, cpu);
        /* end sdas specific */
        for (dp = usetxt; *dp; dp++)
                fprintf(stderr, "%s\n", *dp);
        asexit(n);
}
