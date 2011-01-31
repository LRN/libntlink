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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "walk.h"
#include "quasisymlink.h"

/*
  basedir - root of the tree
  name - absolute or relative to basedir
 */
int backup_links (wchar_t *basedir, wchar_t *name, FILE *f, int dry, int recursive, int reljunc)
{
  wchar_t *absname;
  int r = 0;
  int islink, isdir, isdirlnk, isjunc;
  struct stat s;
  if (f == NULL)
    f = stdout;
  r = GetAbsNameW (name, &absname, basedir, 2);
  if (r != 0)
    return r;
  r = ntlink_lstatw (absname, &s);
  if (r < 0)
  {
    free (absname);
    return 0;
  }
  islink = S_ISLNK (s.st_mode);
  isdirlnk = S_ISDIRLNK (s.st_mode);
  isdir = S_ISDIR (s.st_mode);
  isjunc = S_ISJUN (s.st_mode);
  if (islink || isdirlnk || isjunc)
  {
    wchar_t *rel;
    if (GetRelNameW (name, &rel, basedir) >= 0)
    {
      wchar_t tmp[MAX_PATH + 1];
      wchar_t *tmpptr = NULL;
      ssize_t linklen = ntlink_readlinkw (absname, tmp, MAX_PATH);
      if (tmp[linklen] != L'\0') 
        tmp[linklen] = L'\0';
      if (isjunc && reljunc)
      {
        if ((linklen < 4) || tmp[0] != L'\\' || tmp[3] != L'\\' || tmp[1] != L'?' || tmp[2] != L'?')
          r = GetRelNameW (tmp, &tmpptr, basedir);
        else
          r = GetRelNameW (&tmp[4], &tmpptr, basedir);
        if (r >= 0)
        {
          linklen = wcslen (tmpptr);
        }
        else
        {
          tmpptr = NULL;
        }
      }
      else
      {
        if (linklen != wcslen (tmp))
        {
          fprintf (stderr, "warning: link length mismatch\n");
        }
        if ((linklen < 4) || tmp[0] != L'\\' || tmp[3] != L'\\' || tmp[1] != L'?' || tmp[2] != L'?')
          tmpptr = wcsdup(tmp);
        else
        {
          tmpptr = wcsdup(&tmp[4]);
          linklen -= 4;
        }
      }
      /* now linklen is the same as the result of wcslen */
      if (tmpptr != NULL)
      {
        if (dry || ntlink_unlinkw(absname) == 0)
          fwprintf (f, L"link %c %d %s %d %s\n", isdirlnk ? L'd' : isjunc ? L'j' : L'f', wcslen (rel), rel, linklen, tmpptr);
        free (tmpptr);
      }
      free (rel);
    }
  }
  else if (isdir && recursive)
  {
    int i, cleanup = 0;
    wchar_t tmp[MAX_PATH];
    walk_iteratorw *walker;
    int wlen = wcslen (absname);
    walker = walk_allocw (NULL, absname, WALK_FLAG_DONT_FOLLOW_SYMLINKS);
    walker = walk_nextw (walker);
    for (i = 0; walker != NULL && i < walker->nitems; i++)
    {
      if (walker->items[i].cFileName[0] == L'\0')
        continue;
      if (MAX_PATH - wlen - 1 - wcslen (walker->items[i].cFileName) - 1 < 0)
        continue;
      wcsncpy (tmp, absname, MAX_PATH);
      wcsncat (tmp, L"\\", MAX_PATH - wlen);
      wcsncat (tmp, walker->items[i].cFileName, MAX_PATH - wlen - 1);
      tmp[MAX_PATH - 1] = 0;
      r = backup_links (basedir, tmp, f, dry, recursive, reljunc);
      if (r < 0)
        break;
    }
    freeiterw (walker);
  }
  free (absname);
  return r;
}

