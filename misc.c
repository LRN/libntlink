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
  *retstr = str;
  if (lossy)
    return 1;
  return 0;
}

/**
 * PathExistsW:
 * @path: a path (UTF-16) to verify
 * @finddata: pointer to WIN32_FIND_DATAW that will hold the information
 * about the file found. Can be NULL.
 *
 * Tries to find a file or directory pointed by @path.
 * If the searh succeeds, fills *@finddata with the file information.
 * Use FILE_ATTRIBUTE_* constants on @finddata->dwFileAttributes to
 * determine file type.
 * @finddata->cFileName will contain absolute file name.
 * If file does not exist, *@finddata remains unchanged.
 *
 * Returns:
 * -1 - the search have failed
 *  0 - file/directory does not exist
 *  1 - file/directory exists
 */
int
PathExistsW (wchar_t *path, WIN32_FIND_DATAW *finddata)
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
  if (finddata != NULL)
  {
    *finddata = finddataw;
  }
  return 1;
}

/**
 * IsAbsName:
 * @name: a filesystem name (UTF-16) to check
 *
 * Decides whether the name is an absolute path or not.
 *
 * Returns:
 *  0 - name is not absolute
 *  1 - name is absolute
 */
int
IsAbsName (wchar_t *name)
{
  if (name[0] != L'\0' && name[1] != L'\0')
  {
    if (name[0] == L'\\' && name[1] == L'\\')
      return 1;
    if (name[2] != L'\0' && name[0] >= L'A' && name[0] <= L'z' && name[1] == L':' && (name[2] == L'\\' || name[2] == L'/'))
      return 1;
  }
  return 0;
}

/**
 * GetAbsName:
 * @relative: a supposedly relative filesystem name (UTF-16)
 * @absolute: a pointer to a wchar_t * that receives the result
 *
 * Makes absolute name out of a relative one.
 * *@absolute is not modified in case of failure
 * *@absolute is allocated internally and must be freed with free()
 *
 * Returns:
 *  0 - success
 * -1 - failed to obtain current directory
 * -2 - failed to allocate memory
 */
int
GetAbsName (wchar_t *relative, wchar_t **absolute)
{
  wchar_t tmp[MAX_PATH];
  wchar_t *tmpptr;
  if (relative == NULL)
    return -1;
  if (IsAbsName (relative))
    tmpptr = wcsdup (relative);
  else if (GetFullPathNameW (relative, MAX_PATH, tmp, NULL) <= 0)
  {
    DWORD dirlen = GetCurrentDirectoryW (MAX_PATH, tmp);
    if (dirlen > 0)
    {
      wcsncat (tmp, L"\\", MAX_PATH - dirlen);
      wcsncat (tmp, relative, MAX_PATH - dirlen - 1);
      tmpptr = wcsdup (tmp);
    }
    else
      return -1;
  }
  else
    tmpptr = wcsdup (tmp);
  if (tmpptr != NULL)
  {
    *absolute = tmpptr;
    return 0;
  }
  else
    return -2;
}
