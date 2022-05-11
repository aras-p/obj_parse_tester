/* SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup obj
 */

#pragma once

#include <fstream>
#include <stdio.h>
#include "IO_wavefront_obj.h"
#include "obj_import_mtl.hh"
#include "obj_import_objects.hh"

namespace blender::io::obj {

class OBJParser {
 private:
  const OBJImportParams &import_params_;
  FILE *obj_file_;
  Vector<std::string> mtl_libraries_;

 public:
  OBJParser(const OBJImportParams &import_params);
  ~OBJParser();

  void parse(Vector<std::unique_ptr<Geometry>> &r_all_geometries,
             GlobalVertices &r_global_vertices);
  Span<std::string> mtl_libraries() const;
};


/**
 * All texture map options with number of arguments they accept.
 */
class TextureMapOptions {
 private:
  Map<const std::string, int> tex_map_options;

 public:
  TextureMapOptions()
  {
    tex_map_options.add_new("-blendu", 1);
    tex_map_options.add_new("-blendv", 1);
    tex_map_options.add_new("-boost", 1);
    tex_map_options.add_new("-mm", 2);
    tex_map_options.add_new("-o", 3);
    tex_map_options.add_new("-s", 3);
    tex_map_options.add_new("-t", 3);
    tex_map_options.add_new("-texres", 1);
    tex_map_options.add_new("-clamp", 1);
    tex_map_options.add_new("-bm", 1);
    tex_map_options.add_new("-imfchan", 1);
  }

  /**
   * All valid option strings.
   */
  Map<const std::string, int>::KeyIterator all_options() const
  {
    return tex_map_options.keys();
  }

  int number_of_args(StringRef option) const
  {
    return tex_map_options.lookup_as(std::string(option));
  }
};

class MTLParser {
 private:
  char mtl_file_path_[260];
  /**
   * Directory in which the MTL file is found.
   */
  char mtl_dir_path_[260];
  std::fstream mtl_file_;

 public:
  MTLParser(StringRef mtl_library_, StringRefNull obj_filepath);

  void parse_and_store(Map<std::string, std::unique_ptr<MTLMaterial>> &r_mtl_materials);
};
}  // namespace blender::io::obj