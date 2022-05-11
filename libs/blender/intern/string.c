/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 */

/** \file
 * \ingroup bli
 */

#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../MEM_guardedalloc.h"

//#include "../BLI_dynstr.h"
#include "../BLI_string.h"

#include "../BLI_utildefines.h"

#ifdef __GNUC__
#  pragma GCC diagnostic error "-Wsign-conversion"
#endif

// #define DEBUG_STRSIZE

/* -------------------------------------------------------------------- */
/** \name String Duplicate/Copy
 * \{ */

char *BLI_strdupn(const char *str, const size_t len)
{
  char *n = MEM_mallocN(len + 1, "strdup");
  memcpy(n, str, len);
  n[len] = '\0';

  return n;
}

char *BLI_strdup(const char *str)
{
  return BLI_strdupn(str, strlen(str));
}

char *BLI_strdupcat(const char *__restrict str1, const char *__restrict str2)
{
  /* include the NULL terminator of str2 only */
  const size_t str1_len = strlen(str1);
  const size_t str2_len = strlen(str2) + 1;
  char *str, *s;

  str = MEM_mallocN(str1_len + str2_len, "strdupcat");
  s = str;

  memcpy(s, str1, str1_len); /* NOLINT: bugprone-not-null-terminated-result */
  s += str1_len;
  memcpy(s, str2, str2_len);

  return str;
}

char *BLI_strncpy(char *__restrict dst, const char *__restrict src, const size_t maxncpy)
{
  size_t srclen = BLI_strnlen(src, maxncpy - 1);
  BLI_assert(maxncpy != 0);

#ifdef DEBUG_STRSIZE
  memset(dst, 0xff, sizeof(*dst) * maxncpy);
#endif

  memcpy(dst, src, srclen);
  dst[srclen] = '\0';
  return dst;
}

char *BLI_strncpy_ensure_pad(char *__restrict dst,
                             const char *__restrict src,
                             const char pad,
                             size_t maxncpy)
{
  BLI_assert(maxncpy != 0);

#ifdef DEBUG_STRSIZE
  memset(dst, 0xff, sizeof(*dst) * maxncpy);
#endif

  if (src[0] == '\0') {
    dst[0] = '\0';
  }
  else {
    /* Add heading/trailing wildcards if needed. */
    size_t idx = 0;
    size_t srclen;

    if (src[idx] != pad) {
      dst[idx++] = pad;
      maxncpy--;
    }
    maxncpy--; /* trailing '\0' */

    srclen = BLI_strnlen(src, maxncpy);
    if ((src[srclen - 1] != pad) && (srclen == maxncpy)) {
      srclen--;
    }

    memcpy(&dst[idx], src, srclen);
    idx += srclen;

    if (dst[idx - 1] != pad) {
      dst[idx++] = pad;
    }
    dst[idx] = '\0';
  }

  return dst;
}

size_t BLI_strncpy_rlen(char *__restrict dst, const char *__restrict src, const size_t maxncpy)
{
  size_t srclen = BLI_strnlen(src, maxncpy - 1);
  BLI_assert(maxncpy != 0);

#ifdef DEBUG_STRSIZE
  memset(dst, 0xff, sizeof(*dst) * maxncpy);
#endif

  memcpy(dst, src, srclen);
  dst[srclen] = '\0';
  return srclen;
}

