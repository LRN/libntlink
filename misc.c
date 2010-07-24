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

#include <stdlib.h>

#include <misc.h>

/**
 * strtowchar:
 * @str: a string (UTF-8-encoded) to convert
 * @wretstr: a pointer to variable (pointer to wchar_t) to receive the result
 * @cp: codepage to convert from
 *
 * Allocates new wchar_t string, fills it with converted @str and writes the
 * pointer to it into @wretstr. If the function fails, *@wretstr remains
 * unmodified. Converts from UTF-8 to UTF-16
 * See http://msdn.microsoft.com/en-us/library/dd319072%28VS.85%29.aspx
 * MultiByteToWideChar() documentation for values of cp.
 * CP_THREAD_ACP and CP_UTF8 are recommended.
 * Free the string returned in @wretstr with free() when it is no longer needed
 *
 * Returns:
 *  0 - conversion is successful
 * -1 - conversion failed at length-counting phase
 * -2 - conversion failed at memory allocation phase
 * -3 - conversion failed at string conversion phase
 *
 * 
 */
int
strtowchar (const char *str, wchar_t **wretstr, UINT cp)
{
  wchar_t *wstr;
  int len, lenc;
  len = MultiByteToWideChar (cp, 0, str, -1, NULL, 0);
  if (len <= 0)
  {
    return -1;
  }
  
  wstr = malloc (sizeof (wchar_t) * len);
  if (wstr == NULL)
  {
    return -2;
  }
  
  lenc = MultiByteToWideChar (cp, 0, str, -1, wstr, len);
  if (lenc != len)
  {
    free (wstr);
    return -3;
  }
  wstr[lenc] = 0;
  *wretstr = wstr;
  return 0;
}

/**
 * wchartostr:
 * @wstr: a string (UTF-16-encoded) to convert
 * @wretstr: a pointer to variable (pointer to char) to receive the result
 * @cp: codepage to convert from
 *
 * Allocates new wchar_t string, fills it with converted @wstr and writes the
 * pointer to it into @retstr. If the function fails, *@retstr remains
 * unmodified. Converts from UTF-8 to UTF-16
 * See http://msdn.microsoft.com/en-us/library/dd319072%28VS.85%29.aspx
 * WideCharToMultiByte() documentation for values of cp.
 * CP_THREAD_ACP and CP_UTF8 are recommended.
 * Free the string returned in @retstr with free() when it is no longer needed
 *
 * Returns:
 *  1 - conversion is successful, but some characters were replaced by placeholders
 *  0 - conversion is successful
 * -1 - conversion failed at length-counting phase
 * -2 - conversion failed at memory allocation phase
 * -3 - conversion failed at string conversion phase
 *
 * 
 */
int
wchartostr (const wchar_t *wstr, char **retstr, UINT cp)
{
  char *str;
  int len, lenc;
  BOOL lossy = FALSE;
  len = WideCharToMultiByte (cp, 0, wstr, -1, NULL, 0, NULL, &lossy);
  if (len <= 0)
  {
    return -1;
  }
  
  str = malloc (sizeof (char) * len);
  if (wstr == NULL)
  {
    return -2;
  }
  
  lenc = WideCharToMultiByte (cp, 0, wstr, -1, str, len, NULL, &lossy);
  if (lenc != len)
  {
    free (str);
    return -3;
  }
  str[lenc] = 0;
  *retstr = str;
  if (lossy)
    return 1;
  return 0;
}

/**
 * PathExistsW:
 * @path: a path (UTF-16) to verify
 * @attributes: pointer to DWORD that will hold found file attributes. Can
 * be NULL.
 *
 * Tries to find a file or directory pointed by @path.
 * If the searh succeeds, fills *@attributes with the file attributes.
 * Use FILE_ATTRIBUTE_* constants to determine file type.
 * If file does not exist, *@attributes remains unchanged.
 *
 * Returns:
 * -1 - the search failed
 *  0 - file/directory does not exist
 *  1 - file/directory exists
 */
int
PathExistsW (wchar_t *path, DWORD *attributes)
{
  HANDLE findhandle;
  WIN32_FIND_DATAW finddataw;
  findhandle = FindFirstFileW (path, &finddataw);
  if (findhandle == INVALID_HANDLE_VALUE)
  {
    DWORD err = GetLastError ();
    if (err == ERROR_FILE_NOT_FOUND)
      return 0;
    return -1;
  }
  FindClose (findhandle);
  if (attributes != NULL)
    *attributes = finddataw.dwFileAttributes;
  return 1;
}
