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

#ifndef uid_t
  #define uid_t int
#endif

#ifndef gid_t
  #define gid_t uid_t
#endif

int ntlink_symlink(const char *path1, const char *path2);
int ntlink_lchown(const char *path, uid_t owner, gid_t group);
int ntlink_link(const char *path1, const char *path2);
int ntlink_lstat(const char *restrict path, struct _stat *restrict buf);
ssize_t ntlink_readlink(const char *restrict path, char *restrict buf,
    size_t bufsize);
int ntlink_unlink(const char *path);

#endif /* __NTLINK_SYMLINK_H__ */