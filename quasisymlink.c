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
  WIN32_FIND_DATAW finddata;
#if _WIN32_WINNT >= 0x0600
  BOOL err;
  DWORD lerr;
#endif

  if (strtowchar (path1, &wpath1, CP_THREAD_ACP) < 0)
    goto fail;
  if (strtowchar (path2, &wpath2, CP_THREAD_ACP) < 0)
    goto fail;

  exists = PathExistsW (wpath2, &finddata);
  if (exists != 0)
  {
    if (exists > 0)
      errno = EEXIST;
    else
      errno = EACCESS;
    goto fail;
  }

  exists = PathExistsW (wpath1, &finddata);
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

#if _WIN32_WINNT >= 0x0600
  SetLastError (0);
  err = CreateSymbolicLinkW (wpath2, wpath1, (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ? 0 : SYMBOLIC_LINK_FLAG_DIRECTORY);
  lerr = GetLastError ();
  if (err == 0)
  {
    /* TODO: set errno properly from lerr */
    errno = EIO;
    goto fail;
  }
#else
  if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
  {
    /* Create a junction point to target directory */
    int err;
    wchar_t *wpath1_unp = NULL;
    wpath1_unp = malloc (sizeof (wchar_t) * (wcslen (wpath1) + 1 + 4));
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
  else if (finddata.dwFileAttributes & FILE_ATTRIBUTE_NORMAL)
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
#endif

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
  wchar_t *wpath1 = NULL, *wpath2 = NULL/*, *wpath1_unp = NULL*/;
  int exists;
  WIN32_FIND_DATAW finddata;

  if (strtowchar (path1, &wpath1, CP_THREAD_ACP) < 0)
    goto fail;
  if (strtowchar (path2, &wpath2, CP_THREAD_ACP) < 0)
    goto fail;

  exists = PathExistsW (wpath2, &finddata);
  if (exists != 0)
  {
    if (exists > 0)
      errno = EEXIST;
    else
      errno = EACCESS;
    goto fail;
  }

  exists = PathExistsW (wpath1, &finddata);
  if (exists <= 0)
  {
    /* path1 must exist */
    if (exists == 0)
      errno = ENOENT;
    else
      errno = EACCESS;
    goto fail;
  }

  if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
  {
    /* This implementation does not support link() on directories */
    errno = EPERM;
    goto fail;
  }
  else if (finddata.dwFileAttributes & FILE_ATTRIBUTE_NORMAL)
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
ntlink_lstat(const char *restrict path, struct stat *restrict buf)
{
  wchar_t *wpath = NULL;
  int exists;
  int result = 0;
  WIN32_FIND_DATAW finddata;
  wchar_t *abswpath = NULL;
#if _WIN32_WINNT >= 0x0600
  HANDLE fileh = NULL;
  DWORD lerr;
#endif

  if (strtowchar (path, &wpath, CP_THREAD_ACP) < 0)
    goto fail;

  GetAbsName (wpath, &abswpath);

  exists = PathExistsW (wpath, &finddata);
  if (exists <= 0)
  {
    /* path1 must exist */
    if (exists == 0)
      errno = ENOENT;
    else
      errno = EACCESS;
    goto fail;
  }

#if _WIN32_WINNT >= 0x0600
  {
    FILE_BASIC_INFO bi;
    FILE_STANDARD_INFO stdi;
    BY_HANDLE_FILE_INFORMATION info;

    SetLastError (0);
    fileh = CreateFileW (wpath, 0, 0, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | (bi.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FILE_FLAG_BACKUP_SEMANTICS : 0,
        NULL);
    lerr = GetLastError ();
    if (fileh == INVALID_HANDLE_VALUE)
    {
      /* TODO: set errno from GetLastError() */
      errno = EIO;
      goto fail;
    }
    SetLastError (0);
    if (GetFileInformationByHandle(fileh, &info) == 0)
    {
      lerr = GetLastError ();
      /* TODO: set errno from GetLastError () */
      errno = EIO;
      goto fail;
    }
    SetLastError (0);
    if (fileh == INVALID_HANDLE_VALUE)
    {
      lerr = GetLastError ();
      /* TODO: set errno from GetLastError () */
      errno = EIO;
      goto fail;
    }
    SetLastError (0);
    if (GetFileInformationByHandleEx (fileh, FileBasicInfo, &bi, sizeof (bi)) == 0)
    {
      lerr = GetLastError ();
      /* TODO: set errno from GetLastError () */
      errno = EIO;
      goto fail;
    }
    SetLastError (0);
    if (GetFileInformationByHandleEx (fileh, FileStandardInfo, &stdi, sizeof (stdi)) == 0)
    {
      lerr = GetLastError ();
      /* TODO: set errno from GetLastError () */
      errno = EIO;
      goto fail;
    }
    CloseHandle (fileh);
    fileh = NULL;
    buf->st_gid = 0;
    buf->st_uid = 0;
/*
#st_mode
#
#    Bit mask for file-mode information. The _S_IFDIR bit is set if path specifies a directory;
#    the _S_IFREG bit is set if path specifies an ordinary file or a device.
#    User read/write bits are set according to the file's permission mode;
#    user execute bits are set according to the filename extension.
*/
    buf->st_nlink = stdi.NumberOfLinks;
    buf->st_mode = 0;
    buf->st_mode |= bi.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ? _S_IFLNK : 0;
    buf->st_mode |= (bi.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(bi.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ? _S_IFDIR : _S_IFREG;
    buf->st_size = stdi.EndOfFile.QuadPart;
    buf->st_dev = buf->st_rdev = info.dwVolumeSerialNumber;
    buf->st_ino = ((info.nFileIndexHigh << sizeof(DWORD)) | info.nFileIndexLow);
    /* buf->st_*time is either 32-bit or 64-bit integer */
#ifdef _USE_32BIT_TIME_T
    buf->st_atime = bi.LastAccessTime;
    buf->st_ctime = bi.CreationTime;
    buf->st_mtime = bi.LastWriteTime > bi.ChangeTime ? bi.LastWriteTime : bi.ChangeTime;
#else
    buf->st_atime = bi.LastAccessTime.LowPart;
    buf->st_ctime = bi.CreationTime.LowPart;
    buf->st_mtime = bi.LastWriteTime.LowPart > bi.ChangeTime.LowPart ? bi.LastWriteTime.LowPart : bi.ChangeTime.LowPart;
#endif
  }
#else
  if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
  {
    /* Obtain juncpoint information and fill the buf */

    wchar_t *jptarget = NULL;
    int jpresult = GetJuncPointW (&jptarget, abswpath);
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
  else if (finddata.dwFileAttributes & FILE_ATTRIBUTE_NORMAL)
  {
    /* This is a hard link, use normal stat() */
    result = _wstat (wpath, buf);
  }
#endif

  if (abswpath != NULL)
    free (abswpath);
  free (wpath);

  return result;
fail:
#if _WIN32_WINNT >= 0x0600
  if (fileh != NULL)
    CloseHandle (fileh);
#endif
  if (wpath != NULL)
    free (wpath);
  if (abswpath != NULL)
    free (abswpath);
  return -1;
}

ssize_t
ntlink_readlink(const char *restrict path, char *restrict buf,
    size_t bufsize)
{
  wchar_t *wpath = NULL;
  wchar_t *abswpath = NULL;
  int exists;
  int result = 0;
  WIN32_FIND_DATAW finddata;
#if _WIN32_WINNT >= 0x0600
  HANDLE fileh = NULL;
  wchar_t *wtarget = NULL;
  DWORD lerr;
#endif
  if (strtowchar (path, &wpath, CP_THREAD_ACP) < 0)
    goto fail;

  GetAbsName (wpath, &abswpath);

  exists = PathExistsW (wpath, &finddata);
  if (exists <= 0)
  {
    /* path1 must exist */
    if (exists == 0)
      errno = ENOENT;
    else
      errno = EACCESS;
    goto fail;
  }

#if _WIN32_WINNT >= 0x0600
  if (!(finddata.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
  {
    /* This is not a symlink */
    errno = EINVAL;
    goto fail;
  }
  else
  {
    DWORD required_size, required_size2;
    char *target;
    int len = 0;
    int conv_result = 0;
    SetLastError (0);
    fileh = CreateFileW (wpath, GENERIC_READ, 0, NULL, OPEN_EXISTING, finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? FILE_FLAG_BACKUP_SEMANTICS : 0, NULL);
    if (fileh == INVALID_HANDLE_VALUE)
    {
      lerr = GetLastError ();
      /* TODO: set errno from GetLastError () */
      errno = EIO;
      goto fail;
    }
    SetLastError (0);
    required_size = GetFinalPathNameByHandleW (fileh, (wchar_t *) &required_size, 0, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if (required_size == 0)
    {
      lerr = GetLastError ();
      /* TODO: set errno from GetLastError () */
      errno = EIO;
      goto fail;
    }
    wtarget = (wchar_t *) malloc ((required_size) * sizeof (wchar_t));
    if (wtarget == NULL)
    {
      errno = ENOMEM;
      goto fail;
    }
    SetLastError (0);
    required_size2 = GetFinalPathNameByHandleW (fileh, wtarget, required_size, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    if (required_size2 == 0)
    {
      lerr = GetLastError ();
      /* TODO: set errno from GetLastError () */
      errno = EIO;
      goto fail;
    }
    if (required_size2 != required_size - 1)
    {
      /* TODO: Figure out what would that mean... */
      errno = EIO;
      goto fail;
    }
    CloseHandle (fileh);
    fileh = NULL;
    conv_result = wchartostr (wtarget, &target, CP_THREAD_ACP);
    free (wtarget);
    wtarget = NULL;
    if (conv_result < 0)
      goto fail;
    len = strlen (target);
    if (len > bufsize)
      len = bufsize;
    memcpy (buf, target, len);
    free (target);
    result = len;
  }
#else
  if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
  {
    if (!(finddata.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
    {
      /* This is not a junction point */
      errno = EINVAL;
      goto fail;
    }
    /* Obtain juncpoint information and fill the buf */

    wchar_t *wjptarget = NULL;
    char *jptarget = NULL;
    int len = 0;
    int conv_result = 0;
    int jpresult = GetJuncPointW (&wjptarget, abswpath);
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
  else if (finddata.dwFileAttributes & FILE_ATTRIBUTE_NORMAL)
  {
    /* This must be a hard link, return its own name */
    int len = strlen (path);
    if (len > bufsize)
      len = bufsize;
    memcpy (buf, path, len);
    result = len;
  }
#endif

  if (abswpath != NULL)
    free (abswpath);
  free (wpath);

  return result;
fail:
#if _WIN32_WINNT >= 0x0600
  if (wtarget != NULL)
    free (wtarget);
  if (fileh != NULL)
    CloseHandle (fileh);
#endif
  if (wpath != NULL)
    free (wpath);
  if (abswpath != NULL)
    free (abswpath);

  return -1;
}

int
ntlink_unlink(const char *path)
{
  wchar_t *wpath = NULL;
  int exists;
  WIN32_FIND_DATAW finddata;
  DWORD lerr;

  if (strtowchar (path, &wpath, CP_THREAD_ACP) < 0)
    goto fail;

  exists = PathExistsW (wpath, &finddata);
  if (exists <= 0)
  {
    /* path must exist */
    if (exists == 0)
      errno = ENOENT;
    else
      errno = EACCESS;
    goto fail;
  }
  if (finddata.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
  {
    BOOL bres;
    SetLastError (0);
    if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      bres = RemoveDirectoryW (wpath);
    else
      bres = DeleteFileW (wpath);
    lerr = GetLastError ();
    if (bres == 0)
    {
      /* TODO: Set errno from lerr */
      errno = EIO;
      goto fail;
    }
#if 0
  } else if (finddata.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))
  {

/* This code is disabled, because unlink() is not required to leave an empty
 * directory behind ('break link' behaviour), it is required to remove the link
 * completely ('remove link' behaviour), and simple RemoveDirectory() will suffice.
 */

    /* Unjunc that junction */

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
#endif
  }
  else if (finddata.dwFileAttributes & FILE_ATTRIBUTE_NORMAL)
  {
    _wunlink (wpath);
  }

  free (wpath);

  return 0;
fail:
  if (wpath != NULL)
    free (wpath);

  return -1;
}
