/* sdcclib.c: sdcc librarian
   Copyright (C) 2003, Jesus Calvino-Fraga jesusc(at)ece.ubc.ca

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
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define _POSIX_
#include <limits.h>

char ProgName[PATH_MAX];
char LibName[PATH_MAX];
char LibNameTmp[PATH_MAX];
char IndexName[PATH_MAX];
char RelName[PATH_MAX];
char AdbName[PATH_MAX];

#define version "0.1"

#define OPT_ADD_REL 0
#define OPT_EXT_REL 1
#define OPT_DEL_REL 2
#define OPT_DUMP_SYM 3
#define OPT_DUMP_MOD 4

#define MAXLINE 254
#define EQ(A,B) !strcmp((A),(B))
#define NEQ(A,B) strcmp((A),(B))

int action=0;
FILE *lib, *newlib, *rel, *adb, *libindex;
char FLine[MAXLINE+1];
char ModName[MAXLINE+1];
int state=0;

void GetNameFromPath(char * path, char * name)
{
	int i, j;

	for(i=0; path[i]!=0; i++);
	for(; (path[i]!='\\')&&(path[i]!='/')&&(i>=0); i--);
	for(j=0, i++; (path[i]!='.')&&(path[i]!=0); i++, j++) name[j]=path[i];
	name[j]=0;
}

void ChangeExtension(char * path, char * ext)
{
	int i, j;

	for(i=0; path[i]!=0; i++);
	for(; (path[i]!='.')&&(path[i]!='\\')&&(path[i]!='/')&&(i>=0); i--);
	if(path[i]=='.')
	{
	    path[i+1]=0;
        strcat(path, ext);
	}
	else
	{
	    printf("ERROR: Filename '%s' must have an extension\n", path);
	    exit(1);
	}
}

void CleanLine(char * buff)
{
    int j, l;
    l=strlen(buff);
    for(j=0; j<l; j++)
    {
        if((buff[j]=='\n')||(buff[j]=='\r')) buff[j]=0;
    }
}

int set_options (char * opt)
{
	char temp[255];
	int rvalue=0, unknown=0;
	static char Help[] =
	"Usage: %s [-options] library relfile\n\n"
	"available options:\n"
	"a - Adds relfile to library.  If relfile exists, replaces it.\n"
	"d - Deletes relfile from library.\n"
	"e - Extracts relfile from library.\n"
	"s - Dumps symbols of library.\n"
	"m - Dumps modules of library.\n"
	"v - Displays program version.\n"
	"h - This help.\n";

	switch (opt[0])
	{
		default:
		    unknown=1;
		case 'h':
		case '?':
		case 'v':
		    printf("%s: SDCC librarian version %s\n", ProgName, version);
		    printf("by Jesus Calvino-Fraga\n\n");
			if (unknown) printf("Unknown option: %c\n", opt[0]);
		    if (opt[0]=='v') exit(0);
			printf(Help, ProgName);
			exit(1);
		break;
		case 'a':
		    action=OPT_ADD_REL;
		break;
		case 'e':
		    action=OPT_EXT_REL;
		break;
		case 'd':
		    action=OPT_DEL_REL;
		break;
		case 's':
		    action=OPT_DUMP_SYM;
		break;
		case 'm':
		    action=OPT_DUMP_MOD;
		break;
	}
	return (rvalue);
}

void ProcLineOptions (int argc, char **argv)
{
	int cont_par=0;
	int i, j;

	/*Get the program name*/
    GetNameFromPath(argv[0], ProgName);

	for (j=1; j<argc; j++)
	{
		if(argv[j][0]=='-')
		{
			for(i=1; argv[j][i]!=0 ; i++)
				if (set_options(&argv[j][i])) break;
		}
		else
		{
			switch(cont_par)
			{
				case 0:
					cont_par++;
					strcpy(LibName, argv[j]);
				break;

				case 1:
					cont_par++;
					strcpy(RelName, argv[j]);
				break;

				default:
					cont_par++;
				break;
			}
		}
	}

	if ( (cont_par!=2) && (action<OPT_DUMP_SYM) )
	{
		printf("Error: Too %s arguments.\n", cont_par<2?"few":"many");
		set_options("h"); /*Show help and exit*/
	}
	else if ( (cont_par!=1) && (action>=OPT_DUMP_SYM) )
	{
		printf("Error: Too %s arguments.\n", cont_par<1?"few":"many");
		set_options("h"); /*Show help and exit*/
	}
}

