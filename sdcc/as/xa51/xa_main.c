/* Paul's XA51 Assembler, Copyright 1997,2002 Paul Stoffregen (paul@pjrc.com)
 *
 * Paul's XA51 Assembler is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* adapted from the osu8asm project, 1995 */
/* http://www.pjrc.com/tech/osu8/index.html */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xa_main.h"

extern void yyrestart(FILE *new_file);
extern void hexout(int byte, int memory_location, int end);
extern int yyparse();


/* global variables */

FILE *fhex, *fmem, *list_fp;
extern FILE *yyin;
extern char *yytext;
extern char last_line_text[];
struct symbol *sym_list=NULL;
struct target *targ_list=NULL;
int lineno=1, mem=0;   /* mem is location in memory */
int p1=0, p2=0, p3=0;
int expr_result, expr_ok, jump_dest, inst;
int opcode, operand;
char symbol_name[1000];


/* add symbols to list when we find their definition in pass #1 */
/* we will evaluate their values in pass #2, and figure out if */
/* they are branch targets betweem passes 1 and 2.  Every symbol */
/* should appear exactly once in this list, since it can't be redefined */

struct symbol * build_sym_list(char *thename)
{
	struct symbol *new, *p;

/*	printf("  Symbol: %s  Line: %d\n", thename, lineno); */
	new = (struct symbol *) malloc(sizeof(struct symbol));
	new->name = (char *) malloc(strlen(thename)+1);
	strcpy(new->name, thename);
	new->value = 0;
	new->istarget = 0;
	new->isdef = 0;
	new->isbit = 0;
	new->isreg = 0;
	new->line_def = lineno - 1;
	new->next = NULL;
	if (sym_list == NULL) return (sym_list = new);
	p = sym_list;
	while (p->next != NULL) p = p->next;
	p->next = new;
	return (new);
}

int assign_value(char *thename, int thevalue)
{
	struct symbol *p;

	p = sym_list;
 	while (p != NULL) {
		if (!(strcasecmp(thename, p->name))) {
			p->value = thevalue;
			p->isdef = 1;
			return (0);
		}
		p = p->next;
	}
	fprintf(stderr, "Internal Error!  Couldn't find symbol\n");
	exit(1);
}

int mk_bit(char *thename)
{
        struct symbol *p;

        p = sym_list;
        while (p != NULL) {
                if (!(strcasecmp(thename, p->name))) {
                        p->isbit = 1;
                        return (0);
                }
                p = p->next;
        }
        fprintf(stderr, "Internal Error!  Couldn't find symbol\n");
        exit(1);
}


int mk_reg(char *thename)
{
        struct symbol *p;

        p = sym_list;
        while (p != NULL) {
                if (!(strcasecmp(thename, p->name))) {
                        p->isreg = 1;
                        return (0);
                }
                p = p->next;
        }
        fprintf(stderr, "Internal Error!  Couldn't find symbol\n");
        exit(1);
}



int get_value(char *thename)
{
	struct symbol *p;
	p = sym_list;
	while (p != NULL) {
		if (!(strcasecmp(thename, p->name)))
			return (p->value);
		p = p->next;
	}
	fprintf(stderr, "Internal Error!  Couldn't find symbol value\n");
	exit(1);
}
		


/* add every branch target to this list as we find them */
/* ok if multiple entries of same symbol name in this list */

struct target * build_target_list(char *thename)
{
	struct target *new, *p;
	new = (struct target *) malloc(sizeof(struct target));
	new->name = (char *) malloc(strlen(thename)+1);
	strcpy(new->name, thename);
	new->next = NULL;
	if (targ_list == NULL) return (targ_list = new);
	p = targ_list;
	while (p->next != NULL) p = p->next;
	p->next = new;
	return (new);
}

/* figure out which symbols are branch targets */

void flag_targets()
{
	struct symbol *p_sym;
	struct target *p_targ;
	p_targ = targ_list;
	while (p_targ != NULL) {
		p_sym = sym_list;
		while (p_sym != NULL) {
			if (!strcasecmp(p_sym->name, p_targ->name))
				p_sym->istarget = 1;
			p_sym = p_sym->next;
		}
		p_targ = p_targ->next;
	}
}

void print_symbol_table()
{
	struct symbol *p;
	p = sym_list;
	while (p != NULL) {
		printf("Sym:%12s = %5d (%04X)  Def:", \
		  p->name, p->value, p->value);
		if (p->isdef) printf("Yes"); else printf("No ");
		printf("  Bit:");
		if (p->isbit) printf("Yes"); else printf("No ");
		printf("  Target:");
		if (p->istarget) printf("Yes"); else printf("No ");
		printf(" Line %d\n", p->line_def);
		p = p->next;
	}
}

/* check that every symbol is in the table only once */

void check_redefine()
{
	struct symbol *p1, *p2;
	p1 = sym_list;
	while (p1 != NULL) {
		p2 = p1->next;
		while (p2 != NULL) {
			if (!strcasecmp(p1->name, p2->name)) {
				fprintf(stderr, "Error: symbol '%s' redefined on line %d", p1->name, p2->line_def);
				fprintf(stderr, ", first defined on line %d\n", p1->line_def);
			exit(1);
			}
			p2 = p2->next;
		}
		p1 = p1->next;
	}
}

int is_target(char *thename)
{
	struct symbol *p;
	p = sym_list;
	while (p != NULL) {
		if (!strcasecmp(thename, p->name)) return (p->istarget);
		p = p->next;
	}
	return (0);
}

int is_bit(char *thename)
{
        struct symbol *p;
        p = sym_list;
        while (p != NULL) {
                if (!strcasecmp(thename, p->name)) return (p->isbit);
                p = p->next;
        }
        return (0);
}

