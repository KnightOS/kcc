/*
 * Simulator of microcontrollers (set.h)
 *
 * Copyright (C) 1999,99 Drotos Daniel, Talker Bt.
 * 
 * To contact author send email to drdani@mazsola.iit.uni-miskolc.hu
 *
 */

/* This file is part of microcontroller simulator: ucsim.

UCSIM is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

UCSIM is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with UCSIM; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA. */
/*@1@*/

#ifndef SET_HEADER
#define SET_HEADER

#include "ddconfig.h"


extern bool cmd_set_iram(char *cmd, class cl_uc *uc, class cl_sim *sim);
extern bool cmd_set_xram(char *cmd, class cl_uc *uc, class cl_sim *sim);
extern bool cmd_set_code(char *cmd, class cl_uc *uc, class cl_sim *sim);
extern bool cmd_set_sfr(char *cmd, class cl_uc *uc, class cl_sim *sim);
extern bool cmd_set_bit(char *cmd, class cl_uc *uc, class cl_sim *sim);
extern bool cmd_set_port(char *cmd, class cl_uc *uc, class cl_sim *sim);
extern bool cmd_fill_iram(char *cmd, class cl_uc *uc, class cl_sim *sim);
extern bool cmd_fill_xram(char *cmd, class cl_uc *uc, class cl_sim *sim);
extern bool cmd_fill_sfr(char *cmd, class cl_uc *uc, class cl_sim *sim);
extern bool cmd_fill_code(char *cmd, class cl_uc *uc, class cl_sim *sim);


#endif

/* End of s51.src/set.h */
