/*-------------------------------------------------------------------------
  SDCCutil.c - Small utility functions.

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

#include <math.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#undef TRUE
#undef FALSE
#endif
#include <sys/stat.h>
#include "dbuf.h"
#include "SDCCglobl.h"
#include "SDCCmacro.h"
#include "SDCCutil.h"
#include "newalloc.h"
#include "dbuf_string.h"
#ifndef _WIN32
#include "findme.h"
#endif

#include "version.h"

/** Given an array of name, value string pairs creates a new hash
    containing all of the pairs.
*/
hTab *
populateStringHash (const char **pin)
{
  hTab *pret = NULL;

  while (*pin)
    {
      shash_add (&pret, pin[0], pin[1]);
      pin += 2;
    }

  return pret;
}


/** shell escape string
    returns dynamically allocated string, which should be free-ed
*/
char *
shell_escape (const char *str)
{
#ifdef _WIN32
  /* see http://www.autohotkey.net/~deleyd/parameters/parameters.htm */

  struct dbuf_s dbuf;
  int backshl = 0;
  const char *begin = str;
  char backslbuf[16];
  char *backslp;

  dbuf_init (&dbuf, 128);

  /* opening double quotes */
  dbuf_append_char(&dbuf, '"');
  while (*str)
    {
      if (NULL == begin)
        begin = str;

      if ('\\' == *str)
          ++backshl;
      else
        {
          if ('"' == *str)
            {
              /* append the remaining characters */
              if (str > begin)
                dbuf_append (&dbuf, begin, str - begin);

              /* append additional beckslash */
              ++backshl;

              /* special handling if last chars before double quote are backslashes */
              backslp = backslbuf;
              while (backshl--)
                {
                  *backslp++ = '\\';
                  if (sizeof (backslbuf) == backslp - backslbuf)
                    {
                      dbuf_append (&dbuf, backslbuf, sizeof (backslbuf));
                      backslp = backslbuf;
                    }
                }
              if (backslp > backslbuf)
                dbuf_append (&dbuf, backslbuf, backslp - backslbuf);

              begin = str;
            }
          else if ('%' == *str)
            {
              /* diseble env. variable expansion */
              /* append the remaining characters */
              if (begin && str > begin)
                dbuf_append (&dbuf, begin, str - begin);

              /* special handling if last chars before double quote are backslashes */
              backslp = backslbuf;
              while (backshl--)
                {
                  *backslp++ = '\\';
                  if (sizeof (backslbuf) == backslp - backslbuf)
                    {
                      dbuf_append (&dbuf, backslbuf, sizeof (backslbuf));
                      backslp = backslbuf;
                    }
                }
              if (backslp > backslbuf)
                dbuf_append (&dbuf, backslbuf, backslp - backslbuf);

              /* closing doube quotes */
              backslbuf[0] = '"';
              backslbuf[1] = *str;
              /* re opening double quotes */
              backslbuf[2] = '"';
              dbuf_append (&dbuf, backslbuf, 3);
              begin = NULL;
            }
          backshl = 0;
        }
      ++str;
    }
  /* append the remaining characters */
  if (begin && str > begin)
    dbuf_append (&dbuf, begin, str - begin);

  /* special handling if last chars before double quote are backslashes */
  backslp = backslbuf;
  while (backshl--)
    {
      *backslp++ = '\\';
      if (sizeof (backslbuf) == backslp - backslbuf)
        {
          dbuf_append (&dbuf, backslbuf, sizeof (backslbuf));
          backslp = backslbuf;
        }
    }
  if (backslp > backslbuf)
    dbuf_append (&dbuf, backslbuf, backslp - backslbuf);

  /* closing doube quotes */
  dbuf_append_char (&dbuf, '"');

  return dbuf_detach_c_str (&dbuf);
#else
  struct dbuf_s dbuf;
  const char *s = str;
  const char *begin = str;

  dbuf_init (&dbuf, 128);

  while (*s)
    {
      switch (*s)
        {
        case ' ': case '\t': case '\n':         /* IFS white space */
        case '\'': case '"': case '\\':         /* quoting chars */
        case '|': case '&': case ';':           /* shell metacharacters */
        case '(': case ')': case '<': case '>':
        case '!': case '{': case '}':           /* reserved words */
        case '*': case '[': case '?': case ']': /* globbing chars */
        case '^':
        case '$': case '`':                      /* expansion chars */
        case ',':                                 /* brace expansion */
          /* flush */
          if (s > begin)
            dbuf_append (&dbuf, begin, s - begin);

          dbuf_append_char (&dbuf, '\\');
          begin = s;
          break;

        case '~':                               /* tilde expansion */
          if (s == str || s[-1] == '=' || s[-1] == ':')
            {
              /* flush */
              if (s > begin)
                dbuf_append (&dbuf, begin, s - begin);

              dbuf_append_char (&dbuf, '\\');
              begin = s;
            }
          break;

        case '#':                               /* comment char */
            {
              /* flush */
              if (s > begin)
                dbuf_append (&dbuf, begin, s - begin);

              dbuf_append_char (&dbuf, '\\');
              begin = s;
            }
          /* FALLTHROUGH */
        default:
          break;
        }
      ++s;
    }
  /* flush */
  if (s > begin)
    dbuf_append (&dbuf, begin, s - begin);

  return dbuf_detach_c_str (&dbuf);
#endif
}

