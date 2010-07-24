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

#include <unistd.h>
#include <windows.h>

#include "quasisymlink.h"
#include "misc.h"
#include "juncpoint.h"

int
ntlink_symlink(const char *path1, const char *path2)
{
  wchar_t *wpath1 = NULL, *wpath2 = NULL;
  int exists;
  DWORD attributes;

  if (strtowchar (path1, &wpath1, CP_THREAD_ACP) < 0)
    goto fail;
  if (strtowchar (path2, &wpath2, CP_THREAD_ACP) < 0)
    goto fail;

  exists = PathExistsW (wpath2, &attributes);
  if (exists != 0)
  {
    if (exists > 0)
      errno = EEXIST;
    else
      errno = EACCESS;
    goto fail;
  }

  exists = PathExistsW (wpath1, &attributes);
  if (exists <= 0)
  {
    /* Since we don't know anything about the target,
     * we can't link to it.
     */
    if (exists == 0)
      errno = ENOENT;
    else
      errno = EACCESS;
    goto fail;
  }

  if (attributes & FILE_ATTRIBUTE_DIRECTORY)
  {
    /* Create a junction point to target directory */
    int err;
    wchar_t *wpath1_unp = NULL;
    wpath1_unp = malloc (sizeof (wchar_t) * (wcslen (wpath1) + 1));
    memcpy (wpath1_unp, L"\\??\\", sizeof (wchar_t) * 4);
    memcpy (&wpath1_unp[4], wpath1, (wcslen (wpath1) + 1) * sizeof (wchar_t));
    err = SetJuncPointW (wpath1_unp, wpath2);
    free (wpath1_unp);
    switch (err)
    {
    case 0:
      break;
    case -1:
      errno = EACCESS;
      goto fail;
      break;
    case -2:
      errno = ENOMEM;
      goto fail;
      break;
    case -3:
      errno = EIO;
      goto fail;
      break;
    default:
      break;
    }
  }
  else if (attributes & FILE_ATTRIBUTE_NORMAL)
  {
    /* Create a hard link to target file */
    BOOL ret;
    ret = CreateHardLinkW (wpath2, wpath1, NULL);
    if (ret == 0)
    {
      errno = EIO;
      goto fail;
    }
  }

  free (wpath1);
  free (wpath2);

  return 0;
fail:
  if (wpath1 != NULL)
    free (wpath1);
  if (wpath2 != NULL)
    free (wpath2);

  return -1;
}

int
ntlink_lchown(const char *path, uid_t owner, gid_t group)
{
  errno = EINVAL;
  return -1;
}

int
ntlink_link(const char *path1, const char *path2)
{
  wchar_t *wpath1 = NULL, *wpath2 = NULL, *wpath1_unp = NULL;
  int exists;
  DWORD attributes;

  if (strtowchar (path1, &wpath1, CP_THREAD_ACP) < 0)
    goto fail;
  if (strtowchar (path2, &wpath2, CP_THREAD_ACP) < 0)
    goto fail;

  exists = PathExistsW (wpath2, &attributes);
  if (exists != 0)
  {
    if (exists > 0)
      errno = EEXIST;
    else
      errno = EACCESS;
    goto fail;
  }

  exists = PathExistsW (wpath1, &attributes);
  if (exists <= 0)
  {
    /* path1 must exist */
    if (exists == 0)
      errno = ENOENT;
    else
      errno = EACCESS;
    goto fail;
  }

  if (attributes & FILE_ATTRIBUTE_DIRECTORY)
  {
    /* This implementation does not support link() on directories */
    errno = EPERM;
    goto fail;
  }
  else if (attributes & FILE_ATTRIBUTE_NORMAL)
  {
    /* Create a hard link to target file */
    BOOL ret;
    ret = CreateHardLinkW (wpath2, wpath1, NULL);
    if (ret == 0)
    {
      errno = EIO;
      goto fail;
    }
  }

  free (wpath1);
  free (wpath2);

  return 0;
fail:
  if (wpath1 != NULL)
    free (wpath1);
  if (wpath2 != NULL)
    free (wpath2);

  return -1;
}

