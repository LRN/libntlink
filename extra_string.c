#include <stdio.h>
#include <string.h>
#include <mbstring.h>
#include <wchar.h>

#include "extra_string.h"

/**
 * strtowchar:
 * @str: a string (UTF-8-encoded) to convert
 * @wretstr: a pointer to variable (pointer to wchar_t) to receive the result
 * @cp: codepage to convert from
 *
 * Allocates new wchar_t string, fills it with converted @str and writes the
 * pointer to it into @wretstr. If the function fails, *@wretstr remains
 * unmodified. Converts from UTF-8 to UTF-16
 * See http://msdn.microsoft.com/en-us/library/dd319072%28VS.85%29.aspx
 * MultiByteToWideChar() documentation for values of cp.
 * CP_THREAD_ACP and CP_UTF8 are recommended.
 * Free the string returned in @wretstr with free() when it is no longer needed
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
strtowchar (const char *str, wchar_t **wretstr, UINT cp)
{
  wchar_t *wstr;
  int len, lenc;
  len = MultiByteToWideChar (cp, 0, str, -1, NULL, 0);
  if (len <= 0)
  {
    return -1;
  }
  
  wstr = malloc (sizeof (wchar_t) * len);
  if (wstr == NULL)
  {
    return -2;
  }
  
  lenc = MultiByteToWideChar (cp, 0, str, -1, wstr, len);
  if (lenc != len)
  {
    free (wstr);
    return -3;
  }
  *wretstr = wstr;
  return 0;
}

/**
 * wchartostr:
 * @wstr: a string (UTF-16-encoded) to convert
 * @wretstr: a pointer to variable (pointer to char) to receive the result
 * @cp: codepage to convert from
 *
 * Allocates new wchar_t string, fills it with converted @wstr and writes the
 * pointer to it into @retstr. If the function fails, *@retstr remains
 * unmodified. Converts from UTF-8 to UTF-16
 * See http://msdn.microsoft.com/en-us/library/dd319072%28VS.85%29.aspx
 * WideCharToMultiByte() documentation for values of cp.
 * CP_THREAD_ACP and CP_UTF8 are recommended.
 * Free the string returned in @retstr with free() when it is no longer needed
 *
 * Returns:
 *  1 - conversion is successful, but some characters were replaced by placeholders
 *  0 - conversion is successful
 * -1 - conversion failed at length-counting phase
 * -2 - conversion failed at memory allocation phase
 * -3 - conversion failed at string conversion phase
 *
 * 
 */
int
wchartostr (const wchar_t *wstr, char **retstr, UINT cp)
{
  char *str;
  int len, lenc;
  BOOL lossy = FALSE;
  len = WideCharToMultiByte (cp, 0, wstr, -1, NULL, 0, NULL, &lossy);
  if (len <= 0)
  {
    return -1;
  }
  
  str = malloc (sizeof (char) * len);
  if (wstr == NULL)
  {
    return -2;
  }
  
  lenc = WideCharToMultiByte (cp, 0, wstr, -1, str, len, NULL, &lossy);
  if (lenc != len)
  {
    free (str);
    return -3;
  }
  *retstr = str;
  if (lossy)
    return 1;
  return 0;
}

/**
 * dup_swprintf:
 * @rlen: pointer to an integer which will hold the length of the resulting
 *   string on return. May be NULL.
 * @format: a format string acceptable by vswprintf ()
 * ...: parameters for @format
 *
 * Allocates new wchar_t string, fills it with the result of vswprintf (@format, ...).
 * Fills @rlen with the length of the new string in wide characters (without counting NULL-terminator)
 * The resulting string must be freed with free () when no longer needed.
 * Calls _vscwprintf () and vswprintf (), so the invalid parameter handler is invoked
 * in some cases, be aware.
 *
 * Returns:
 * NULL - @format is NULL OR _vscwprintf () and vswprintf () do not agree on
 *   the resulting string length OR vswprintf () have failed
 * non-NULL - the resulting string (deallocate it with free ())
 */
wchar_t *dup_swprintf (int *rlen, wchar_t *format, ...)
{
  va_list argptr;
  wchar_t *result = NULL;
  int len = 0;

  if (format == NULL)
    return NULL;

  va_start(argptr, format);

  len = _vscwprintf (format, argptr);
  if (len >= 0)
  {
    result = (wchar_t *) malloc (sizeof (wchar_t *) * (len + 1));
    if (result != NULL)
    {
      int len2 = vswprintf (result, format, argptr);
      if (len2 != len || len2 <= 0)
      {
        free (result);
        result = NULL;
      }
      else if (rlen != NULL)
        *rlen = len;
    }
  }
  va_end(argptr);
  return result;
}

