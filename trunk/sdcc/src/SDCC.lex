/*-----------------------------------------------------------------------
  SDCC.lex - lexical analyser for use with sdcc ( a freeware compiler for
  8/16 bit microcontrollers)
  Written by : Sandeep Dutta . sandeep.dutta@usa.net (1997)
  
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

D        [0-9]
L        [a-zA-Z_]
H        [a-fA-F0-9]
E        [Ee][+-]?{D}+
FS       (f|F|l|L)
IS       (u|U|l|L)*
%{

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
    
char *stringLiteral();
char *currFname;

extern int lineno			;
extern char *filename ;
extern char *fullSrcFileName ;
int   yylineno = 1               ;
void count()                     ;
void comment();
int process_pragma(char *);
#undef yywrap

int yywrap YY_PROTO((void))
{
   return(1);
}
#define TKEYWORD(token) return (isTargetKeyword(yytext) ? token :\
			        check_type(yytext))
char asmbuff[MAX_INLINEASM]			;
char *asmp ;
extern int check_type		();
 extern int isTargetKeyword     ();
extern int checkCurrFile	(char *);
extern int processPragma	(char *);
extern int printListing		(int   );
struct optimize save_optimize ;
struct options  save_options  ;

 enum {
     P_SAVE = 1,
     P_RESTORE ,
     P_NOINDUCTION,
     P_NOINVARIANT,
     P_INDUCTION ,
     P_STACKAUTO ,
     P_NOJTBOUND ,
     P_NOOVERLAY ,
     P_NOGCSE    ,
     P_CALLEE_SAVES,
     P_EXCLUDE   ,
     P_LOOPREV
 };

%}
%x asm
%%
"_asm"        {  count(); asmp = asmbuff ;BEGIN(asm) ;}
<asm>"_endasm" { count()	  	; 
                  *asmp = '\0'				; 
                  strcpy(yylval.yyinline,asmbuff)		; 
                  BEGIN(INITIAL)	;
                  return (INLINEASM)			; }
<asm>.         { *asmp++ = yytext[0]	; }
<asm>\n        { count(); *asmp++ = '\n' ;}
"/*"	       { comment(); }
"at"	       { count(); TKEYWORD(AT)  ; }
"auto"	       { count(); return(AUTO); }
"bit"	       { count(); TKEYWORD(BIT) ; }
"break"        { count(); return(BREAK); }
"case"         { count(); return(CASE); }
"char"         { count(); return(CHAR); }
"code"         { count(); TKEYWORD(CODE); }
"const"        { count(); return(CONST); }
"continue"     { count(); return(CONTINUE); }
"critical"     { count(); TKEYWORD(CRITICAL); } 
"data"	       { count(); TKEYWORD(DATA);   }
"default"      { count(); return(DEFAULT); }
"do"           { count(); return(DO); }
"double"       { count(); werror(W_DOUBLE_UNSUPPORTED);return(FLOAT); }
"else"         { count(); return(ELSE); }
"enum"         { count(); return(ENUM); }
"extern"       { count(); return(EXTERN); }
"far"          { count(); TKEYWORD(XDATA);  }
"eeprom"       { count(); TKEYWORD(EEPROM);  }
"float"        { count(); return(FLOAT); }
"for"          { count(); return(FOR); }
"goto"	       { count(); return(GOTO); }
"idata"        { count(); TKEYWORD(IDATA);}
"if"           { count(); return(IF); }
"int"          { count(); return(INT); }
"interrupt"    { count(); return(INTERRUPT);}
"long"	       { count(); return(LONG); }
"near"	       { count(); TKEYWORD(DATA);}
"pdata"        { count(); TKEYWORD(PDATA); }
"reentrant"    { count(); TKEYWORD(REENTRANT);}
"register"     { count(); return(REGISTER); }
"return"       { count(); return(RETURN); }
"sfr"	       { count(); TKEYWORD(SFR)	; }
"sbit"	       { count(); TKEYWORD(SBIT)	; }
"short"        { count(); return(SHORT); }
"signed"       { count(); return(SIGNED); }
"sizeof"       { count(); return(SIZEOF); }
"static"       { count(); return(STATIC); }
"struct"       { count(); return(STRUCT); }
"switch"       { count(); return(SWITCH); }
"typedef"      { count(); return(TYPEDEF); }
"union"        { count(); return(UNION); }
"unsigned"     { count(); return(UNSIGNED); }
"void"         { count(); return(VOID); }
"volatile"     { count(); return(VOLATILE); }
"using"        { count(); TKEYWORD(USING); }
"while"        { count(); return(WHILE); }
"xdata"        { count(); TKEYWORD(XDATA); }
"_data"	       { count(); TKEYWORD(_NEAR); }
"_code"	       { count(); TKEYWORD(_CODE); }
"_eeprom"      { count(); TKEYWORD(_EEPROM); }
"_generic"     { count(); TKEYWORD(_GENERIC); }
"_near"	       { count(); TKEYWORD(_NEAR); }
"_xdata"       { count(); TKEYWORD(_XDATA);}
"_pdata"       { count(); TKEYWORD(_PDATA); }
"_idata"       { count(); TKEYWORD(_IDATA); }
"..."	       { count(); return(VAR_ARGS);}
{L}({L}|{D})*  { count(); return(check_type()); }
0[xX]{H}+{IS}? { count(); yylval.val = constVal(yytext); return(CONSTANT); }
0{D}+{IS}?     { count(); yylval.val = constVal(yytext); return(CONSTANT); }
{D}+{IS}?      { count(); yylval.val = constVal(yytext); return(CONSTANT); }
'(\\.|[^\\'])+' { count();yylval.val = charVal (yytext); return(CONSTANT); }
{D}+{E}{FS}?   { count(); yylval.val = constFloatVal(yytext);return(CONSTANT); }
{D}*"."{D}+({E})?{FS}?  { count(); yylval.val = constFloatVal(yytext);return(CONSTANT); }
{D}+"."{D}*({E})?{FS}?	{ count(); yylval.val = constFloatVal(yytext);return(CONSTANT); }
\"             { count(); yylval.val=strVal(stringLiteral()); return(STRING_LITERAL);}
">>=" { count(); yylval.yyint = RIGHT_ASSIGN ; return(RIGHT_ASSIGN); }
"<<=" { count(); yylval.yyint = LEFT_ASSIGN  ; return(LEFT_ASSIGN) ; }
"+="  { count(); yylval.yyint = ADD_ASSIGN   ; return(ADD_ASSIGN)  ; }
"-="  { count(); yylval.yyint = SUB_ASSIGN   ; return(SUB_ASSIGN)  ; }
"*="  { count(); yylval.yyint = MUL_ASSIGN   ; return(MUL_ASSIGN)  ; }
"/="  { count(); yylval.yyint = DIV_ASSIGN   ; return(DIV_ASSIGN)  ; }
"%="  { count(); yylval.yyint = MOD_ASSIGN   ; return(MOD_ASSIGN)  ; }
"&="  { count(); yylval.yyint = AND_ASSIGN   ; return(AND_ASSIGN)  ; }
"^="  { count(); yylval.yyint = XOR_ASSIGN   ; return(XOR_ASSIGN)  ; }
"|="  { count(); yylval.yyint = OR_ASSIGN    ; return(OR_ASSIGN)   ; }
">>"           { count(); return(RIGHT_OP); }
"<<"           { count(); return(LEFT_OP); }
"++"           { count(); return(INC_OP); }
"--"           { count(); return(DEC_OP); }
"->"           { count(); return(PTR_OP); }
"&&"           { count(); return(AND_OP); }
"||"           { count(); return(OR_OP); }
"<="           { count(); return(LE_OP); }
">="           { count(); return(GE_OP); }
"=="           { count(); return(EQ_OP); }
"!="           { count(); return(NE_OP); }
";"            { count(); return(';'); }
"{"	       { count(); NestLevel++ ;  return('{'); }
"}"	       { count(); NestLevel--; return('}'); }
","            { count(); return(','); }
":"            { count(); return(':'); }
"="            { count(); return('='); }
"("            { count(); return('('); }
")"            { count(); return(')'); }
"["            { count(); return('['); }
"]"            { count(); return(']'); }
"."            { count(); return('.'); }
"&"            { count(); return('&'); }
"!"            { count(); return('!'); }
"~"            { count(); return('~'); }
"-"            { count(); return('-'); }
"+"            { count(); return('+'); }
"*"            { count(); return('*'); }
"/"            { count(); return('/'); }
"%"            { count(); return('%'); }
"<"            { count(); return('<'); }
">"            { count(); return('>'); }
"^"            { count(); return('^'); }
"|"            { count(); return('|'); }
"?"            { count(); return('?'); }
^#line.*"\n"	   { count(); checkCurrFile(yytext); }
^#pragma.*"\n"   { count(); process_pragma(yytext); }

^[^(]+"("[0-9]+") : error"[^\n]+ { werror(E_PRE_PROC_FAILED,yytext);count(); }
^[^(]+"("[0-9]+") : warning"[^\n]+ { werror(W_PRE_PROC_WARNING,yytext);count(); }
"\r\n"		   { count(); }
"\n"		   { count(); }
[ \t\v\f]      { count(); }
.			   { count()	; }
%%
   
int checkCurrFile ( char *s)
{
    char lineNum[10]			;
    int  lNum				;
    char *tptr				;
       
    /* first check if this is a #line */
    if ( strncmp(s,"#line",5) )
	return  0				;
    
    /* get to the line number */
    while (!isdigit(*s))
	s++ ;
    tptr = lineNum ;
    while (isdigit(*s))
	*tptr++ = *s++ ;
    *tptr = '\0'; 
    sscanf(lineNum,"%d",&lNum);
    
    /* now see if we have a file name */
    while (*s != '\"' && *s) 
	s++ ;
    
    /* if we don't have a filename then */
    /* set the current line number to   */
    /* line number if printFlag is on   */
    if (!*s) {		
	yylineno = lNum ;
	return 0;
    }
    
    /* if we have a filename then check */
    /* if it is "standard in" if yes then */
    /* get the currentfile name info    */
    s++ ;

    if ( strncmp(s,fullSrcFileName,strlen(fullSrcFileName)) == 0) {
	    yylineno = lNum - 2;					
	    currFname = fullSrcFileName ;
    }  else {
	char *sb = s;
	/* mark the end of the filename */
	while (*s != '"') s++;
	*s = '\0';
	ALLOC_ATOMIC(currFname,strlen(sb)+1);
	strcpy(currFname,sb);
	yylineno = lNum - 2;
    }
    filename = currFname ;
    return 0;
}
    
