/* SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup obj
 */

#pragma once

#include <cstdio>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

#include "../BLI_compiler_attrs.h"
#include "../BLI_string_ref.hh"

namespace blender::io::obj {

enum class eFileType {
  OBJ,
  MTL,
};

enum class eOBJSyntaxElement {
  vertex_coords,
  vertex_coords_color,
  uv_vertex_coords,
  normal,
  poly_element_begin,
  vertex_uv_normal_indices,
  vertex_normal_indices,
  vertex_uv_indices,
  vertex_indices,
  poly_element_end,
  poly_usemtl,
  edge,
  cstype,
  nurbs_degree,
  curve_element_begin,
  curve_element_end,
  nurbs_parameter_begin,
  nurbs_parameters,
  nurbs_parameter_end,
  nurbs_group_end,
  new_line,
  mtllib,
  smooth_group,
  object_group,
  object_name,
  /* Use rarely. New line is NOT included for string. */
  string,
};

enum class eMTLSyntaxElement {
  newmtl,
  Ni,
  d,
  Ns,
  illum,
  Ka,
  Kd,
  Ks,
  Ke,
  map_Kd,
  map_Ks,
  map_Ns,
  map_d,
  map_refl,
  map_Ke,
  map_Bump,
  /* Use rarely. New line is NOT included for string. */
  string,
};


}  // namespace blender::io::obj
