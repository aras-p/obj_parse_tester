# CMakeList.txt : CMake project for obj_parse_tester, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

#add_subdirectory(libs/assimp)
add_subdirectory(libs/fast_obj)
add_subdirectory(libs/rapidobj)
add_subdirectory(libs/tinyobjloader)

project ("obj_parse_tester")


# Add source to this project's executable.
add_executable (obj_parse_tester "obj_parse_tester.cpp")
target_compile_features(obj_parse_tester PRIVATE cxx_std_17)

# TODO: Add tests and install targets if needed.
