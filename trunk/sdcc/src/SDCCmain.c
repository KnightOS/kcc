/*-------------------------------------------------------------------------
  SDCCmain.c - main file

             Written By -  Sandeep Dutta . sandeep.dutta@usa.net (1999)

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   In other words, you are welcome to use, share and improve this program.
   You are forbidden to forbid anyone else to use, share and improve
   what you give them.   Help stamp out software-hoarding!
-------------------------------------------------------------------------*/

#include "common.h"
#include <ctype.h>
#include "newalloc.h"
#include "SDCCerr.h"
#include "BuildCmd.h"
#include "MySystem.h"

#if NATIVE_WIN32
#include <process.h>
#else
#include "spawn.h"
#endif

// This is a bit messy because we define link ourself
#if !defined(__BORLANDC__) && !defined(_MSC_VER)

#include <unistd.h>

#else
// No unistd.h in Borland C++
/*
extern int access (const char *, int);
#define X_OK 1
*/

#endif

//REMOVE ME!!!
extern int yyparse ();

FILE *srcFile;			/* source file          */
FILE *cdbFile = NULL;		/* debugger information output file */
char *fullSrcFileName;		/* full name for the source file */
char *srcFileName;		/* source file name with the .c stripped */
char *moduleName;		/* module name is srcFilename stripped of any path */
const char *preArgv[128];	/* pre-processor arguments  */
int currRegBank = 0;
struct optimize optimize;
struct options options;
char *VersionString = SDCC_VERSION_STR /*"Version 2.1.8a" */ ;
short preProcOnly = 0;
short noAssemble = 0;
char *linkOptions[128];
const char *asmOptions[128];
char *libFiles[128];
int nlibFiles = 0;
char *libPaths[128];
int nlibPaths = 0;
char *relFiles[128];
int nrelFiles = 0;
bool verboseExec = FALSE;
char *preOutName;

// In MSC VC6 default search path for exe's to path for this

char DefaultExePath[128];

/* Far functions, far data */
#define OPTION_LARGE_MODEL "-model-large"
/* Far functions, near data */
#define OPTION_MEDIUM_MODEL "-model-medium"
#define OPTION_SMALL_MODEL "-model-small"
#define OPTION_FLAT24_MODEL "-model-flat24"
#define OPTION_STACK_AUTO  "-stack-auto"
#define OPTION_STACK_8BIT "-stack-8bit"
#define OPTION_STACK_10BIT "-stack-10bit"
#define OPTION_XSTACK      "-xstack"
#define OPTION_GENERIC     "-generic"
#define OPTION_NO_GCSE     "-nogcse"
#define OPTION_NO_LOOP_INV "-noinvariant"
#define OPTION_NO_LOOP_IND "-noinduction"
#define OPTION_NO_JTBOUND  "-nojtbound"
#define OPTION_NO_LOOPREV  "-noloopreverse"
#define OPTION_XREGS       "-regextend"
#define OPTION_COMP_ONLY   "-compile-only"
#define OPTION_DUMP_RAW    "-dumpraw"
#define OPTION_DUMP_GCSE   "-dumpgcse"
#define OPTION_DUMP_LOOP   "-dumploop"
#define OPTION_DUMP_KILL   "-dumpdeadcode"
#define OPTION_DUMP_RANGE  "-dumpliverange"
#define OPTION_DUMP_PACK   "-dumpregpack"
#define OPTION_DUMP_RASSGN "-dumpregassign"
#define OPTION_DUMP_ALL    "-dumpall"
#define OPTION_XRAM_LOC    "-xram-loc"
#define OPTION_IRAM_SIZE   "-iram-size"
#define OPTION_XSTACK_LOC  "-xstack-loc"
#define OPTION_CODE_LOC    "-code-loc"
#define OPTION_STACK_LOC   "-stack-loc"
#define OPTION_DATA_LOC    "-data-loc"
#define OPTION_IDATA_LOC   "-idata-loc"
#define OPTION_PEEP_FILE   "-peep-file"
#define OPTION_LIB_PATH    "-lib-path"
#define OPTION_INTLONG_RENT "-int-long-reent"
#define OPTION_FLOAT_RENT  "-float-reent"
#define OPTION_OUT_FMT_IHX "-out-fmt-ihx"
#define OPTION_OUT_FMT_S19 "-out-fmt-s19"
#define OPTION_CYCLOMATIC  "-cyclomatic"
#define OPTION_NOOVERLAY   "-nooverlay"
#define OPTION_MAINRETURN  "-main-return"
#define OPTION_NOPEEP      "-no-peep"
#define OPTION_ASMPEEP     "-peep-asm"
#define OPTION_DEBUG       "-debug"
#define OPTION_NODEBUG     "-nodebug"
#define OPTION_VERSION     "-version"
#define OPTION_STKAFTRDATA "-stack-after-data"
#define OPTION_PREPROC_ONLY "-preprocessonly"
#define OPTION_C1_MODE   "-c1mode"
#define OPTION_HELP         "-help"
#define OPTION_CALLEE_SAVES "-callee-saves"
#define OPTION_NOSTDLIB     "-nostdlib"
#define OPTION_NOSTDINC     "-nostdinc"
#define OPTION_VERBOSE      "-verbose"
#define OPTION_LESS_PEDANTIC "-lesspedantic"

