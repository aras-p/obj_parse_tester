cmake_minimum_required (VERSION 3.8)

option(BUILD_SHARED_LIBS "Build shared libs" OFF)
option(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT "" OFF)
option(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT "" OFF)
option(ASSIMP_BUILD_ASSIMP_TOOLS "" OFF)
option(ASSIMP_BUILD_TESTS "" OFF)
option(ASSIMP_INSTALL "" OFF)
option(ASSIMP_BUILD_ASSIMP_VIEW "" OFF)
option(ASSIMP_BUILD_OBJ_IMPORTER "" ON)
add_subdirectory(libs/assimp)

add_subdirectory(libs/fast_obj)
add_subdirectory(libs/rapidobj)
add_subdirectory(libs/tinyobjloader)

set(XXHASH_BUILD_ENABLE_INLINE_API OFF)
set(XXHASH_BUILD_XXHSUM OFF)
add_subdirectory(libs/xxHash/cmake_unofficial)

project ("obj_parse_tester")

add_executable (obj_parse_tester "obj_parse_tester.cpp")
add_definitions(-DNOMINMAX)
target_sources(obj_parse_tester PRIVATE
	# embedded Blender OBJ importer parts
	libs/blender/importer/obj_import_file_reader.cc
	libs/blender/importer/obj_importer.cc
	libs/blender/importer/obj_import_string_utils.cc
	libs/blender/intern/BLI_assert.c
	libs/blender/intern/leak_detector.cc
	libs/blender/intern/mallocn.c
	libs/blender/intern/mallocn_guarded_impl.c
	libs/blender/intern/mallocn_lockfree_impl.c
	libs/blender/intern/math_base_inline.c
	libs/blender/intern/math_color_inline.c
	libs/blender/intern/math_geom_inline.c
	libs/blender/intern/math_vector_inline.c
	libs/blender/intern/path_util.c
	libs/blender/intern/string.c
	# minimal amount of OpenSceneGraph to get OBJ parser
	libs/OpenSceneGraph-min/obj.cpp
)
target_compile_features(obj_parse_tester PRIVATE cxx_std_17)
target_include_directories(obj_parse_tester PRIVATE
	libs/tinyobjloader/experimental
	libs/blender
)

target_link_libraries(obj_parse_tester
	tinyobjloader
	xxHash::xxhash
	fast_obj_lib
	assimp::assimp
	${CMAKE_SOURCE_DIR}/libs/blender/pthreads/lib/pthreadVC3.lib)