/** Prints elements of the set to the file, each element on new line
*/
void
fputStrSet (FILE * fp, set * list)
{
  const char *s;

  for (s = setFirstItem (list); s != NULL; s = setNextItem (list))
    {
      fputs (s, fp);
      fputc ('\n', fp);
    }
}

/** Prepend / append / process given strings to each item of string set.
    The result is in a new string set.
*/
set *
processStrSet (set * list, const char *pre, const char *post, char *(*func)(const char *))
{
  set *new_list = NULL;
  const char *item;
  struct dbuf_s dbuf;

  for (item = setFirstItem (list); item != NULL; item = setNextItem (list))
    {
      dbuf_init (&dbuf, PATH_MAX);

      if (pre)
        dbuf_append_str (&dbuf, pre);

      if (func)
        {
          char *s = func (item);
          
          dbuf_append_str (&dbuf, s);
          Safe_free (s);
        }
      else
        dbuf_append_str (&dbuf, item);

      if (post)
        dbuf_append_str (&dbuf, post);

      addSet (&new_list, dbuf_detach_c_str (&dbuf));
    }

  return new_list;
}

/** Given a set returns a string containing all of the strings seperated
    by spaces. The returned string is on the heap.
*/
const char *
joinStrSet (set * list)
{
  const char *s;
  struct dbuf_s dbuf;

  dbuf_init (&dbuf, PATH_MAX);

  for (s = setFirstItem (list); s != NULL; s = setNextItem (list))
    {
      dbuf_append_str (&dbuf, s);
      dbuf_append_char (&dbuf, ' ');
    }

  return dbuf_detach_c_str (&dbuf);
}

/** Split the path string to the directory and file name (including extension) components.
    The directory component doesn't contain trailing directory separator.
    Returns true if the path contains the directory separator. */
int
dbuf_splitPath (const char *path, struct dbuf_s *dir, struct dbuf_s *file)
{
  const char *p;
  int ret;
  const char *end = &path[strlen (path)];

  for (p = end - 1; p >= path && !IS_DIR_SEPARATOR (*p); --p)
    ;

  ret = p >= path;

  if (NULL != dir)
    {
      int len = p - path;

      if (0 < len)
        dbuf_append (dir, path, len);
    }

  if (NULL != file)
    {
      int len;

      ++p;
      len = end - p;

      if (0 < len)
        dbuf_append (file, p, len);
    }

  return ret;
}