static const char *_preCmd[] =
{
  "sdcpp", "-Wall", "-lang-c++", "-DSDCC=1",
  "$l", "$1", "$2", NULL
};

PORT *port;

static PORT *_ports[] =
{
#if !OPT_DISABLE_MCS51
  &mcs51_port,
#endif
#if !OPT_DISABLE_GBZ80
  &gbz80_port,
#endif
#if !OPT_DISABLE_Z80
  &z80_port,
#endif
#if !OPT_DISABLE_AVR
  &avr_port,
#endif
#if !OPT_DISABLE_DS390
  &ds390_port,
#endif
#if !OPT_DISABLE_PIC
  &pic_port,
#endif
#if !OPT_DISABLE_I186
  &i186_port,
#endif
#if !OPT_DISABLE_TLCS900H
  &tlcs900h_port,
#endif
};

#define NUM_PORTS (sizeof(_ports)/sizeof(_ports[0]))

/**
   remove me - TSD a hack to force sdcc to generate gpasm format .asm files.
 */
extern void picglue ();

/** Sets the port to the one given by the command line option.
    @param    The name minus the option (eg 'mcs51')
    @return     0 on success.
*/
static void
_setPort (const char *name)
{
  int i;
  for (i = 0; i < NUM_PORTS; i++)
    {
      if (!strcmp (_ports[i]->target, name))
	{
	  port = _ports[i];
	  return;
	}
    }
  /* Error - didnt find */
  werror (E_UNKNOWN_TARGET, name);
  exit (1);
}

static void
_validatePorts (void)
{
  int i;
  for (i = 0; i < NUM_PORTS; i++)
    {
      if (_ports[i]->magic != PORT_MAGIC)
	{
	  printf ("Error: port %s is incomplete.\n", _ports[i]->target);
	  wassert (0);
	}
    }
}
/*-----------------------------------------------------------------*/
/* printVersionInfo - prints the version info        */
/*-----------------------------------------------------------------*/
void
printVersionInfo ()
{
  int i;

  fprintf (stderr,
	   "SDCC : ");
  for (i = 0; i < NUM_PORTS; i++)
    fprintf (stderr, "%s%s", i == 0 ? "" : "/", _ports[i]->target);

  fprintf (stderr, " %s"
#ifdef SDCC_SUB_VERSION_STR
	   "/" SDCC_SUB_VERSION_STR
#endif
#ifdef __CYGWIN32__
	   " (CYGWIN32)\n"
#else
#ifdef __DJGPP__
	   " (DJGPP) \n"
#else
#if defined(_MSC_VER)
	   " (WIN32) \n"
#else
	   " (UNIX) \n"
#endif
#endif
#endif

	   ,VersionString
    );
}

/*-----------------------------------------------------------------*/
/* printUsage - prints command line syntax         */
/*-----------------------------------------------------------------*/
void
printUsage ()
{
  printVersionInfo ();
  fprintf (stderr,
	   "Usage : [options] filename\n"
	   "Options :-\n"
       "\t-m<proc>             -     Target processor <proc>.  Default %s\n"
	   "\t                           Try --version for supported values of <proc>\n"
	   "\t--model-large        -     Large Model\n"
	   "\t--model-small        -     Small Model (default)\n"
	   "\t--stack-auto         -     Stack automatic variables\n"
	   "\t--xstack             -     Use external stack\n"
	   "\t--xram-loc <nnnn>    -     External Ram start location\n"
	   "\t--xstack-loc <nnnn>  -     Xternal Stack Location\n"
	   "\t--code-loc <nnnn>    -     Code Segment Location\n"
	   "\t--stack-loc <nnnn>   -     Stack pointer initial value\n"
	   "\t--data-loc <nnnn>    -     Direct data start location\n"
	   "\t--idata-loc <nnnn>   -     Indirect data start location\n"
	   "\t--iram-size <nnnn>   -     Internal Ram size\n"
	   "\t--nojtbound          -     Don't generate boundary check for jump tables\n"
	   "\t--generic            -     All unqualified ptrs converted to '_generic'\n"
	   "PreProcessor Options :-\n"
	   "\t-Dmacro   - Define Macro\n"
	   "\t-Ipath    - Include \"*.h\" path\n"
      "Note: this is NOT a complete list of options see docs for details\n",
	   _ports[0]->target
    );
  exit (0);
}

