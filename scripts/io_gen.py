#! /usr/bin/env python
###############################################################################
#
# avarice - The "avarice" program.
# Copyright (C) 2004  Theodore A. Roth
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
###############################################################################
#
# $Id$
#

#
# This script is for generating the .io_reg field initializer of the
# DevSuppDefn for a specfic device. The input should be one of the io*.h files
# provided by avr-libc in the include/avr/ directory.
#
# Usage: io_gen.py <io_header>
#
# This best way to use this script is while using vim to edit the ioreg.cc.
# You can run the script from vim and have the output of the script inserted
# directly into the ioreg.cc file with a command like this:
#
#   :r!../misc/io_gen.py ~/dev/tools/avr-libc-cvs/include/avr/iomega128.h
#

import os, sys, re

base_regx = r'[#]define\s*?(\S+)\s*?%s\s*[(]\s*?(\S+?)\s*?[)]'

re_io8 = re.compile (base_regx % ('_SFR_IO8'))
re_mem8 = re.compile (base_regx % ('_SFR_MEM8'))

# Open the input file.

try:
    in_file_name = sys.argv[1]
except:
    print('Usage: %s <io_header>' % (os.path.basename (sys.argv[0])))
    sys.exit (1)

f = open (in_file_name).read ()

register = {}

# Find all the _SFR_IO8 defs.

for name, addr_str in re_io8.findall (f):
    addr = int (addr_str, 0) + 0x20
    register[addr] = name

# Find all the _SFR_MEM8 defs.

for name, addr_str in re_mem8.findall (f):
    addr = int (addr_str, 0)
    register[addr] = name

# Print the field initializer to stdout.

addrs = sorted(register)
print('gdb_io_reg_def_type [PUT DEVICE NAME HERE]_io_registers[] =')
print('{')
for addr in addrs:
    print('    { %-12s, 0x%X, 0x00 },' % ('"%s"' %(register[addr]), addr))
print('    /* May need to add SREG, SPL, SPH, and eeprom registers. */')
print('    { 0, 0, 0}')
print('}')
