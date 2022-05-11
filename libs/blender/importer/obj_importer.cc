/* SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup obj
 */

#include <string>

#include "obj_import_file_reader.hh"
#include "obj_importer.hh"

namespace blender::io::obj {

void importer_main(const OBJImportParams &import_params, GlobalVertices& global_vertices, Vector<std::unique_ptr<Geometry>>& all_geometries, Map<std::string, std::unique_ptr<MTLMaterial>>& materials)
{
  OBJParser obj_parser{import_params, 1<<16};
  obj_parser.parse(all_geometries, global_vertices);

  for (StringRefNull mtl_library : obj_parser.mtl_libraries()) {
    MTLParser mtl_parser{mtl_library, import_params.filepath};
    mtl_parser.parse_and_store(materials);
  }
}
}  // namespace blender::io::obj
