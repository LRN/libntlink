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

#ifndef __NTLINK_SYMLINK_H__
#define __NTLINK_SYMLINK_H__

#include <unistd.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef uid_t
  #define uid_t int
#endif

#ifndef gid_t
  #define gid_t uid_t
#endif

#ifndef _S_IFLNK
#define _S_IFLNK (_S_IFREG & _S_IFCHR)
#endif
#ifndef S_IFLNK
#define S_IFLNK _S_IFLNK
#endif
#ifndef _S_ISLNK
#define _S_ISLNK(a) (a & _S_IFLNK)
#endif
#ifndef S_ISLNK
#define S_ISLNK _S_ISLNK
#endif

#ifndef lstat
#define lstat ntlink_lstat
#endif
#ifndef symlink
#define symlink ntlink_symlink
#endif
#ifndef lchown
#define lchown ntlink_lchown
#endif
#ifndef link
#define link ntlink_link
#endif
#ifndef readlink
#define readlink ntlink_readlink
#endif
#ifndef unlink
#define unlink ntlink_unlink
#endif
#define rename ntlink_rename

int ntlink_symlink(const char *path1, const char *path2);
int ntlink_lchown(const char *path, uid_t owner, gid_t group);
int ntlink_link(const char *path1, const char *path2);
int ntlink_lstat(const char *path, struct stat *buf);
ssize_t ntlink_readlink(const char *path, char *buf,
    size_t bufsize);
int ntlink_unlink(const char *path);
int ntlink_rename(const char *path1, const char *path2);

int ntlink_symlinkw(const wchar_t *path1, const wchar_t *path2);
int ntlink_lchownw(const wchar_t *path, uid_t owner, gid_t group);
int ntlink_linkw(const wchar_t *path1, const wchar_t *path2);
int ntlink_lstatw(const wchar_t *path, struct stat *buf);
ssize_t ntlink_readlinkw(const wchar_t *path, wchar_t *buf,
    size_t bufsize);
int ntlink_unlinkw(const wchar_t *path);
int ntlink_renamew(const wchar_t *path1, const wchar_t *path2);

#ifdef __cplusplus
}
#endif

#endif /* __NTLINK_SYMLINK_H__ */