/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <charconv>
#include <fstream>
#include <iostream>
#include <sstream>

#include "../BLI_math_vec_types.hh"
#include "../BLI_span.hh"
#include "../BLI_string_ref.hh"
#include "../BLI_vector.hh"

#include "parser_string_utils.hh"

#include "../intern/fast_float.h"

namespace blender::io::obj {
using std::string;

const char* read_next_line(const char* buffer, const char* end, StringRef &r_line)
{
  const char* start = buffer;
  size_t len = 0;
  //@TODO: handling of backslash continuations?
  while (buffer < end) {
    char c = *buffer++;
    if (c == '\n')
      break;
    ++len;
  }
  r_line = StringRef(start, len);
  return buffer;
}

/**
 * Split a line string into the first word (key) and the rest of the line.
 * Also remove leading & trailing spaces as well as `\r` carriage return
 * character if present.
 */
void split_line_key_rest(const StringRef line, StringRef &r_line_key, StringRef &r_rest_line)
{
  if (line.is_empty()) {
    return;
  }

  const int64_t pos_split{line.find_first_of(' ')};
  if (pos_split == StringRef::not_found) {
    /* Use the first character if no space is found in the line. It's usually a comment like:
     * #This is a comment. */
    r_line_key = line.substr(0, 1);
  }
  else {
    r_line_key = line.substr(0, pos_split);
  }

  /* Eat the delimiter also using "+ 1". */
  r_rest_line = line.drop_prefix(r_line_key.size() + 1);
  if (r_rest_line.is_empty()) {
    return;
  }

  /* Remove any leading spaces, trailing spaces & \r character, if any. */
  const int64_t leading_space{r_rest_line.find_first_not_of(' ')};
  if (leading_space != StringRef::not_found) {
    r_rest_line = r_rest_line.drop_prefix(leading_space);
  }

  /* Another way is to do a test run before the actual parsing to find the newline
   * character and use it in the getline. */
  const int64_t carriage_return{r_rest_line.find_first_of('\r')};
  if (carriage_return != StringRef::not_found) {
    r_rest_line = r_rest_line.substr(0, carriage_return + 1);
  }

  const int64_t trailing_space{r_rest_line.find_last_not_of(' ')};
  if (trailing_space != StringRef::not_found) {
    /* The position is of a character that is not ' ', so count of characters is position + 1. */
    r_rest_line = r_rest_line.substr(0, trailing_space + 1);
  }
}

/**
 * Split the given string by the delimiter and fill the given vector.
 * If an intermediate string is empty, or space or null character, it is not appended to the
 * vector.
 */
void split_by_char(StringRef in_string, const char delimiter, Vector<StringRef> &r_out_list)
{
  r_out_list.clear();

  while (!in_string.is_empty()) {
    const int64_t pos_delim{in_string.find_first_of(delimiter)};
    const int64_t word_len = pos_delim == StringRef::not_found ? in_string.size() : pos_delim;

    StringRef word{in_string.data(), word_len};
    if (!word.is_empty() && !(word == " " && !(word[0] == '\0'))) {
      r_out_list.append(word);
    }
    if (pos_delim == StringRef::not_found) {
      return;
    }
    /* Skip the word already stored. */
    in_string = in_string.drop_prefix(word_len);
    /* Skip all delimiters. */
    const int64_t pos_non_delim = in_string.find_first_not_of(delimiter);
    if (pos_non_delim == StringRef::not_found) {
      return;
    }
    in_string = in_string.drop_prefix(std::min(pos_non_delim, in_string.size()));
  }
}

static const char* skip_ws(const char* p, const char* end)
{
  while (p < end && *p <= ' ')
    ++p;
  return p;
}
static const char* skip_plus(const char* p, const char* end)
{
  if (p < end && *p == '+')
    ++p;
  return p;
}

/**
 * Convert the given string to float and assign it to the destination value.
 *
 * If the string cannot be converted to a float, the fallback value is used.
 */
void copy_string_to_float(StringRef src, const float fallback_value, float &r_dst)
{
  const char* p = src.data();
  const char* end = p + src.size();
  /* Skip whitespace and possible plus sign. */
  p = skip_ws(p, end);
  p = skip_plus(p, end);
  fast_float::from_chars_result res = fast_float::from_chars(p, end, r_dst);
  if (res.ec == std::errc::invalid_argument) {
    std::cerr << "Bad conversion to float:'" << src << "'" << std::endl;
    r_dst = fallback_value;
  }
  else if (res.ec == std::errc::result_out_of_range) {
    std::cerr << "Out of range for float:'" << src << "'" << std::endl;
    r_dst = fallback_value;
  }
}

const char* parse_floats(const char* p, const char* end, const float fallback_value, float* dst, int count)
{
  for (int i = 0; i < count; ++i) {
    /* Skip whitespace and possible plus sign. */
    p = skip_ws(p, end);
    p = skip_plus(p, end);
    /* Parse float. */
    fast_float::from_chars_result res = fast_float::from_chars(p, end, dst[i]);
    if (res.ec == std::errc::invalid_argument || res.ec == std::errc::result_out_of_range) {
      dst[i] = fallback_value;
    }
    p = res.ptr;
  }
  return p;
}


/**
 * Convert all members of the Span of strings to floats and assign them to the float
 * array members. Usually used for values like coordinates.
 *
 * If a string cannot be converted to a float, the fallback value is used.
 */
void copy_string_to_float(Span<StringRef> src,
                          const float fallback_value,
                          MutableSpan<float> r_dst)
{
  for (int i = 0; i < r_dst.size(); ++i) {
    if (i < src.size()) {
      copy_string_to_float(src[i], fallback_value, r_dst[i]);
    }
    else {
      r_dst[i] = fallback_value;
    }
  }
}

/**
 * Convert the given string to int and assign it to the destination value.
 *
 * If the string cannot be converted to an integer, the fallback value is used.
 */
void copy_string_to_int(StringRef src, const int fallback_value, int &r_dst)
{
  const char* p = src.data();
  const char* end = p + src.size();
  /* Skip whitespace and possible plus sign. */
  p = skip_ws(p, end);
  if (p < end && *p == '+')
    ++p;
  std::from_chars_result res = std::from_chars(p, end, r_dst);
  if (res.ec == std::errc::invalid_argument) {
    std::cerr << "Bad conversion to int:'" << src << "'" << std::endl;
    r_dst = fallback_value;
  }
  else if (res.ec == std::errc::result_out_of_range) {
    std::cerr << "Out of range for int:'" << src << "'" << std::endl;
    r_dst = fallback_value;
  }
}

const char* parse_int(const char* p, const char* end, const int fallback_value, int& r_dst)
{
  /* Skip possible plus sign. */
  if (p < end && *p == '+')
    ++p;
  std::from_chars_result res = std::from_chars(p, end, r_dst);
  if (res.ec == std::errc::invalid_argument) {
    std::cerr << "Bad conversion to int:'" << StringRef(p, end) << "'" << std::endl;
    r_dst = fallback_value;
    return p;
  }
  else if (res.ec == std::errc::result_out_of_range) {
    std::cerr << "Out of range for int:'" << StringRef(p, end) << "'" << std::endl;
    r_dst = fallback_value;
    return p;
  }
  return res.ptr;
}


/**
 * Convert the given strings to ints and fill the destination int buffer.
 *
 * If a string cannot be converted to an integer, the fallback value is used.
 */
void copy_string_to_int(Span<StringRef> src, const int fallback_value, MutableSpan<int> r_dst)
{
  for (int i = 0; i < r_dst.size(); ++i) {
    if (i < src.size()) {
      copy_string_to_int(src[i], fallback_value, r_dst[i]);
    }
    else {
      r_dst[i] = fallback_value;
    }
  }
}

std::string replace_all_occurences(StringRef original, StringRef to_remove, StringRef to_add)
{
  std::string clean{original};
  while (true) {
    const std::string::size_type pos = clean.find(to_remove);
    if (pos == std::string::npos) {
      break;
    }
    clean.replace(pos, to_add.size(), to_add);
  }
  return clean;
}

}  // namespace blender::io::obj
