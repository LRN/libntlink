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

#include <stdio.h>
#include <windows.h>

enum WALK_FLAGS
{
  WALK_FLAG_NONE =                 0x00000000,
  WALK_FLAG_DONT_FOLLOW_SYMLINKS = 0x00000001,
  WALK_FLAG_DEPTH_FIRST =          0x00000002
};

struct _walk_iteratorw
{
  int depth;
  wchar_t wdir[MAX_PATH];
  int nitems;
  WIN32_FIND_DATAW *items;
};

typedef struct _walk_iteratorw walk_iteratorw;

walk_iteratorw *walk_allocw (wchar_t *root, wchar_t *wdir, unsigned flags);
walk_iteratorw *walk_nextw (walk_iteratorw *iter);
void freeiterw (walk_iteratorw *iter);