/*-----------------------------------------------------------------*/
/* parseWithComma - separates string with comma                    */
/*-----------------------------------------------------------------*/
void
parseWithComma (char **dest, char *src)
{
  int i = 0;

  strtok (src, "\n \t");
  /* skip the initial white spaces */
  while (isspace (*src))
    src++;
  dest[i++] = src;
  while (*src)
    {
      if (*src == ',')
	{
	  *src = '\0';
	  src++;
	  if (*src)
	    dest[i++] = src;
	  continue;
	}
      src++;
    }
}

/*-----------------------------------------------------------------*/
/* setDefaultOptions - sets the default options                    */
/*-----------------------------------------------------------------*/
static void
setDefaultOptions ()
{
  int i;

  for (i = 0; i < 128; i++)
    preArgv[i] = asmOptions[i] =
      linkOptions[i] = relFiles[i] = libFiles[i] =
      libPaths[i] = NULL;

  /* first the options part */
  options.stack_loc = 0;	/* stack pointer initialised to 0 */
  options.xstack_loc = 0;	/* xternal stack starts at 0 */
  options.code_loc = 0;		/* code starts at 0 */
  options.data_loc = 0x0030;	/* data starts at 0x0030 */
  options.xdata_loc = 0;
  options.idata_loc = 0x80;
  options.genericPtr = 1;	/* default on */
  options.nopeep = 0;
  options.model = port->general.default_model;
  options.nostdlib = 0;
  options.nostdinc = 0;
  options.verbose = 0;

  options.stack10bit=0;

  /* now for the optimizations */
  /* turn on the everything */
  optimize.global_cse = 1;
  optimize.label1 = 1;
  optimize.label2 = 1;
  optimize.label3 = 1;
  optimize.label4 = 1;
  optimize.loopInvariant = 1;
  optimize.loopInduction = 1;

  /* now for the ports */
  port->setDefaultOptions ();
}

/*-----------------------------------------------------------------*/
/* processFile - determines the type of file from the extension    */
/*-----------------------------------------------------------------*/
static void
processFile (char *s)
{
  char *fext = NULL;

  /* get the file extension */
  fext = s + strlen (s);
  while ((fext != s) && *fext != '.')
    fext--;

  /* now if no '.' then we don't know what the file type is
     so give a warning and return */
  if (fext == s)
    {
      werror (W_UNKNOWN_FEXT, s);
      return;
    }

  /* otherwise depending on the file type */
  if (strcmp (fext, ".c") == 0 || strcmp (fext, ".C") == 0 || options.c1mode)
    {
      /* source file name : not if we already have a
         source file */
      if (srcFileName)
	{
	  werror (W_TOO_MANY_SRC, s);
	  return;
	}

      /* the only source file */
      if (!(srcFile = fopen ((fullSrcFileName = s), "r")))
	{
	  werror (E_FILE_OPEN_ERR, s);
	  exit (1);
	}

      /* copy the file name into the buffer */
      strcpy (buffer, s);

      /* get rid of the "." */
      strtok (buffer, ".");
      srcFileName = Safe_calloc (1, strlen (buffer) + 1);
      strcpy (srcFileName, buffer);

      /* get rid of any path information
         for the module name; do this by going
         backwards till we get to either '/' or '\' or ':'
         or start of buffer */
      fext = buffer + strlen (buffer);
      while (fext != buffer &&
	     *(fext - 1) != '\\' &&
	     *(fext - 1) != '/' &&
	     *(fext - 1) != ':')
	fext--;
      moduleName = Safe_calloc (1, strlen (fext) + 1);
      strcpy (moduleName, fext);

      return;
    }

  /* if the extention is type .rel or .r or .REL or .R
     addtional object file will be passed to the linker */
  if (strcmp (fext, ".r") == 0 || strcmp (fext, ".rel") == 0 ||
      strcmp (fext, ".R") == 0 || strcmp (fext, ".REL") == 0 ||
      strcmp (fext, port->linker.rel_ext) == 0)
    {
      relFiles[nrelFiles++] = s;
      return;
    }

  /* if .lib or .LIB */
  if (strcmp (fext, ".lib") == 0 || strcmp (fext, ".LIB") == 0)
    {
      libFiles[nlibFiles++] = s;
      return;
    }

  werror (W_UNKNOWN_FEXT, s);

}

static void
_processC1Arg (char *s)
{
  if (srcFileName)
    {
      if (options.out_name)
	{
	  werror (W_TOO_MANY_SRC, s);
	  return;
	}
      options.out_name = strdup (s);
    }
  else
    {
      processFile (s);
    }
}

