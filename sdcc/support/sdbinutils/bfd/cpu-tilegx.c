/* BFD support for the TILE-Gx processor.
   Copyright 2011 Free Software Foundation, Inc.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "bfd.h"
#include "sysdep.h"
#include "libbfd.h"

const bfd_arch_info_type bfd_tilegx_arch =
  {
    64, /* 64 bits in a word */
    64, /* 64 bits in an address */
    8,  /* 8 bits in a byte */
    bfd_arch_tilegx,
    bfd_mach_tilegx,
    "tilegx",
    "tilegx",
    3,
    TRUE,
    bfd_default_compatible,
    bfd_default_scan,
    0,
  };