/** Split the path string to the file name (including directory) and file extension components.
    File extension component contains the extension separator.
    Returns true if the path contains the extension separator. */
int
dbuf_splitFile (const char *path, struct dbuf_s *file, struct dbuf_s *ext)
{
  const char *p;
  const char *end = &path[strlen (path)];

  for (p = end - 1; p >= path && !IS_DIR_SEPARATOR (*p) && '.' != *p; --p)
    ;

  if (p < path || '.' != *p)
    {
      dbuf_append_str (file, path);

      return 0;
    }
  else
    {
      if (NULL != file)
        {
          int len = p - path;

          if (0 < len)
            dbuf_append (file, path, len);
        }

      if (NULL != ext)
        {
          int len = end - p;

          if (0 < len)
            dbuf_append (ext, p, len);
        }

      return 1;
    }
}

/** Combine directory and the file name to a path string using the DIR_SEPARATOR_CHAR.
 */
void
dbuf_makePath (struct dbuf_s *path, const char *dir, const char *file)
{
  if (dir != NULL)
    dbuf_append_str (path, dir);

  dbuf_append_char (path, DIR_SEPARATOR_CHAR);

  if (file != NULL)
    dbuf_append_str (path, file);
}

/** Given a file with path information in the binary files directory,
    returns the directory component. Used for discovery of bin
    directory of SDCC installation. Returns NULL if the path is
    impossible.
*/
#ifdef _WIN32
const char *
getBinPath (const char *prel)
{
  struct dbuf_s path;
  const char *p;

  dbuf_init (&path, 128);
  dbuf_splitPath (prel, &path, NULL);

  p = dbuf_c_str (&path);
  if ('\0' != *p)
    return p;
  else
    {
      char module[PATH_MAX];

      dbuf_destroy (&path);

      /* not enough info in prel; do it with module name */
      if (0 != GetModuleFileName (NULL, module, sizeof (module)))
        {
          dbuf_init (&path, 128);

          dbuf_splitPath (module, &path, NULL);
          return dbuf_detach_c_str (&path);
        }
      else
        return NULL;
    }
}
#else
const char *
getBinPath (const char *prel)
{
  const char *ret_path;

  if (NULL != (ret_path = findProgramPath (prel)))
    {
      struct dbuf_s path;

      dbuf_init (&path, 128);

      dbuf_splitPath (ret_path, &path, NULL);
      free ((void *) ret_path);
      return dbuf_detach_c_str (&path);
    }
  else
    return NULL;
}
#endif

/** Returns true if the given path exists.
 */
bool
pathExists (const char *ppath)
{
  struct stat s;

  return stat (ppath, &s) == 0;
}

static hTab *_mainValues;

void
setMainValue (const char *pname, const char *pvalue)
{
  assert (pname);

  shash_add (&_mainValues, pname, pvalue);
}

char *
buildMacros (const char *cmd)
{
  return eval_macros (_mainValues, cmd);
}

