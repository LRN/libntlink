/*
 * This file is part of libntlink.
 * Copyright (c) 2011, LRN
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

#include <errno.h>
#include <misc.h>

#include "walk.h"

struct _real_walk_iteratorw
{
  walk_iteratorw iter;
  struct _real_walk_iteratorw *parent;
  wchar_t root[MAX_PATH];
  wchar_t wdir[MAX_PATH];
  wchar_t pattern[MAX_PATH];
  int nitems;
  int index;
  unsigned int flags;
};

typedef struct _real_walk_iteratorw real_walk_iteratorw;

void
freeiterw (walk_iteratorw *iter)
{
  real_walk_iteratorw *riter = (real_walk_iteratorw *) iter;
  real_walk_iteratorw *parent = NULL;

  if (riter != NULL)
    do
    {
      parent = riter->parent;
      if (riter->iter.items)
        free (riter->iter.items);
      free (riter);
      riter = parent;
    }
    while (riter);
}

walk_iteratorw *
walk_allocw (wchar_t *root, wchar_t *wdir, unsigned flags)
{
  real_walk_iteratorw *riter = NULL;

  if (wcslen (wdir) >= MAX_PATH)
  {
    errno = ENAMETOOLONG;
    goto fail;
  }

  riter = (real_walk_iteratorw *) malloc (sizeof (real_walk_iteratorw));
  if (riter == NULL)
  {
    errno = ENOMEM;
    goto fail;
  }

  memset (riter, 0, sizeof (real_walk_iteratorw));

  if (!IsAbsName (wdir))
  {
    if ((root == NULL || !IsAbsName (root)))
    {
      DWORD reqsize = 0;
      reqsize = GetCurrentDirectoryW (MAX_PATH, riter->root);
      if (reqsize == 0)
      {
        errno = EINVAL;
        goto fail;
      }
      else if (reqsize > MAX_PATH - 1)
      {
        errno = ENAMETOOLONG;
        goto fail;
      }
    }
  }
  else
    riter->root[0] = L'\0';

  wcsncpy (riter->wdir, wdir, MAX_PATH);

  riter->flags = flags;
end:
  return (walk_iteratorw *) riter;

fail:
  freeiterw ((walk_iteratorw *) riter);
  riter = NULL;
  goto end;
}

WIN32_FIND_DATAW *
walk_fillw (real_walk_iteratorw *riter)
{
  WIN32_FIND_DATAW finddata;
  HANDLE hFind = NULL;
  DWORD err;
  DWORD nitems = 0;
  DWORD i;
  WIN32_FIND_DATAW *ret = NULL;

  SetLastError (0);
  hFind = FindFirstFileW (riter->pattern, &finddata);
  if (hFind == INVALID_HANDLE_VALUE)
  {
    err = GetLastError ();
    if (err != ERROR_FILE_NOT_FOUND)
    {
      /* FIXME: set errno */
      errno = ENOENT;
      hFind = NULL;
      goto fail;
    }
    goto end;
  }

  do
  {
    if (wcscmp (finddata.cFileName, L".") != 0 && wcscmp (finddata.cFileName, L"..") != 0)
      nitems += 1;
    SetLastError (0);
  } while (FindNextFileW (hFind, &finddata) != 0);

  err = GetLastError ();
  if (err != ERROR_NO_MORE_FILES)
  {
    errno = EINTR;
    goto fail;
  }

  FindClose (hFind);
  hFind = NULL;

  if (nitems == 0)
    return NULL;
  
  ret = (WIN32_FIND_DATAW *) malloc (sizeof (WIN32_FIND_DATAW) * nitems);
  if (ret == NULL)
  {
    errno = ENOMEM;
    goto fail;
  }
  
  SetLastError (0);
  hFind = FindFirstFileW (riter->pattern, &finddata);
  if (hFind == INVALID_HANDLE_VALUE)
  {
    err = GetLastError ();
    hFind = NULL;
    errno = EIO;
    goto fail;
  }
  i = 0;
  do
  {
    if (i == nitems)
    {
      errno = EAGAIN;
      goto fail;
    }
    if (wcscmp (finddata.cFileName, L".") != 0 && wcscmp (finddata.cFileName, L"..") != 0)
    {
      memcpy (&ret[i], &finddata, sizeof (WIN32_FIND_DATAW));
      i += 1;
    }
    SetLastError (0);
  } while (FindNextFileW (hFind, &finddata) != 0);

  err = GetLastError ();
  if (err != ERROR_NO_MORE_FILES)
  {
    errno = EINTR;
    goto fail;
  }

  FindClose (hFind);
  hFind = NULL;

  riter->nitems = i;
  riter->iter.nitems = i;
  riter->iter.items = ret;
  wcscpy (riter->iter.wdir, riter->wdir);