void comment()
{
	char c, c1;

loop:
	while ((c = input()) != '*' && c != 0)
		if ( c == '\n')
			yylineno++ ;

	if ((c1 = input()) != '/' && c != 0)  {
		if ( c1 == '\n' )
			yylineno++ ;

		unput(c1);
		goto loop;
   }

}
   
   

int column = 0;
int plineIdx=0;

void count()
{
	int i;
	for (i = 0; yytext[i] != '\0'; i++)   {				
		if (yytext[i] == '\n')      {         
		   column = 0;
		   lineno = ++yylineno ;
		}
		else 
			if (yytext[i] == '\t')
				column += 8 - (column % 8);
			else
				column++;
   }
         
   /* ECHO; */
}

int check_type()
{
	/* check if it is in the typedef table */
	if (findSym(TypedefTab,NULL,yytext)) {
		strcpy(yylval.yychar,yytext);
		return (TYPE_NAME) ;
	}
	else   {
		strcpy (yylval.yychar,yytext);
		return(IDENTIFIER);
	}
}

char strLitBuff[2048]			;

char *stringLiteral ()
{
       int ch;
       char *str = strLitBuff			;
       
       *str++ = '\"'			;
       /* put into the buffer till we hit the */
       /* first \" */
       while (1) {

          ch = input()			;
          if (!ch) 	    break	; /* end of input */
	  /* if it is a \ then everything allowed */
	  if (ch == '\\') {
	     *str++ = ch     ; /* backslash in place */
	     *str++ = input()		; /* following char in place */
	     continue			;      /* carry on */
	     }
	     
	 /* if new line we have a new line break */
	 if (ch == '\n') break		;
	 
	 /* if this is a quote then we have work to do */
	 /* find the next non whitespace character     */
	 /* if that is a double quote then carry on    */
	 if (ch == '\"') {
	 
	     while ((ch = input()) && isspace(ch)) ;
	     if (!ch) break		; 
	     if (ch != '\"') {
	          unput(ch)			;
		  break			;
		  }
		  
		  continue		;
        }
	*str++  = ch;	  
     }  
     *str++ = '\"'			;
     *str = '\0';
     return strLitBuff			;
}

