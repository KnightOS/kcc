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

#include "common.h"
#include "SDCCmacro.h"
#include "SDCCutil.h"
#include "newalloc.h"
#include <sys/stat.h>

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
  *list = strdup (str);
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
  char *pinto = buffer;
  *pinto = '\0';

  while (*pplist)
    {
      strcpy(pinto, *pplist);
      pinto += strlen(*pplist);
      *pinto++ = ' ';
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
  char *pinto = buffer;
  *pinto = '\0';

  while (n--)
    {
      strcpy(pinto, *pplist);
      pinto += strlen(*pplist);
      *pinto++ = ' ';
      pplist++;
    }
  *pinto = '\0';

  return buffer;
}

/** Returns the characters in p2 past the last matching characters in
    p1.  
*/
char *
getStringDifference (char *pinto, const char *p1, const char *p2)
{
  char *p = pinto;

  while (*p1 != '\0' && *p2 != '\0')
    {
      if (*p1++ != *p2++)
        {
          break;
        }
    }
  while (*p2)
    {
      *p++ = *p2++;
    }
  *p = '\0';

  return pinto;
}

/** Given a file with path information in the binary files directory,
    returns what PREFIX must be to get this path.  Used for discovery
    of where SDCC is installed.  Returns NULL if the path is
    impossible.
*/
char *
getPrefixFromBinPath (const char *prel)
{
  strcpy(scratchFileName, prel);
  /* Strip off the /sdcc at the end */
  *strrchr(scratchFileName, DIR_SEPARATOR_CHAR) = '\0';
  /* Compute what the difference between the prefix and the bin dir
     should be. */
  getStringDifference (buffer, PREFIX, BINDIR);

  /* Verify that the path in has the expected suffix */
  if (strlen(buffer) > strlen(scratchFileName))
    {
      /* Not long enough */
      return NULL;
    }
  if (strcmp(buffer, scratchFileName + strlen(scratchFileName) - strlen(buffer)) != 0)
    {
      /* Doesn't match */
      return NULL;
    }

  scratchFileName[strlen(scratchFileName) - strlen(buffer)] = '\0';

  return gc_strdup (scratchFileName);
}

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
  shash_add (&_mainValues, pname, pvalue);
}

void
buildCmdLine2 (char *pbuffer, const char *pcmd)
{
  char *poutcmd = msprintf(_mainValues, pcmd);
  strcpy(pbuffer, poutcmd);
}

void
populateMainValues (const char **ppin)
{
  _mainValues = populateStringHash(ppin);
}

char *
gc_strdup (const char *s)
{
  char *ret;
  ret = Safe_calloc (1, strlen (s) + 1);
  strcpy (ret, s);
  return ret;
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

