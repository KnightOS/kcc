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

/** Definition of the structures used by the options parser.  The port
    may implement one of these for any options it wants parsed
    automatically. 
*/
#ifndef SDCCARGS_H
#define SDCCARGS_H

/** Table of all options supported by all ports.
    This table provides:
      * A reference for all options.
      * An easy way to maintain help for the options.
      * Automatic support for setting flags on simple options.
*/
typedef struct {
    /** The short option character e.g. 'h' for -h.  0 for none. */
    char shortOpt;
    /** Long option e.g. "--help".  Includes the -- prefix.  NULL for
        none. */
    const char *longOpt;
    /** Pointer to an int that will be incremented every time the
        option is encountered.  May be NULL.
    */
    int *pparameter;
    /** Help text to go with this option.  May be NULL. */
    const char *help;
} OPTION;

#endif
