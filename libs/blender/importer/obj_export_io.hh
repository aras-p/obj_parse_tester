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
 */

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
  map_Ka,
  map_d,
  map_refl,
  map_Ke,
  map_Bump,
  /* Use rarely. New line is NOT included for string. */
  string,
};


}  // namespace blender::io::obj