int
ntlink_lstat(const char *restrict path, struct _stat *restrict buf)
{
  wchar_t *wpath = NULL;
  int exists;
  int result = 0;
  DWORD attributes;

  if (strtowchar (path, &wpath, CP_THREAD_ACP) < 0)
    goto fail;

  exists = PathExistsW (wpath, &attributes);
  if (exists <= 0)
  {
    /* path1 must exist */
    if (exists == 0)
      errno = ENOENT;
    else
      errno = EACCESS;
    goto fail;
  }

  if (attributes & FILE_ATTRIBUTE_DIRECTORY)
  {
    /* Obtain juncpoint information and fill the buf */

    /* FIXME: check that wpath is absolute */
    wchar_t *jptarget = NULL;
    int jpresult = GetJuncPointW (&jptarget, wpath);
    switch (jpresult)
    {
    case -1:
      errno = EACCESS;
      goto fail;
    case -2:
      errno = EIO;
      goto fail;
    case -3:
      errno = ENOMEM;
      goto fail;
    default:
      ;
    }
    buf->st_size = wcslen (jptarget);
    free (jptarget);
  }
  else if (attributes & FILE_ATTRIBUTE_NORMAL)
  {
    /* This is a hard link, use normal stat() */
    result = _wstat (wpath, buf);
  }

  free (wpath);

  return result;
fail:
  if (wpath != NULL)
    free (wpath);

  return -1;
}

ssize_t
ntlink_readlink(const char *restrict path, char *restrict buf,
    size_t bufsize)
{
  wchar_t *wpath = NULL;
  int exists;
  int result = 0;
  DWORD attributes;

  if (strtowchar (path, &wpath, CP_THREAD_ACP) < 0)
    goto fail;

  exists = PathExistsW (wpath, &attributes);
  if (exists <= 0)
  {
    /* path1 must exist */
    if (exists == 0)
      errno = ENOENT;
    else
      errno = EACCESS;
    goto fail;
  }

  if (attributes & FILE_ATTRIBUTE_DIRECTORY)
  {
    /* Obtain juncpoint information and fill the buf */

    /* FIXME: check that wpath is absolute */
    wchar_t *wjptarget = NULL;
    char *jptarget = NULL;
    int len = 0;
    int conv_result = 0;
    int jpresult = GetJuncPointW (&wjptarget, wpath);
    switch (jpresult)
    {
    case -1:
      errno = EACCESS;
      goto fail;
    case -2:
      errno = EIO;
      goto fail;
    case -3:
      errno = ENOMEM;
      goto fail;
    default:
      ;
    }
    conv_result = wchartostr (wjptarget, &jptarget, CP_THREAD_ACP);
    free (wjptarget);
    if (conv_result < 0)
      goto fail;
    len = strlen (jptarget);
    if (len > bufsize)
      len = bufsize;
    memcpy (buf, jptarget, len);
    result = len;
  }
  else if (attributes & FILE_ATTRIBUTE_NORMAL)
  {
    /* This must be a hard link, return its own name */
    int len = strlen (path);
    if (len > bufsize)
      len = bufsize;
    memcpy (buf, path, len);
    result = len;
  }

  free (wpath);

  return result;
fail:
  if (wpath != NULL)
    free (wpath);

  return -1;
}

int
ntlink_unlink(const char *path)
{
  wchar_t *wpath = NULL;
  int exists;
  DWORD attributes;

  if (strtowchar (path, &wpath, CP_THREAD_ACP) < 0)
    goto fail;

  exists = PathExistsW (wpath, &attributes);
  if (exists <= 0)
  {
    /* path must exist */
    if (exists == 0)
      errno = ENOENT;
    else
      errno = EACCESS;
    goto fail;
  }

  if (attributes & FILE_ATTRIBUTE_DIRECTORY)
  {
    /* Unjunc that junction */

    /* FIXME: check that wpath is absolute */
    int jpresult = UnJuncPointW (wpath);
    switch (jpresult)
    {
    case -1:
      errno = EACCESS;
      goto fail;
    case -2:
      errno = EIO;
      goto fail;
    case -3:
      errno = ENOMEM;
      goto fail;
    default:
      ;
    }
  }
  else if (attributes & FILE_ATTRIBUTE_NORMAL)
  {
    /* This must be a hard link
     * If the file is referenced somewhere else (linkcount > 1), delete it
     * (remove @path and reduce link count by 1). Otherwise it becomes a
     * difficult choice:
     * 1) Follow unlink() logic for hard links and remove the last link,
     * essentially deleting the file
     * 2) Follow unlink() logic for symlinks and do nothing - as long as
     * the application doesn't check that the file was deleted, we're good.
     */
    HANDLE hFile;
    int firet = 0;
    BY_HANDLE_FILE_INFORMATION fi;
    hFile = CreateFileW (wpath, GENERIC_READ, 0, NULL,
      OPEN_ALWAYS, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE)
    {
      goto fail;
    }

    firet = GetFileInformationByHandle (hFile, &fi);
    CloseHandle (hFile);
    if (firet == 0)
      goto fail;
    if (fi.nNumberOfLinks > 1)
    {
      _wunlink (wpath);
    }
  }

  free (wpath);

  return 0;
fail:
  if (wpath != NULL)
    free (wpath);

  return -1;
}
