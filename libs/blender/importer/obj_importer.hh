/* SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup obj
 */

#pragma once

#include "IO_wavefront_obj.h"
#include "obj_import_objects.hh"
#include "obj_export_mtl.hh"
#include "../BLI_map.hh"

namespace blender::io::obj {

void importer_main(const OBJImportParams &import_params, GlobalVertices& global_vertices, Vector<std::unique_ptr<Geometry>>& all_geometries, Map<std::string, std::unique_ptr<MTLMaterial>>& materials);

}  // namespace blender::io::obj
