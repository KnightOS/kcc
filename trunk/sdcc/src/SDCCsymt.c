/*-------------------------------------------------------------------------
  SDCCsymt.c - Code file for Symbols table related structures and MACRO's.              
	      Written By -  Sandeep Dutta . sandeep.dutta@usa.net (1998)

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

#define ENABLE_MICHAELH_REGPARM_HACK	0

bucket   *SymbolTab [256]  ;  /* the symbol    table  */
bucket   *StructTab [256]  ;  /* the structure table  */
bucket   *TypedefTab[256]  ;  /* the typedef   table  */
bucket   *LabelTab  [256]  ;  /* the Label     table  */
bucket   *enumTab   [256]  ;  /* enumerated    table  */

extern   struct set *publics;

/*------------------------------------------------------------------*/
/* initSymt () - initialises symbol table related stuff             */
/*------------------------------------------------------------------*/
void  initSymt ()
{
   int i = 0 ;

   for ( i = 0 ; i < 256 ; i++ )
      SymbolTab[i] = StructTab[i] = (void *) NULL ;
   
   
}
/*-----------------------------------------------------------------*/
/* newBucket - allocates & returns a new bucket 		   */
/*-----------------------------------------------------------------*/
bucket	 *newBucket ()
{
	bucket *bp ;
	      
	ALLOC(bp,sizeof(bucket));
	
	return bp ;
}

/*-----------------------------------------------------------------*/
/* hashKey - computes the hashkey given a symbol name              */
/*-----------------------------------------------------------------*/
int hashKey (char *s)
{
    unsigned long key = 0;

    while (*s)
	key += *s++ ;
    return key % 256 ;
}

/*-----------------------------------------------------------------*/
/* addSym - adds a symbol to the hash Table                        */
/*-----------------------------------------------------------------*/
void  addSym ( bucket **stab , 
	       void *sym     , 
	       char *sname   , 
	       int level     , 
	       int block)
{
    int i ;	  /* index into the hash Table */
    bucket   *bp ; /* temp bucket    *	       */
    
    /* the symbols are always added at the head of the list  */
    i = hashKey(sname) ;      
    /* get a free entry */
    ALLOC(bp,sizeof(bucket));
    
    bp->sym = sym ;	/* update the symbol pointer  */
    bp->level = level;   /* update the nest level      */   
    bp->block = block;
    strcpy(bp->name,sname);	/* copy the name into place	*/
    
    /* if this is the first entry */
    if (stab[i] == NULL) {
	bp->prev = bp->next = (void *) NULL ;  /* point to nothing */
	stab[i] = bp ;
    }
    /* not first entry then add @ head of list */
    else {       
	bp->prev      = NULL ;
	stab[i]->prev = bp ;
	bp->next      = stab[i] ;
	stab[i]	      = bp ;
    }
}

/*-----------------------------------------------------------------*/
/* deleteSym - deletes a symbol from the hash Table  entry	   */
/*-----------------------------------------------------------------*/
void  deleteSym ( bucket **stab, void *sym, char *sname)
{
    int i = 0 ;
    bucket *bp ;
    
    i = hashKey(sname) ;
    
    bp = stab[i] ;
    /* find the symbol */
    while (bp) {
	if (bp->sym == sym)  /* found it then break out */
	    break ;	   /* of the loop	      */
	bp = bp->next ;
    }
    
    if (!bp)   /* did not find it */
	return ;
    /* if this is the first one in the chain */
    if ( ! bp->prev ) {
	stab[i] = bp->next ;
	if ( stab[i] ) /* if chain ! empty */
	    stab[i]->prev = (void *) NULL ;
    }
    /* middle || end of chain */
    else {
	if ( bp->next ) /* if not end of chain */
	    bp->next->prev = bp->prev ;
	
	bp->prev->next = bp->next ;
    }
    
}

/*-----------------------------------------------------------------*/
/* findSym - finds a symbol in a table				   */
/*-----------------------------------------------------------------*/
void  *findSym ( bucket **stab, void *sym, char *sname)
{
   bucket *bp ;

   bp = stab[hashKey(sname)] ;
   while (bp)
   {
      if ( bp->sym == sym || strcmp (bp->name,sname) == 0 )
	      break ;
      bp = bp->next ;
   }

   return ( bp ? bp->sym : (void *) NULL ) ;
}

/*-----------------------------------------------------------------*/
/* findSymWithLevel - finds a symbol with a name & level           */
/*-----------------------------------------------------------------*/
void *findSymWithLevel ( bucket **stab, symbol *sym)
{
  bucket *bp ;

  bp = stab[hashKey(sym->name)];

  /**
   **  do the search from the head of the list since the 
   **  elements are added at the head it is ensured that
   ** we will find the deeper definitions before we find
   ** the global ones. we need to check for symbols with 
   ** level <= to the level given, if levels match then block
   ** numbers need to match as well
   **/
  while (bp) {

    if ( strcmp(bp->name,sym->name) == 0 && bp->level <= sym->level) {
      /* if this is parameter then nothing else need to be checked */
      if (((symbol *)(bp->sym))->_isparm)
	return (bp->sym) ;
      /* if levels match then block numbers hsould also match */
      if (bp->level && bp->level == sym->level && bp->block == sym->block )
	      return ( bp->sym );
      /* if levels don't match then we are okay */
      if (bp->level && bp->level != sym->level)
	      return ( bp->sym );
      /* if this is a global variable then we are ok too */
      if (bp->level == 0 )
	      return (bp->sym);
    }
    
    bp = bp->next; 
  }

  return (void *) NULL ;
}

/*-----------------------------------------------------------------*/
/* findSymWithBlock - finds a symbol with name in with a block     */
/*-----------------------------------------------------------------*/
void  *findSymWithBlock ( bucket **stab, symbol *sym, int block)
{
   bucket *bp ;

   bp = stab[hashKey(sym->name)] ;
   while (bp)
   {
      if ( strcmp (bp->name,sym->name) == 0 &&
	   bp->block <= block )
	      break ;
      bp = bp->next ;
   }

   return ( bp ? bp->sym : (void *) NULL ) ;
}

/*------------------------------------------------------------------*/
/* newSymbol () - returns a new pointer to a symbol                 */
/*------------------------------------------------------------------*/
symbol *newSymbol (char *name, int scope )
{
   symbol *sym ;
   
   ALLOC(sym,sizeof(symbol));   

   strcpy(sym->name,name);             /* copy the name    */
   sym->level = scope ;                /* set the level    */
   sym->block = currBlockno ;
   sym->lineDef = yylineno ;    /* set the line number */   
   return sym ;
}

/*------------------------------------------------------------------*/
/* newLink - creates a new link (declarator,specifier)              */
/*------------------------------------------------------------------*/
link  *newLink ()
{
   link *p ;

   ALLOC(p,sizeof(link));

   return p;
}

/*------------------------------------------------------------------*/
/* newStruct - creats a new structdef from the free list            */
/*------------------------------------------------------------------*/
structdef   *newStruct  ( char   *tag   )
{
   structdef  *s;

   ALLOC(s,sizeof(structdef));
   
   strcpy(s->tag,tag) ;                     /* copy the tag            */
   return s ;
}

