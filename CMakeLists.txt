cmake_minimum_required (VERSION 3.8)

add_subdirectory(libs/fast_obj)
add_subdirectory(libs/rapidobj)
add_subdirectory(libs/tinyobjloader)

option(BUILD_SHARED_LIBS "Build shared libs" OFF)
set(XXHASH_BUILD_ENABLE_INLINE_API OFF)
set(XXHASH_BUILD_XXHSUM OFF)
add_subdirectory(libs/xxHash/cmake_unofficial)

project ("obj_parse_tester")

add_executable (obj_parse_tester "obj_parse_tester.cpp")
target_compile_features(obj_parse_tester PRIVATE cxx_std_17)

target_link_libraries(obj_parse_tester tinyobjloader xxHash::xxhash fast_obj_lib)
