/* SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup obj
 */

#include "../BLI_map.hh"
#include "../BLI_path_util.h"
#include "../BLI_string_ref.hh"
#include "../BLI_vector.hh"

#include "parser_string_utils.hh"

#include "obj_import_file_reader.hh"

namespace blender::io::obj {

using std::string;

/**
 * Based on the properties of the given Geometry instance, create a new Geometry instance
 * or return the previous one.
 *
 * Also update index offsets which should always happen if a new Geometry instance is created.
 */
static Geometry *create_geometry(Geometry *const prev_geometry,
                                 const eGeometryType new_type,
                                 StringRef name,
                                 const GlobalVertices &global_vertices,
                                 Vector<std::unique_ptr<Geometry>> &r_all_geometries,
                                 VertexIndexOffset &r_offset)
{
  auto new_geometry = [&]() {
    r_all_geometries.append(std::make_unique<Geometry>());
    Geometry *g = r_all_geometries.last().get();
    g->geom_type_ = new_type;
    g->geometry_name_ = name.is_empty() ? "New object" : name;
    g->vertex_start_ = global_vertices.vertices.size();
    r_offset.set_index_offset(g->vertex_start_);
    return g;
  };

  if (prev_geometry && prev_geometry->geom_type_ == GEOM_MESH) {
    /* After the creation of a Geometry instance, at least one element has been found in the OBJ
     * file that indicates that it is a mesh (basically anything but the vertex positions). */
    if (!prev_geometry->face_elements_.is_empty() ||
        prev_geometry->has_vertex_normals_ || !prev_geometry->edges_.is_empty()) {
      return new_geometry();
    }
    if (new_type == GEOM_MESH) {
      /* A Geometry created initially with a default name now found its name. */
      prev_geometry->geometry_name_ = name;
      return prev_geometry;
    }
    if (new_type == GEOM_CURVE) {
      /* The object originally created is not a mesh now that curve data
       * follows the vertex coordinates list. */
      prev_geometry->geom_type_ = GEOM_CURVE;
      return prev_geometry;
    }
  }

  if (prev_geometry && prev_geometry->geom_type_ == GEOM_CURVE) {
    return new_geometry();
  }

  return new_geometry();
}

static void geom_add_vertex(Geometry *geom,
                            const StringRef line,
                            GlobalVertices &r_global_vertices)
{
  float3 vert;
  parse_floats(line.begin(), line.end(), FLT_MAX, vert, 3);
  r_global_vertices.vertices.append(vert);
  geom->vertex_count_++;
}

static void geom_add_vertex_normal(Geometry *geom,
                                   const StringRef line,
                                   GlobalVertices &r_global_vertices)
{
  float3 normal;
  parse_floats(line.begin(), line.end(), FLT_MAX, normal, 3);
  r_global_vertices.vertex_normals.append(normal);
  geom->has_vertex_normals_ = true;
}

static void geom_add_uv_vertex(Geometry *geom,
                               const StringRef line,
                               GlobalVertices &r_global_vertices)
{
  float2 uv;
  parse_floats(line.begin(), line.end(), FLT_MAX, uv, 2);
  r_global_vertices.uv_vertices.append(uv);
}

static void geom_add_edge(Geometry *geom,
                          const StringRef rest_line,
                          const VertexIndexOffset &offsets,
                          GlobalVertices &r_global_vertices)
{
  int edge_v1 = -1, edge_v2 = -1;
  Vector<StringRef> str_edge_split;
  split_by_char(rest_line, ' ', str_edge_split);
  copy_string_to_int(str_edge_split[0], -1, edge_v1);
  copy_string_to_int(str_edge_split[1], -1, edge_v2);
  /* Always keep stored indices non-negative and zero-based. */
  edge_v1 += edge_v1 < 0 ? r_global_vertices.vertices.size() : -offsets.get_index_offset() - 1;
  edge_v2 += edge_v2 < 0 ? r_global_vertices.vertices.size() : -offsets.get_index_offset() - 1;
  BLI_assert(edge_v1 >= 0 && edge_v2 >= 0);
  geom->edges_.append({static_cast<uint>(edge_v1), static_cast<uint>(edge_v2)});
}

static const char* skip_ws(const char* p, const char* end)
{
  while (p < end && *p <= ' ')
    ++p;
  return p;
}


static void geom_add_polygon(Geometry *geom,
                             const StringRef rest_line,
                             const GlobalVertices &global_vertices,
                             const VertexIndexOffset &offsets,
                             const int state_material_index,
                             const int state_group_index,
                             const bool state_shaded_smooth)
{
  PolyElem curr_face;
  curr_face.shaded_smooth = state_shaded_smooth;
  curr_face.material_index = state_material_index;
  if (state_group_index >= 0) {
    curr_face.vertex_group_index = state_group_index;
    geom->use_vertex_groups_ = true;
  }

  const int orig_corners_size = geom->face_corners_.size();
  curr_face.start_index_ = orig_corners_size;

  bool face_invalid = false;
  const char* str = rest_line.data();
  const char* str_end = str + rest_line.size();
  while (str < str_end) {
    PolyCorner corner;
    bool got_uv = false, got_normal = false;
    str = parse_int(str, str_end, INT32_MAX, corner.vert_index);
    face_invalid |= corner.vert_index == INT32_MAX;
    if (str < str_end && *str == '/') {
      ++str;
      if (*str != '/') {
        str = parse_int(str, str_end, INT32_MAX, corner.uv_vert_index);
        got_uv = corner.uv_vert_index != INT32_MAX;
      }
      if (str < str_end && *str == '/') {
        ++str;
        str = parse_int(str, str_end, INT32_MAX, corner.vertex_normal_index);
        got_normal = corner.uv_vert_index != INT32_MAX;
      }
    }
    /* Always keep stored indices non-negative and zero-based. */
    corner.vert_index += corner.vert_index < 0 ? global_vertices.vertices.size() :
                                                 -offsets.get_index_offset() - 1;
    if (corner.vert_index < 0 || corner.vert_index >= global_vertices.vertices.size()) {
      fprintf(stderr,
              "Invalid vertex index %i (valid range [0, %zi)), ignoring face\n",
              corner.vert_index,
              global_vertices.vertices.size());
      face_invalid = true;
    }
    if (got_uv) {
      corner.uv_vert_index += corner.uv_vert_index < 0 ? global_vertices.uv_vertices.size() : -1;
      if (corner.uv_vert_index < 0 || corner.uv_vert_index >= global_vertices.uv_vertices.size()) {
        fprintf(stderr,
                "Invalid UV index %i (valid range [0, %zi)), ignoring face\n",
                corner.uv_vert_index,
                global_vertices.uv_vertices.size());
        face_invalid = true;
      }
    }
    if (got_normal) {
      corner.vertex_normal_index += corner.vertex_normal_index < 0 ?
                                        global_vertices.vertex_normals.size() :
                                        -1;
      if (corner.vertex_normal_index < 0 ||
          corner.vertex_normal_index >= global_vertices.vertex_normals.size()) {
        fprintf(stderr,
                "Invalid normal index %i (valid range [0, %zi)), ignoring face\n",
                corner.vertex_normal_index,
                global_vertices.vertex_normals.size());
        face_invalid = true;
      }
    }
    geom->face_corners_.append(corner);
    curr_face.corner_count_++;

    str = skip_ws(str, str_end);
  }

  if (!face_invalid) {
    geom->face_elements_.append(curr_face);
    geom->total_loops_ += curr_face.corner_count_;
  }
  else {
    /* Remove just-added corners for the invalid face. */
    geom->face_corners_.resize(orig_corners_size);
  }
}

static Geometry *geom_set_curve_type(Geometry *geom,
                                     const StringRef rest_line,
                                     const GlobalVertices &global_vertices,
                                     const StringRef state_object_group,
                                     VertexIndexOffset &r_offsets,
                                     Vector<std::unique_ptr<Geometry>> &r_all_geometries)
{
  if (rest_line.find("bspline") != StringRef::not_found) {
    geom = create_geometry(
        geom, GEOM_CURVE, state_object_group, global_vertices, r_all_geometries, r_offsets);
    geom->nurbs_element_.group_ = state_object_group;
  }
  else {
    std::cerr << "Curve type not supported:'" << rest_line << "'" << std::endl;
  }
  return geom;
}

static void geom_set_curve_degree(Geometry *geom, const StringRef rest_line)
{
  copy_string_to_int(rest_line, 3, geom->nurbs_element_.degree);
}

static void geom_add_curve_vertex_indices(Geometry *geom,
                                          const StringRef rest_line,
                                          const GlobalVertices &global_vertices)
{
  Vector<StringRef> str_curv_split;
  split_by_char(rest_line, ' ', str_curv_split);
  /* Remove "0.0" and "1.0" from the strings. They are hardcoded. */
  str_curv_split.remove(0);
  str_curv_split.remove(0);
  geom->nurbs_element_.curv_indices.resize(str_curv_split.size());
  copy_string_to_int(str_curv_split, INT32_MAX, geom->nurbs_element_.curv_indices);
  for (int &curv_index : geom->nurbs_element_.curv_indices) {
    /* Always keep stored indices non-negative and zero-based. */
    curv_index += curv_index < 0 ? global_vertices.vertices.size() : -1;
  }
}

static void geom_add_curve_parameters(Geometry *geom, const StringRef rest_line)
{
  Vector<StringRef> str_parm_split;
  split_by_char(rest_line, ' ', str_parm_split);
  if (str_parm_split[0] == "u" || str_parm_split[0] == "v") {
    str_parm_split.remove(0);
    geom->nurbs_element_.parm.resize(str_parm_split.size());
    copy_string_to_float(str_parm_split, FLT_MAX, geom->nurbs_element_.parm);
  }
  else {
    std::cerr << "Surfaces are not supported:'" << str_parm_split[0] << "'" << std::endl;
  }
}

static void geom_update_object_group(const StringRef rest_line,
                                     std::string &r_state_object_group)
{

  if (rest_line.find("off") != string::npos || rest_line.find("null") != string::npos ||
      rest_line.find("default") != string::npos) {
    /* Set group for future elements like faces or curves to empty. */
    r_state_object_group = "";
    return;
  }
  r_state_object_group = rest_line;
}

static void geom_update_smooth_group(const StringRef rest_line, bool &r_state_shaded_smooth)
{
  /* Some implementations use "0" and "null" too, in addition to "off". */
  if (rest_line != "0" && rest_line.find("off") == StringRef::not_found &&
      rest_line.find("null") == StringRef::not_found) {
    int smooth = 0;
    copy_string_to_int(rest_line, 0, smooth);
    r_state_shaded_smooth = smooth != 0;
  }
  else {
    /* The OBJ file explicitly set shading to off. */
    r_state_shaded_smooth = false;
  }
}

/**
 * Open OBJ file at the path given in import parameters.
 */
OBJParser::OBJParser(const OBJImportParams &import_params) : import_params_(import_params)
{
  obj_file_ = fopen(import_params_.filepath, "rb");
  if (!obj_file_) {
    fprintf(stderr, "Cannot read from OBJ file:'%s'.\n", import_params_.filepath);
    return;
  }
}

OBJParser::~OBJParser()
{
  if (obj_file_) {
    fclose(obj_file_);
  }
}

/**
 * Read the OBJ file line by line and create OBJ Geometry instances. Also store all the vertex
 * and UV vertex coordinates in a struct accessible by all objects.
 */
void OBJParser::parse(Vector<std::unique_ptr<Geometry>> &r_all_geometries,
                      GlobalVertices &r_global_vertices)
{
  if (!obj_file_) {
    return;
  }

  /* Store vertex coordinates that belong to other Geometry instances.  */
  VertexIndexOffset offsets;
  /* Non owning raw pointer to a Geometry. To be updated while creating a new Geometry. */
  Geometry *curr_geom = create_geometry(
      nullptr, GEOM_MESH, "", r_global_vertices, r_all_geometries, offsets);

  /* State-setting variables: if set, they remain the same for the remaining
   * elements in the object. */
  bool state_shaded_smooth = false;
  string state_object_group;
  int state_object_group_index = -1;
  string state_material_name;
  int state_material_index = -1;

  const size_t kBufferSize = 1 << 16;
  //const size_t kBufferSize = 256;
  char buffer[kBufferSize * 2];

  size_t buffer_offset = 0;
  while (true) {
    /* Read a chunk of input from the file. */
    size_t bytes_read = fread(buffer + buffer_offset, 1, kBufferSize, obj_file_);
    if (bytes_read == 0 && buffer_offset == 0) {
      break; /* No more data to read. */
    }

    /* Ensure buffer ends in a newline. */
    if (bytes_read < kBufferSize) {
      if (bytes_read == 0 || buffer[buffer_offset + bytes_read - 1] != '\n') {
        buffer[buffer_offset + bytes_read] = '\n';
        bytes_read++;
      }
    }

    size_t buffer_end = buffer_offset + bytes_read;
    if (buffer_end == 0)
      break;

    /* Find last newline. */
    size_t last_nl = buffer_end;
    while (last_nl > 0) {
      --last_nl;
      if (buffer[last_nl] == '\n')
        break;
    }
    if (buffer[last_nl] != '\n')
      break; /* No newline at all, break (TODO: why?) */
    ++last_nl;

    /* Parse the buffer we have so far. */
    const char* ptr = buffer;
    const char* end = buffer + last_nl;
    while (ptr < end) {
      StringRef line;
      ptr = read_next_line(ptr, end, line);
      if (line.is_empty())
        continue;
      if (line[0] == 'v') {
        /* @TODO: tabs? */
        if (line.startswith("v ")) {
          line = line.drop_prefix(2);
          geom_add_vertex(curr_geom, line, r_global_vertices);
        }
        else if (line.startswith("vn ")) {
          line = line.drop_prefix(3);
          geom_add_vertex_normal(curr_geom, line, r_global_vertices);
        }
        else if (line.startswith("vt ")) {
          line = line.drop_prefix(3);
          geom_add_uv_vertex(curr_geom, line, r_global_vertices);
        }
      }
      else if (line.startswith("f ")) {
        line = line.drop_prefix(2);
        geom_add_polygon(curr_geom,
          line,
          r_global_vertices,
          offsets,
          state_material_index,
          state_object_group_index, /* TODO was wrongly material name! */
          state_shaded_smooth);
      }
      else if (line.startswith("l ")) {
        line = line.drop_prefix(2);
        geom_add_edge(curr_geom, line, offsets, r_global_vertices);
      }
      else if (line.startswith("cstype ")) {
        line = line.drop_prefix(7);
        curr_geom = geom_set_curve_type(curr_geom,
          line,
          r_global_vertices,
          state_object_group,
          offsets,
          r_all_geometries);
      }
      else if (line.startswith("deg ")) {
        line = line.drop_prefix(4);
        geom_set_curve_degree(curr_geom, line);
      }
      else if (line.startswith("curv ")) {
        line = line.drop_prefix(5);
        geom_add_curve_vertex_indices(curr_geom, line, r_global_vertices);
      }
      else if (line.startswith("parm ")) {
        line = line.drop_prefix(5);
        geom_add_curve_parameters(curr_geom, line);
        break;
      }
      else if (line.startswith("o ")) {
        line = line.drop_prefix(2);
        state_shaded_smooth = false;
        state_object_group = "";
        state_material_name = "";
        curr_geom = create_geometry(
          curr_geom, GEOM_MESH, line, r_global_vertices, r_all_geometries, offsets);
      }
      else if (line.startswith("g ")) {
        line = line.drop_prefix(2);
        geom_update_object_group(line, state_object_group);
        state_object_group_index = curr_geom->group_indices_.lookup_or_add(state_object_group, curr_geom->group_indices_.size());
      }
      else if (line.startswith("s ")) {
        line = line.drop_prefix(2);
        geom_update_smooth_group(line, state_shaded_smooth);
      }
      else if (line.startswith("usemtl ")) {
        line = line.drop_prefix(7);
        state_material_name = line;
        state_material_index = curr_geom->material_indices_.lookup_or_add(state_material_name, curr_geom->material_indices_.size());
      }
      else if (line.startswith("mtllib ")) {
        line = line.drop_prefix(7);
        mtl_libraries_.append(string(line));
      }
      else if (line.startswith("#")) {
        /* ignore comments */
      }
      else {
        std::cout << "Element not recognised: '" << line << "'" << std::endl;
      }
    }

    /* Put overflow in for next buffer. */
    size_t left_size = buffer_end - last_nl;
    memmove(buffer, buffer + last_nl, left_size);
    buffer_offset = left_size;
  }
}

/**
 * Skip all texture map options and get the filepath from a "map_" line.
 */
static StringRef skip_unsupported_options(StringRef line)
{
  TextureMapOptions map_options;
  StringRef last_option;
  int64_t last_option_pos = 0;

  /* Find the last texture map option. */
  for (StringRef option : map_options.all_options()) {
    const int64_t pos{line.find(option)};
    /* Equality (>=) takes care of finding an option in the beginning of the line. Avoid messing
     * with signed-unsigned int comparison. */
    if (pos != StringRef::not_found && pos >= last_option_pos) {
      last_option = option;
      last_option_pos = pos;
    }
  }

  if (last_option.is_empty()) {
    /* No option found, line is the filepath */
    return line;
  }

  /* Remove upto start of the last option + size of the last option + space after it. */
  line = line.drop_prefix(last_option_pos + last_option.size() + 1);
  for (int i = 0; i < map_options.number_of_args(last_option); i++) {
    const int64_t pos_space{line.find_first_of(' ')};
    if (pos_space != StringRef::not_found) {
      BLI_assert(pos_space + 1 < line.size());
      line = line.drop_prefix(pos_space + 1);
    }
  }

  return line;
}

/**
 * Fix incoming texture map line keys for variations due to other exporters.
 */
static string fix_bad_map_keys(StringRef map_key)
{
  string new_map_key(map_key);
  if (map_key == "refl") {
    new_map_key = "map_refl";
  }
  if (map_key.find("bump") != StringRef::not_found) {
    /* Handles both "bump" and "map_Bump" */
    new_map_key = "map_Bump";
  }
  return new_map_key;
}

/**
 * Return a list of all material library filepaths referenced by the OBJ file.
 */
Span<std::string> OBJParser::mtl_libraries() const
{
  return mtl_libraries_;
}

/**
 * Open material library file.
 */
MTLParser::MTLParser(StringRef mtl_library, StringRefNull obj_filepath)
{
  char obj_file_dir[FILE_MAXDIR];
  BLI_split_dir_part(obj_filepath.data(), obj_file_dir, FILE_MAXDIR);
  BLI_path_join(mtl_file_path_, FILE_MAX, obj_file_dir, mtl_library.data(), NULL);
  BLI_split_dir_part(mtl_file_path_, mtl_dir_path_, FILE_MAXDIR);
  mtl_file_.open(mtl_file_path_);
  if (!mtl_file_.good()) {
    fprintf(stderr, "Cannot read from MTL file:'%s'\n", mtl_file_path_);
    return;
  }
}

/**
 * Read MTL file(s) and add MTLMaterial instances to the given Map reference.
 */
void MTLParser::parse_and_store(Map<string, std::unique_ptr<MTLMaterial>> &r_mtl_materials)
{
  if (!mtl_file_.good()) {
    return;
  }

  string line;
  MTLMaterial *current_mtlmaterial = nullptr;

  while (std::getline(mtl_file_, line)) {
    StringRef line_key, rest_line;
    split_line_key_rest(line, line_key, rest_line);
    if (line.empty() || rest_line.is_empty()) {
      continue;
    }

    /* Fix lower case/ incomplete texture map identifiers. */
    const string fixed_key = fix_bad_map_keys(line_key);
    line_key = fixed_key;

    if (line_key == "newmtl") {
      if (r_mtl_materials.remove_as(rest_line)) {
        std::cerr << "Duplicate material found:'" << rest_line
                  << "', using the last encountered Material definition." << std::endl;
      }
      current_mtlmaterial =
          r_mtl_materials.lookup_or_add(string(rest_line), std::make_unique<MTLMaterial>()).get();
    }
    else if (line_key == "Ns") {
      copy_string_to_float(rest_line, 324.0f, current_mtlmaterial->Ns);
    }
    else if (line_key == "Ka") {
      Vector<StringRef> str_ka_split;
      split_by_char(rest_line, ' ', str_ka_split);
      copy_string_to_float(str_ka_split, 0.0f, {current_mtlmaterial->Ka, 3});
    }
    else if (line_key == "Kd") {
      Vector<StringRef> str_kd_split;
      split_by_char(rest_line, ' ', str_kd_split);
      copy_string_to_float(str_kd_split, 0.8f, {current_mtlmaterial->Kd, 3});
    }
    else if (line_key == "Ks") {
      Vector<StringRef> str_ks_split;
      split_by_char(rest_line, ' ', str_ks_split);
      copy_string_to_float(str_ks_split, 0.5f, {current_mtlmaterial->Ks, 3});
    }
    else if (line_key == "Ke") {
      Vector<StringRef> str_ke_split;
      split_by_char(rest_line, ' ', str_ke_split);
      copy_string_to_float(str_ke_split, 0.0f, {current_mtlmaterial->Ke, 3});
    }
    else if (line_key == "Ni") {
      copy_string_to_float(rest_line, 1.45f, current_mtlmaterial->Ni);
    }
    else if (line_key == "d") {
      copy_string_to_float(rest_line, 1.0f, current_mtlmaterial->d);
    }
    else if (line_key == "illum") {
      copy_string_to_int(rest_line, 2, current_mtlmaterial->illum);
    }

    /* Parse image textures. */
    else if (line_key.find("map_") != StringRef::not_found) {
      /* TODO howardt: fix this */
      eMTLSyntaxElement line_key_enum = mtl_line_key_str_to_enum(line_key);
      if (line_key_enum == eMTLSyntaxElement::string ||
          !current_mtlmaterial->texture_maps.contains_as(line_key_enum)) {
        /* No supported texture map found. */
        //std::cerr << "Texture map type not supported:'" << line_key << "'" << std::endl;
        continue;
      }
      tex_map_XX &tex_map = current_mtlmaterial->texture_maps.lookup(line_key_enum);
      Vector<StringRef> str_map_xx_split;
      split_by_char(rest_line, ' ', str_map_xx_split);

      /* TODO ankitm: use `skip_unsupported_options` for parsing these options too? */
      const int64_t pos_o{str_map_xx_split.first_index_of_try("-o")};
      if (pos_o != -1 && pos_o + 3 < str_map_xx_split.size()) {
        copy_string_to_float({str_map_xx_split[pos_o + 1],
                              str_map_xx_split[pos_o + 2],
                              str_map_xx_split[pos_o + 3]},
                             0.0f,
                             {tex_map.translation, 3});
      }
      const int64_t pos_s{str_map_xx_split.first_index_of_try("-s")};
      if (pos_s != -1 && pos_s + 3 < str_map_xx_split.size()) {
        copy_string_to_float({str_map_xx_split[pos_s + 1],
                              str_map_xx_split[pos_s + 2],
                              str_map_xx_split[pos_s + 3]},
                             1.0f,
                             {tex_map.scale, 3});
      }
      /* Only specific to Normal Map node. */
      const int64_t pos_bm{str_map_xx_split.first_index_of_try("-bm")};
      if (pos_bm != -1 && pos_bm + 1 < str_map_xx_split.size()) {
        copy_string_to_float(
            str_map_xx_split[pos_bm + 1], 0.0f, current_mtlmaterial->map_Bump_strength);
      }
      const int64_t pos_projection{str_map_xx_split.first_index_of_try("-type")};
      if (pos_projection != -1 && pos_projection + 1 < str_map_xx_split.size()) {
        /* Only Sphere is supported, so whatever the type is, set it to Sphere.  */
        tex_map.projection_type = 2;
        if (str_map_xx_split[pos_projection + 1] != "sphere") {
          std::cerr << "Using projection type 'sphere', not:'"
                    << str_map_xx_split[pos_projection + 1] << "'." << std::endl;
        }
      }

      /* Skip all unsupported options and arguments. */
      tex_map.image_path = string(skip_unsupported_options(rest_line));
      tex_map.mtl_dir_path = mtl_dir_path_;
    }
  }
}
}  // namespace blender::io::obj