void doPragma (int op, char *cp)
{
    switch (op) {
    case P_SAVE:
	memcpy(&save_options,&options,sizeof(options));
	memcpy(&save_optimize,&optimize,sizeof(optimize));
	break;
    case P_RESTORE:
	memcpy(&options,&save_options,sizeof(options));
	memcpy(&optimize,&save_optimize,sizeof(optimize));
	break;
    case P_NOINDUCTION:
	optimize.loopInduction = 0 ;
	break;
    case P_NOINVARIANT:
	optimize.loopInvariant = 0 ;
	break;
    case P_INDUCTION:
        optimize.loopInduction = 1 ;
        break;
    case P_STACKAUTO:
	options.stackAuto = 1;
	break;
    case P_NOJTBOUND:
	optimize.noJTabBoundary = 1;
	break;
    case P_NOGCSE:
	optimize.global_cse = 0;
	break;
    case P_NOOVERLAY:
	options.noOverlay = 1;
	break;
    case P_CALLEE_SAVES:
	{
	    int i=0;
	    /* append to the functions already listed
	       in callee-saves */
	    for (; options.calleeSaves[i] ;i++);
	    parseWithComma(&options.calleeSaves[i],strdup(cp));
	}
	break;
    case P_EXCLUDE:
	parseWithComma(options.excludeRegs,strdup(cp));
	break;
    case P_LOOPREV:
	optimize.noLoopReverse = 1;
	break;
    }
}