/*------------------------------------------------------------------*/
/* pointerTypes - do the computation for the pointer types          */
/*------------------------------------------------------------------*/
void pointerTypes (link *ptr, link *type)
{    
    if (IS_SPEC(ptr))
	return ;
    
    /* find the first pointer type */
    while (ptr && !IS_PTR(ptr)) 
	ptr = ptr->next;

    /* could not find it */
    if (!ptr || IS_SPEC(ptr) ||
	DCL_TYPE(ptr) != UPOINTER)
	return ;
	
    /* change the pointer type depending on the 
       storage class of the type */
    if (IS_SPEC(type)) {
	DCL_PTR_CONST(ptr) = SPEC_CONST(type);
	DCL_PTR_VOLATILE(ptr) = SPEC_VOLATILE(type);
	switch (SPEC_SCLS(type)) {
	case S_XDATA:
	    DCL_TYPE(ptr) = FPOINTER;
	    break;
	case S_IDATA:
	    DCL_TYPE(ptr) = IPOINTER ;
	    break;
	case S_PDATA:
	    DCL_TYPE(ptr) = PPOINTER ;
	    break;
	case S_DATA:
	    DCL_TYPE(ptr) = POINTER ;
	    break;
	case S_CODE:
	    DCL_PTR_CONST(ptr) = port->mem.code_ro;
	    DCL_TYPE(ptr) = CPOINTER ;
	    break;
	case S_EEPROM:
	    DCL_TYPE(ptr) = EEPPOINTER;
	    break;
	default:
	    DCL_TYPE(ptr) = GPOINTER;
	    break;
	}
	/* the storage class of type ends here */
	SPEC_SCLS(type) =
	    SPEC_CONST(type) =
	    SPEC_VOLATILE(type) = 0;
    }    
    
    /* now change all the remaining unknown pointers
       to generic pointers */
    while (ptr) {
	if (!IS_SPEC(ptr) && DCL_TYPE(ptr) == UPOINTER)
	    DCL_TYPE(ptr) = GPOINTER;
	ptr = ptr->next;
    }

    /* same for the type although it is highly unlikely that
       type will have a pointer */
    while (type) {
	if (!IS_SPEC(type) && DCL_TYPE(type) == UPOINTER)
	    DCL_TYPE(type) = GPOINTER;
	type = type->next;
    }

}

/*------------------------------------------------------------------*/
/* addDecl - adds a declarator @ the end of a chain                 */
/*------------------------------------------------------------------*/
void  addDecl ( symbol *sym, int type , link *p )
{
    link    *head;
    link    *tail;
    link        *t ;
    
    /* if we are passed a link then set head & tail */
    if ( p )  {
	tail = head = p ;
	while ( tail->next )
	    tail = tail->next ;
    }
    else {
	head = tail = newLink() ;
	DCL_TYPE(head) = type   ;
    }
    
    /* if this is the first entry   */
    if ( !sym->type )  {
	sym->type = head ;
	sym->etype = tail ;
    }
    else   {
	if ( IS_SPEC(sym->etype) && IS_SPEC(head) && head == tail ) {
	    sym->etype = mergeSpec(sym->etype,head);
	}
	else {
	    if ( IS_SPEC(sym->etype) && !IS_SPEC(head) && head == tail ) {
		t = sym->type ;
		while (t->next != sym->etype) t = t->next ;
		t->next = head ;
		tail->next = sym->etype;
	    } else {
		sym->etype->next = head;
		sym->etype   = tail;
	    }
	}
    }
    
    /* if the type is a unknown pointer and has
       a tspec then take the storage class const & volatile
       attribute from the tspec & make it those of this
       symbol */
    if (p &&
	!IS_SPEC(p) && 
	DCL_TYPE(p) == UPOINTER &&
	DCL_TSPEC(p)) {
	if (!IS_SPEC(sym->etype)) {
	    sym->etype = sym->etype->next = newLink();
	    sym->etype->class = SPECIFIER;
	}
	SPEC_SCLS(sym->etype) = SPEC_SCLS(DCL_TSPEC(p));
	SPEC_CONST(sym->etype) = SPEC_CONST(DCL_TSPEC(p));
	SPEC_VOLATILE(sym->etype) = SPEC_VOLATILE(DCL_TSPEC(p));
	DCL_TSPEC(p) = NULL;
    }
    return  ;
}

/*------------------------------------------------------------------*/
/* mergeSpec - merges two specifiers and returns the new one       */
/*------------------------------------------------------------------*/
link  *mergeSpec ( link *dest, link *src )
{
    /* if noun different then src overrides */
    if ( SPEC_NOUN(dest) != SPEC_NOUN(src) && !SPEC_NOUN(dest))
	SPEC_NOUN(dest) = SPEC_NOUN(src) ;
    
    if (! SPEC_SCLS(dest))  /* if destination has no storage class */
	SPEC_SCLS(dest) = SPEC_SCLS(src) ;
    
    /* copy all the specifications  */
    SPEC_LONG(dest) |= SPEC_LONG(src);
    SPEC_SHORT(dest) |= SPEC_SHORT(src);
    SPEC_USIGN(dest) |= SPEC_USIGN(src);
    SPEC_STAT(dest) |= SPEC_STAT(src);
    SPEC_EXTR(dest) |= SPEC_EXTR(src);
    SPEC_ABSA(dest) |= SPEC_ABSA(src);
    SPEC_RENT(dest) |= SPEC_RENT(src);
    SPEC_INTN(dest) |= SPEC_INTN(src);
    SPEC_BANK(dest) |= SPEC_BANK(src);
    SPEC_VOLATILE(dest) |= SPEC_VOLATILE(src);
    SPEC_CRTCL(dest) |= SPEC_CRTCL(src);
    SPEC_ADDR(dest) |= SPEC_ADDR(src);
    SPEC_OCLS(dest)  = SPEC_OCLS(src);
    SPEC_BLEN(dest) |= SPEC_BLEN(src);
    SPEC_BSTR(dest) |= SPEC_BSTR(src);
    SPEC_TYPEDEF(dest) |= SPEC_TYPEDEF(src);
    SPEC_NONBANKED(dest) |= SPEC_NONBANKED(src);

    if ( IS_STRUCT(dest) && SPEC_STRUCT(dest) == NULL )
	SPEC_STRUCT(dest) = SPEC_STRUCT(src);   
    
    return dest ;
}

/*------------------------------------------------------------------*/
/* cloneSpec - copies the entire spec and returns a new spec        */
/*------------------------------------------------------------------*/
link  *cloneSpec ( link *src )
{
    link  *spec ;
    
    /* go thru chain till we find the specifier */
    while ( src && src->class != SPECIFIER )
	src = src->next ;
    
    spec = newLink() ;
    memcpy (spec,src,sizeof(link));
    return spec ;
}

/*------------------------------------------------------------------*/
/* genSymName - generates and returns a name used for anonymous vars*/
/*------------------------------------------------------------------*/
char  *genSymName ( int level )
{
    static   int gCount = 0 ;
    static   char gname[SDCC_NAME_MAX+1] ;
    
    sprintf (gname,"__%04d%04d",level,gCount++);
    return gname ;
}

/*------------------------------------------------------------------*/
/* getSpec - returns the specifier part from a declaration chain    */
/*------------------------------------------------------------------*/
link  *getSpec ( link *p )
{
    link *loop ;
    
    loop = p ;
    while ( p && ! (IS_SPEC(p)))
	p = p->next ;
    
    return p ;
}

/*------------------------------------------------------------------*/
/* newCharLink() - creates an int type                              */
/*------------------------------------------------------------------*/
link  *newCharLink()
{
    link *p;
    
    p = newLink();
    p->class = SPECIFIER ;
    SPEC_NOUN(p) = V_CHAR ;
    
    return p;
}

/*------------------------------------------------------------------*/
/* newFloatLink - a new Float type                                  */
/*------------------------------------------------------------------*/
link *newFloatLink()
{
    link *p;
    
    p = newLink();
    p->class = SPECIFIER ;
    SPEC_NOUN(p) = V_FLOAT ;
    
    return p;
}

/*------------------------------------------------------------------*/
/* newLongLink() - new long type                                    */
/*------------------------------------------------------------------*/
link *newLongLink()
{
    link *p;

    p = newLink();
    p->class = SPECIFIER ;
    SPEC_NOUN(p) = V_INT ;
    SPEC_LONG(p) = 1;
    
    return p;
}

