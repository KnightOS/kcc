/* Various stuff needed to hack the build for SDCC. */
#define xmalloc   malloc
#define xstrdup   strdup
#define xrealloc  realloc
#define xcalloc   calloc

#define xstrerror strerror
#define xmalloc_set_program_name(n) /* nada */

/*
 * From defaults.h
 */
/* Define results of standard character escape sequences.  */
#define TARGET_BELL	007
#define TARGET_BS	010
#define TARGET_TAB	011
#define TARGET_NEWLINE	012
#define TARGET_VT	013
#define TARGET_FF	014
#define TARGET_CR	015
#define TARGET_ESC	033

#define CHAR_TYPE_SIZE 8
#define WCHAR_TYPE_SIZE 32	/* ? maybe ? */

#define SUPPORTS_ONE_ONLY 0

#define TARGET_OBJECT_SUFFIX ".rel"

#ifndef WCHAR_UNSIGNED
#define WCHAR_UNSIGNED 0
#endif

#ifdef _WIN32
#define HAVE_DOS_BASED_FILE_SYSTEM
#endif