void AddRel(void)
{
    int inrel=0;
    long newlibpos, indexsize;
    char symname[MAXLINE+1];
    char c;

    strcpy(LibNameTmp, LibName);
    ChangeExtension(LibNameTmp, "__L");

    strcpy(IndexName, LibName);
    ChangeExtension(IndexName, "__I");

    strcpy(AdbName, RelName);
    ChangeExtension(AdbName, "adb");

    lib=fopen(LibName, "r");

    if(action==OPT_ADD_REL)
    {
        rel=fopen(RelName, "r");
        if(rel==NULL)
        {
            printf("ERROR: Couldn't open file '%s'", RelName);
            fclose(lib);
            return;
        }
    }
    GetNameFromPath(RelName, ModName);

    newlib=fopen(LibNameTmp, "w");
    if(newlib==NULL)
    {
        printf("ERROR: Couldn't create temporary file '%s'", LibNameTmp);
        fclose(lib);
        fclose(rel);
        return;
    }
    fprintf(newlib, "<FILES>\n\n");

    libindex=fopen(IndexName, "w");
    if(libindex==NULL)
    {
        printf("ERROR: Couldn't create temporary file '%s'", IndexName);
        fclose(lib);
        fclose(newlib);
        fclose(rel);
        return;
    }

    if(lib!=NULL) while(!feof(lib))
    {
        FLine[0]=0;
        fgets(FLine, MAXLINE, lib);
        CleanLine(FLine);

        switch(state)
        {
            case 0:
                if(EQ(FLine, "<FILE>"))
                {
                    FLine[0]=0;
                    fgets(FLine, MAXLINE, lib);
                    CleanLine(FLine);
                    if(NEQ(FLine, ModName))
                    {
                        newlibpos=ftell(newlib);
                        fprintf(newlib, "<FILE>\n%s\n", FLine);
                        fprintf(libindex, "<MODULE>\n%s %ld\n", FLine, newlibpos);
                        state++;
                    }
                }                
            break;
            case 1:
                fprintf(newlib, "%s\n", FLine);
                if(EQ(FLine, "</FILE>"))
                {
                    fprintf(newlib, "\n");
                    fprintf(libindex, "</MODULE>\n\n");
                    state=0;
                    inrel=0;
                }
                else if(EQ(FLine, "<REL>")) inrel=1;
                else if(EQ(FLine, "</REL>")) inrel=0;
                if(inrel)
                {
                    if(FLine[0]=='S')
                    {
                        sscanf(FLine, "S %s %c", symname, &c);
                        if(c=='D') fprintf(libindex, "%s\n", symname);
                    }
                }
            break;
        }
    }

    if(action==OPT_ADD_REL)
    {
        newlibpos=ftell(newlib);
        fprintf(newlib, "<FILE>\n%s\n<REL>\n", ModName);
        fprintf(libindex, "<MODULE>\n%s %ld\n", ModName, newlibpos);
        while(!feof(rel))
        {
            FLine[0]=0;
            fgets(FLine, MAXLINE, rel);
            CleanLine(FLine);
            if(strlen(FLine)>0)
            {
                fprintf(newlib, "%s\n", FLine);
            }
            if(FLine[0]=='S')
            {
                sscanf(FLine, "S %s %c", symname, &c);
                if(c=='D') fprintf(libindex, "%s\n", symname);
            }
        }
        fclose(rel);
        fprintf(libindex, "</MODULE>\n");
        fprintf(newlib, "</REL>\n<ADB>\n");
    
        adb=fopen(AdbName, "r");
        if(adb!=NULL)
        {
            while(!feof(rel))
            {
                FLine[0]=0;
                fgets(FLine, MAXLINE, adb);
                CleanLine(FLine);
                if(strlen(FLine)>0)
                {
                    fprintf(newlib, "%s\n", FLine);
                }
            }
            fclose(adb);
        }
        fprintf(newlib, "</ADB>\n</FILE>\n");
    }

    /*Put the temporary files together as a new library file*/
    indexsize=ftell(libindex);
    fflush(libindex);
    fflush(newlib);
    fclose(newlib);
    if(lib!=NULL) fclose(lib);
    fclose(libindex);

    newlib=fopen(LibNameTmp, "r");
    lib=fopen(LibName, "w");
    libindex=fopen(IndexName, "r");

    fprintf(lib, "<SDCCLIB>\n\n");
    fprintf(lib, "<INDEX>\n");

    indexsize+=ftell(lib)+12+14;
    fprintf(lib, "%10ld\n", indexsize);

    while(!feof(libindex))
    {
        FLine[0]=0;
        fgets(FLine, MAXLINE, libindex);
        fprintf(lib, "%s", FLine);
    }
    fprintf(lib, "\n</INDEX>\n\n");

    while(!feof(newlib))
    {
        FLine[0]=0;
        fgets(FLine, MAXLINE, newlib);
        fprintf(lib, "%s", FLine);
    }
    fprintf(lib, "\n</FILES>\n\n");
    fprintf(lib, "</SDCCLIB>\n");

    fclose(newlib);
    fclose(lib);
    fclose(libindex);

    unlink(LibNameTmp);
    unlink(IndexName);
}

