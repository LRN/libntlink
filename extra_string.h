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

#ifndef __NTLINK_EXTRA_STRING_H__
#define __NTLINK_EXTRA_STRING_H__

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define template_tok_r_header(chartype, tok_r_prefix) chartype *tok_r_prefix##tok_r(chartype *s, const chartype *sep, chartype **lasts)

template_tok_r_header(unsigned char, mbs);
template_tok_r_header(char, str);
template_tok_r_header(wchar_t, wcs);

int strtowchar (const char *str, wchar_t **wretstr, UINT cp);
int wchartostr (const wchar_t *wstr, char **retstr, UINT cp);

wchar_t *dup_swprintf (int *rlen, wchar_t *format, ...);
char *dup_sprintf (int *rlen, char *format, ...);

#define strdupa(s) (strcpy ((char *) alloca (sizeof (s[0]) * (strlen (s) + 1)), s))

#ifdef __cplusplus
}
#endif

#endif