/*------------------------------------------------------------------*/
/* newIntLink() - creates an int type                               */
/*------------------------------------------------------------------*/
link  *newIntLink()
{
    link *p;
    
    p = newLink();
    p->class = SPECIFIER ;
    SPEC_NOUN(p) = V_INT ;
    
    return p;
}

/*------------------------------------------------------------------*/
/* getSize - returns size of a type chain in bits                   */
/*------------------------------------------------------------------*/
unsigned int   getSize ( link *p )
{
    /* if nothing return 0 */
    if ( ! p )
	return 0 ;
    if ( IS_SPEC(p) ) { /* if this is the specifier then */
	switch (SPEC_NOUN(p)) { /* depending on the specifier type */
	case V_INT:
	    return (IS_LONG(p) ? LONGSIZE : ( IS_SHORT(p) ? SHORTSIZE: INTSIZE)) ;
	case V_FLOAT:
	    return FLOATSIZE ;
	case V_CHAR:
	    return CHARSIZE ;
	case V_VOID:
	    return   0 ;
	case V_STRUCT:
	    return SPEC_STRUCT(p)->size ;
	case V_LABEL:
	    return 0 ;
	case V_SBIT:
	    return BITSIZE ;
	case V_BIT:
	    return ((SPEC_BLEN(p) / 8) + (SPEC_BLEN(p) % 8 ? 1 : 0)) ;
	default  :
	    return 0 ;
	}
    }
    
    /* this is a specifier  */
    switch (DCL_TYPE(p))  {
    case FUNCTION:
	return 2;
    case ARRAY:
	return DCL_ELEM(p) * getSize (p->next) ;
    case IPOINTER:
    case PPOINTER:
    case POINTER:
	return ( PTRSIZE ) ;
    case EEPPOINTER:
    case FPOINTER:
    case CPOINTER:
	return ( FPTRSIZE );
    case GPOINTER:	
	return ( GPTRSIZE );
	
    default     :
	return  0 ;
    }
}

/*------------------------------------------------------------------*/
/* bitsForType - returns # of bits required to store this type      */
/*------------------------------------------------------------------*/
unsigned int   bitsForType ( link *p )
{
    /* if nothing return 0 */
    if ( ! p )
	return 0 ;
    
    if ( IS_SPEC(p) ) { /* if this is the specifier then */
	
	switch (SPEC_NOUN(p)) { /* depending on the specifier type */
	case V_INT:
	    return (IS_LONG(p) ? LONGSIZE*8 : ( IS_SHORT(p) ? SHORTSIZE*8: INTSIZE*8)) ;
	case V_FLOAT:
	    return FLOATSIZE*8 ;
	case V_CHAR:
	    return   CHARSIZE*8 ;
	case V_VOID:
	    return   0 ;
	case V_STRUCT:
	    return   SPEC_STRUCT(p)->size*8 ;
	case V_LABEL:
	    return 0 ;
	case V_SBIT:
	    return 1 ;
	case V_BIT:
	    return SPEC_BLEN(p);
	default  :
	    return 0 ;
	}
    }
    
    /* this is a specifier  */
    switch (DCL_TYPE(p))  {
    case FUNCTION:
	return 2;
    case ARRAY:
	return DCL_ELEM(p) * getSize (p->next) *8 ;
    case IPOINTER:
    case PPOINTER:
    case POINTER:
	return ( PTRSIZE * 8) ;
    case EEPPOINTER:
    case FPOINTER:
    case CPOINTER:
	return ( FPTRSIZE * 8);
    case GPOINTER:
	return ( GPTRSIZE * 8);
	
    default     :
	return  0 ;
    }
}

/*------------------------------------------------------------------*/
/* copySymbolChain - copies a symbol chain                          */
/*------------------------------------------------------------------*/
symbol *copySymbolChain (symbol *src)
{
	symbol *dest ;

	if (!src)
		return NULL ;

	dest = copySymbol(src);
	dest->next = copySymbolChain(src->next);
	return dest ;
}

/*------------------------------------------------------------------*/
/* copySymbol - makes a copy of a symbol                            */
/*------------------------------------------------------------------*/
symbol  *copySymbol     (symbol *src)
{
	symbol *dest;

	if (!src )
		return NULL ;

	dest = newSymbol( src->name, src->level );
	memcpy(dest,src,sizeof(symbol));
	dest->level = src->level ;
	dest->block = src->block ;
	dest->ival = copyIlist(src->ival);
	dest->type = copyLinkChain(src->type);
	dest->etype= getSpec(dest->type);
	dest->next = NULL ;
	dest->args = copyValueChain (src->args);
	dest->key = src->key;
	dest->calleeSave = src->calleeSave;
	dest->allocreq = src->allocreq;
	return dest;
}

/*------------------------------------------------------------------*/
/* reverseSyms - reverses the links for a symbol chain      */
/*------------------------------------------------------------------*/
symbol   *reverseSyms ( symbol *sym)
{
   symbol *prev  , *curr, *next ;

   if (!sym)
      return NULL ;

   prev = sym ;
   curr = sym->next ;

   while (curr)
   {
      next = curr->next ;
      curr->next = prev   ;
      prev = curr ;
      curr = next ;
   }
   sym->next = (void *) NULL ;
   return prev ;
}

/*------------------------------------------------------------------*/
/* reverseLink - reverses the links for a type chain        */
/*------------------------------------------------------------------*/
link     *reverseLink ( link *type)
{
   link *prev  , *curr, *next ;

   if (!type)
      return NULL ;

   prev = type ;
   curr = type->next ;

   while (curr)
   {
      next = curr->next ;
      curr->next = prev   ;
      prev = curr ;
      curr = next ;
   }
   type->next = (void *) NULL ;
   return prev ;
}

/*------------------------------------------------------------------*/
/* addSymChain - adds a symbol chain to the symboltable             */
/*------------------------------------------------------------------*/
void addSymChain ( symbol *symHead )
{
  symbol *sym  = symHead ;
  symbol *csym = NULL ;

  for (;sym != NULL ; sym = sym->next ) {

    /* if already exists in the symbol table then check if 
       the previous was an extern definition if yes then   
       then check if the type match, if the types match then 
       delete the current entry and add the new entry      */   
      if ((csym = findSymWithLevel (SymbolTab,sym)) &&
	csym->level == sym->level) {
      
      /* previous definition extern ? */
      if ( IS_EXTERN(csym->etype) ) {
	/* do types match ? */
	if (checkType ( csym->type,sym->type) != 1)
	  /* no then error */
	  werror (E_DUPLICATE,csym->name);

	/* delete current entry */
	deleteSym (SymbolTab, csym, csym->name );
	/* add new entry */
	addSym (SymbolTab, sym, sym->name, sym->level,sym->block);
      } else /* not extern */
	  werror(E_DUPLICATE,sym->name) ;
      continue ;
    }
    
    /* check if previously defined */
    if (csym  && csym->level == sym->level  ) {
      /* if the previous one was declared as extern */
      /* then check the type with the current one         */
      if ( IS_EXTERN(csym->etype) ) {
	if ( checkType (csym->type, sym->type ) <= 0 )
	  werror (W_EXTERN_MISMATCH,csym->name);
      }
    }
    
    addSym (SymbolTab,sym,sym->name,sym->level,sym->block) ;
  }
}


/*------------------------------------------------------------------*/
/* funcInChain - DCL Type 'FUNCTION' found in type chain            */
/*------------------------------------------------------------------*/
int funcInChain (link *lnk)
{
    while (lnk) {
	if (IS_FUNC(lnk))
	    return 1;
	lnk = lnk->next;
    }
    return 0;
}

