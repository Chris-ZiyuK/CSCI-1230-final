cmake_minimum_required(VERSION 3.16)

# Sets project name
project(projects_realtime LANGUAGES CXX C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Sets C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specifies required Qt components
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Gui)
find_package(Qt6 REQUIRED COMPONENTS OpenGL)
find_package(Qt6 REQUIRED COMPONENTS OpenGLWidgets)
find_package(Qt6 REQUIRED COMPONENTS Xml)

# Allows you to include files from within those directories, without prefixing their filepaths
include_directories(src)

# Specifies .cpp and .h files to be passed to the compiler
add_executable(${PROJECT_NAME}
    src/main.cpp

    src/realtime.cpp
    src/mainwindow.cpp
    src/settings.cpp
    src/utils/scenefilereader.cpp
    src/utils/sceneparser.cpp
    src/utils/animation_director.cpp

    src/mainwindow.h
    src/realtime.h
    src/settings.h
    src/utils/scenedata.h
    src/utils/scenefilereader.h
    src/utils/sceneparser.h
    src/utils/animation_director.h
    src/utils/shaderloader.h
    src/utils/aspectratiowidget/aspectratiowidget.hpp

    src/shapes/Cube.cpp
    src/shapes/Sphere.cpp
    src/shapes/Cone.cpp
    src/shapes/Cylinder.cpp
    src/shapes/Star.cpp

    src/shapes/Cube.h
    src/shapes/Sphere.h
    src/shapes/Cone.h
    src/shapes/Cylinder.h
    src/shapes/Star.h
    src/camera.h
    src/camera.cpp
    src/defaultscene.h
    src/defaultscene.cpp

    # For monster
    src/utils/tessellator.cpp
    src/utils/matrix_utils.cpp
    src/utils/glb_loader.cpp
    src/utils/tessellator.h
    src/utils/matrix_utils.h
    src/utils/glb_loader.h
)

# GLM: this creates its library and allows you to `#include "glm/..."`
add_subdirectory(glm)

# GLEW: this creates its library and allows you to `#include "GL/glew.h"`
add_library(StaticGLEW STATIC glew/src/glew.c

)
include_directories(${PROJECT_NAME} PRIVATE glew/include)

# tinygltf: GLB/glTF file loader
# Using FetchContent to download dependencies
include(FetchContent)

# Download tinygltf (header-only library)
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG v2.8.13
)

# Populate dependencies
FetchContent_MakeAvailable(tinygltf)

# Handle single-file dependencies manually
# Note: For single-header files, we use file(DOWNLOAD) to directly download the files

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/third_party)

# Download json.hpp
set(JSON_HPP_PATH ${CMAKE_BINARY_DIR}/third_party/json.hpp)
if(NOT EXISTS ${JSON_HPP_PATH})
    message(STATUS "Downloading json.hpp...")
    file(DOWNLOAD
        https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
        ${JSON_HPP_PATH}
        SHOW_PROGRESS
    )
endif()

# Download stb_image.h
set(STB_IMAGE_H_PATH ${CMAKE_BINARY_DIR}/third_party/stb_image.h)
if(NOT EXISTS ${STB_IMAGE_H_PATH})
    message(STATUS "Downloading stb_image.h...")
    file(DOWNLOAD
        https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
        ${STB_IMAGE_H_PATH}
        SHOW_PROGRESS
    )
endif()

# Download stb_image_write.h
set(STB_IMAGE_WRITE_H_PATH ${CMAKE_BINARY_DIR}/third_party/stb_image_write.h)
if(NOT EXISTS ${STB_IMAGE_WRITE_H_PATH})
    message(STATUS "Downloading stb_image_write.h...")
    file(DOWNLOAD
        https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
        ${STB_IMAGE_WRITE_H_PATH}
        SHOW_PROGRESS
    )
endif()

# Add include directories for tinygltf and dependencies
target_include_directories(${PROJECT_NAME} PRIVATE
    ${tinygltf_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/third_party
)

# Specifies libraries to be linked (Qt components, glew, etc)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt::Core
    Qt::Gui
    Qt::OpenGL
    Qt::OpenGLWidgets
    Qt::Xml
    StaticGLEW
)

# Specifies other files
qt6_add_resources(${PROJECT_NAME} "Resources"
    PREFIX
        "/"
    FILES
        resources/shaders/default.frag
        resources/shaders/default.vert
        resources/shaders/bright.frag
        resources/shaders/fullscreen_quad.vert
        resources/shaders/screen.frag
        resources/shaders/blur.frag
        resources/textures/background.png
        resources/textures/bg.png
)

# GLEW: this provides support for Windows (including 64-bit)
if (WIN32)
  add_compile_definitions(GLEW_STATIC)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    opengl32
    glu32
  )
endif()

# Set this flag to silence warnings on Windows
if (MSVC OR MSYS OR MINGW)
  set(CMAKE_CXX_FLAGS "-Wno-volatile")
endif()
# Set this flag to silence warnings on MacOS
if (APPLE)
  set(CMAKE_CXX_FLAGS "-Wno-deprecated-volatile")
endif()
