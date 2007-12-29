/*-------------------------------------------------------------------------
  SDCCasm.h - header file for all types of stuff to support different assemblers.

  Written By - Michael Hope <michaelh@juju.net.nz> 2000

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

#ifndef ASM_PORT_INCLUDE
#define ASM_PORT_INCLUDE

#include "dbuf.h"

void tfprintf (FILE * fp, const char *szFormat, ...);
void tsprintf (char *buffer, size_t len, const char *szFormat, ...);
void dbuf_tprintf (struct dbuf_s *dbuf, const char *szFormat, ...);
void dbuf_tvprintf (struct dbuf_s *dbuf, const char *szFormat, va_list ap);

typedef struct
  {
    const char *szKey;
    const char *szValue;
  }
ASM_MAPPING;

typedef struct _ASM_MAPPINGS ASM_MAPPINGS;

/* PENDING: could include the peephole rules here as well.
 */
struct _ASM_MAPPINGS
  {
    const ASM_MAPPINGS *pParent;
    const ASM_MAPPING *pMappings;
  };

/* The default asxxxx token mapping.
 */
extern const ASM_MAPPINGS asm_asxxxx_mapping;
extern const ASM_MAPPINGS asm_gas_mapping;
extern const ASM_MAPPINGS asm_a390_mapping;
extern const ASM_MAPPINGS asm_xa_asm_mapping;

/** Last entry has szKey = NULL.
 */
void asm_addTree (const ASM_MAPPINGS *pMappings);

char *FileBaseName (char *fileFullName);

const char *printILine (iCode *ic);
const char *printCLine (const char *srcFile, int lineno);
#endif
