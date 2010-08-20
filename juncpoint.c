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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <windows.h>
#include <winioctl.h>

#include <misc.h>

/**
 * utf8towchar:
 * @str: a string (UTF-8-encoded) to convert
 * @wretstr: a pointer to variable (pointer to wchar_t) to receive the result
 *
 * Allocates new wchar_t string, fills it with converted @str and writes the
 * pointer to it into @wretstr. If the function fails, *@wretstr remains
 * unmodified.
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
utf8towchar (char *str, wchar_t **wretstr)
{
  wchar_t *wstr;
  size_t len, lenc;
  len = mbstowcs (NULL, str, 0);
  if (len < 0)
  {
    return -1;
  }
  
  wstr = malloc (sizeof (wchar_t) * (len + 1));
  if (wstr == NULL)
  {
    return -2;
  }
  
  lenc = mbstowcs (wstr, str, len);
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
 * IsJunctionPointA:
 * @path: full directory path (8-byte ASCII)
 *
 * Checks whether @path directory is a junction point or not
 *
 * Returns:
 *  0 - @path is not a junction point
 *  1 - @path is a junction point
 *  2 - @path is a directory AND a junction point
 * -1 - failed to get file attributes
 * 
 */
int
IsJunctionPointA (char *path)
{
  BOOL result;
  WIN32_FILE_ATTRIBUTE_DATA attr_data;
  
  result = GetFileAttributesExA (path, GetFileExInfoStandard, &attr_data);
  if (result == 0)
    return -1;
  if (attr_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT & FILE_ATTRIBUTE_DIRECTORY)
    return 2;
  if (attr_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
    return 1;
  return 0;
}

/**
 * IsJunctionPointW:
 * @path: full directory path (UTF-16)
 *
 * Checks whether @path directory is a junction point or not
 *
 * Returns:
 *  0 - @path is not a junction point
 *  1 - @path is a junction point
 *  2 - @path is a directory AND a junction point
 * -1 - failed to get file attributes
 * 
 */
int
IsJunctionPointW (wchar_t *path)
{
  BOOL result;
  WIN32_FILE_ATTRIBUTE_DATA attr_data;
  
  result = GetFileAttributesExW (path, GetFileExInfoStandard, &attr_data);
  if (result == 0)
    return -1;
  if (attr_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT & FILE_ATTRIBUTE_DIRECTORY)
    return 2;
  if (attr_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
    return 1;
  return 0;
}

/**
 * IsJunctionPointU:
 * @path: full directory path (UTF-8)
 *
 * Checks whether @path directory is a junction point or not
 *
 * Returns:
 *  0 - @path is not a junction point
 *  1 - @path is a junction point
 *  2 - @path is a directory AND a junction point
 * -1 - failed to get file attributes
 * -2 - failed to convert @path to UTF-16
 *
 */
int
IsJunctionPointU (char *path)
{
  int err;
  wchar_t *wstr;
  err = utf8towchar (path, &wstr);
  if (err < 0)
  {
    return -2;
  }
  return IsJunctionPointW (wstr);
}

/**
 * SetJuncPointW:
 * @path1: full directory path (UTF-16), unparseable
 * @path2: full directory path (UTF-16)
 *
 * Makes @path2 directory into a junction point that points
 * to @path1 directory.
 * If @path2 does not exist, it will be created.
 * If @path2 exists, it must be empty.
 * If @path2 is a junction point, it will be re-targeted.
 * @path1 must be in unparseable form (e.g. \??\C:\Window)
 * Trailing slash does not matter (for both @path1 and @path2).
 *
 * Returns:
 *  0 - success
 * -1 - failed to create/open @path2
 * -2 - failed to allocate memory
 * -3 - failed to set junction
 *
 */
int
SetJuncPointW (wchar_t *path1, wchar_t *path2)
{
  HANDLE dir_handle;
  BOOL ret;
  REPARSE_DATA_BUFFER *rep_buf;
  size_t len1, len2, reparse_size;
  DWORD returned_bytes;

  len1 = wcslen (path1);
  len2 = 0;

  if (PathExistsW (path2, NULL) <= 0)
  {
    if (CreateDirectoryW (path2, NULL) == 0)
    {
      return -4;
    }
  }
  /* FIXME: narrow access rights (just enough to link) */
  dir_handle = CreateFileW (path2, GENERIC_READ | GENERIC_WRITE, 0, NULL,
      OPEN_ALWAYS, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
      0);
  if (dir_handle == INVALID_HANDLE_VALUE)
  {
    return -1;
  }

  reparse_size = (len1 + 1 + len2 + 1) * sizeof (wchar_t) +
    sizeof (rep_buf->MountPointReparseBuffer) +
    REPARSE_DATA_BUFFER_HEADER_SIZE -
    sizeof (rep_buf->MountPointReparseBuffer.PathBuffer);

  rep_buf = malloc (reparse_size);
  if (rep_buf == NULL)
  {
    CloseHandle (dir_handle);
    return -2;
  }
  memset (rep_buf, 0, reparse_size);

  rep_buf->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
  rep_buf->ReparseDataLength = sizeof (rep_buf->MountPointReparseBuffer) -
      sizeof (rep_buf->MountPointReparseBuffer.PathBuffer) +
      (len1 + 1 + len2 + 1) * sizeof (wchar_t);
  rep_buf->Reserved = 0;
  rep_buf->MountPointReparseBuffer.SubstituteNameOffset = 0;
  rep_buf->MountPointReparseBuffer.SubstituteNameLength = len1 *
      sizeof (wchar_t);
  rep_buf->MountPointReparseBuffer.PrintNameOffset = (len1 + 1) *
      sizeof (wchar_t);
  rep_buf->MountPointReparseBuffer.PrintNameLength = len2 * sizeof (wchar_t);

  memcpy (&((BYTE *) rep_buf->MountPointReparseBuffer.PathBuffer)
      [rep_buf->MountPointReparseBuffer.SubstituteNameOffset],
      path1, rep_buf->MountPointReparseBuffer.SubstituteNameLength);
  memcpy (&((BYTE *) rep_buf->MountPointReparseBuffer.PathBuffer)
      [rep_buf->MountPointReparseBuffer.PrintNameOffset],
      "", rep_buf->MountPointReparseBuffer.PrintNameLength);

  ret = DeviceIoControl (dir_handle, FSCTL_SET_REPARSE_POINT, rep_buf, reparse_size, NULL, 0, &returned_bytes, NULL);
  if (ret == 0)
  {
    return -3;
  }

  free (rep_buf);

  CloseHandle (dir_handle);

  return 0;
}

/**
 * UnJuncPointW:
 * @path: full directory path (UTF-16)
 *
 * Removes a junction point @path. Leaves empty directory @path afterwards.
 *
 * Returns:
 *  0 - success
 * -1 - failed to create/open @path
 * -2 - failed to allocate memory
 * -3 - failed to delete junction
 *
 */
int
UnJuncPointW (wchar_t *path)
{
  HANDLE dir_handle;
  BOOL ret;
  REPARSE_DATA_BUFFER *rep_buf;
  size_t len1, len2, reparse_size;
  DWORD returned_bytes2;

  len1 = wcslen (path);
  len2 = 0;

  /* FIXME: narrow access rights (just enough to link) */
  dir_handle = CreateFileW (path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
      OPEN_ALWAYS, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
      0);
  if (dir_handle == INVALID_HANDLE_VALUE)
  {
    return -1;
  }

  len1 = 0;

  reparse_size = (len1 + 1 + len2 + 1) * sizeof (wchar_t) +
    sizeof (rep_buf->MountPointReparseBuffer) +
    REPARSE_DATA_BUFFER_HEADER_SIZE -
    sizeof (rep_buf->MountPointReparseBuffer.PathBuffer);

  rep_buf = malloc (reparse_size);
  if (rep_buf == NULL)
  {
    CloseHandle (dir_handle);
    return -2;
  }
  memset (rep_buf, 0, reparse_size);

  rep_buf->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
  rep_buf->ReparseDataLength = 0;

  ret = DeviceIoControl (dir_handle, FSCTL_DELETE_REPARSE_POINT, rep_buf, REPARSE_GUID_DATA_BUFFER_HEADER_SIZE, NULL, 0, &returned_bytes2, NULL);
  if (ret == 0)
  {
    return -3;
  }

  if (rep_buf != NULL)
    free (rep_buf);

  CloseHandle (dir_handle);

  return 0;
}

/**
 * GetJuncPointW:
 * @path1: a pointer to variable (pointer to wchar_t) to receive result
 * @path2: full directory path (UTF-16)
 *
 * Retrieves target directory for junction point @path2, allocates a buffer
 * for it and writes a pointer to that buffer into @path1

 * @path2 must exist.
 * @path2 must be a junction point.
 * String in @path1 will be in unparseable form - with \??\ prefix
 * (i.e. \??\C:\Windows) - and without a trailing slash.
 * If the function fails, *@path1 remains unmodified.
 * Free @path1 with free() when it is no longer needed.
 *
 * Returns:
 * 0  - success
 * -1 - failed to open @path2
 * -2 - failed to get junction
 * -3 - failed to allocate memory
 *
 */
int
GetJuncPointW (wchar_t **path1, wchar_t *path2)
{
  HANDLE dir_handle;
  BOOL ret;
  REPARSE_DATA_BUFFER *rep_buf;
  size_t len1, len2;
  DWORD returned_bytes;
  BYTE returned_data[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];

  len2 = wcslen (path2);

  /* FIXME: narrow access rights (just enough to link) */
  dir_handle = CreateFileW (path2, GENERIC_READ | GENERIC_WRITE, 0, NULL,
      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
      0);
  if (dir_handle == INVALID_HANDLE_VALUE)
  {
    return -1;
  }

  ret = DeviceIoControl (dir_handle, FSCTL_GET_REPARSE_POINT, NULL, 0, returned_data, 1024, &returned_bytes, NULL);
  if (ret == 0)
  {
    return -2;
  }

  CloseHandle (dir_handle);

  rep_buf = (REPARSE_DATA_BUFFER *) returned_data;
  len1 = rep_buf->MountPointReparseBuffer.SubstituteNameLength;

  *path1 = malloc (len1 + sizeof (wchar_t));
  if (*path1 == NULL)
  {
    return -3;
  }
  
  memcpy (*path1, &((BYTE *) rep_buf->MountPointReparseBuffer.PathBuffer)
      [rep_buf->MountPointReparseBuffer.SubstituteNameOffset], len1);

  (*path1)[len1 / sizeof (wchar_t)] = 0;

  return 0;
}