/**
 * dup_sprintf:
 * @rlen: pointer to an integer which will hold the length of the resulting
 *   string on return. May be NULL.
 * @format: a format string acceptable by vsprintf ()
 * ...: parameters for @format
 *
 * Allocates new char string, fills it with the result of vsprintf (@format, ...).
 * Fills @rlen with the length of the new string in characters (without counting NULL-terminator)
 * The resulting string must be freed with free () when no longer needed.
 * Calls _vscprintf () and vsprintf (), so the invalid parameter handler is invoked
 * in some cases, be aware.
 *
 * Returns:
 * NULL - @format is NULL OR _vscprintf () and vsprintf () do not agree on
 *   the resulting string length OR vsprintf () have failed
 * non-NULL - the resulting string (deallocate it with free ())
 */
char *dup_sprintf (int *rlen, char *format, ...)
{
  va_list argptr;
  char *result = NULL;
  int len = 0;

  if (format == NULL)
    return NULL;

  va_start(argptr, format);

  len = _vscprintf (format, argptr);
  if (len >= 0)
  {
    result = (char *) malloc (sizeof (char *) * (len + 1));
    if (result != NULL)
    {
      int len2 = vsprintf (result, format, argptr);
      if (len2 != len || len2 <= 0)
      {
        free (result);
        result = NULL;
      }
      else if (rlen != NULL)
        *rlen = len;
    }
  }
  va_end(argptr);
  return result;
}

/* inc_function must take one argument (a string of some sort) and
 * return pointer to the next character in that string
 */