char *
buildCmdLine (const char **cmds, const char *p1, const char *p2, const char *p3, set * list)
{
  int first = 1;
  struct dbuf_s dbuf;

  assert (cmds != NULL);

  dbuf_init (&dbuf, 256);

  while (*cmds)
    {
      const char *p, *from, *par;
      int sep = 1;

      from = *cmds;
      cmds++;

      /* See if it has a '$' anywhere - if not, just copy */
      if ((p = strchr (from, '$')))
        {
          /* include first part of cmd */
          if (p != from)
            {
              if (!first && sep)
                dbuf_append_char (&dbuf, ' ');
              dbuf_append (&dbuf, from, p - from);
              sep = 0;
            }
          from = p + 2;

          /* include parameter */
          p++;
          switch (*p)
            {
            case '1':
              par = p1;
              break;

            case '2':
              par = p2;
              break;

            case '3':
              par = p3;
              break;

            case 'l':
              {
                const char *tmp;
                par = NULL;

                if (list != NULL && (tmp = (const char *) setFirstItem (list)) != NULL)
                  {
                    do
                      {
                        if (*tmp != '\0')
                          {
                            if (sep)
                              dbuf_append_char (&dbuf, ' ');    /* seperate it */
                            dbuf_append_str (&dbuf, tmp);
                            tmp++;
                            sep = 1;
                          }
                      }
                    while ((tmp = (const char *) setNextItem (list)) != NULL);
                  }
              }
              break;

            default:
              par = NULL;
              assert (0);
            }

          if (par && *par != '\0')
            {
              if (!first && sep)
                dbuf_append_char (&dbuf, ' ');  /* seperate it */
              dbuf_append_str (&dbuf, par);
              sep = 0;
            }
        }

      /* include the rest of cmd, e.g. ".asm" from "$1.asm" */
      if (*from != '\0')
        {
          if (!first && sep)
            dbuf_append_char (&dbuf, ' ');      /* seperate it */
          dbuf_append_str (&dbuf, from);
          sep = 0;
        }

      first = 0;
    }

  return dbuf_detach_c_str (&dbuf);
}

char *
buildCmdLine2 (const char *pcmd, ...)
{
  va_list ap;
  char *poutcmd;

  assert (pcmd);
  assert (_mainValues);

  va_start (ap, pcmd);

  poutcmd = mvsprintf (_mainValues, pcmd, ap);

  va_end (ap);

  return poutcmd;
}

void
populateMainValues (const char **ppin)
{
  _mainValues = populateStringHash (ppin);
}

/** Returns true if sz starts with the string given in key.
 */
bool
startsWith (const char *sz, const char *key)
{
  return !strncmp (sz, key, strlen (key));
}

/** Removes any newline characters from the string.  Not strictly the
    same as perl's chomp.
*/
void
chomp (char *sz)
{
  char *nl;
  while ((nl = strrchr (sz, '\n')))
    *nl = '\0';
}

hTab *
getRuntimeVariables (void)
{
  return _mainValues;
}

/* strncpy() with guaranteed NULL termination. */
char *
strncpyz (char *dest, const char *src, size_t n)
{
  assert (n > 0);

  --n;
  /* paranoia... */
  if (strlen (src) > n)
    {
      fprintf (stderr, "strncpyz prevented buffer overrun!\n");
    }

  strncpy (dest, src, n);
  dest[n] = 0;
  return dest;
}

/* like strncat() with guaranteed NULL termination
 * The passed size should be the size of the dest buffer, not the number of 
 * bytes to copy.
 */
char *
strncatz (char *dest, const char *src, size_t n)
{
  size_t maxToCopy;
  size_t destLen = strlen (dest);

  assert (n > 0);
  assert (n > destLen);

  maxToCopy = n - destLen - 1;

  /* paranoia... */
  if (strlen (src) + destLen >= n)
    {
      fprintf (stderr, "strncatz prevented buffer overrun!\n");
    }

  strncat (dest, src, maxToCopy);
  dest[n - 1] = 0;
  return dest;
}

/*-----------------------------------------------------------------*/
/* getBuildNumber - return build number                            */
/*-----------------------------------------------------------------*/
const char *
getBuildNumber (void)
{
  return (SDCC_BUILD_NUMBER);
}

/*-----------------------------------------------------------------*/
/* getBuildDate - return build date                                */
/*-----------------------------------------------------------------*/
const char *
getBuildDate (void)
{
  return __DATE__;
}

