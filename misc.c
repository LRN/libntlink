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
#include <string.h>
#include <stdio.h>

#include "extra_string.h"
#include "misc.h"
#include "quasisymlink.h"

/**
 *
 */
int PathContainsSymlinksW (wchar_t *path, int excludeLastComponent)
{
  int ret = 0;
  wchar_t *copyOfPath;
  wchar_t *lasts;
  wchar_t *copyOfPath2;
  wchar_t *token;
  int component = -1;
  int i;
  WIN32_FIND_DATAW finddata;

  copyOfPath = wcsdup (path);
  if (copyOfPath == NULL)
    return -2;

  copyOfPath2 = wcsdup (path);
  if (copyOfPath2 == NULL)
  {
    free (copyOfPath);
    return -2;
  }

  if (wcsncmp (copyOfPath, L"\\\\?\\" , 4) == 0)
  {
    copyOfPath2[4] = 0;
    token = wcstok_r (copyOfPath, L"/\\", &lasts);
    token = wcstok_r (NULL, L"/\\", &lasts);
  }
  else
  {
    token = wcstok_r (copyOfPath, L"/\\", &lasts);
    if (token > copyOfPath)
    {
      copyOfPath2[token - copyOfPath] = 0;
    }
    else
      copyOfPath2[0] = 0;
  }
  if (token[0] >= L'A' && token[0] <= 'z' && token[1] == ':')
  {
    wcscat (copyOfPath2, token);
    token = wcstok_r (NULL, L"/\\", &lasts);
    //if (wcslen (copyOfPath2) < wcslen (path))
    if (token != NULL)
      wcscat (copyOfPath2, L"\\");
  }
  for (i = 0; 1 == 1; token = wcstok_r (NULL, L"/\\", &lasts), i++)
  {
    if (PathExistsW (copyOfPath2, &finddata, PATH_EXISTS_FLAG_NOTHING) <= 0)
    {
      ret = -1;
      component = -1;
      break;
    }
    else if (finddata.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
    {
      ret = 1;
      component = i;
      break;
    }
    if (lasts != NULL)
    {
      if (token != NULL)
      {
        wcscat (copyOfPath2, L"\\");
        wcscat (copyOfPath2, token);
      }
      else
        break;
    }
    else
      break;
  }

  if (component >= 0 && component == i && token == NULL && excludeLastComponent)
    ret = 0;

  free (copyOfPath2);
  free (copyOfPath);
  return ret;
}

/**
 * PathExistsW:
 * @path: a path (UTF-16) to verify
 * @finddata: pointer to WIN32_FIND_DATAW that will hold the information
 * about the file found. Can be NULL.
 * @flags: a combination of one or more members of PathExistsFlags
 *
 * Tries to find a file or directory pointed by @path.
 * If @path contains wildcards in the last component, finds the first matching
 *   file or directory.
 * If the searh succeeds, fills *@finddata with the file information.
 * Use FILE_ATTRIBUTE_* constants ( http://msdn.microsoft.com/en-us/library/ee332330%28v=VS.85%29.aspx )
 *   on @finddata->dwFileAttributes to determine file type.
 * @finddata->cFileName will contain absolute file name.
 * If file does not exist, *@finddata remains unchanged.
 *
 * If PATH_EXISTS_FLAG_DONT_FOLLOW_SYMLINKS IS SET in @flags, then intermediate
 *   directory symlinks in @path will not be resolved. If there are directory symlinks in
 *   @path, the search will fail with the code -2 once the first such symlink is discovered.
 *   This does not affect the last component of @path (see below)
 * If PATH_EXISTS_FLAG_DONT_FOLLOW_SYMLINKS IS NOT set in @flags, then intermediate
 *   directory symlinks in @path will be resolved transparently, and the search will
 *   fail if one of these links (if any) resolves to non-existing directory.
 *   This does not affect the last component of @path (see below)
 * If PATH_EXISTS_FLAG_FOLLOW_LAST_SYMLINK IS NOT set in @flags, then the last
 *   component of @path (or the first file that matches the last component if it
 *   contains wildcards) will not be resolved, even if it is a symlink, @finddata
 *   will contain the data for that symlink, not for its target, and the target's
 *   existence is not checked.
 * If PATH_EXISTS_FLAG_FOLLOW_LAST_SYMLINK IS SET, then the last component will be
 *   resolved until the next link either points to a real existing real file, or until
 *   the next link does not exist.
 *
 * Returns:
 * -2 - there are directory symlinks in @path and PATH_EXISTS_FLAG_DONT_FOLLOW_SYMLINKS is set
 * -1 - FindFirstFileW() have failed for any reason other than ERROR_FILE_NOT_FOUND
 *  0 - file/directory does not exist
 *  1 - file/directory exists
 */
int
PathExistsW (wchar_t *path, WIN32_FIND_DATAW *finddata, PathExistsFlags flags)
{
  HANDLE findhandle;
  WIN32_FIND_DATAW finddataw;
  DWORD attributes;
  wchar_t target[MAX_PATH + 1];
  wchar_t source[MAX_PATH + 1];
  int containsSymlinks = 0;

  if (flags & PATH_EXISTS_FLAG_DONT_FOLLOW_SYMLINKS)
  {
    containsSymlinks = PathContainsSymlinksW (path, 1);
    if (containsSymlinks == 1)
      return -2;
    else if (containsSymlinks == -1)
      return 0;
    else if (containsSymlinks == -2)
      return -1;
  }
#if 0
  findhandle = FindFirstFileW (path, &finddataw);
  if (findhandle == INVALID_HANDLE_VALUE)
  {
    DWORD err = GetLastError ();
    if (err == ERROR_FILE_NOT_FOUND)
      return 0;
    return -1;
  }
  FindClose (findhandle);
#else
  finddataw.dwFileAttributes = GetFileAttributesW (path);
  if (finddataw.dwFileAttributes == INVALID_FILE_ATTRIBUTES)
  {
    DWORD err = GetLastError ();
    if (err == ERROR_FILE_NOT_FOUND)
      return 0;
    return -1;
  }
#endif

  wcsncpy (source, path, MAX_PATH + 1);

  errno = 0;

  while (flags & PATH_EXISTS_FLAG_FOLLOW_LAST_SYMLINK)
  {
    int linklen;
    linklen = ntlink_readlinkw (source, target, MAX_PATH + 1);
    if (linklen > 0 && wcscmp (source, target) != 0)
    {
      if (IsAbsName (target))
        wcsncpy (source, target, MAX_PATH + 1);
      else
      {
        int newsrclen = 0;
        wchar_t srcdrive[_MAX_DRIVE], srcdir[_MAX_DIR];
        errno = 0;
        _wsplitpath (source, srcdrive, srcdir, NULL, NULL);
        if (errno != 0)
          return -3;
        newsrclen = snwprintf (source, MAX_PATH + 1, L"%s\\%s\\%s", srcdrive, srcdir, target);
        if (newsrclen < 0)
          return -4;
      }
      errno = 0;
    }
    else
    {
      if (errno == EINVAL)
        return PathExistsW (source, finddata, flags & ~PATH_EXISTS_FLAG_FOLLOW_LAST_SYMLINK);
      else if (errno != 0)
        return -5;
    }
  }

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
