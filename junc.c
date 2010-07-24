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
#include <stdio.h>
#include <string.h>

#include "quasisymlink.h"

void usage (char **argv)
{
    printf (
"\
Usage: %s <operation> [arguments ...]\n\
Operations:\n\
  l - link\n\
    arguments - <link destination> <link name>\n\
  u - unlink\n\
    arguments - <link name>\n\
  i - link information\n\
    arguments - <link name>\n\
", argv[0]);
}

int
main (int argc, char **argv)
{
  if (argc < 2)
  {
    if (argc > 0)
      usage (argv);
    return 1;
  }
  if (strcmp (argv[1],"l") == 0)
  {
    if (argc < 4)
    {
      usage (argv);
      return 1;
    }
    return ntlink_symlink (argv[2], argv[3]);
  }
  else if (strcmp (argv[1],"u") == 0)
  {
    if (argc < 3)
    {
      usage (argv);
      return 1;
    }
    return ntlink_unlink (argv[2]);
  }
  else if (strcmp (argv[1],"i") == 0)
  {
    char buffer[1024];
    int read_ret;
    if (argc < 3)
    {
      usage (argv);
      return 1;
    }
    read_ret = ntlink_readlink (argv[2], buffer, 1024);
    if (read_ret > 0)
      buffer[read_ret] = 0;
    printf ("%s\n", buffer);
    return 0;
  }
  usage (argv);
  return 1;
}