int is_reg(char *thename)
{
        struct symbol *p;
        p = sym_list;
        while (p != NULL) {
                if (!strcasecmp(thename, p->name)) return (p->isreg);
                p = p->next;
        }
        return (0);
}


int is_def(char *thename)
{
	struct symbol *p;
	p = sym_list;
	while (p != NULL) {
		if (!strcasecmp(thename, p->name) && p->isdef) return(1);
		p = p->next;
	}
	return (0);
}

/* this routine is used to dump a group of bytes to the output */
/* it is responsible for generating the list file and sending */
/* the bytes one at a time to the object code generator */
/* this routine is also responsible for generatine the list file */
/* though is it expected that the lexer has placed all the actual */
/* original text from the line in "last_line_text" */

void out(int *byte_list, int num)
{
	int i, first=1;

	if (num > 0) fprintf(list_fp, "%06X: ", mem);
	else fprintf(list_fp, "\t");

	for (i=0; i<num; i++) {
		hexout(byte_list[i], mem+i, 0);
		if (!first && (i % 4) == 0) fprintf(list_fp, "\t");
		fprintf(list_fp, "%02X", byte_list[i]);
		if ((i+1) % 4 == 0) {
			if (first) fprintf(list_fp, "\t%s\n", last_line_text);
			else fprintf(list_fp, "\n");
			first = 0;
		} else {
			if (i<num-1) fprintf(list_fp, " ");
		}
	}
	if (first) {
		if (num < 3) fprintf(list_fp, "\t");
		fprintf(list_fp, "\t%s\n", last_line_text);
	} else {
		if (num % 4) fprintf(list_fp, "\n");
	}
}


/* add NOPs to align memory location on a valid branch target address */

void pad_with_nop()
{
	static int nops[] = {NOP_OPCODE, NOP_OPCODE, NOP_OPCODE, NOP_OPCODE};
	int num;

	last_line_text[0] = '\0';

	for(num=0; (mem + num) % BRANCH_SPACING; num++) ;
	if (p3) out(nops, num);
	mem += num;
}

/* print branch out of bounds error */

void boob_error()
{
	fprintf(stderr, "Error: branch out of bounds");
	fprintf(stderr, " in line %d\n", lineno);
	exit(1);
}

/* output the jump either direction on carry */
/* jump_dest and mem must have the proper values */

/* 
void do_jump_on_carry()
{
	if (p3) {
		operand = REL4(jump_dest, mem);
		if (operand < 0) {
			operand *= -1;
			operand -= 1;
			if (operand > 15) boob_error();
			out(0x20 + (operand & 15));
		} else {
			if (operand > 15) boob_error();
			out(0x30 + (operand & 15));
		}
	}
}
*/ 

/* turn a string like "10010110b" into an int */

int binary2int(char *str)
{
	register int i, j=1, sum=0;
	
	for (i=strlen(str)-2; i >= 0; i--) {
		sum += j * (str[i] == '1');
		j *= 2;
	}
	return (sum);
}

void print_usage();

/* pass #1 (p1=1) find all symbol defs and branch target names */
/* pass #2 (p2=1) align branch targets, evaluate all symbols */
/* pass #3 (p3=1) produce object code */

int main(int argc, char **argv)
{
	char infilename[200], outfilename[200], listfilename[200];

	if (argc < 2) print_usage();
	strcpy(infilename, argv[1]);
	if(strlen(infilename) > 3) {
		if (strncasecmp(infilename+strlen(infilename)-3, ".xa", 3))
			strcat(infilename, ".xa");
	} else strcat(infilename, ".xa");
	strcpy(outfilename, infilename);
	outfilename[strlen(outfilename)-3] = '\0';
	strcpy(listfilename, outfilename);
	strcat(outfilename, ".hex");
	strcat(listfilename, ".lst");
	yyin = fopen(infilename, "r");
	if (yyin == NULL) {
		fprintf(stderr, "Can't open file '%s'.\n", infilename);
		exit(1);
	}
	fhex = fopen(outfilename, "w");
	if (fhex == NULL) {
		fprintf(stderr, "Can't write file '%s'.\n", outfilename);
		exit(1);
	}
	list_fp = fopen(listfilename, "w");
	if (list_fp == NULL) {
		fprintf(stderr, "Can't write file '%s'.\n", listfilename);
		exit(1);
	}

	/* todo: add a command line option to supress verbose messages */
	printf("\nPaul's XA51 Assembler\n");
	printf("Copyright 1997,2002 Paul Stoffregen\n\n");
	printf("This program is free software; you can redistribute it\n");
	printf("and/or modify it under the terms of the GNU General Public\n");
	printf("License, Version 2, published by the Free Software Foundation\n\n");
	printf("This program is distributed in the hope that it will be useful,\n");
	printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");



	printf("    Building Symbol Table:\n");
	p1 = 1;
	mem = 0;
	yyparse();
	flag_targets();
	// print_symbol_table();
	check_redefine();
	p1 = 0;
	p2 = 1;
	rewind(yyin);
	yyrestart(yyin);
	lineno = 1;
	printf("    Aligning Branch Targets:\n");
	mem = 0;
	yyparse();
	// print_symbol_table();
	p2 = 0;
	p3 = 1;
	rewind(yyin);
	yyrestart(yyin);
	lineno = 1;
	printf("    Generating Object Code:\n");
	mem = 0;
	yyparse();
	fclose(yyin);
	hexout(0, 0, 1);  /* flush and close intel hex file output */
	return 0;
}


void print_usage()
{
	fprintf(stderr, "Usage: xa_asm file\n");
	fprintf(stderr, "   or  xa_asm file.asm\n");
	exit(1);
}

