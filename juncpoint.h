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

#ifndef __NTLINK_JUNCPOINT_H__
#define __NTLINK_JUNCPOINT_H__

int IsJunctionPointA (char *path);
int IsJunctionPointW (wchar_t *path);
int IsJunctionPointU (char *path);
int SetJuncPointW (wchar_t *path1, wchar_t *path2);
int UnJuncPointW (wchar_t *path);
int GetJuncPointW (wchar_t **path1, wchar_t *path2);


#endif /* __NTLINK_JUNCPOINT_H__ */