static void
_addToList (const char **list, const char *str)
{
  /* This is the bad way to do things :) */
  while (*list)
    list++;
  *list = strdup (str);
  if (!*list)
    {
      werror (E_OUT_OF_MEM, __FILE__, 0);
      exit (1);
    }
  *(++list) = NULL;
}

static void
_setModel (int model, const char *sz)
{
  if (port->general.supported_models & model)
    options.model = model;
  else
    werror (W_UNSUPPORTED_MODEL, sz, port->target);
}

/*-----------------------------------------------------------------*/
/* parseCmdLine - parses the command line and sets the options     */
/*-----------------------------------------------------------------*/
int
parseCmdLine (int argc, char **argv)
{
  int i;
  char cdbfnbuf[50];

  /* go thru all whole command line */
  for (i = 1; i < argc; i++)
    {
      if (i >= argc)
	break;

      /* options */
      if (argv[i][0] == '-' && argv[i][1] == '-')
	{

	  if (strcmp (&argv[i][1], OPTION_HELP) == 0)
	    {
	      printUsage ();
	      exit (0);
	    }

	  if (strcmp (&argv[i][1], OPTION_XREGS) == 0)
	    {
	      options.regExtend = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_LARGE_MODEL) == 0)
	    {
	      _setModel (MODEL_LARGE, argv[i]);
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_MEDIUM_MODEL) == 0)
	    {
	      _setModel (MODEL_MEDIUM, argv[i]);
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_SMALL_MODEL) == 0)
	    {
	      _setModel (MODEL_SMALL, argv[i]);
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_FLAT24_MODEL) == 0)
	    {
	      _setModel (MODEL_FLAT24, argv[i]);
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_STACK_10BIT) == 0)
	    {
	      options.stack10bit = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_STACK_8BIT) == 0)
	    {
	      options.stack10bit = 0;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_STACK_AUTO) == 0)
	    {
	      options.stackAuto = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_DUMP_RAW) == 0)
	    {
	      options.dump_raw = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_CYCLOMATIC) == 0)
	    {
	      options.cyclomatic = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_DUMP_GCSE) == 0)
	    {
	      options.dump_gcse = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_DUMP_LOOP) == 0)
	    {
	      options.dump_loop = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_DUMP_KILL) == 0)
	    {
	      options.dump_kill = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_INTLONG_RENT) == 0)
	    {
	      options.intlong_rent = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_FLOAT_RENT) == 0)
	    {
	      options.float_rent = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_DUMP_RANGE) == 0)
	    {
	      options.dump_range = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_DUMP_PACK) == 0)
	    {
	      options.dump_pack = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_DUMP_RASSGN) == 0)
	    {
	      options.dump_rassgn = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_OUT_FMT_IHX) == 0)
	    {
	      options.out_fmt = 0;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_OUT_FMT_S19) == 0)
	    {
	      options.out_fmt = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_NOOVERLAY) == 0)
	    {
	      options.noOverlay = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_STKAFTRDATA) == 0)
	    {
	      options.stackOnData = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_PREPROC_ONLY) == 0)
	    {
	      preProcOnly = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_C1_MODE) == 0)
	    {
	      options.c1mode = 1;
	      continue;
	    }


	  if (strcmp (&argv[i][1], OPTION_DUMP_ALL) == 0)
	    {
	      options.dump_rassgn =
		options.dump_pack =
		options.dump_range =
		options.dump_kill =
		options.dump_loop =
		options.dump_gcse =
		options.dump_raw = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_COMP_ONLY) == 0)
	    {
	      options.cc_only = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_GENERIC) == 0)
	    {
	      options.genericPtr = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_NOPEEP) == 0)
	    {
	      options.nopeep = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_ASMPEEP) == 0)
	    {
	      options.asmpeep = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_DEBUG) == 0)
	    {
	      options.debug = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_NODEBUG) == 0)
	    {
	      options.nodebug = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_PEEP_FILE) == 0)
	    {
	      if (argv[i][1 + strlen (OPTION_PEEP_FILE)])
		options.peep_file =
		  &argv[i][1 + strlen (OPTION_PEEP_FILE)];
	      else
		options.peep_file = argv[++i];
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_LIB_PATH) == 0)
	    {
	      if (argv[i][1 + strlen (OPTION_LIB_PATH)])
		libPaths[nlibPaths++] =
		  &argv[i][1 + strlen (OPTION_PEEP_FILE)];
	      else
		libPaths[nlibPaths++] = argv[++i];
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_XSTACK_LOC) == 0)
	    {

	      if (argv[i][1 + strlen (OPTION_XSTACK_LOC)])
		options.xstack_loc =
		  (int) floatFromVal (constVal (&argv[i][1 + strlen (OPTION_XSTACK_LOC)]));
	      else
		options.xstack_loc =
		  (int) floatFromVal (constVal (argv[++i]));
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_XSTACK) == 0)
	    {
	      options.useXstack = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_MAINRETURN) == 0)
	    {
	      options.mainreturn = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_CALLEE_SAVES) == 0)
	    {
	      if (argv[i][1 + strlen (OPTION_CALLEE_SAVES)])
		parseWithComma (options.calleeSaves
				,&argv[i][1 + strlen (OPTION_CALLEE_SAVES)]);
	      else
		parseWithComma (options.calleeSaves, argv[++i]);
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_STACK_LOC) == 0)
	    {

	      if (argv[i][1 + strlen (OPTION_STACK_LOC)])
		options.stack_loc =
		  (int) floatFromVal (constVal (&argv[i][1 + strlen (OPTION_STACK_LOC)]));
	      else
		options.stack_loc =
		  (int) floatFromVal (constVal (argv[++i]));
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_XRAM_LOC) == 0)
	    {

	      if (argv[i][1 + strlen (OPTION_XRAM_LOC)])
		options.xdata_loc =
		  (unsigned int) floatFromVal (constVal (&argv[i][1 + strlen (OPTION_XRAM_LOC)]));
	      else
		options.xdata_loc =
		  (unsigned int) floatFromVal (constVal (argv[++i]));
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_IRAM_SIZE) == 0)
	    {

	      if (argv[i][1 + strlen (OPTION_IRAM_SIZE)])
		options.iram_size =
		  (int) floatFromVal (constVal (&argv[i][1 + strlen (OPTION_IRAM_SIZE)]));
	      else
		options.iram_size =
		  (int) floatFromVal (constVal (argv[++i]));
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_VERSION) == 0)
	    {
	      printVersionInfo ();
	      exit (0);
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_DATA_LOC) == 0)
	    {

	      if (argv[i][1 + strlen (OPTION_DATA_LOC)])
		options.data_loc =
		  (int) floatFromVal (constVal (&argv[i][1 + strlen (OPTION_DATA_LOC)]));
	      else
		options.data_loc =
		  (int) floatFromVal (constVal (argv[++i]));
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_IDATA_LOC) == 0)
	    {

	      if (argv[i][1 + strlen (OPTION_IDATA_LOC)])
		options.idata_loc =
		  (int) floatFromVal (constVal (&argv[i][1 + strlen (OPTION_IDATA_LOC)]));
	      else
		options.idata_loc =
		  (int) floatFromVal (constVal (argv[++i]));
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_CODE_LOC) == 0)
	    {

	      if (argv[i][1 + strlen (OPTION_CODE_LOC)])
		options.code_loc =
		  (int) floatFromVal (constVal (&argv[i][1 + strlen (OPTION_CODE_LOC)]));
	      else
		options.code_loc =
		  (int) floatFromVal (constVal (argv[++i]));
	      continue;
	    }


	  if (strcmp (&argv[i][1], OPTION_NO_JTBOUND) == 0)
	    {
	      optimize.noJTabBoundary = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_NO_GCSE) == 0)
	    {
	      optimize.global_cse = 0;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_NO_LOOP_INV) == 0)
	    {
	      optimize.loopInvariant = 0;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_NO_LOOP_IND) == 0)
	    {
	      optimize.loopInduction = 0;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_NO_LOOPREV) == 0)
	    {
	      optimize.noLoopReverse = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_NOSTDLIB) == 0)
	    {
	      options.nostdlib = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_NOSTDINC) == 0)
	    {
	      options.nostdinc = 1;
	      continue;
	    }

	  if (strcmp (&argv[i][1], OPTION_VERBOSE) == 0)
	    {
	      options.verbose = 1;
	      continue;
	    }
          
          if (strcmp (argv[i] +1, OPTION_LESS_PEDANTIC) == 0) 
              {
                  setErrorLogLevel(ERROR_LEVEL_WARNINGS);
                  continue;
              }

	  if (!port->parseOption (&argc, argv, &i))
	    {
	      werror (W_UNKNOWN_OPTION, argv[i]);
	    }
	  else
	    {
	      continue;
	    }
	}


      /* if preceded by  '-' then option */
      if (*argv[i] == '-')
	{
	  switch (argv[i][1])
	    {
	    case 'h':
	      printUsage ();
	      exit (0);
	      break;

	    case 'E':
	      preProcOnly = 1;
	      break;

	    case 'm':
	      /* Used to select the port */
	      _setPort (argv[i] + 2);
	      break;

	    case 'a':
	      werror (W_UNSUPP_OPTION, "-a", "use --stack-auto instead");
	      break;

	    case 'g':
	      werror (W_UNSUPP_OPTION, "-g", "use --generic instead");
	      break;

	    case 'X':		/* use external stack   */
	      werror (W_UNSUPP_OPTION, "-X", "use --xstack-loc instead");
	      break;

	    case 'x':
	      werror (W_UNSUPP_OPTION, "-x", "use --xstack instead");
	      break;

	    case 'p':		/* stack pointer intial value */
	    case 'P':
	      werror (W_UNSUPP_OPTION, "-p", "use --stack-loc instead");
	      break;

	    case 'i':
	      werror (W_UNSUPP_OPTION, "-i", "use --idata-loc instead");
	      break;

	    case 'r':
	      werror (W_UNSUPP_OPTION, "-r", "use --xdata-loc instead");
	      break;

	    case 's':
	      werror (W_UNSUPP_OPTION, "-s", "use --code-loc instead");
	      break;

	    case 'c':
	      options.cc_only = 1;
	      break;

	    case 'Y':
	      werror (W_UNSUPP_OPTION, "-Y", "use -I instead");
	      break;

	    case 'L':
	      if (argv[i][2])
		libPaths[nlibPaths++] = &argv[i][2];
	      else
		libPaths[nlibPaths++] = argv[++i];
	      break;

	    case 'W':
	      /* linker options */
	      if (argv[i][2] == 'l')
		{
		  if (argv[i][3])
		    parseWithComma (linkOptions, &argv[i][3]);
		  else
		    parseWithComma (linkOptions, argv[++i]);
		}
	      else
		{
		  /* assembler options */
		  if (argv[i][2] == 'a')
		    {
		      if (argv[i][3])
			parseWithComma ((char **) asmOptions, &argv[i][3]);
		      else
			parseWithComma ((char **) asmOptions, argv[++i]);

		    }
		  else
		    {
		      werror (W_UNKNOWN_OPTION, argv[i]);
		    }
		}
	      break;
	    case 'S':
	      noAssemble = 1;
	      break;

	    case 'V':
	      verboseExec = TRUE;
	      break;

	    case 'v':
	      printVersionInfo ();
	      exit (0);
	      break;

	      /* preprocessor options */
	    case 'M':
	      {
		preProcOnly = 1;
		_addToList (preArgv, "-M");
		break;
	      }
	    case 'C':
	      {
		_addToList (preArgv, "-C");
		break;
	      }
	    case 'd':
	    case 'D':
	    case 'I':
	    case 'A':
	    case 'U':
	      {
		char sOpt = argv[i][1];
		char *rest;

		if (argv[i][2] == ' ' || argv[i][2] == '\0')
		  {
		    i++;
		    rest = argv[i];
		  }
		else
		  rest = &argv[i][2];

		if (argv[i][1] == 'Y')
		  argv[i][1] = 'I';

		sprintf (buffer, "-%c%s", sOpt, rest);
		_addToList (preArgv, buffer);
	      }
	      break;

	    default:
	      if (!port->parseOption (&argc, argv, &i))
		werror (W_UNKNOWN_OPTION, argv[i]);
	    }
	  continue;
	}

      if (!port->parseOption (&argc, argv, &i))
	{
	  /* no option must be a filename */
	  if (options.c1mode)
	    _processC1Arg (argv[i]);
	  else
	    processFile (argv[i]);
	}
    }

  /* set up external stack location if not explicitly specified */
  if (!options.xstack_loc)
    options.xstack_loc = options.xdata_loc;

  /* if debug option is set the open the cdbFile */
  if (!options.nodebug && srcFileName)
    {
      sprintf (cdbfnbuf, "%s.cdb", srcFileName);
      if ((cdbFile = fopen (cdbfnbuf, "w")) == NULL)
	werror (E_FILE_OPEN_ERR, cdbfnbuf);
      else
	{
	  /* add a module record */
	  fprintf (cdbFile, "M:%s\n", moduleName);
	}
    }
  return 0;
}