/*-----------------------------------------------------------------*/
/* getBuildEnvironment - return environment used to build SDCC     */
/*-----------------------------------------------------------------*/
const char *
getBuildEnvironment (void)
{
#ifdef __CYGWIN__
  return "CYGWIN";
#elif defined __MINGW64__
  return "MINGW64";
#elif defined __MINGW32__
  return "MINGW32";
#elif defined __DJGPP__
  return "DJGPP";
#elif defined(_MSC_VER)
  return "MSVC";
#elif defined(__BORLANDC__)
  return "BORLANDC";
#elif defined(__APPLE__)
#if defined(__i386__)
  return "Mac OS X i386";
#elif defined(__x86_64__)
  return "Mac OS X x86_64";
#else
  return "Mac OS X ppc";
#endif
#elif defined(__linux__)
  return "Linux";
#elif defined(__NetBSD__)
  return "NetBSD";
#elif defined(__FreeBSD__)
  return "FreeBSD";
#elif defined(__OpenBSD__)
  return "OpenBSD";
#elif defined(__sun)
#if defined(__i386)
  return "Solaris i386";
#elif defined(__amd64)
  return "Solaris amd64";
#else
  return "Solaris SPARC";
#endif
#else
  return "UNIX";
#endif
}

#if defined(HAVE_VSNPRINTF) || defined(HAVE_VSPRINTF)
size_t
SDCCsnprintf (char *dst, size_t n, const char *fmt, ...)
{
  va_list args;
  int len;

  va_start (args, fmt);

#if defined(HAVE_VSNPRINTF)
  len = vsnprintf (dst, n, fmt, args);
#else
  vsprintf (dst, fmt, args);
  len = strlen (dst) + 1;
#endif

  va_end (args);

  /* on some gnu systems, vsnprintf returns -1 if output is truncated.
   * In the C99 spec, vsnprintf returns the number of characters that 
   * would have been written, were space available.
   */
  if ((len < 0) || (size_t) len >= n)
    {
      fprintf (stderr, "internal error: sprintf truncated.\n");
    }

  return len;
}
#endif

/** Pragma tokenizer
 */
void
init_pragma_token (struct pragma_token_s *token)
{
  dbuf_init (&token->dbuf, 16);
  token->type = TOKEN_UNKNOWN;
}

char *
get_pragma_token (const char *s, struct pragma_token_s *token)
{
  dbuf_set_length (&token->dbuf, 0);

  /* skip leading spaces */
  while ('\n' != *s && isspace (*s))
    ++s;

  if ('\0' == *s || '\n' == *s)
    {
      token->type = TOKEN_EOL;
    }
  else
    {
      char *end;

      long val = strtol (s, &end, 0);

      if (end != s && ('\0' == *end || isspace (*end)))
        {
          token->val.int_val = val;
          token->type = TOKEN_INT;
          dbuf_append (&token->dbuf, s, end - s);
          s = end;
        }
      else
        {
          while ('\0' != *s && !isspace (*s))
            {
              dbuf_append_char (&token->dbuf, *s);
              ++s;
            }

          token->type = TOKEN_STR;
        }
    }

  return (char *) s;
}

const char *
get_pragma_string (struct pragma_token_s *token)
{
  return dbuf_c_str (&token->dbuf);
}

void
free_pragma_token (struct pragma_token_s *token)
{
  dbuf_destroy (&token->dbuf);
}

/*! /fn char hexEscape(char **src)

    /param src Pointer to 'x' from start of hex character value
*/

unsigned char
hexEscape (const char **src)
{
  char *s;
  unsigned long value;

  ++*src;                       /* Skip over the 'x' */

  value = strtoul (*src, &s, 16);

  if (s == *src)
    {
      /* no valid hex found */
      werror (E_INVALID_HEX);
    }
  else
    {
      if (value > 255)
        {
          werror (W_ESC_SEQ_OOR_FOR_CHAR);
        }
    }
  *src = s;

  return (unsigned char) value;
}

/*------------------------------------------------------------------*/
/* universalEscape - process an hex constant of exactly four digits */
/* return the hex value, throw a warning for illegal octal          */
/* adjust src to point at the last proccesed char                   */
/*------------------------------------------------------------------*/