/*------------------------------------------------------------------*/
/* structElemType - returns the type info of a sturct member        */
/*------------------------------------------------------------------*/
link *structElemType (link *stype, value *id ,value **argsp)
{
    symbol *fields = (SPEC_STRUCT(stype) ? SPEC_STRUCT(stype)->fields : NULL);
    
    if ( ! fields || ! id)
	return NULL ;
       
    /* look for the id */
    while (fields) {
	if (strcmp(fields->rname,id->name) == 0) {
	    if (argsp) {
		*argsp = fields->args;
	    }
	    return copyLinkChain (fields->type) ;
	}
	fields = fields->next ;
    }
    werror(E_NOT_MEMBER,id->name);
    
    return NULL ;
}

/*------------------------------------------------------------------*/
/* getStructElement - returns element of a tructure definition      */
/*------------------------------------------------------------------*/
symbol *getStructElement ( structdef *sdef, symbol *sym)
{
    symbol *field ;
    
    for ( field = sdef->fields ; field ; field = field->next )
	if ( strcmp(field->name,sym->name) == 0)
	    return field ;
    
    werror(E_NOT_MEMBER,sym->name);
    
    return sdef->fields ;
}

/*------------------------------------------------------------------*/
/* compStructSize - computes the size of a structure                */
/*------------------------------------------------------------------*/
int   compStructSize (int su, structdef  *sdef )
{
    int sum = 0 , usum =0;
    int bitOffset = 0 ;
    symbol   *loop ;
    
    /* for the identifiers  */
    loop = sdef->fields ;
    while ( loop ) {

	/* create the internal name for this variable */
	sprintf (loop->rname,"_%s",loop->name);
	loop->offset = ( su == UNION ? sum = 0 : sum ) ;
	SPEC_VOLATILE(loop->etype) |= (su == UNION ? 1 : 0);

	/* if this is a bit field  */
	if (loop->bitVar)       {
	    
	    /* change it to a unsigned bit */
	    SPEC_NOUN(loop->etype) = V_BIT ;
	    SPEC_USIGN(loop->etype) = 1     ;
	    /* check if this fit into the remaining   */
	    /* bits of this byte else align it to the */
	    /* next byte boundary                     */
	    if ((SPEC_BLEN(loop->etype)=loop->bitVar) <= (8 - bitOffset))  {
		SPEC_BSTR(loop->etype) = bitOffset   ;
		sum += (loop->bitVar / 8)  ;
		bitOffset += (loop->bitVar % 8);
	    } 
	    else  /* does not fit */
		{
		    bitOffset = 0 ;
		    loop->offset++;   /* go to the next byte  */
		    sum++         ;
		    SPEC_BSTR(loop->etype) = bitOffset   ;
		    sum += (loop->bitVar / 8)  ;
		    bitOffset += (loop->bitVar % 8);
		}
	    /* if this is the last field then pad */
	    if (!loop->next && bitOffset) {
		bitOffset = 0 ;
		sum++ ;
	    }
	}
	else {
	    checkDecl (loop);
	    sum += getSize (loop->type) ;
	}

	/* if function then do the arguments for it */
	if (funcInChain(loop->type)) {
	    processFuncArgs (loop, 1);
	}

	loop = loop->next ;

	/* if this is not a bitfield but the */
	/* previous one was and did not take */
	/* the whole byte then pad the rest  */
	if ((loop && !loop->bitVar) && bitOffset) {
	    bitOffset = 0 ;
	    sum++ ;
	}
	
	/* if union then size = sizeof larget field */
   	if (su == UNION)
	    usum = max(usum,sum);
    
    }
    
    return (su == UNION ? usum : sum);
}

/*------------------------------------------------------------------*/
/* checkSClass - check the storage class specification              */
/*------------------------------------------------------------------*/
static void  checkSClass ( symbol *sym )
{         
    /* type is literal can happen foe enums change
       to auto */
    if (SPEC_SCLS(sym->etype) == S_LITERAL && !SPEC_ENUM(sym->etype))
	SPEC_SCLS(sym->etype) = S_AUTO;

    /* if sfr or sbit then must also be */
    /* volatile the initial value will be xlated */
    /* to an absolute address */
    if (SPEC_SCLS(sym->etype) == S_SBIT ||
	SPEC_SCLS(sym->etype) == S_SFR ) {
	SPEC_VOLATILE(sym->etype) = 1;
	/* if initial value given */
	if (sym->ival) {
	    SPEC_ABSA(sym->etype) = 1;
	    SPEC_ADDR(sym->etype) =
		(int) list2int(sym->ival);
	    sym->ival = NULL;
	}
    }

    /* if absolute address given then it mark it as
       volatile */
    if (IS_ABSOLUTE(sym->etype))
	SPEC_VOLATILE(sym->etype) = 1;

    /* global variables declared const put into code */
    if (sym->level == 0 && 
	SPEC_SCLS(sym->etype) == S_CONSTANT) {
	SPEC_SCLS(sym->etype) = S_CODE ;
	SPEC_CONST(sym->etype) = 1;
    }

    /* global variable in code space is a constant */
    if (sym->level == 0 && 
	SPEC_SCLS(sym->etype) == S_CODE &&
	port->mem.code_ro )
	SPEC_CONST(sym->etype) = 1;
    

    /* if bit variable then no storage class can be */
    /* specified since bit is already a storage */
    if ( IS_BITVAR(sym->etype)              && 
	 ( SPEC_SCLS(sym->etype) != S_FIXED &&   
	   SPEC_SCLS(sym->etype) != S_SBIT  &&
	   SPEC_SCLS(sym->etype) != S_BIT )
	 ) {
	werror (E_BITVAR_STORAGE,sym->name);
	SPEC_SCLS(sym->etype) = S_FIXED ;
    }
    
    /* extern variables cannot be initialized */
    if (IS_EXTERN(sym->etype) && sym->ival)     {
	werror(E_EXTERN_INIT,sym->name);
	sym->ival = NULL;
    }    
    
    /* if this is an automatic symbol then */
    /* storage class will be ignored and   */
    /* symbol will be allocated on stack/  */
    /* data depending on flag             */
    if ( sym->level                             &&
	 (options.stackAuto || reentrant )      &&
	 ( SPEC_SCLS(sym->etype) != S_AUTO      &&
	   SPEC_SCLS(sym->etype) != S_FIXED     &&
	   SPEC_SCLS(sym->etype) != S_REGISTER  &&
	   SPEC_SCLS(sym->etype) != S_STACK     &&
	   SPEC_SCLS(sym->etype) != S_XSTACK    &&
	   SPEC_SCLS(sym->etype) != S_CONSTANT  )) {

	werror(E_AUTO_ASSUMED,sym->name) ;
	SPEC_SCLS(sym->etype) = S_AUTO   ;
    }
	
    /* automatic symbols cannot be given   */
    /* an absolute address ignore it      */
    if ( sym->level  && 
	 SPEC_ABSA(sym->etype) &&
	 (options.stackAuto || reentrant) )  {
	werror(E_AUTO_ABSA,sym->name);
	SPEC_ABSA(sym->etype) = 0 ;
    }
        
    /* arrays & pointers cannot be defined for bits   */
    /* SBITS or SFRs or BIT                           */
    if ((IS_ARRAY(sym->type) || IS_PTR(sym->type))  &&
	( SPEC_NOUN(sym->etype) == V_BIT          || 
	 SPEC_NOUN(sym->etype) == V_SBIT             ||
	 SPEC_SCLS(sym->etype) == S_SFR              ))
	werror(E_BIT_ARRAY,sym->name);
    
    /* if this is a bit|sbit then set length & start  */
    if (SPEC_NOUN(sym->etype) == V_BIT              ||
	SPEC_NOUN(sym->etype) == V_SBIT             )  {
	SPEC_BLEN(sym->etype) = 1 ;
	SPEC_BSTR(sym->etype) = 0 ;
    }    
    
    /* variables declared in CODE space must have */
    /* initializers if not an extern */
    if (SPEC_SCLS(sym->etype) == S_CODE && 
	sym->ival == NULL               &&
	!sym->level                     &&
	port->mem.code_ro               &&
	!IS_EXTERN(sym->etype)) 
	werror(E_CODE_NO_INIT,sym->name);
    
    /* if parameter or local variable then change */
    /* the storage class to reflect where the var will go */
    if ( sym->level && SPEC_SCLS(sym->etype) == S_FIXED) {
	if ( options.stackAuto || (currFunc && IS_RENT(currFunc->etype)))
	    SPEC_SCLS(sym->etype) = (options.useXstack  ?
				     S_XSTACK : S_STACK ) ;
	else
	    SPEC_SCLS(sym->etype) = (options.useXstack  ?
				     S_XDATA :S_DATA ) ;
    }
}

