/*
 *	avarice - The "avarice" program.
 *	Copyright (C) 2011 Joerg Wunsch
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License Version 2
 *	as published by the Free Software Foundation.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 *
 * Pragma abstractions for various compilers/compiler versions.
 *
 * $Id$
 */

#ifndef PRAGMA_H
#define PRAGMA_H

/*
 * Evaluate which diagnostic pragmas can be used.
 */
#if defined(__GNUC__)
#  if __GNUC__ > 4
#      define PRAGMA_DIAG_PUSH       _Pragma("GCC diagnostic push")
#      define PRAGMA_DIAG_POP        _Pragma("GCC diagnostic pop")
#      define PRAGMA_(x)             _Pragma(#x)
#      define PRAGMA_DIAG_IGNORED(x) PRAGMA_(GCC diagnostic ignored x)
#  elif __GNUC__ == 4
#    if __GNUC_MINOR__ >= 6
#      define PRAGMA_DIAG_PUSH       _Pragma("GCC diagnostic push")
#      define PRAGMA_DIAG_POP        _Pragma("GCC diagnostic pop")
#      define PRAGMA_(x)             _Pragma(#x)
#      define PRAGMA_DIAG_IGNORED(x) PRAGMA_(GCC diagnostic ignored x)
#    else
#      define PRAGMA_DIAG_PUSH
#      define PRAGMA_DIAG_POP
#      define PRAGMA_(x)             _Pragma(#x)
#      define PRAGMA_DIAG_IGNORED(x) PRAGMA_(GCC diagnostic ignored x)
#    endif  /* GCC 4.x */
#  else /* too old */
#      define PRAGMA_DIAG_PUSH
#      define PRAGMA_DIAG_POP
#      define PRAGMA_DIAG_IGNORED(x)
#  endif
#else /* not GCC */
#  define PRAGMA_DIAG_PUSH
#  define PRAGMA_DIAG_POP
#  define PRAGMA_DIAG_IGNORED(x)
#endif

#endif