int restore_links (wchar_t *basedir, wchar_t *name, FILE *f, int dry)
{
  if (f == NULL)
    f = stdin;
  int b, c, go = 1;
  wchar_t magic[6];
  magic[5] = L'\0';
  while (1)
  {
    /* Some of the return statements will leak memory, but we don't care, since the process is going to die anyway */
    size_t linklen = 0, targetlen = 0;
    wchar_t *link, *target;
    wchar_t linktype;
    int i;
    b = fread (magic, 1, sizeof (wchar_t) * 5, f);
    if (b == 0)
      return 0;
    if ((b < sizeof (wchar_t) * 5) || (wcscmp (magic, L"link ") != 0))
      return -1;

    b = fread (magic, 1, sizeof (wchar_t) * 2, f);
    if (b != sizeof (wchar_t) * 2 || magic[1] != L' ')
      return -2;
    linktype = magic[0];

    b = fread (magic, 1, sizeof (wchar_t), f);
    for (i = 0; b == sizeof (wchar_t) && magic[0] != L' ' && magic[0] != L'\n' && magic[0] >= L'0' && magic[0] <= L'9'; i++)
    {
      linklen = linklen * (10 * i) + (magic[0] - L'0');
      b = fread (magic, 1, sizeof (wchar_t), f);
    }
    if (linklen == 0 || magic[0] != L' ')
      return -3;
    link = malloc (sizeof (wchar_t) * (linklen + 1));
    b = fread (link, 1, sizeof (wchar_t) * (linklen), f);
    if (b != sizeof (wchar_t) * (linklen))
      return -4;
    link[linklen] = L'\0';
    b = fread (magic, 1, sizeof (wchar_t), f);
    if (b != sizeof (wchar_t) || magic[0] != L' ')
      return -5;

    b = fread (magic, 1, sizeof (wchar_t), f);
    for (i = 0; b == sizeof (wchar_t) && magic[0] != L' ' && magic[0] != L'\n' && magic[0] >= L'0' && magic[0] <= L'9'; i++)
    {
      targetlen = targetlen * (10 * i) + (magic[0] - L'0');
      b = fread (magic, 1, sizeof (wchar_t), f);
    }
    if (targetlen == 0 || magic[0] != L' ')
      return -6;
    target = malloc (sizeof (wchar_t) * (targetlen + 1));
    b = fread (target, 1, sizeof (wchar_t) * (targetlen), f);
    if (b != sizeof (wchar_t) * (targetlen))
      return -7;
    target[targetlen] = L'\0';
    b = fread (magic, 1, sizeof (wchar_t), f);
    if (b != sizeof (wchar_t) || magic[0] != L'\n')
      return -8;

    {
      WIN32_FIND_DATAW finddata;
      int r;
      /* the link name must exist in the link-less tree */
      r = PathExistsW (link, &finddata, PATH_EXISTS_FLAG_DONT_FOLLOW_SYMLINKS);
      /* zero means that the link name does not exist yet, and that we haven't had
       * any problems checking that (access restriction), and that the path to it
       * does not contain links; note that the absolute path MIGHT contain links, but
       * we only test relative path here.
       */
      if (r == 0)
      {
        switch (linktype)
        {
          /* Use blind linking to be able to create relative links (otherwise a call
           * to PathExistsW() will break on paths relative to the link instead of %CD% */
        case L'd':
          r = ntlink_blind_symlinkw (target, link, BLIND_SYMLINK_DIR, basedir);
          break;
        case L'f':
          r = ntlink_blind_symlinkw (target, link, BLIND_SYMLINK_FILE, basedir);
          break;
        case L'j':
          r = ntlink_blind_symlinkw (target, link, BLIND_JUNCTION, basedir);
          break;
        default:
          break;
        }
      }
    }

    free (link);
    free (target);
  }
  return 0;
}

void usage (wchar_t **argv)
{
  fwprintf (stderr,
L"\
Usage: %s <operation> <base directory> <file or directory name> [option [option argument] ...]\n\
Operations:\n\
  b - backup\n\
  r - restore\n\
Options:\n\
  j - make junction point paths relative when packing them up (they are always absolute)\n\
  r - when backing up a directory examine its contents as well\n\
  d - dry run: do not remove/restore links, print only\n\
  f <filename> - read/append backup information from/to <filename>\n\
  (otherwise - read/write backup information from/to stdin/stdout)\n\
", argv[0]);
}

int
wmain (int argc, wchar_t **argv)
{
  FILE *f = NULL;
  int dry = 0;
  int recursive = 0;
  int count = 0;
  int reljunc = 0;
  if (argc < 4)
  {
    fwprintf (stderr, L"Must have at least 3 arguments\n");
    if (argc > 0)
      usage (argv);
    return 1;
  }
  if (argc > 4)
  {
    int i;
    for (i = 4; i < argc; i++)
    {
      if (wcscmp (argv[i], L"r") == 0)
        recursive = 1;
      else if (wcscmp (argv[i], L"d") == 0)
        dry = 1;
      else if (wcscmp (argv[i], L"j") == 0)
        reljunc = 1;
      else if (wcscmp (argv[i], L"f") == 0)
      {
        const wchar_t *mode = NULL;
        if (i + 1 >= argc)
        {
          fwprintf (stderr, L"Option 'f' requires extra argument\n");
          usage (argv);
          return 1;
        }
        if (wcscmp (argv[1],L"b") == 0)
          mode = L"ab";
        else if (wcscmp (argv[1],L"r") == 0)
          mode = L"rb";
        else
        {
          fwprintf (stderr, L"Unknown operation `%s'\n", argv[1]);
          usage (argv);
          return 1;
        }
        f = _wfopen (argv[i + 1], mode);
        if (f == NULL)
        {
          fwprintf (stderr, L"Failed to open file `%s' with mode `%s': %d - %S\n", argv[i + 1], mode, errno, strerror (errno));
          return 2;
        }
        i += 1;
      }
    }
  }
  if (wcscmp (argv[1],L"b") == 0)
  {
    int r = backup_links (argv[2], argv[3], f, dry, recursive, reljunc);
    if (f != NULL)
      fclose (f);
    return r;
  }
  else if (wcscmp (argv[1],L"r") == 0)
  {
    int r = restore_links (argv[2], argv[3], f, dry);
    if (f != NULL)
      fclose (f);
    return r;
  }
  fwprintf (stderr, L"Unknown operation `%s'\n", argv[1]);
  usage (argv);
  return 1;
}