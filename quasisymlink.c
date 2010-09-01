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
#include <direct.h>
#include <stdio.h>

#include "quasisymlink.h"
#include "extra_string.h"
#include "misc.h"
#include "juncpoint.h"

int
ntlink_symlinkw(const wchar_t *wpath1, const wchar_t *wpath2)
{
  int exists;
  WIN32_FIND_DATAW finddata;
#if _WIN32_WINNT >= 0x0600
  BOOL err;
  DWORD lerr;
#endif

  exists = PathExistsW ((wchar_t *) wpath2, &finddata, PATH_EXISTS_FLAG_NOTHING);
  if (exists != 0)
  {
    if (exists > 0)
      errno = EEXIST;
    else
      errno = EACCESS;
    goto fail;
  }

  exists = PathExistsW ((wchar_t *) wpath1, &finddata, PATH_EXISTS_FLAG_NOTHING);
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
  err = CreateSymbolicLinkW ((wchar_t *) wpath2, (wchar_t *) wpath1, (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ? 0 : SYMBOLIC_LINK_FLAG_DIRECTORY);
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

  return 0;
fail:

  return -1;
}

int
ntlink_symlink(const char *path1, const char *path2)
{
  int ret;
  wchar_t *wpath1 = NULL, *wpath2 = NULL;

  if (strtowchar (path1, &wpath1, CP_THREAD_ACP) < 0)
    goto fail;
  if (strtowchar (path2, &wpath2, CP_THREAD_ACP) < 0)
    goto fail;

  ret = ntlink_symlinkw (wpath1, wpath2);

  free (wpath1);
  free (wpath2);

  return ret;
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
ntlink_linkw(const wchar_t *wpath1, const wchar_t *wpath2)
{
  int exists;
  WIN32_FIND_DATAW finddata;

  exists = PathExistsW ((wchar_t *) wpath2, &finddata, PATH_EXISTS_FLAG_NOTHING);
  if (exists != 0)
  {
    if (exists > 0)
      errno = EEXIST;
    else
      errno = EACCESS;
    goto fail;
  }

  exists = PathExistsW ((wchar_t *) wpath1, &finddata, PATH_EXISTS_FLAG_NOTHING);
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
  else
  {
    /* Create a hard link to target file */
    BOOL ret;
    ret = CreateHardLinkW ((wchar_t *) wpath2, (wchar_t *) wpath1, NULL);
    if (ret == 0)
    {
      errno = EIO;
      goto fail;
    }
  }

  return 0;
fail:
  return -1;
}

int
ntlink_link(const char *path1, const char *path2)
{
  int ret;
  wchar_t *wpath1 = NULL, *wpath2 = NULL/*, *wpath1_unp = NULL*/;

  if (strtowchar (path1, &wpath1, CP_THREAD_ACP) < 0)
    goto fail;
  if (strtowchar (path2, &wpath2, CP_THREAD_ACP) < 0)
    goto fail;

  ret = ntlink_linkw (wpath1, wpath2);

  free (wpath1);
  free (wpath2);

  return ret;
fail:
  if (wpath1 != NULL)
    free (wpath1);
  if (wpath2 != NULL)
    free (wpath2);

  return -1;
}

int
ntlink_lstatw(const wchar_t *wpath, struct stat *buf)
{
  int exists;
  int result = 0;
  WIN32_FIND_DATAW finddata;
  wchar_t *abswpath = NULL;
#if _WIN32_WINNT >= 0x0600
  HANDLE fileh = NULL;
  DWORD lerr;
#endif

  GetAbsName ((wchar_t *) wpath, &abswpath);

  exists = PathExistsW ((wchar_t *) wpath, &finddata, PATH_EXISTS_FLAG_NOTHING);
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

  return result;
fail:
#if _WIN32_WINNT >= 0x0600
  if (fileh != NULL)
    CloseHandle (fileh);
#endif
  if (abswpath != NULL)
    free (abswpath);
  return -1;
}


int
ntlink_lstat(const char *path, struct stat *buf)
{
  int ret;
  wchar_t *wpath = NULL;

  if (strtowchar (path, &wpath, CP_THREAD_ACP) < 0)
    goto fail;

  ret = ntlink_lstatw (wpath, buf);

  free (wpath);

  return ret;
fail:
  if (wpath != NULL)
    free (wpath);
  return -1;
}

ssize_t
ntlink_readlinkw(const wchar_t *wpath, wchar_t *buf,
    size_t bufsize)
{
  wchar_t *abswpath = NULL;
  int exists;
  int result = 0;
  WIN32_FIND_DATAW finddata;
#if _WIN32_WINNT >= 0x0600
  HANDLE fileh = NULL;
  wchar_t *wtarget = NULL;
  DWORD lerr;
#endif

  GetAbsName ((wchar_t *) wpath, &abswpath);

  exists = PathExistsW ((wchar_t *) wpath, &finddata, PATH_EXISTS_FLAG_NOTHING);
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
    fileh = CreateFileW (wpath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? FILE_FLAG_BACKUP_SEMANTICS : 0, NULL);
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
/*
    conv_result = wchartostr (wtarget, &target, CP_THREAD_ACP);
    free (wtarget);
    wtarget = NULL;
    if (conv_result < 0)
      goto fail;
*/
    len = wcslen (wtarget);
    if (len > bufsize)
      len = bufsize;
    memcpy (buf, wtarget, (len + 1) * sizeof (wchar_t));
    free (wtarget);
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
    int len = wcslen (wpath);
    if (len > bufsize)
      len = bufsize;
    memcpy (buf, wpath, len * sizeof (wchar_t));
    result = len;
  }
#endif

  if (abswpath != NULL)
    free (abswpath);

  return result;
fail:
#if _WIN32_WINNT >= 0x0600
  if (wtarget != NULL)
    free (wtarget);
  if (fileh != NULL)
    CloseHandle (fileh);
#endif
  if (abswpath != NULL)
    free (abswpath);

  return -1;
}


ssize_t
ntlink_readlink(const char *path, char *buf, size_t bufsize)
{
  int ret;
  wchar_t *wpath = NULL;
  wchar_t *wbuf = NULL;

  if (strtowchar (path, &wpath, CP_THREAD_ACP) < 0)
    goto fail;

  if (bufsize > 0)
    wbuf = (wchar_t *) malloc (sizeof (wchar_t) * bufsize);

  ret = ntlink_readlinkw (wpath, wbuf, bufsize);

  free (wpath);

  if (bufsize > 0)
  {
    if (ret == 0)
    {
      char *tmp;
      if (wchartostr (wbuf, &tmp, CP_THREAD_ACP) < 0)
        ret = -1;
      else
      {
        strncpy (buf, tmp, bufsize);
        free (tmp);
      }
    }
    free (wbuf);
  }

  return ret;
fail:
  if (wpath != NULL)
    free (wpath);

  return -1;
}

int
ntlink_unlinkw(const wchar_t *wpath)
{
  int exists;
  WIN32_FIND_DATAW finddata;
  DWORD lerr;

  exists = PathExistsW ((wchar_t *) wpath, &finddata, PATH_EXISTS_FLAG_NOTHING);
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
  else if (~finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
  {
    _wunlink (wpath);
  }
  else if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
  {
    _wrmdir (wpath);
  }

  return 0;
fail:
  return -1;
}


int
ntlink_unlink(const char *path)
{
  int ret;
  wchar_t *wpath = NULL;

  if (strtowchar (path, &wpath, CP_THREAD_ACP) < 0)
    goto fail;

  ret = ntlink_unlinkw (wpath);

  free (wpath);

  return ret;
fail:
  if (wpath != NULL)
    free (wpath);

  return -1;
}

int ntlink_renamew(const wchar_t *wpath1, const wchar_t *wpath2)
{
  int exists;
  struct stat stat1, stat2;
  WIN32_FIND_DATAW finddata1, finddata2;

  memset (&stat1, 0, sizeof (stat1));
  memset (&stat2, 0, sizeof (stat2));

  ntlink_lstatw (wpath1, &stat1);
  ntlink_lstatw (wpath2, &stat2);

  /* If the old argument and the new argument resolve to the same existing file, rename() shall return successfully and perform no other action. */
  if (stat1.st_ino != 0 && stat1.st_ino == stat2.st_ino)
    return 0;

  exists = PathExistsW ((wchar_t *) wpath1, &finddata1, PATH_EXISTS_FLAG_NOTHING);
  if (exists <= 0)
  {
    /* path1 must exist */
    if (exists > 0)
      errno = ENOENT;
    else
      errno = EACCESS;
    goto fail;
  }

  exists = PathExistsW ((wchar_t *) wpath2, &finddata2, PATH_EXISTS_FLAG_NOTHING);
  if (exists > 0)
  {
    /* If the old argument points to the pathname of a directory, the new argument shall not point to the pathname of a file that is not a directory. */
    if ((finddata1.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && !(finddata2.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)))
    {
      errno = ENOTDIR;
      goto fail;
    }
    /* If the old argument points to the pathname of a file that is not a directory, the new argument shall not point to the pathname of a directory. */
    if ((~finddata1.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && !(~finddata2.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)))
    {
      errno = EISDIR;
      goto fail;
    }

    /* If the directory named by the new argument exists, it shall be removed and old renamed to new. */
    /* ntlink_unlink() removes a directory */

    /* If the link named by the new argument exists, it shall be removed and old renamed to new. */
    /* ntlink_unlink() removes a file*/

    /* If new names an existing directory, it shall be required to be an empty directory.*/
    /* ntlink_unlink() does not remove non-empty directories, fails with ENOTEMPTY */

    /* If the new argument points to a pathname of a symbolic link, the symbolic link shall be removed. */
    if (ntlink_unlinkw (wpath2) != 0)
      goto fail;
  }

  /* If the old argument points to a pathname of a symbolic link, the symbolic link shall be renamed. */
  /* TODO: make sure that this is what really happens */

  /* The new pathname shall not contain a path prefix that names old. */
  /* Hope that MoveFileEx will figure this out */

  /* If the rename() function fails for any reason other than [EIO], any file named by new shall be unaffected. */
  /* Can't see this happening. We can't tell MoveFileEx to remove the existing directory */

  /* TODO: MoveFileEx can't move directories between drives, fix that */
  if (MoveFileExW (wpath1, wpath2, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH) == 0)
  {
    DWORD err = GetLastError ();
    /*FIXME: set errno from GetLastError()*/
    errno = EINVAL;
    goto fail;
  }

  return 0;
fail:
  return -1;
}

int ntlink_rename(const char *path1, const char *path2)
{
  int ret;
  wchar_t *wpath1 = NULL, *wpath2 = NULL;

  if (strtowchar (path1, &wpath1, CP_THREAD_ACP) < 0)
    goto fail;
  if (strtowchar (path2, &wpath2, CP_THREAD_ACP) < 0)
    goto fail;

  ret = ntlink_renamew (wpath1, wpath2);

  free (wpath1);
  free (wpath2);

  return ret;

fail:
  if (wpath1 != NULL)
    free (wpath1);
  if (wpath2 != NULL)
    free (wpath2);

  return -1;

}