size_t BLI_strcpy_rlen(char *__restrict dst, const char *__restrict src)
{
  size_t srclen = strlen(src);
  memcpy(dst, src, srclen + 1);
  return srclen;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Printing
 * \{ */

size_t BLI_vsnprintf(char *__restrict buffer,
                     size_t maxncpy,
                     const char *__restrict format,
                     va_list arg)
{
  size_t n;

  BLI_assert(buffer != NULL);
  BLI_assert(maxncpy > 0);
  BLI_assert(format != NULL);

  n = (size_t)vsnprintf(buffer, maxncpy, format, arg);

  if (n != -1 && n < maxncpy) {
    buffer[n] = '\0';
  }
  else {
    buffer[maxncpy - 1] = '\0';
  }

  return n;
}

size_t BLI_vsnprintf_rlen(char *__restrict buffer,
                          size_t maxncpy,
                          const char *__restrict format,
                          va_list arg)
{
  size_t n;

  BLI_assert(buffer != NULL);
  BLI_assert(maxncpy > 0);
  BLI_assert(format != NULL);

  n = (size_t)vsnprintf(buffer, maxncpy, format, arg);

  if (n != -1 && n < maxncpy) {
    /* pass */
  }
  else {
    n = maxncpy - 1;
  }
  buffer[n] = '\0';

  return n;
}

size_t BLI_snprintf(char *__restrict dst, size_t maxncpy, const char *__restrict format, ...)
{
  size_t n;
  va_list arg;

#ifdef DEBUG_STRSIZE
  memset(dst, 0xff, sizeof(*dst) * maxncpy);
#endif

  va_start(arg, format);
  n = BLI_vsnprintf(dst, maxncpy, format, arg);
  va_end(arg);

  return n;
}

size_t BLI_snprintf_rlen(char *__restrict dst, size_t maxncpy, const char *__restrict format, ...)
{
  size_t n;
  va_list arg;

#ifdef DEBUG_STRSIZE
  memset(dst, 0xff, sizeof(*dst) * maxncpy);
#endif

  va_start(arg, format);
  n = BLI_vsnprintf_rlen(dst, maxncpy, format, arg);
  va_end(arg);

  return n;
}


/** \} */

/* -------------------------------------------------------------------- */
/** \name String Escape/Un-Escape
 * \{ */

size_t BLI_str_escape(char *__restrict dst, const char *__restrict src, const size_t dst_maxncpy)
{

  BLI_assert(dst_maxncpy != 0);

  size_t len = 0;
  for (; (len < dst_maxncpy) && (*src != '\0'); dst++, src++, len++) {
    char c = *src;
    if (ELEM(c, '\\', '"') ||                       /* Use as-is. */
        ((c == '\t') && ((void)(c = 't'), true)) || /* Tab. */
        ((c == '\n') && ((void)(c = 'n'), true)) || /* Newline. */
        ((c == '\r') && ((void)(c = 'r'), true)) || /* Carriage return. */
        ((c == '\a') && ((void)(c = 'a'), true)) || /* Bell. */
        ((c == '\b') && ((void)(c = 'b'), true)) || /* Backspace. */
        ((c == '\f') && ((void)(c = 'f'), true)))   /* Form-feed. */
    {
      if (UNLIKELY(len + 1 >= dst_maxncpy)) {
        /* Not enough space to escape. */
        break;
      }
      *dst++ = '\\';
      len++;
    }
    *dst = c;
  }
  *dst = '\0';

  return len;
}

BLI_INLINE bool str_unescape_pair(char c_next, char *r_out)
{
#define CASE_PAIR(value_src, value_dst) \
  case value_src: { \
    *r_out = value_dst; \
    return true; \
  }
  switch (c_next) {
    CASE_PAIR('"', '"');   /* Quote. */
    CASE_PAIR('\\', '\\'); /* Backslash. */
    CASE_PAIR('t', '\t');  /* Tab. */
    CASE_PAIR('n', '\n');  /* Newline. */
    CASE_PAIR('r', '\r');  /* Carriage return. */
    CASE_PAIR('a', '\a');  /* Bell. */
    CASE_PAIR('b', '\b');  /* Backspace. */
    CASE_PAIR('f', '\f');  /* Form-feed. */
  }
#undef CASE_PAIR
  return false;
}

size_t BLI_str_unescape_ex(char *__restrict dst,
                           const char *__restrict src,
                           const size_t src_maxncpy,
                           /* Additional arguments to #BLI_str_unescape */
                           const size_t dst_maxncpy,
                           bool *r_is_complete)
{
  size_t len = 0;
  bool is_complete = true;
  for (const char *src_end = src + src_maxncpy; (src < src_end) && *src; src++) {
    if (UNLIKELY(len == dst_maxncpy)) {
      is_complete = false;
      break;
    }
    char c = *src;
    if (UNLIKELY(c == '\\') && (str_unescape_pair(*(src + 1), &c))) {
      src++;
    }
    dst[len++] = c;
  }
  dst[len] = 0;
  *r_is_complete = is_complete;
  return len;
}

size_t BLI_str_unescape(char *__restrict dst, const char *__restrict src, const size_t src_maxncpy)
{
  size_t len = 0;
  for (const char *src_end = src + src_maxncpy; (src < src_end) && *src; src++) {
    char c = *src;
    if (UNLIKELY(c == '\\') && (str_unescape_pair(*(src + 1), &c))) {
      src++;
    }
    dst[len++] = c;
  }
  dst[len] = 0;
  return len;
}

const char *BLI_str_escape_find_quote(const char *str)
{
  bool escape = false;
  while (*str && (*str != '"' || escape)) {
    /* A pair of back-slashes represents a single back-slash,
     * only use a single back-slash for escaping. */
    escape = (escape == false) && (*str == '\\');
    str++;
  }
  return (*str == '"') ? str : NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Quote/Un-Quote
 * \{ */

bool BLI_str_quoted_substr_range(const char *__restrict str,
                                 const char *__restrict prefix,
                                 int *__restrict r_start,
                                 int *__restrict r_end)
{
  const char *str_start = strstr(str, prefix);
  if (str_start == NULL) {
    return false;
  }
  const size_t prefix_len = strlen(prefix);
  if (UNLIKELY(prefix_len == 0)) {
    BLI_assert_msg(0,
                   "Zero length prefix passed in, "
                   "caller must prevent this from happening!");
    return false;
  }
  BLI_assert_msg(prefix[prefix_len - 1] != '"',
                 "Prefix includes trailing quote, "
                 "caller must prevent this from happening!");

  str_start += prefix_len;
  if (UNLIKELY(*str_start != '\"')) {
    return false;
  }
  str_start += 1;
  const char *str_end = BLI_str_escape_find_quote(str_start);
  if (UNLIKELY(str_end == NULL)) {
    return false;
  }

  *r_start = (int)(str_start - str);
  *r_end = (int)(str_end - str);
  return true;
}

/* NOTE(@campbellbarton): in principal it should be possible to access a quoted string
 * with an arbitrary size, currently all callers for this functionality
 * happened to use a fixed size buffer, so only #BLI_str_quoted_substr is needed. */
#if 0
/**
 * Makes a copy of the text within the "" that appear after the contents of \a prefix.
 * i.e. for string `pose["apples"]` with prefix `pose[`, it will return `apples`.
 *
 * \param str: is the entire string to chop.
 * \param prefix: is the part of the string to step over.
 *
 * Assume that the strings returned must be freed afterwards,
 * and that the inputs will contain data we want.
 */
char *BLI_str_quoted_substrN(const char *__restrict str, const char *__restrict prefix)
{
  int start_match_ofs, end_match_ofs;
  if (!BLI_str_quoted_substr_range(str, prefix, &start_match_ofs, &end_match_ofs)) {
    return NULL;
  }
  const size_t escaped_len = (size_t)(end_match_ofs - start_match_ofs);
  char *result = MEM_mallocN(sizeof(char) * (escaped_len + 1), __func__);
  const size_t unescaped_len = BLI_str_unescape(result, str + start_match_ofs, escaped_len);
  if (unescaped_len != escaped_len) {
    result = MEM_reallocN(result, sizeof(char) * (unescaped_len + 1));
  }
  return result;
}
#endif

bool BLI_str_quoted_substr(const char *__restrict str,
                           const char *__restrict prefix,
                           char *result,
                           size_t result_maxlen)
{
  int start_match_ofs, end_match_ofs;
  if (!BLI_str_quoted_substr_range(str, prefix, &start_match_ofs, &end_match_ofs)) {
    return false;
  }
  const size_t escaped_len = (size_t)(end_match_ofs - start_match_ofs);
  bool is_complete;
  BLI_str_unescape_ex(result, str + start_match_ofs, escaped_len, result_maxlen, &is_complete);
  if (is_complete == false) {
    *result = '\0';
  }
  return is_complete;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Replace
 * \{ */

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Comparison/Matching
 * \{ */

int BLI_strcaseeq(const char *a, const char *b)
{
  return (BLI_strcasecmp(a, b) == 0);
}

char *BLI_strcasestr(const char *s, const char *find)
{
  char c, sc;
  size_t len;

  if ((c = *find++) != 0) {
    c = tolower(c);
    len = strlen(find);
    do {
      do {
        if ((sc = *s++) == 0) {
          return NULL;
        }
        sc = tolower(sc);
      } while (sc != c);
    } while (BLI_strncasecmp(s, find, len) != 0);
    s--;
  }
  return ((char *)s);
}

int BLI_string_max_possible_word_count(const int str_len)
{
  return (str_len / 2) + 1;
}

bool BLI_string_has_word_prefix(const char *haystack, const char *needle, size_t needle_len)
{
  const char *match = BLI_strncasestr(haystack, needle, needle_len);
  if (match) {
    if ((match == haystack) || (*(match - 1) == ' ') || ispunct(*(match - 1))) {
      return true;
    }
    return BLI_string_has_word_prefix(match + 1, needle, needle_len);
  }
  return false;
}

bool BLI_string_all_words_matched(const char *name,
                                  const char *str,
                                  int (*words)[2],
                                  const int words_len)
{
  int index;
  for (index = 0; index < words_len; index++) {
    if (!BLI_string_has_word_prefix(name, str + words[index][0], (size_t)words[index][1])) {
      break;
    }
  }
  const bool all_words_matched = (index == words_len);

  return all_words_matched;
}

char *BLI_strncasestr(const char *s, const char *find, size_t len)
{
  char c, sc;

  if ((c = *find++) != 0) {
    c = tolower(c);
    if (len > 1) {
      do {
        do {
          if ((sc = *s++) == 0) {
            return NULL;
          }
          sc = tolower(sc);
        } while (sc != c);
      } while (BLI_strncasecmp(s, find, len - 1) != 0);
    }
    else {
      {
        do {
          if ((sc = *s++) == 0) {
            return NULL;
          }
          sc = tolower(sc);
        } while (sc != c);
      }
    }
    s--;
  }
  return ((char *)s);
}

int BLI_strcasecmp(const char *s1, const char *s2)
{
  int i;
  char c1, c2;

  for (i = 0;; i++) {
    c1 = tolower(s1[i]);
    c2 = tolower(s2[i]);

    if (c1 < c2) {
      return -1;
    }
    if (c1 > c2) {
      return 1;
    }
    if (c1 == 0) {
      break;
    }
  }

  return 0;
}

int BLI_strncasecmp(const char *s1, const char *s2, size_t len)
{
  size_t i;
  char c1, c2;

  for (i = 0; i < len; i++) {
    c1 = tolower(s1[i]);
    c2 = tolower(s2[i]);

    if (c1 < c2) {
      return -1;
    }
    if (c1 > c2) {
      return 1;
    }
    if (c1 == 0) {
      break;
    }
  }

  return 0;
}

/* compare number on the left size of the string */
static int left_number_strcmp(const char *s1, const char *s2, int *tiebreaker)
{
  const char *p1 = s1, *p2 = s2;
  int numdigit, numzero1, numzero2;

  /* count and skip leading zeros */
  for (numzero1 = 0; *p1 == '0'; numzero1++) {
    p1++;
  }
  for (numzero2 = 0; *p2 == '0'; numzero2++) {
    p2++;
  }

  /* find number of consecutive digits */
  for (numdigit = 0;; numdigit++) {
    if (isdigit(*(p1 + numdigit)) && isdigit(*(p2 + numdigit))) {
      continue;
    }
    if (isdigit(*(p1 + numdigit))) {
      return 1; /* s2 is bigger */
    }
    if (isdigit(*(p2 + numdigit))) {
      return -1; /* s1 is bigger */
    }
    break;
  }

  /* same number of digits, compare size of number */
  if (numdigit > 0) {
    int compare = (int)strncmp(p1, p2, (size_t)numdigit);

    if (compare != 0) {
      return compare;
    }
  }

  /* use number of leading zeros as tie breaker if still equal */
  if (*tiebreaker == 0) {
    if (numzero1 > numzero2) {
      *tiebreaker = 1;
    }
    else if (numzero1 < numzero2) {
      *tiebreaker = -1;
    }
  }

  return 0;
}

int BLI_strcasecmp_natural(const char *s1, const char *s2)
{
  int d1 = 0, d2 = 0;
  char c1, c2;
  int tiebreaker = 0;

  /* if both chars are numeric, to a left_number_strcmp().
   * then increase string deltas as long they are
   * numeric, else do a tolower and char compare */

  while (1) {
    if (isdigit(s1[d1]) && isdigit(s2[d2])) {
      int numcompare = left_number_strcmp(s1 + d1, s2 + d2, &tiebreaker);

      if (numcompare != 0) {
        return numcompare;
      }

      /* Some wasted work here, left_number_strcmp already consumes at least some digits. */
      d1++;
      while (isdigit(s1[d1])) {
        d1++;
      }
      d2++;
      while (isdigit(s2[d2])) {
        d2++;
      }
    }

    /* Test for end of strings first so that shorter strings are ordered in front. */
    if (ELEM(0, s1[d1], s2[d2])) {
      break;
    }

    c1 = tolower(s1[d1]);
    c2 = tolower(s2[d2]);

    if (c1 == c2) {
      /* Continue iteration */
    }
    /* Check for '.' so "foo.bar" comes before "foo 1.bar". */
    else if (c1 == '.') {
      return -1;
    }
    else if (c2 == '.') {
      return 1;
    }
    else if (c1 < c2) {
      return -1;
    }
    else if (c1 > c2) {
      return 1;
    }

    d1++;
    d2++;
  }

  if (tiebreaker) {
    return tiebreaker;
  }

  /* we might still have a different string because of lower/upper case, in
   * that case fall back to regular string comparison */
  return strcmp(s1, s2);
}

int BLI_strcmp_ignore_pad(const char *str1, const char *str2, const char pad)
{
  size_t str1_len, str2_len;

  while (*str1 == pad) {
    str1++;
  }
  while (*str2 == pad) {
    str2++;
  }

  str1_len = strlen(str1);
  str2_len = strlen(str2);

  while (str1_len && (str1[str1_len - 1] == pad)) {
    str1_len--;
  }
  while (str2_len && (str2[str2_len - 1] == pad)) {
    str2_len--;
  }

  if (str1_len == str2_len) {
    return strncmp(str1, str2, str2_len);
  }
  if (str1_len > str2_len) {
    int ret = strncmp(str1, str2, str2_len);
    if (ret == 0) {
      ret = 1;
    }
    return ret;
  }
  {
    int ret = strncmp(str1, str2, str1_len);
    if (ret == 0) {
      ret = -1;
    }
    return ret;
  }
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Comparison at Start/End
 * \{ */

int BLI_str_index_in_array_n(const char *__restrict str,
                             const char **__restrict str_array,
                             const int str_array_len)
{
  int index;
  const char **str_iter = str_array;

  for (index = 0; index < str_array_len; str_iter++, index++) {
    if (STREQ(str, *str_iter)) {
      return index;
    }
  }
  return -1;
}

int BLI_str_index_in_array(const char *__restrict str, const char **__restrict str_array)
{
  int index;
  const char **str_iter = str_array;

  for (index = 0; *str_iter; str_iter++, index++) {
    if (STREQ(str, *str_iter)) {
      return index;
    }
  }
  return -1;
}

bool BLI_str_startswith(const char *__restrict str, const char *__restrict start)
{
  for (; *str && *start; str++, start++) {
    if (*str != *start) {
      return false;
    }
  }

  return (*start == '\0');
}

bool BLI_strn_endswith(const char *__restrict str, const char *__restrict end, size_t slength)
{
  size_t elength = strlen(end);

  if (elength < slength) {
    const char *iter = &str[slength - elength];
    while (*iter) {
      if (*iter++ != *end++) {
        return false;
      }
    }
    return true;
  }
  return false;
}

bool BLI_str_endswith(const char *__restrict str, const char *__restrict end)
{
  const size_t slength = strlen(str);
  return BLI_strn_endswith(str, end, slength);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Length
 * \{ */

size_t BLI_strnlen(const char *s, const size_t maxlen)
{
  size_t len;

  for (len = 0; len < maxlen; len++, s++) {
    if (!*s) {
      break;
    }
  }
  return len;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Case Conversion
 * \{ */

void BLI_str_tolower_ascii(char *str, const size_t len)
{
  size_t i;

  for (i = 0; (i < len) && str[i]; i++) {
    if (str[i] >= 'A' && str[i] <= 'Z') {
      str[i] += 'a' - 'A';
    }
  }
}

void BLI_str_toupper_ascii(char *str, const size_t len)
{
  size_t i;

  for (i = 0; (i < len) && str[i]; i++) {
    if (str[i] >= 'a' && str[i] <= 'z') {
      str[i] -= 'a' - 'A';
    }
  }
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Stripping
 * \{ */

void BLI_str_rstrip(char *str)
{
  for (int i = (int)strlen(str) - 1; i >= 0; i--) {
    if (isspace(str[i])) {
      str[i] = '\0';
    }
    else {
      break;
    }
  }
}

int BLI_str_rstrip_float_zero(char *str, const char pad)
{
  char *p = strchr(str, '.');
  int totstrip = 0;
  if (p) {
    char *end_p;
    p++;                         /* position at first decimal place */
    end_p = p + (strlen(p) - 1); /* position at last character */
    if (end_p > p) {
      while (end_p != p && *end_p == '0') {
        *end_p = pad;
        end_p--;
        totstrip++;
      }
    }
  }

  return totstrip;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Split (Partition)
 * \{ */

size_t BLI_str_partition(const char *str, const char delim[], const char **sep, const char **suf)
{
  return BLI_str_partition_ex(str, NULL, delim, sep, suf, false);
}

size_t BLI_str_rpartition(const char *str, const char delim[], const char **sep, const char **suf)
{
  return BLI_str_partition_ex(str, NULL, delim, sep, suf, true);
}

size_t BLI_str_partition_ex(const char *str,
                            const char *end,
                            const char delim[],
                            const char **sep,
                            const char **suf,
                            const bool from_right)
{
  const char *d;
  char *(*func)(const char *str, int c) = from_right ? strrchr : strchr;

  BLI_assert(end == NULL || end > str);

  *sep = *suf = NULL;

  for (d = delim; *d != '\0'; d++) {
    const char *tmp;

    if (end) {
      if (from_right) {
        for (tmp = end - 1; (tmp >= str) && (*tmp != *d); tmp--) {
          /* pass */
        }
        if (tmp < str) {
          tmp = NULL;
        }
      }
      else {
        tmp = func(str, *d);
        if (tmp >= end) {
          tmp = NULL;
        }
      }
    }
    else {
      tmp = func(str, *d);
    }

    if (tmp && (from_right ? (*sep < tmp) : (!*sep || *sep > tmp))) {
      *sep = tmp;
    }
  }

  if (*sep) {
    *suf = *sep + 1;
    return (size_t)(*sep - str);
  }

  return end ? (size_t)(end - str) : strlen(str);
}

int BLI_string_find_split_words(
    const char *str, const size_t len, const char delim, int r_words[][2], int words_max)
{
  int n = 0, i;
  bool charsearch = true;

  /* Skip leading spaces */
  for (i = 0; (i < len) && (str[i] != '\0'); i++) {
    if (str[i] != delim) {
      break;
    }
  }

  for (; (i < len) && (str[i] != '\0') && (n < words_max); i++) {
    if ((str[i] != delim) && (charsearch == true)) {
      r_words[n][0] = i;
      charsearch = false;
    }
    else {
      if ((str[i] == delim) && (charsearch == false)) {
        r_words[n][1] = i - r_words[n][0];
        n++;
        charsearch = true;
      }
    }
  }

  if (charsearch == false) {
    r_words[n][1] = i - r_words[n][0];
    n++;
  }

  return n;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Formatting (Numeric)
 * \{ */

static size_t BLI_str_format_int_grouped_ex(char src[16], char dst[16], int num_len)
{
  char *p_src = src;
  char *p_dst = dst;

  const char separator = ',';
  int commas;

  if (*p_src == '-') {
    *p_dst++ = *p_src++;
    num_len--;
  }

  for (commas = 2 - num_len % 3; *p_src; commas = (commas + 1) % 3) {
    *p_dst++ = *p_src++;
    if (commas == 1) {
      *p_dst++ = separator;
    }
  }
  *--p_dst = '\0';

  return (size_t)(p_dst - dst);
}


/** \} */
