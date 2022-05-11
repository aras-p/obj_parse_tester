/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include "BLI_string_ref.hh"

/*
 * Various text parsing utilities used by OBJ importer.
 * The utilities are not directly usable by other formats, since
 * they treat backslash (\) as a whitespace character (OBJ format
 * allows backslashes to function as a line-continuation character).
 */

namespace blender::io::obj {

/**
 * Fetches next line from an input string buffer.
 *
 * The returned line will not have '\n' characters at the end;
 * the `buffer` is modified to contain remaining text without
 * the input line.
 *
 * Note that backslash (\) character is treated as a line
 * continuation.
 */
StringRef read_next_line(StringRef &buffer);

/**
 * Drop leading white-space from a StringRef.
 * Note that backslash character is considered white-space.
 */
const char* drop_whitespace(const char* p, const char* end);

/**
 * Drop leading non-white-space from a StringRef.
 * Note that backslash character is considered white-space.
 */
const char* drop_non_whitespace(const char* p, const char* end);

/**
 * Parse an integer from an input string.
 * The parsed result is stored in `dst`. The function skips
 * leading white-space unless `skip_space=false`. If the
 * number can't be parsed (invalid syntax, out of range),
 * `fallback` value is stored instead.
 *
 * Returns the remainder of the input string after parsing.
 */
const char* parse_int(const char* p, const char* end, int fallback, int &dst, bool skip_space = true);

/**
 * Parse a float from an input string.
 * The parsed result is stored in `dst`. The function skips
 * leading white-space unless `skip_space=false`. If the
 * number can't be parsed (invalid syntax, out of range),
 * `fallback` value is stored instead.
 *
 * Returns the remainder of the input string after parsing.
 */
const char* parse_float(const char* p, const char* end, float fallback, float &dst, bool skip_space = true);

/**
 * Parse a number of white-space separated floats from an input string.
 * The parsed `count` numbers are stored in `dst`. If a
 * number can't be parsed (invalid syntax, out of range),
 * `fallback` value is stored instead.
 *
 * Returns the remainder of the input string after parsing.
 */
const char* parse_floats(const char* p, const char* end, float fallback, float *dst, int count);

inline bool startswith(const char* p, const char* end, StringRef prefix)
{
    size_t size = end - p;
    size_t prefix_size = prefix.size();
    if (size < prefix_size) {
        return false;
    }
    return memcmp(p, prefix.data(), prefix_size) == 0;
}

}  // namespace blender::io::obj
