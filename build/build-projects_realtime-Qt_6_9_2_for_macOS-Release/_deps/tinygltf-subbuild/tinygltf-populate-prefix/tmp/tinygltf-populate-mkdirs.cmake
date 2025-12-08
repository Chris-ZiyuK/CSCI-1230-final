# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/ziyukong/codebase/BRN/CS1230/projects/CSCI-1230-final/build/build-projects_realtime-Qt_6_9_2_for_macOS-Release/_deps/tinygltf-src")
  file(MAKE_DIRECTORY "/Users/ziyukong/codebase/BRN/CS1230/projects/CSCI-1230-final/build/build-projects_realtime-Qt_6_9_2_for_macOS-Release/_deps/tinygltf-src")
endif()
file(MAKE_DIRECTORY
  "/Users/ziyukong/codebase/BRN/CS1230/projects/CSCI-1230-final/build/build-projects_realtime-Qt_6_9_2_for_macOS-Release/_deps/tinygltf-build"
  "/Users/ziyukong/codebase/BRN/CS1230/projects/CSCI-1230-final/build/build-projects_realtime-Qt_6_9_2_for_macOS-Release/_deps/tinygltf-subbuild/tinygltf-populate-prefix"
  "/Users/ziyukong/codebase/BRN/CS1230/projects/CSCI-1230-final/build/build-projects_realtime-Qt_6_9_2_for_macOS-Release/_deps/tinygltf-subbuild/tinygltf-populate-prefix/tmp"
  "/Users/ziyukong/codebase/BRN/CS1230/projects/CSCI-1230-final/build/build-projects_realtime-Qt_6_9_2_for_macOS-Release/_deps/tinygltf-subbuild/tinygltf-populate-prefix/src/tinygltf-populate-stamp"
  "/Users/ziyukong/codebase/BRN/CS1230/projects/CSCI-1230-final/build/build-projects_realtime-Qt_6_9_2_for_macOS-Release/_deps/tinygltf-subbuild/tinygltf-populate-prefix/src"
  "/Users/ziyukong/codebase/BRN/CS1230/projects/CSCI-1230-final/build/build-projects_realtime-Qt_6_9_2_for_macOS-Release/_deps/tinygltf-subbuild/tinygltf-populate-prefix/src/tinygltf-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/ziyukong/codebase/BRN/CS1230/projects/CSCI-1230-final/build/build-projects_realtime-Qt_6_9_2_for_macOS-Release/_deps/tinygltf-subbuild/tinygltf-populate-prefix/src/tinygltf-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/ziyukong/codebase/BRN/CS1230/projects/CSCI-1230-final/build/build-projects_realtime-Qt_6_9_2_for_macOS-Release/_deps/tinygltf-subbuild/tinygltf-populate-prefix/src/tinygltf-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
