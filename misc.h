/*
 * This file is part of libntlink.
 * Copyright (c) 2010, LRN
 *
 * libntlink is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * libntlink is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of
 * the GNU Lesser General Public License and the GNU General Public License
 * along with libntlink.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __NTLINK_MISC_H__
#define __NTLINK_MISC_H__

#include <windows.h>
#include <errno.h>

int strtowchar (const char *str, wchar_t **wretstr, UINT cp);
int wchartostr (const wchar_t *wstr, char **retstr, UINT cp);
int PathExistsW (wchar_t *path, DWORD *attributes);

#define NTLINK_ERROR_BASE 100

#ifndef EACCESS
  #define EACCESS      NTLINK_ERROR_BASE + 1
#endif

#ifndef EEXIST
  #define EEXIST       NTLINK_ERROR_BASE + 2
#endif

#ifndef ENOENT
  #define ENOENT       NTLINK_ERROR_BASE + 3
#endif

#ifndef ENOMEM
  #define ENOMEM       NTLINK_ERROR_BASE + 4
#endif

#ifndef EIO
  #define EIO          NTLINK_ERROR_BASE + 5
#endif


#endif /* __NTLINK_MISC__ */