end:
  return ret;

fail:
  if (hFind != NULL)
    FindClose (hFind);
  if (ret != NULL)
    free (ret);
  ret = NULL;
  goto end;
}

walk_iteratorw *
walk_nextw (walk_iteratorw *iter)
{
  real_walk_iteratorw *riter = (real_walk_iteratorw *) iter;
  WIN32_FIND_DATAW *ret = NULL;
  int first = 0;

  if (riter == NULL)
  {
    return NULL;
  }

  if (riter->iter.items == NULL)
  {
    first = 1;
    int printed =_snwprintf (riter->pattern, MAX_PATH, L"%s%s%s\\*", riter->root, riter->root[0] != L'\0' ? L"\\" : L"", riter->wdir);
    if (printed < 0)
    {
      freeiterw (iter);
      return NULL;
    }
    do
    {
      errno = 0;
      ret = walk_fillw (riter);
    } while (ret == NULL && errno == EAGAIN);
    if (ret == NULL)
    {
      freeiterw (iter);
      return NULL;
    }
  }
  if (!first || riter->flags & WALK_FLAG_DEPTH_FIRST)
  {
    do
    {
      int printed;
      real_walk_iteratorw *new_riter = NULL;
      if (riter->index < riter->nitems)
      {
        new_riter = (real_walk_iteratorw *) walk_allocw (riter->root, riter->pattern, riter->flags);
        if (new_riter == NULL)
          riter->index = riter->nitems;
      }
      for (; riter->index < riter->nitems; riter->index += 1)
      {
        if (riter->iter.items[riter->index].cFileName[0] == L'\0')
          continue;
        if (~riter->iter.items[riter->index].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
          continue;
        if (riter->flags & WALK_FLAG_DONT_FOLLOW_SYMLINKS &&
            riter->iter.items[riter->index].dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT &&
            riter->iter.items[riter->index].dwReserved0 == IO_REPARSE_TAG_SYMLINK)
          continue;
        printed = _snwprintf (new_riter->wdir, MAX_PATH, L"%s%s%s\\%s", riter->root, riter->root[0] != L'\0' ? L"\\" : L"", riter->wdir, riter->iter.items[riter->index].cFileName);
        printed = _snwprintf (new_riter->pattern, MAX_PATH, L"%s%s%s\\%s\\*", riter->root, riter->root[0] != L'\0' ? L"\\" : L"", riter->wdir, riter->iter.items[riter->index].cFileName);
        if (printed < 0)
          continue;
        do
        {
          errno = 0;
          ret = walk_fillw (new_riter);
        } while (ret == NULL && errno == EAGAIN);
        if (ret == NULL)
        {
          if (errno != 0)
            riter->index = riter->nitems - 1;
          else
            continue;
        }
        riter->index += 1;
        break;
      }
      if (riter->flags & WALK_FLAG_DEPTH_FIRST)
      {
        if (riter->index == riter->nitems)
        {
          new_riter->parent = NULL;
          freeiterw ((walk_iteratorw *) new_riter);
          riter->flags &= ~WALK_FLAG_DEPTH_FIRST;
          return (walk_iteratorw *) riter;
        }
      }
      else if (riter->index == riter->nitems)
      {
        freeiterw ((walk_iteratorw *) new_riter);
        return walk_nextw ((walk_iteratorw *) riter->parent);
      }
      new_riter->iter.depth = riter->iter.depth + 1;
      new_riter->parent = riter;
      riter = new_riter;
    } while (0 || riter->flags & WALK_FLAG_DEPTH_FIRST);
  }
  return (walk_iteratorw *) riter;
}