#define template_tok_r(chartype, tok_r_prefix, chr_prefix, inc_function)\
template_tok_r_header(chartype, tok_r_prefix)\
{\
  chartype *notsep;\
  chartype *ret;\
  if (sep == NULL)\
    return NULL;\
  if (s == NULL)\
    if (lasts == NULL || *lasts == NULL || (*lasts)[0] == 0)\
      return NULL;\
\
  if (s)\
    notsep = s;\
  else\
    notsep = *lasts;\
  while (chr_prefix##chr (sep, notsep[0]) != NULL)\
  {\
    if (notsep[0] == 0)\
    {\
      *lasts = NULL;\
      return NULL;\
    }\
    notsep = inc_function(notsep);\
  }\
  \
  for (ret = notsep; notsep[0] != 0 && chr_prefix##chr (sep, notsep[0]) == NULL; notsep = inc_function(notsep));\
\
  if (notsep[0] == 0)\
  {\
    *lasts = NULL;\
    return ret;\
  }\
  else\
  {\
    notsep[0] = 0;\
    *lasts = inc_function(notsep);\
    if (*lasts[0] == 0)\
      *lasts = NULL;\
    return ret;\
  }\
}\


char inline *strinc (char *s) { return s += 1; }
wchar_t inline *wcsinc (wchar_t *s) { return s += 1; }

template_tok_r(unsigned char, mbs, _mbs, _mbsinc)

template_tok_r(char, str, str, strinc)

template_tok_r(wchar_t, wcs, wcs, wcsinc)

/*
unsigned char *mbstok_r(unsigned char *s, const unsigned char *sep, unsigned char **lasts)
{
  unsigned char *notsep;
  unsigned char *ret;

  if (sep == ((void *)0))
    return ((void *)0);
  if (s == ((void *)0))
    if (lasts == ((void *)0) || *lasts == ((void *)0) || (*lasts)[0] == 0)
      return ((void *)0);

  if (s)
    notsep = s;
  else
    notsep = *lasts;

  while (_mbschr (sep, notsep[0]) != ((void *)0))
  {
    if (notsep[0] == 0)
    {
      *lasts = ((void *)0);
      return ((void *)0);
    }
    notsep = _mbsinc(notsep);
  }

  for (ret = notsep; notsep[0] != 0 && _mbschr (sep, notsep[0]) == ((void *)0); _mbsinc(notsep));

  if (notsep[0] == 0)
  {
    *lasts = ((void *)0);
    return ret;
  }
  else
  {
    notsep[0] = 0;
    *lasts = _mbsinc(notsep);
    return ret;
  }
}

char *strtok_r(char *s, const char *sep, char **lasts){ char *notsep; char *ret; if (sep == ((void *)0)) return ((void *)0); if (s == ((void *)0)) if (lasts == ((void *)0) || *lasts == ((void *)0) || (*lasts)[0] == 0) return ((void *)0); if (s) notsep = s; else notsep = *lasts; while (strchr (sep, notsep[0]) != ((void *)0)) { if (notsep[0] == 0) { *lasts = ((void *)0); return ((void *)0); } notsep = strinc(notsep); } for (ret = notsep; notsep[0] != 0 && strchr (sep, notsep[0]) == ((void *)0); strinc(notsep)); if (notsep[0] == 0) { *lasts = ((void *)0); return ret; } else { notsep[0] = 0; *lasts = strinc(notsep); return ret; }}

wchar_t *wcstok_r(wchar_t *s, const wchar_t *sep, wchar_t **lasts)
{
  wchar_t *notsep;
  wchar_t *ret;

  if (sep == ((void *)0))
    return ((void *)0);
  if (s == ((void *)0))
    if (lasts == ((void *)0) || *lasts == ((void *)0) || (*lasts)[0] == 0)
      return ((void *)0);

  if (s)
    notsep = s;
  else
    notsep = *lasts;

  while (wcschr (sep, notsep[0]) != ((void *)0))
  {
    if (notsep[0] == 0)
    {
      *lasts = ((void *)0);
      return ((void *)0);
    }
    notsep = wcsinc(notsep);
  }

  for (ret = notsep; notsep[0] != 0 && wcschr (sep, notsep[0]) == ((void *)0); wcsinc(notsep));

  if (notsep[0] == 0)
  {
    *lasts = ((void *)0);
    return ret;
  }
  else
  {
    notsep[0] = 0;
    *lasts = wcsinc(notsep);
    return ret;
  }
}
*/
/*
char *strtok_r(char *s, const char *sep, char **lasts)
{
  char *notsep;
  char *ret;
  if (sep == NULL)
    return NULL;
  if (s == NULL)
    if (lasts == NULL || *lasts == NULL || (*lasts)[0] == 0)
      return NULL;

  if (s)
    notsep = s;
  else
    notsep = *lasts;
  while (strchr (sep, notsep[0]) != NULL)
  {
    if (notsep[0] == 0)
    {
      *lasts = NULL;
      return NULL;
    }
    notsep += 1;
  }
  
  for (ret = notsep; notsep[0] != 0 && strchr (sep, notsep[0]) == NULL; notsep++);

  if (notsep[0] == 0)
  {
    *lasts = NULL;
    return ret;
  }
  else
  {
    notsep[0] = 0;
    *lasts = notsep + 1;
    return ret;
  }
}

wchar_t *wcstok_r(wchar_t *s, const wchar_t *sep, wchar_t **lasts)
{
  wchar_t *notsep;
  wchar_t *ret;
  if (sep == NULL)
    return NULL;
  if (s == NULL)
    if (lasts == NULL || *lasts == NULL || (*lasts)[0] == 0)
      return NULL;

  if (s)
    notsep = s;
  else
    notsep = *lasts;
  while (wcschr (sep, notsep[0]) != NULL)
  {
    if (notsep[0] == 0)
    {
      *lasts = NULL;
      return NULL;
    }
    notsep += 1;
  }
  
  for (ret = notsep; notsep[0] != 0 && wcschr (sep, notsep[0]) == NULL; notsep++);

  if (notsep[0] == 0)
  {
    *lasts = NULL;
    return ret;
  }
  else
  {
    notsep[0] = 0;
    *lasts = notsep + 1;
    return ret;
  }
}

char *mbstok_r(char *s, const char *sep, char **lasts)
{
  char *notsep;
  char *ret;
  if (sep == NULL)
    return NULL;
  if (s == NULL)
    if (lasts == NULL || *lasts == NULL || (*lasts)[0] == 0)
      return NULL;

  if (s)
    notsep = s;
  else
    notsep = *lasts;
  while (mbschr (sep, notsep[0]) != NULL)
  {
    if (notsep[0] == 0)
    {
      *lasts = NULL;
      return NULL;
    }
    notsep = _mbsinc (notsep);
  }
  
  for (ret = notsep; notsep[0] != 0 && mbschr (sep, notsep[0]) == NULL; notsep = _mbsinc (notsep));

  if (notsep[0] == 0)
  {
    *lasts = NULL;
    return ret;
  }
  else
  {
    notsep[0] = 0;
    *lasts = _mbsinc (notsep);
    return ret;
  }
}
*/