/*------------------------------------------------------------------*/
/* changePointer - change pointer to functions                      */
/*------------------------------------------------------------------*/
void  changePointer  (symbol  *sym)
{
    link *p ;
    
    /* go thru the chain of declarations   */
    /* if we find a pointer to a function  */
    /* unconditionally change it to a ptr  */
    /* to code area                        */
    for ( p = sym->type ; p ; p = p->next) {
	if ( !IS_SPEC(p) && DCL_TYPE(p) == UPOINTER)
	    DCL_TYPE(p) = GPOINTER ;
	if ( IS_PTR(p) && IS_FUNC(p->next))
	    DCL_TYPE(p) = CPOINTER ;
    }
}

/*------------------------------------------------------------------*/
/* checkDecl - does semantic validation of a declaration                   */
/*------------------------------------------------------------------*/
int checkDecl ( symbol *sym )
{
    
    checkSClass  (sym); /* check the storage class      */
    changePointer(sym);  /* change pointers if required */

    /* if this is an array without any dimension
       then update the dimension from the initial value */
    if (IS_ARRAY(sym->type) && !DCL_ELEM(sym->type))
	DCL_ELEM(sym->type) = getNelements (sym->type,sym->ival);    

    return 0 ;
}

/*------------------------------------------------------------------*/
/* copyLinkChain - makes a copy of the link chain & rets ptr 2 head */
/*------------------------------------------------------------------*/
link  *copyLinkChain ( link *p)
{
    link  *head, *curr , *loop;
    
    curr = p ;
    head = loop = ( curr ? newLink() : (void *) NULL) ;
    while (curr)   {
	memcpy(loop,curr,sizeof(link)) ; /* copy it */
	loop->next = (curr->next ? newLink() : (void *) NULL) ;
	loop = loop->next ;
	curr = curr->next ;
    }
    
    return head ;
}


/*------------------------------------------------------------------*/
/* cleanUpBlock - cleansup the symbol table specified for all the   */
/*                symbols in the given block                        */
/*------------------------------------------------------------------*/
void cleanUpBlock ( bucket **table, int block)
{    
    int i ;
    bucket  *chain;
    
    /* go thru the entire  table  */
    for ( i = 0 ; i < 256; i++ ) {
	for ( chain = table[i]; chain ; chain = chain->next ) {
	    if (chain->block >= block) {
		deleteSym (table,chain->sym,chain->name);                                 
	    }   
	}
    }
}

/*------------------------------------------------------------------*/
/* cleanUpLevel - cleansup the symbol table specified for all the   */
/*                symbols in the given level                        */
/*------------------------------------------------------------------*/
void  cleanUpLevel   (bucket  **table, int level )
{
    int i ;
    bucket  *chain;
    
    /* go thru the entire  table  */
    for ( i = 0 ; i < 256; i++ ) {
	for ( chain = table[i]; chain ; chain = chain->next ) {
	    if (chain->level >= level) {
		deleteSym (table,chain->sym,chain->name);                                 
	    }   
	}
    }
}

/*------------------------------------------------------------------*/
/* computeType - computes the resultant type from two types         */
/*------------------------------------------------------------------*/
link *computeType ( link *type1, link *type2)
{
    link *rType ;
    link *reType;
    link *etype1 = getSpec(type1);
    link *etype2 = getSpec(type2);
    
    /* if one of them is a float then result is a float */
    /* here we assume that the types passed are okay */
    /* and can be cast to one another                */
    /* which ever is greater in size */
    if (IS_FLOAT(etype1) || IS_FLOAT(etype2))
	rType = newFloatLink();
    else
	/* if only one of them is a bit variable
	   then the other one prevails */
	if (IS_BITVAR(etype1) && !IS_BITVAR(etype2))
	    rType = copyLinkChain(type2);
	else
	    if (IS_BITVAR(etype2) && !IS_BITVAR(etype1))
		rType = copyLinkChain(type1);
	    else
		/* if one of them is a pointer then that
		   prevails */
		if (IS_PTR(type1))
		    rType = copyLinkChain(type1);
		else
		    if (IS_PTR(type2))
			rType = copyLinkChain(type2);
		    else
			if (getSize (type1) > getSize(type2) )
			    rType = copyLinkChain(type1);
			else
			    rType = copyLinkChain(type2);
    
    reType = getSpec(rType);
    
    /* if either of them unsigned then make this unsigned */
    if ((SPEC_USIGN(etype1) || SPEC_USIGN(etype2)) && !IS_FLOAT(reType))
	SPEC_USIGN(reType) = 1;
    
    /* if result is a literal then make not so */
    if (IS_LITERAL(reType))
	SPEC_SCLS(reType) = S_REGISTER ;
    
    return rType;
}

/*------------------------------------------------------------------*/
/* checkType - will do type check return 1 if match                 */
/*------------------------------------------------------------------*/
int checkType ( link *dest, link *src )
{
    if ( !dest && !src)
	return 1;
    
    if (dest && !src)
	return 0;
    
    if (src && !dest)
	return 0;
    
    /* if dest is a declarator then */
    if (IS_DECL(dest)) {
	if (IS_DECL(src)) {
	    if (DCL_TYPE(src) == DCL_TYPE(dest))
		return checkType(dest->next,src->next);
	    else
		if (IS_PTR(src) && IS_PTR(dest))
		    return -1;
		else
		    if (IS_PTR(dest) && IS_ARRAY(src))
			return -1;
		    else 
			if (IS_PTR(dest) && IS_FUNC(dest->next) && IS_FUNC(src))
			    return -1 * checkType (dest->next,src) ;
			else
			    return 0;
	}
	else
	    if (IS_PTR(dest) && IS_INTEGRAL(src))
		return -1;
	    else
		return 0;
    }
    
    /* if one of them is a void then ok */
    if (SPEC_NOUN(dest) == V_VOID    &&
	SPEC_NOUN(src)  != V_VOID    )
	return -1 ;
    
    if (SPEC_NOUN(dest) != V_VOID &&
	SPEC_NOUN(src) == V_VOID )
	return -1;
    
    /* char === to short */
    if (SPEC_NOUN(dest) == V_CHAR &&
	SPEC_NOUN(src)  == V_INT  &&
	SPEC_SHORT(src)           )
	return (SPEC_USIGN(src) == SPEC_USIGN(dest) ? 1 : -2);

    if (SPEC_NOUN(src) == V_CHAR &&
	SPEC_NOUN(dest)  == V_INT  &&
	SPEC_SHORT(dest)           )
	return (SPEC_USIGN(src) == SPEC_USIGN(dest) ? 1 : -2);    

    /* if they are both bitfields then if the lengths
       and starts don't match */
    if (IS_BITFIELD(dest) && IS_BITFIELD(src) &&
	(SPEC_BLEN(dest) != SPEC_BLEN(src) ||
	 SPEC_BSTR(dest) != SPEC_BSTR(src)))
	return -1;

    /* it is a specifier */
    if (SPEC_NOUN(dest) != SPEC_NOUN(src))      {
	if (SPEC_USIGN(dest) == SPEC_USIGN(src) &&
	    IS_INTEGRAL(dest) && IS_INTEGRAL(src) &&
	    getSize(dest) == getSize(src))
	    return 1;
	else
	    if (IS_ARITHMETIC(dest) && IS_ARITHMETIC(src))
		return -1;
	else
	    return 0;
    }
    else
	if (IS_STRUCT(dest)) {
	    if (SPEC_STRUCT(dest) != SPEC_STRUCT(src))
		return 0 ;
	    else 
		return 1 ;
	}
    if (SPEC_LONG(dest) != SPEC_LONG(src))
	return -1;
    
    if (SPEC_SHORT(dest) != SPEC_SHORT(src))
	return -1;
    
    if (SPEC_USIGN(dest) != SPEC_USIGN(src))
	return -2;
    
    return 1;   
}