unsigned char
universalEscape (const char **str, unsigned int n)
{
  unsigned int digits;
  unsigned value = 0;
  const char *s = *str;

  if (!options.std_c99)
    {
      werror (W_UNIVERSAL_C99);
    }

  ++*str;                       /* Skip over the 'u'  or 'U' */

  for (digits = 0; digits < n; ++digits)
    {
      if (**str >= '0' && **str <= '7')
        {
          value = (value << 4) + (**str - '0');
          ++*str;
        }
      else if ((**str | 0x20) >= 'a' && (**str | 0x20) <= 'f')
        {
          value = (value << 4) + (**str - ('a' + 10));
          ++*str;
        }
      else
          break;
    }
  if (digits != n)
    {
      werror (E_INVALID_UNIVERSAL, s);
    }
  else if (value > 255)
    {
      werror (W_ESC_SEQ_OOR_FOR_CHAR);
    }

  return value;
}

/*------------------------------------------------------------------*/
/* octalEscape - process an octal constant of max three digits      */
/* return the octal value, throw a warning for illegal octal        */
/* adjust src to point at the last proccesed char                   */
/*------------------------------------------------------------------*/

unsigned char
octalEscape (const char **str)
{
  int digits;
  unsigned value = 0;

  for (digits = 0; digits < 3; ++digits)
    {
      if (**str >= '0' && **str <= '7')
        {
          value = value * 8 + (**str - '0');
          ++*str;
        }
      else
        {
          break;
        }
    }
  if (digits)
    {
      if (value > 255 /* || (**str>='0' && **str<='7') */ )
        {
          werror (W_ESC_SEQ_OOR_FOR_CHAR);
        }
    }
  return value;
}

/*!
  /fn const char *copyStr (const char *src, size_t *size)

  Copies source string to a dynamically allocated buffer interpreting
  escape sequences and special characters

  /param src  Buffer containing the source string with escape sequecnes
  /param size Pointer to loction where the resulting buffer length is written
  /return Dynamically allocated resulting buffer
*/

const char *
copyStr (const char *src, size_t *size)
{
 const char *begin = NULL;
 struct dbuf_s dbuf;

  dbuf_init(&dbuf, 128);

  while (*src)
    {
      if (*src == '\"')
        {
          if (begin)
            {
              /* copy what we have until now */
              dbuf_append (&dbuf, begin, src - begin);
              begin = NULL;
            }
          ++src;
        }
      else if (*src == '\\')
        {
          int c;

          if (begin)
            {
              /* copy what we have until now */
              dbuf_append (&dbuf, begin, src - begin);
              begin = NULL;
            }
          ++src;
          switch (*src)
            {
            case 'n':
              c = '\n';
              break;

            case 't':
              c = '\t';
              break;

            case 'v':
              c = '\v';
              break;

            case 'b':
              c = '\b';
              break;

            case 'r':
              c = '\r';
              break;

            case 'f':
              c = '\f';
              break;

            case 'a':
              c = '\a';
              break;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
              c = octalEscape (&src);
              --src;
              break;

            case 'x':
              c = hexEscape (&src);
              --src;
              break;

            case 'u':
              c = universalEscape (&src, 4);
              --src;
              break;

            case 'U':
              c = universalEscape (&src, 8);
              --src;
              break;

            case '\\':
            case '\?':
            case '\'':
            case '\"':
            default:
              c = *src;
              break;
            }
          dbuf_append_char (&dbuf, c);
          ++src;
        }
      else
        {
          if (!begin)
            begin = src;
          ++src;
        }
    }

  if (begin)
    {
      /* copy what we have until now */
      dbuf_append (&dbuf, begin, src - begin);
      begin = NULL;
    }

  if (size)
    {
      /* include null terminator character
         appended by dbuf_detach_c_str() */
      *size = dbuf_get_length (&dbuf) + 1;
    }

  return dbuf_detach_c_str (&dbuf);
}