/*-----------------------------------------------------------------*/
/* linkEdit : - calls the linkage editor  with options             */
/*-----------------------------------------------------------------*/
static void
linkEdit (char **envp)
{
  FILE *lnkfile;
  char *segName, *c;

  int i;
  if (!srcFileName)
    srcFileName = "temp";

  /* first we need to create the <filename>.lnk file */
  sprintf (buffer, "%s.lnk", srcFileName);
  if (!(lnkfile = fopen (buffer, "w")))
    {
      werror (E_FILE_OPEN_ERR, buffer);
      exit (1);
    }

  /* now write the options */
  fprintf (lnkfile, "-mux%c\n", (options.out_fmt ? 's' : 'i'));

  /* if iram size specified */
  if (options.iram_size)
    fprintf (lnkfile, "-a 0x%04x\n", options.iram_size);

  /*if (options.debug) */
  fprintf (lnkfile, "-z\n");

#define WRITE_SEG_LOC(N, L) \
    segName = strdup(N); \
    c = strtok(segName, " \t"); \
    fprintf (lnkfile,"-b %s = 0x%04x\n", c, L); \
    if (segName) { free(segName); }

  /* code segment start */
  WRITE_SEG_LOC (CODE_NAME, options.code_loc);

  /* data segment start */
  WRITE_SEG_LOC (DATA_NAME, options.data_loc);

  /* xdata start */
  WRITE_SEG_LOC (XDATA_NAME, options.xdata_loc);

  /* indirect data */
  WRITE_SEG_LOC (IDATA_NAME, options.idata_loc);

  /* bit segment start */
  WRITE_SEG_LOC (BIT_NAME, 0);

  /* add the extra linker options */
  for (i = 0; linkOptions[i]; i++)
    fprintf (lnkfile, "%s\n", linkOptions[i]);

  /* other library paths if specified */
  for (i = 0; i < nlibPaths; i++)
    fprintf (lnkfile, "-k %s\n", libPaths[i]);

  /* standard library path */
  if (!options.nostdlib)
    {
      if (TARGET_IS_DS390)
	{
	  c = "ds390";
	}
      else
	{
	  switch (options.model)
	    {
	    case MODEL_SMALL:
	      c = "small";
	      break;
	    case MODEL_LARGE:
	      c = "large";
	      break;
	    case MODEL_FLAT24:
	      c = "flat24";
	      break;
	    default:
	      werror (W_UNKNOWN_MODEL, __FILE__, __LINE__);
	      c = "unknown";
	      break;
	    }
	}
      fprintf (lnkfile, "-k %s/%s\n", SDCC_LIB_DIR /*STD_LIB_PATH */ , c);

      /* standard library files */
      if (strcmp (port->target, "ds390") == 0)
	{
	  fprintf (lnkfile, "-l %s\n", STD_DS390_LIB);
	}
      fprintf (lnkfile, "-l %s\n", STD_LIB);
      fprintf (lnkfile, "-l %s\n", STD_INT_LIB);
      fprintf (lnkfile, "-l %s\n", STD_LONG_LIB);
      fprintf (lnkfile, "-l %s\n", STD_FP_LIB);
    }

  /* additional libraries if any */
  for (i = 0; i < nlibFiles; i++)
    fprintf (lnkfile, "-l %s\n", libFiles[i]);

  /* put in the object files */
  if (strcmp (srcFileName, "temp"))
    fprintf (lnkfile, "%s ", srcFileName);

  for (i = 0; i < nrelFiles; i++)
    fprintf (lnkfile, "%s\n", relFiles[i]);

  fprintf (lnkfile, "\n-e\n");
  fclose (lnkfile);

  if (options.verbose)
    printf ("sdcc: Calling linker...\n");

  buildCmdLine (buffer, port->linker.cmd, srcFileName, NULL, NULL, NULL);
  if (my_system (buffer))
    {
      exit (1);
    }

  if (strcmp (srcFileName, "temp") == 0)
    {
      /* rename "temp.cdb" to "firstRelFile.cdb" */
      char *f = strtok (strdup (relFiles[0]), ".");
      f = strcat (f, ".cdb");
      rename ("temp.cdb", f);
      srcFileName = NULL;
    }
}

