/* SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup obj
 */

#pragma once

#include "obj_export_mtl.hh"

namespace blender::io::obj {

constexpr eMTLSyntaxElement mtl_line_key_str_to_enum(const std::string_view key_str)
{
  if (key_str == "map_Kd") {
    return eMTLSyntaxElement::map_Kd;
  }
  if (key_str == "map_Ka") {
      return eMTLSyntaxElement::map_Ka;
  }
  if (key_str == "map_Ks") {
    return eMTLSyntaxElement::map_Ks;
  }
  if (key_str == "map_Ns") {
    return eMTLSyntaxElement::map_Ns;
  }
  if (key_str == "map_d") {
    return eMTLSyntaxElement::map_d;
  }
  if (key_str == "refl" || key_str == "map_refl") {
    return eMTLSyntaxElement::map_refl;
  }
  if (key_str == "map_Ke") {
    return eMTLSyntaxElement::map_Ke;
  }
  if (key_str == "map_Bump" || key_str == "bump") {
    return eMTLSyntaxElement::map_Bump;
  }
  return eMTLSyntaxElement::string;
}
}  // namespace blender::io::obj
