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

#ifdef _WIN32
#include <ctype.h>
#include <windows.h>
#endif
#include <sys/stat.h>
#include "SDCCglobl.h"
#include "SDCCmacro.h"
#include "SDCCutil.h"
#include "newalloc.h"

/** Given an array of name, value string pairs creates a new hash
    containing all of the pairs.
*/
hTab *
populateStringHash(const char **pin)
{
  hTab *pret = NULL;

  while (*pin)
    {
      shash_add (&pret, pin[0], pin[1]);
      pin += 2;
    }

  return pret;
}

/** Given an array of string pointers and another string, adds the
    string to the end of the list.  The end of the list is assumed to
    be the first NULL pointer.
*/
void
addToList (const char **list, const char *str)
{
  /* This is the bad way to do things :) */
  while (*list)
    list++;
  *list = Safe_strdup (str);
  if (!*list)
    {
      werror (E_OUT_OF_MEM, __FILE__, 0);
      exit (1);
    }
  *(++list) = NULL;
}

/** Given an array of string pointers returns a string containing all
    of the strings seperated by spaces.  The returned string is on the
    heap.  The join stops when a NULL pointer is hit.
*/
char *
join(const char **pplist)
{
    buffer[0] = 0;  
    
    while (*pplist)
    {
	strncatz(buffer, *pplist, PATH_MAX);
	strncatz(buffer, " ", PATH_MAX);
	pplist++;
    }

    return buffer;
}

/** Given an array of string pointers, returns a string containing all
    of the strings seperated by spaces.  The returned string is on the
    heap.  n is the number of strings in the list.
*/
char *
joinn(char **pplist, int n)
{
    buffer[0] = 0;  
    
    while (n--)
    {
	strncatz(buffer, *pplist, PATH_MAX);
	strncatz(buffer, " ", PATH_MAX);
	pplist++;
    }

    return buffer;
}

/** Returns TRUE if for the host the two path characters are
    equivalent.
*/
static bool
pathCharsEquivalent(char c1, char c2)
{
#if NATIVE_WIN32
  /* win32 is case insensitive */
  if (tolower(c1) == tolower(c2))
    {
      return TRUE;
    }
  /* And / is equivalent to \ */
  else if (c1 == '/' && c2 == '\\')
    {
      return TRUE;
    }
  else if (c1 == '\\' && c2 == '/')
    {
      return TRUE;
    }
  else
    {
      return FALSE;
    }
#else
  /* Assume a Unix host where they must match exactly. */
  return c1 == c2;
#endif
}

static char
pathCharTransform(char c)
{
#if NATIVE_WIN32
  if (c == '/')
    {
      return DIR_SEPARATOR_CHAR;
    }
  else
    {
      return c;
    }
#else
  return c;
#endif
}

/** Fixes up a potentially mixed path to the proper representation for
    the host.  Fixes up in place.
*/
static char *
fixupPath(char *pin)
{
  char *p = pin;

  while (*p)
    {
      *p = pathCharTransform(*p);
      p++;
    }
  *p = '\0';

  return pin;
}

/** Returns the characters in p2 past the last matching characters in
    p1.  
*/
char *
getPathDifference (char *pinto, const char *p1, const char *p2)
{
  char *p = pinto;

#if NATIVE_WIN32
  /* win32 can have a path at the start. */
  if (strchr(p2, ':'))
    {
      p2 = strchr(p2, ':')+1;
    }
#endif  

  while (*p1 != '\0' && *p2 != '\0')
    {
      if (pathCharsEquivalent(*p1, *p2) == FALSE)
        {
          break;
        }
      p1++;
      p2++;
    }
  while (*p2)
    {
      *p++ = *p2++;
    }
  *p = '\0';

  return fixupPath(pinto);
}


/** Given a file with path information in the binary files directory,
    returns the directory component. Used for discovery of bin
    directory of SDCC installation. Returns NULL if the path is
    impossible.
*/
#ifdef _WIN32
char *
getBinPath(const char *prel)
{
  char *p;
  size_t len;
  static char path[PATH_MAX];
    
  /* try DOS and *nix dir separator on WIN32 */
  if (NULL != (p = strrchr(prel, DIR_SEPARATOR_CHAR)) ||
    NULL != (p = strrchr(prel, UNIX_DIR_SEPARATOR_CHAR))) {
    len = min((sizeof path) - 1, p - prel);
    strncpy(path, prel, len);
    path[len] = '\0';
    return path;
  }
  /* not enough info in prel; do it with module name */
  else if (0 != GetModuleFileName(NULL, path, sizeof path) != 0 &&
    NULL != (p = strrchr(path, DIR_SEPARATOR_CHAR))) {
    *p = '\0';
    return path;
  }
  else
    return NULL;
}
#else
char *
getBinPath(const char *prel)
{
  char *p;
  size_t len;
  static char path[PATH_MAX];
    
  if ((p = strrchr(prel, DIR_SEPARATOR_CHAR)) == NULL)
    return NULL;

  len = min((sizeof path) - 1, p - prel);
  strncpy(path, prel, len);
  path[len] = '\0';

  return path;
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
  assert(pname);

  shash_add (&_mainValues, pname, pvalue);
}

void
buildCmdLine2 (char *pbuffer, const char *pcmd, size_t len)
{
  char *poutcmd;
  assert(pbuffer && pcmd);
  assert(_mainValues);

  poutcmd = msprintf(_mainValues, pcmd);
  strncpyz(pbuffer, poutcmd, len);
}

void
populateMainValues (const char **ppin)
{
  _mainValues = populateStringHash(ppin);
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
getRuntimeVariables(void)
{
  return _mainValues;
}


/* strncpy() with guaranteed NULL termination. */
char *strncpyz(char *dest, const char *src, size_t n)
{
    assert(n > 0);

    --n;
    // paranoia...
    if (strlen(src) > n)
    {
	fprintf(stderr, "strncpyz prevented buffer overrun!\n");
    }
    
    strncpy(dest, src, n);
    dest[n] = 0;
    return dest;
}

/* like strncat() with guaranteed NULL termination
 * The passed size should be the size of the dest buffer, not the number of 
 * bytes to copy.
 */
char *strncatz(char *dest, const char *src, size_t n)
{
    size_t maxToCopy;
    size_t destLen = strlen(dest);
    
    assert(n > 0);
    assert(n > destLen);
    
    maxToCopy = n - destLen - 1;
    
    // paranoia...
    if (strlen(src) + destLen >= n)
    {
	fprintf(stderr, "strncatz prevented buffer overrun!\n");
    }
    
    strncat(dest, src, maxToCopy);
    dest[n - 1] = 0;
    return dest;
}


#if defined(HAVE_VSNPRINTF) || defined(HAVE_VSPRINTF)
size_t SDCCsnprintf(char *dst, size_t n, const char *fmt, ...)
{
  va_list args;
  int len;

  va_start(args, fmt);

# if defined(HAVE_VSNPRINTF)
  len = vsnprintf(dst, n, fmt, args);
# else
  vsprintf(dst, fmt, args);
  len = strlen(dst) + 1;
# endif

  va_end(args);

  /* on some gnu systems, vsnprintf returns -1 if output is truncated.
   * In the C99 spec, vsnprintf returns the number of characters that 
   * would have been written, were space available.
   */
  if ((len < 0) || (size_t) len >= n) {
    fprintf(stderr, "internal error: sprintf truncated.\n");
  }

  return len;
}

#endif