/*-----------------------------------------------------------------*/
/* assemble - spawns the assembler with arguments                  */
/*-----------------------------------------------------------------*/
static void
assemble (char **envp)
{
  buildCmdLine (buffer, port->assembler.cmd, srcFileName, NULL, NULL, asmOptions);
  if (my_system (buffer))
    {
      /* either system() or the assembler itself has reported an error
         perror ("Cannot exec assembler");
       */
      exit (1);
    }
}



/*-----------------------------------------------------------------*/
/* preProcess - spawns the preprocessor with arguments       */
/*-----------------------------------------------------------------*/
static int
preProcess (char **envp)
{
  char procDef[128];

  preOutName = NULL;

  if (!options.c1mode)
    {
      /* if using external stack define the macro */
      if (options.useXstack)
	_addToList (preArgv, "-DSDCC_USE_XSTACK");

      /* set the macro for stack autos  */
      if (options.stackAuto)
	_addToList (preArgv, "-DSDCC_STACK_AUTO");

      /* set the macro for stack autos  */
      if (options.stack10bit)
	_addToList (preArgv, "-DSDCC_STACK_TENBIT");

      /* set the macro for large model  */
      switch (options.model)
	{
	case MODEL_LARGE:
	  _addToList (preArgv, "-DSDCC_MODEL_LARGE");
	  break;
	case MODEL_SMALL:
	  _addToList (preArgv, "-DSDCC_MODEL_SMALL");
	  break;
	case MODEL_COMPACT:
	  _addToList (preArgv, "-DSDCC_MODEL_COMPACT");
	  break;
	case MODEL_MEDIUM:
	  _addToList (preArgv, "-DSDCC_MODEL_MEDIUM");
	  break;
	case MODEL_FLAT24:
	  _addToList (preArgv, "-DSDCC_MODEL_FLAT24");
	  break;
	default:
	  werror (W_UNKNOWN_MODEL, __FILE__, __LINE__);
	  break;
	}

      /* standard include path */
      if (!options.nostdinc) {
	_addToList (preArgv, "-I" SDCC_INCLUDE_DIR);
      }

      /* add port (processor information to processor */
      sprintf (procDef, "-DSDCC_%s", port->target);
      _addToList (preArgv, procDef);
      sprintf (procDef, "-D__%s", port->target);
      _addToList (preArgv, procDef);

      if (!preProcOnly)
	preOutName = strdup (tmpnam (NULL));

      if (options.verbose)
	printf ("sdcc: Calling preprocessor...\n");

      buildCmdLine (buffer, _preCmd, fullSrcFileName,
		    preOutName, srcFileName, preArgv);
      if (my_system (buffer))
	{
          // @FIX: Dario Vecchio 03-05-2001
          if (preOutName)
            {
              unlink (preOutName);
              free (preOutName);
            }
          // EndFix
	  exit (1);
	}

      if (preProcOnly)
	exit (0);
    }
  else
    {
      preOutName = fullSrcFileName;
    }

  yyin = fopen (preOutName, "r");
  if (yyin == NULL)
    {
      perror ("Preproc file not found\n");
      exit (1);
    }

  return 0;
}

