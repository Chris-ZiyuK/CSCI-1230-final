# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles/StaticGLEW_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/StaticGLEW_autogen.dir/ParseCache.txt"
  "CMakeFiles/projects_realtime_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/projects_realtime_autogen.dir/ParseCache.txt"
  "StaticGLEW_autogen"
  "_deps/tinygltf-build/CMakeFiles/loader_example_autogen.dir/AutogenUsed.txt"
  "_deps/tinygltf-build/CMakeFiles/loader_example_autogen.dir/ParseCache.txt"
  "_deps/tinygltf-build/CMakeFiles/tinygltf_autogen.dir/AutogenUsed.txt"
  "_deps/tinygltf-build/CMakeFiles/tinygltf_autogen.dir/ParseCache.txt"
  "_deps/tinygltf-build/loader_example_autogen"
  "_deps/tinygltf-build/tinygltf_autogen"
  "projects_realtime_autogen"
  )
endif()