/*------------------------------------------------------------------*/
/* inCalleeSaveList - return 1 if found in calle save list          */
/*------------------------------------------------------------------*/
bool inCalleeSaveList ( char *s)
{
    int i;
    
    for (i = 0 ; options.calleeSaves[i] ; i++ )
	if (strcmp(options.calleeSaves[i],s) == 0)
	    return 1;

    return 0;
}

/*------------------------------------------------------------------*/
/* checkFunction - does all kinds of check on a function            */
/*------------------------------------------------------------------*/
int   checkFunction (symbol   *sym)
{
    symbol *csym ;
    value  *exargs, *acargs ;
    int argCnt = 0 ;
    
    /* if not type then some kind of error */
    if ( !sym->type )
	return 0;
    
    /* if the function has no type then make it return int */
    if ( !sym->type->next )
	sym->type->next = sym->etype = newIntLink();
   
    /* function cannot return aggregate */
    if (IS_AGGREGATE(sym->type->next))   {
	werror(E_FUNC_AGGR,sym->name);
	return 0;      
    }
	   
    /* function cannot return bit */
    if (IS_BITVAR(sym->type->next)) {
	werror(E_FUNC_BIT,sym->name);
	return 0;
    }
    
    /* check if this function is defined as calleeSaves
       then mark it as such */
    sym->calleeSave = inCalleeSaveList(sym->name); 

    /* if interrupt service routine  */
    /* then it cannot have arguments */
    if ( sym->args  && IS_ISR(sym->etype) && !IS_VOID(sym->args->type))   {
	werror(E_INT_ARGS,sym->name);     
	sym->args = NULL ;
    }
    
    if (!(csym = findSym (SymbolTab, sym, sym->name )))
	return 1 ;  /* not defined nothing more to check  */
    
    /* check if body already present */
    if ( csym && csym->fbody )   {
	werror(E_FUNC_BODY,sym->name);
	return 0;
    }
    
    /* check the return value type   */
    if (checkType (csym->type,sym->type) <= 0) {
	werror(E_PREV_DEF_CONFLICT,csym->name,"type") ;
	werror (E_CONTINUE,"previous defintion type ");
	printTypeChain(csym->type,stderr);fprintf(stderr,"\n");
	werror (E_CONTINUE,"current definition type ");
	printTypeChain(sym->type,stderr);fprintf(stderr,"\n");
	return 0;
    }

    if ( SPEC_INTRTN(csym->etype) != SPEC_INTRTN(sym->etype)) {
	werror (E_PREV_DEF_CONFLICT,csym->name,"interrupt");
	return 0;
    }

    if (SPEC_BANK(csym->etype) != SPEC_BANK(sym->etype)) {
	werror (E_PREV_DEF_CONFLICT,csym->name,"using");
	return 0;
    }

    /* compare expected agrs with actual args */
    exargs = csym->args ;
    acargs = sym->args  ;
    
    /* for all the expected args do */
    for (argCnt =  1    ; 
	 exargs && acargs ; 
         exargs = exargs->next, acargs = acargs->next, argCnt++ ) {
	if ( checkType(exargs->type,acargs->type) <= 0) {
	    werror(E_ARG_TYPE,argCnt);
	    return 0;
	}
    }
    
    /* if one them ended we have a problem */
    if ((exargs && !acargs && !IS_VOID(exargs->type)) || 
	(!exargs && acargs && !IS_VOID(acargs->type)))
	werror(E_ARG_COUNT);
    
    /* replace with this defition */
    sym->cdef = csym->cdef;
    deleteSym (SymbolTab,csym,csym->name);
    addSym    (SymbolTab,sym,sym->name,sym->level,sym->block);
    if (IS_EXTERN(csym->etype) && !
	IS_EXTERN(sym->etype))
	addSet(&publics,sym);
    return 1 ;      
}

/*-----------------------------------------------------------------*/
/* processFuncArgs - does some processing with function args       */
/*-----------------------------------------------------------------*/
void  processFuncArgs   (symbol *func, int ignoreName)
{
    value *val ;
    int pNum = 1;   
    

    /* if this function has variable argument list */
    /* then make the function a reentrant one	   */
    if (func->hasVargs)
	SPEC_RENT(func->etype) = 1;

    /* check if this function is defined as calleeSaves
       then mark it as such */
    func->calleeSave = inCalleeSaveList(func->name); 

    val = func->args; /* loop thru all the arguments   */
        
    /* if it is void then remove parameters */
    if (val && IS_VOID(val->type)) {     
	func->args = NULL ;
	return ;
    }

    /* reset regparm for the port */
    (*port->reset_regparms)();
    /* if any of the arguments is an aggregate */
    /* change it to pointer to the same type */
    while (val) {
	/* mark it as a register parameter if
	   the function does not have VA_ARG
	   and as port dictates
	   not inhibited by command line option or #pragma */
	if (!func->hasVargs       && 	    
	    !options.noregparms   &&
	    (*port->reg_parm)(val->type)) {

	    SPEC_REGPARM(val->etype) = 1;
	}

#if ENABLE_MICHAELH_REGPARM_HACK
	/* HACK: pull out later */
	if (
	    (
	     !strcmp(func->name, "memcpy") ||
	     !strcmp(func->name, "strcpy") ||
	     !strcmp(func->name, "strcmp") ||
	     0
	     ) &&
	    port->reg_parm(val->type)) {
	    SPEC_REGPARM(val->etype) = 1;
	}
#endif						
	
	if ( IS_AGGREGATE(val->type)) {
	    /* if this is a structure */
	    /* then we need to add a new link */
	    if (IS_STRUCT(val->type)) {
				/* first lets add DECLARATOR type */
		link *p = val->type ;
		
		werror(W_STRUCT_AS_ARG,val->name);
		val->type = newLink();
		val->type->next = p ;				
	    }
	    
	    /* change to a pointer depending on the */
	    /* storage class specified				*/
	    switch (SPEC_SCLS(val->etype)) {
	    case S_IDATA:
		DCL_TYPE(val->type) = IPOINTER;
		break;
	    case S_PDATA:
		DCL_TYPE(val->type) = PPOINTER;
		break;
	    case S_FIXED:
	    case S_AUTO:
	    case S_DATA:
	    case S_REGISTER:
		DCL_TYPE(val->type) = POINTER ;
		break;
	    case S_CODE:
		DCL_TYPE(val->type) = CPOINTER;
		break;
	    case S_XDATA:
		DCL_TYPE(val->type) = FPOINTER;
		break;
	    case S_EEPROM:
		DCL_TYPE(val->type) = EEPPOINTER;
		break;
	    default :
		DCL_TYPE(val->type) = GPOINTER;
	    }
	    
	    /* is there is a symbol associated then */
	    /* change the type of the symbol as well*/
	    if ( val->sym ) {	       
		val->sym->type = copyLinkChain(val->type);
		val->sym->etype = getSpec(val->sym->type);
	    }
	}
	
	val = val->next ;
	pNum++;
    }
    
    /* if this function is reentrant or */
    /* automatics r 2b stacked then nothing */
    if (IS_RENT(func->etype) || options.stackAuto )
	return ;
    
    val = func->args;
    pNum = 1;
    while (val) {
	
	/* if a symbolname is not given  */
	/* synthesize a variable name */
	if (!val->sym) {

	    sprintf(val->name,"_%s_PARM_%d",func->name,pNum++);
	    val->sym = newSymbol(val->name,1);
	    SPEC_OCLS(val->etype) = (options.model ? xdata : data);
	    val->sym->type = copyLinkChain (val->type);
	    val->sym->etype = getSpec (val->sym->type);
	    val->sym->_isparm = 1;
	    strcpy (val->sym->rname,val->name);	 
	    SPEC_STAT(val->etype) = SPEC_STAT(val->sym->etype) =
		SPEC_STAT(func->etype);
	    addSymChain(val->sym);
   
	}
	else  /* symbol name given create synth name */      {
	    
	    sprintf(val->name,"_%s_PARM_%d",func->name,pNum++);
	    strcpy (val->sym->rname,val->name);
	    val->sym->_isparm = 1;
	    SPEC_OCLS(val->etype) = SPEC_OCLS(val->sym->etype) = 
		(options.model ? xdata : data);
	    SPEC_STAT(val->etype) = SPEC_STAT(val->sym->etype) = 
		SPEC_STAT(func->etype);
	}
	val = val->next ;
    }
}