static void
_findPort (int argc, char **argv)
{
  _validatePorts ();

  argc--;
  while (argc)
    {
      if (!strncmp (*argv, "-m", 2))
	{
	  _setPort (*argv + 2);
	  return;
	}
      argv++;
      argc--;
    }
  /* Use the first in the list */
  port = _ports[0];
}

/*
 * main routine
 * initialises and calls the parser
 */

int
main (int argc, char **argv, char **envp)
{
  /* turn all optimizations off by default */
  memset (&optimize, 0, sizeof (struct optimize));

  /*printVersionInfo (); */

  if (NUM_PORTS==0) {
    fprintf (stderr, "Build error: no ports are enabled.\n");
    exit (1);
  }

  _findPort (argc, argv);
  /* Initalise the port. */
  if (port->init)
    port->init ();

  // Create a default exe search path from the path to the sdcc command



  if (strchr (argv[0], DIR_SEPARATOR_CHAR))
    {
      strcpy (DefaultExePath, argv[0]);
      *(strrchr (DefaultExePath, DIR_SEPARATOR_CHAR)) = 0;
      ExePathList[0] = DefaultExePath;
    }


  setDefaultOptions ();
  parseCmdLine (argc, argv);

  initMem ();

  port->finaliseOptions ();

  /* if no input then printUsage & exit */
  if ((!options.c1mode && !srcFileName && !nrelFiles) || 
      (options.c1mode && !srcFileName && !options.out_name))
    {
      printUsage ();
      exit (0);
    }

  if (srcFileName)
    {
      preProcess (envp);

      initSymt ();
      initiCode ();
      initCSupport ();
      initPeepHole ();

      if (options.verbose)
	printf ("sdcc: Generating code...\n");

      yyparse ();

      if (!fatalError)
	{
	  if (TARGET_IS_PIC) {
	    /* TSD PIC port hack - if the PIC port option is enabled
	       and SDCC is used to generate PIC code, then we will
	       generate .asm files in gpasm's format instead of SDCC's
	       assembler's format
	    */
#if !OPT_DISABLE_PIC
	    picglue ();
#endif
	  } else {
	    glue ();
	  }

	  if (fatalError)
	    {
              // @FIX: Dario Vecchio 03-05-2001
              if (preOutName)
                {
                  if (yyin && yyin != stdin)
                    fclose (yyin);
                  unlink (preOutName);
                  free (preOutName);
                }
              // EndFix
	      return 1;
	    }
	  if (!options.c1mode)
	    {
	      if (options.verbose)
		printf ("sdcc: Calling assembler...\n");
	      assemble (envp);
	    }
	}
      else
	{
          // @FIX: Dario Vecchio 03-05-2001
          if (preOutName)
            {
              if (yyin && yyin != stdin)
                fclose (yyin);
              unlink (preOutName);
              free (preOutName);
            }
          // EndFix
	  return 1;
	}

    }

  closeDumpFiles();

  if (cdbFile)
    fclose (cdbFile);

  if (!options.cc_only &&
      !fatalError &&
      !noAssemble &&
      !options.c1mode &&
      (srcFileName || nrelFiles))
    {
      if (port->linker.do_link)
	port->linker.do_link ();
      else
	linkEdit (envp);
    }

  if (yyin && yyin != stdin)
    fclose (yyin);

  if (preOutName && !options.c1mode)
    {
      unlink (preOutName);
      free (preOutName);
    }

  return 0;

}