int process_pragma(char *s)
{
    char *cp ;
    /* find the pragma */
    while (strncmp(s,"#pragma",7))
	s++;
    s += 7;
    
    /* look for the directive */
    while(isspace(*s)) s++;

    cp = s;
    /* look for the end of the directive */
    while ((! isspace(*s)) && 
	   (*s != '\n')) 
	s++ ;    

    /* now compare and do what needs to be done */
    if (strncmp(cp,PRAGMA_SAVE,strlen(PRAGMA_SAVE)) == 0) {
	doPragma(P_SAVE,cp+strlen(PRAGMA_SAVE));
	return 0;
    }

    if (strncmp(cp,PRAGMA_RESTORE,strlen(PRAGMA_RESTORE)) == 0) {
	doPragma (P_RESTORE,cp+strlen(PRAGMA_RESTORE));
	return 0;
    }

    if (strncmp(cp,PRAGMA_NOINDUCTION,strlen(PRAGMA_NOINDUCTION)) == 0) {
	doPragma (P_NOINDUCTION,cp+strlen(PRAGMA_NOINDUCTION))	;
	return 0;
    }

    if (strncmp(cp,PRAGMA_NOINVARIANT,strlen(PRAGMA_NOINVARIANT)) == 0) {
	doPragma (P_NOINVARIANT,NULL)	;
	return 0;
    }

    if (strncmp(cp,PRAGMA_INDUCTION,strlen(PRAGMA_INDUCTION)) == 0) {
	doPragma (P_INDUCTION,NULL)	;
	return 0;
    }

    if (strncmp(cp,PRAGMA_STACKAUTO,strlen(PRAGMA_STACKAUTO)) == 0) {
	doPragma (P_STACKAUTO,NULL);
	return 0;
    }

    if (strncmp(cp,PRAGMA_NOJTBOUND,strlen(PRAGMA_NOJTBOUND)) == 0) {
	doPragma (P_NOJTBOUND,NULL);
	return 0;
    }

    if (strncmp(cp,PRAGMA_NOGCSE,strlen(PRAGMA_NOGCSE)) == 0) {
	doPragma (P_NOGCSE,NULL);
	return 0;
    }

    if (strncmp(cp,PRAGMA_NOOVERLAY,strlen(PRAGMA_NOOVERLAY)) == 0) {
	doPragma (P_NOOVERLAY,NULL);
	return 0;
    }
    
    if (strncmp(cp,PRAGMA_CALLEESAVES,strlen(PRAGMA_CALLEESAVES)) == 0) {
	doPragma(P_CALLEE_SAVES,cp+strlen(PRAGMA_CALLEESAVES));
	return 0;
    }
    
    if (strncmp(cp,PRAGMA_EXCLUDE,strlen(PRAGMA_EXCLUDE)) == 0) {
	doPragma(P_EXCLUDE,cp+strlen(PRAGMA_EXCLUDE));
	return 0;
    }

    if (strncmp(cp,PRAGMA_NOLOOPREV,strlen(PRAGMA_NOLOOPREV)) == 0) {
	doPragma(P_EXCLUDE,NULL);
	return 0;
    }

    werror(W_UNKNOWN_PRAGMA,cp);
    return 0;
}

/* will return 1 if the string is a part
   of a target specific keyword */
int isTargetKeyword(char *s)
{
    int i;
    
    if (port->keywords == NULL)
	return 0;
    for ( i = 0 ; port->keywords[i] ; i++ ) {
	if (strcmp(port->keywords[i],s) == 0)
	    return 1;
    }
    
    return 0;
}