/*-----------------------------------------------------------------*/
/* isSymbolEqual - compares two symbols return 1 if they match     */
/*-----------------------------------------------------------------*/
int isSymbolEqual (symbol *dest, symbol *src)
{
    /* if pointers match then equal */
    if (dest == src)
	return 1;

    /* if one of them is null then don't match */
    if (!dest || !src)
	return 0;
    
    /* if both of them have rname match on rname */
    if (dest->rname[0] && src->rname[0]) 
	return (!strcmp(dest->rname,src->rname));
    
    /* otherwise match on name */
    return (!strcmp(dest->name,src->name));
}

/*-----------------------------------------------------------------*/ 
/* printTypeChain - prints the type chain in human readable form   */
/*-----------------------------------------------------------------*/ 
void printTypeChain (link *type, FILE *of)
{
    int nlr = 0;

    if (!of) {
	of = stdout;
	nlr = 1;
    }

    while (type) {
	if (IS_DECL(type)) {
	    switch (DCL_TYPE(type)) {
	    case FUNCTION:
		fprintf (of,"function ");
		break;
	    case GPOINTER:
		fprintf (of,"_generic * ");
		if (DCL_PTR_CONST(type))
		    fprintf(of,"const ");
		break;		
	    case CPOINTER:
		fprintf (of,"_code * ");
		if (DCL_PTR_CONST(type))
		    fprintf(of,"const ");
		break;
	    case FPOINTER:
		fprintf (of,"_far * ");
		if (DCL_PTR_CONST(type))
		    fprintf(of,"const ");
		break;
	    case EEPPOINTER:
		fprintf (of,"_eeprom * ");
		if (DCL_PTR_CONST(type))
		    fprintf(of,"const ");
		break;
		
	    case POINTER:
		fprintf (of,"_near * ");
		if (DCL_PTR_CONST(type))
		    fprintf(of,"const ");	
		break;
	    case IPOINTER:
		fprintf (of,"_idata *");
		if (DCL_PTR_CONST(type))
		    fprintf(of,"const ");	
		break; 
	    case PPOINTER:
		fprintf (of,"_pdata *");
		if (DCL_PTR_CONST(type))
		    fprintf(of,"const ");	
		break;
	    case UPOINTER:
		fprintf (of," _unkown *");
		if (DCL_PTR_CONST(type))
		    fprintf(of,"const ");	
		break;
		
	    case ARRAY :
		fprintf (of,"array of ");
		break;
	    }
	} else { 
	    if (SPEC_VOLATILE(type))
		fprintf (of,"volatile "); 
	    if (SPEC_USIGN(type))
		fprintf (of,"unsigned ");
	    
	    switch (SPEC_NOUN(type)) {
	    case V_INT:
		if (IS_LONG(type))
		    fprintf (of,"long ");
		else
		    if (IS_SHORT(type))
			fprintf (of,"short ");
		    else
			fprintf (of,"int ");
		break;

	    case V_CHAR:
		fprintf(of,"char ");
		break;

	    case V_VOID:
		fprintf(of,"void ");
		break;

            case V_FLOAT:
	        fprintf(of,"float ");
		break;

	    case V_STRUCT:
		fprintf(of,"struct %s",SPEC_STRUCT(type)->tag);
		break;
				  
	    case V_SBIT:
		fprintf(of,"sbit ");
		break;

	    case V_BIT:
		fprintf(of,"bit {%d,%d}",SPEC_BSTR(type),SPEC_BLEN(type));
		break;
		
	    default:
		break;
	    }
	}
	type = type->next;
    }
    if (nlr)
	fprintf(of,"\n");
}

/*-----------------------------------------------------------------*/ 
/* cdbTypeInfo - print the type information for debugger           */
/*-----------------------------------------------------------------*/
void cdbTypeInfo (link *type,FILE *of)
{
    fprintf(of,"{%d}",getSize(type));
    while (type) {
	if (IS_DECL(type)) {
	    switch (DCL_TYPE(type)) {
	    case FUNCTION:
		fprintf (of,"DF,");
		break;
	    case GPOINTER:
		fprintf (of,"DG,");		
		break;		
	    case CPOINTER:
		fprintf (of,"DC,");		
		break;
	    case FPOINTER:
		fprintf (of,"DX,");		
		break;
	    case POINTER:
		fprintf (of,"DD,");		
		break;
	    case IPOINTER:
		fprintf (of,"DI,");
		break;
	    case PPOINTER:
		fprintf (of,"DP,");
		break;
	    case EEPPOINTER:
		fprintf (of,"DA,");
		break;
	    case ARRAY :
		fprintf (of,"DA%d,",DCL_ELEM(type));
		break;
	    default:
		break;
	    }
	} else { 
	    switch (SPEC_NOUN(type)) {
	    case V_INT:
		if (IS_LONG(type))
		    fprintf (of,"SL");
		else
		    if (IS_SHORT(type))
			fprintf (of,"SS");
		    else
			fprintf (of,"SI");
		break;

	    case V_CHAR:
		fprintf(of,"SC");
		break;

	    case V_VOID:
		fprintf(of,"SV");
		break;

            case V_FLOAT:
	        fprintf(of,"SF");
		break;

	    case V_STRUCT:
		fprintf(of,"ST%s",SPEC_STRUCT(type)->tag);
		break;
				  
	    case V_SBIT:
		fprintf(of,"SX");
		break;

	    case V_BIT:
		fprintf(of,"SB%d$%d",SPEC_BSTR(type),SPEC_BLEN(type));
		break;

	    default:
		break;
	    }
	    fputs(":",of);
	    if (SPEC_USIGN(type))
		fputs("U",of);
	    else
		fputs("S",of);
	}
	type = type->next;
    }
}
/*-----------------------------------------------------------------*/ 
/* cdbSymbol - prints a symbol & its type information for debugger */
/*-----------------------------------------------------------------*/ 
void cdbSymbol ( symbol *sym, FILE *of, int isStructSym, int isFunc)
{
    memmap *map;

    if (!sym)
	return ;
    if (!of)
	of = stdout;
    
    if (isFunc)
	fprintf(of,"F:");
    else
	fprintf(of,"S:"); /* symbol record */
    /* if this is not a structure symbol then
       we need to figure out the scope information */
    if (!isStructSym) {
	if (!sym->level) {
	    /* global */
	    if (IS_STATIC(sym->etype))
		fprintf(of,"F%s$",moduleName); /* scope is file */
	    else
		fprintf(of,"G$"); /* scope is global */
	}
	else 
	    /* symbol is local */
	    fprintf(of,"L%s$",(sym->localof ? sym->localof->name : "-null-"));
    } else
	fprintf(of,"S$"); /* scope is structure */
    
    /* print the name, & mangled name */
    fprintf(of,"%s$%d$%d(",sym->name,
	    sym->level,sym->block);

    cdbTypeInfo(sym->type,of);
    fprintf(of,"),");
    
    /* print the address space */
    map = SPEC_OCLS(sym->etype);
    fprintf(of,"%c,%d,%d",
	    (map ? map->dbName : 'Z') ,sym->onStack,SPEC_STAK(sym->etype));
    
    /* if assigned to registers then output register names */   
    /* if this is a function then print
       if is it an interrupt routine & interrupt number
       and the register bank it is using */
    if (isFunc)
	fprintf(of,",%d,%d,%d",SPEC_INTRTN(sym->etype),
		SPEC_INTN(sym->etype),SPEC_BANK(sym->etype));
    /* alternate location to find this symbol @ : eg registers
       or spillication */
   
    if (!isStructSym)
	fprintf(of,"\n");
}		    