void ExtractRel(void)
{
    strcpy(AdbName, RelName);
    ChangeExtension(AdbName, "adb");

    lib=fopen(LibName, "r");
    if(lib==NULL)
    {
        printf("ERROR: Couldn't open file '%s'", LibName);
        return;
    }

    rel=fopen(RelName, "w");
    if(rel==NULL)
    {
        printf("ERROR: Couldn't create file '%s'", RelName);
        fclose(lib);
        return;
    }
    GetNameFromPath(RelName, ModName);

    adb=fopen(AdbName, "w");
    if(adb==NULL)
    {
        printf("ERROR: Couldn't create file '%s'", AdbName);
        fclose(lib);
        fclose(rel);
        return;
    }

    while(!feof(lib))
    {
        if(state==5) break;
        FLine[0]=0;
        fgets(FLine, MAXLINE, lib);
        CleanLine(FLine);

        switch(state)
        {
            case 0:
                if(EQ(FLine, "<FILE>"))
                {
                    FLine[0]=0;
                    fgets(FLine, MAXLINE, lib);
                    CleanLine(FLine);
                    if(EQ(FLine, ModName)) state=1;
                }                
            break;
            case 1:
                if(EQ(FLine, "<REL>")) state=2;
            break;
            case 2:
                if(EQ(FLine, "</REL>"))
                    state=3;
                else
                    fprintf(rel, "%s\n", FLine);
            break;
            case 3:
                if(EQ(FLine, "<ADB>")) state=4;
            break;
            case 4:
                if(EQ(FLine, "</ADB>"))
                    state=5;
                else
                    fprintf(adb, "%s\n", FLine);
            break; 
        }
    }
    
    fclose(rel);
    fclose(lib);
    fclose(adb);
}

void DumpSymbols(void)
{
    lib=fopen(LibName, "r");
    if(lib==NULL)
    {
        printf("ERROR: Couldn't open file '%s'", LibName);
        return;
    }

    while(!feof(lib))
    {
        if(state==3) break;
        FLine[0]=0;
        fgets(FLine, MAXLINE, lib);
        CleanLine(FLine);

        switch(state)
        {
            case 0:
                if(EQ(FLine, "<INDEX>")) state=1;
            break;
            case 1:
                if(EQ(FLine, "<MODULE>"))
                {
                    FLine[0]=0;
                    fgets(FLine, MAXLINE, lib);
                    sscanf(FLine, "%s", ModName);
                    if(action==OPT_DUMP_SYM)
                    {
                        printf("%s.rel:\n", ModName);
                        state=2;
                    }
                    else
                    {
                        printf("%s.rel\n", ModName);
                    }
                }
                else if(EQ(FLine, "</INDEX>")) state=3;
            break;
            case 2:
                if(EQ(FLine, "</MODULE>"))
                {
                    state=1;
                    printf("\n");
                }
                else printf("   %s\n", FLine);
            break;
            default:
                state=3;
            case 3:
            break;
        }
    }
    
    fclose(lib);
}

int main(int argc, char **argv)
{
	ProcLineOptions (argc, argv);

	switch(action)
	{
	    default:
	        action=OPT_ADD_REL;
		case OPT_ADD_REL:
		case OPT_DEL_REL:
		    AddRel();
		break;
		
		case OPT_EXT_REL:
            ExtractRel();
        break;
        
		case OPT_DUMP_SYM:
		case OPT_DUMP_MOD:
		    DumpSymbols();
		break;
	}
	return 0; //Success!!!
}