/*-----------------------------------------------------------------*/ 
/* cdbStruct - print a structure for debugger                      */
/*-----------------------------------------------------------------*/
void cdbStruct ( structdef *sdef,int block,FILE *of,
		 int inStruct, char *tag)
{
    symbol *sym;

    fprintf(of,"T:");
    /* if block # then must have function scope */
    fprintf(of,"F%s$",moduleName);
    fprintf(of,"%s[",(tag ? tag : sdef->tag));
    for (sym=sdef->fields ; sym ; sym = sym->next) {
	fprintf(of,"({%d}",sym->offset);
	    cdbSymbol(sym,of,TRUE,FALSE);
	fprintf(of,")");
    }
    fprintf(of,"]");
    if (!inStruct)
	fprintf(of,"\n");
}

/*------------------------------------------------------------------*/
/* cdbStructBlock - calls struct printing for a blcks               */
/*------------------------------------------------------------------*/
void cdbStructBlock (int block , FILE *of)
{    
    int i ;
    bucket **table = StructTab;
    bucket  *chain;
    
    /* go thru the entire  table  */
    for ( i = 0 ; i < 256; i++ ) {
	for ( chain = table[i]; chain ; chain = chain->next ) {
	    if (chain->block >= block) {
		cdbStruct((structdef *)chain->sym, chain->block ,of,0,NULL);
	    }   
	}
    }
}
		
/*-----------------------------------------------------------------*/ 
/* powof2 - returns power of two for the number if number is pow 2 */
/*-----------------------------------------------------------------*/ 
int powof2 (unsigned long num)
{
    int nshifts = 0;
    int n1s = 0 ;
    
    while (num) {
	if (num & 1) n1s++ ;
	num >>= 1 ;
	nshifts++ ;
    }
    
    if (n1s > 1 || nshifts == 0) return 0;
    return nshifts - 1 ;
}

symbol *__fsadd ;
symbol *__fssub ;
symbol *__fsmul ;
symbol *__fsdiv ;
symbol *__fseq  ;
symbol *__fsneq ;
symbol *__fslt  ;
symbol *__fslteq;
symbol *__fsgt  ;
symbol *__fsgteq;

/* Dims: mul/div/mod, BYTE/WORD/DWORD, SIGNED/UNSIGNED */
symbol *__muldiv[3][3][2];
/* Dims: BYTE/WORD/DWORD SIGNED/UNSIGNED */
link *__multypes[3][2];
/* Dims: to/from float, BYTE/WORD/DWORD, SIGNED/USIGNED */
symbol *__conv[2][3][2];

link *floatType;

#if ENABLE_MICHAELH_REGPARM_HACK
static void _makeRegParam(symbol *sym)
{
    value *val ;

    val = sym->args; /* loop thru all the arguments   */

    /* reset regparm for the port */
    (*port->reset_regparms)();
    while (val) {
	SPEC_REGPARM(val->etype) = 1;
	sym->argStack -= getSize(val->type);
	val = val->next ;
    }
}
#endif

/*-----------------------------------------------------------------*/ 
/* initCSupport - create functions for C support routines          */
/*-----------------------------------------------------------------*/ 
void initCSupport ()
{
    const char *smuldivmod[] = {
	"mul", "div", "mod"
    };
    const char *sbwd[] = {
	"char", "int", "long"
    };
    const char *ssu[] = {
	"s", "u"
    };
	
    int bwd, su, muldivmod, tofrom;

    floatType= newFloatLink();

    for (bwd = 0; bwd < 3; bwd++) {
	link *l;
	switch (bwd) {
	case 0:
	    l = newCharLink();
	    break;
	case 1:
	    l = newIntLink();
	    break;
	case 2:
	    l = newLongLink();
	    break;
	default:
	    assert(0);
	}
	__multypes[bwd][0] = l;
	__multypes[bwd][1] = copyLinkChain(l);
	SPEC_USIGN(__multypes[bwd][1]) = 1;
    }

    __fsadd = funcOfType ("__fsadd", floatType, floatType, 2, options.float_rent);
    __fssub = funcOfType ("__fssub", floatType, floatType, 2, options.float_rent);
    __fsmul = funcOfType ("__fsmul", floatType, floatType, 2, options.float_rent);
    __fsdiv = funcOfType ("__fsdiv", floatType, floatType, 2, options.float_rent);
    __fseq  = funcOfType ("__fseq", CHARTYPE, floatType, 2, options.float_rent);
    __fsneq = funcOfType ("__fsneq", CHARTYPE, floatType, 2, options.float_rent);
    __fslt  = funcOfType ("__fslt", CHARTYPE, floatType, 2, options.float_rent);
    __fslteq= funcOfType ("__fslteq", CHARTYPE, floatType, 2, options.float_rent);
    __fsgt  = funcOfType ("__fsgt", CHARTYPE, floatType, 2, options.float_rent);
    __fsgteq= funcOfType ("__fsgteq", CHARTYPE, floatType, 2, options.float_rent);

    for (tofrom = 0; tofrom < 2; tofrom++) {
	for (bwd = 0; bwd < 3; bwd++) {
	    for (su = 0; su < 2; su++) {
		if (tofrom) {
		    sprintf(buffer, "__fs2%s%s", ssu[su], sbwd[bwd]);
		    __conv[tofrom][bwd][su] = funcOfType(buffer, __multypes[bwd][su], floatType, 1, options.float_rent);
		}
		else {
		    sprintf(buffer, "__%s%s2fs", ssu[su], sbwd[bwd]);
		    __conv[tofrom][bwd][su] = funcOfType(buffer, floatType, __multypes[bwd][su], 1, options.float_rent);
		}
	    }
	}
    }

    for (muldivmod = 0; muldivmod < 3; muldivmod++) {
	for (bwd = 0; bwd < 3; bwd++) {
	    for (su = 0; su < 2; su++) {
		sprintf(buffer, "_%s%s%s", 
			smuldivmod[muldivmod],
			ssu[su],
			sbwd[bwd]);
		__muldiv[muldivmod][bwd][su] = funcOfType(buffer, __multypes[bwd][su], __multypes[bwd][su], 2, options.intlong_rent);
		SPEC_NONBANKED(__muldiv[muldivmod][bwd][su]->etype) = 1;
#if ENABLE_MICHAELH_REGPARM_HACK
		if (bwd < 2) 
		    _makeRegParam(__muldiv[muldivmod][bwd][su]);
#endif
	    }
	}
    }
}
