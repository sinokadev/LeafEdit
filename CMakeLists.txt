cmake_minimum_required(VERSION 3.14)

cmake_policy(SET CMP0135 NEW)

project(LeafEdit)

include(FetchContent)

FetchContent_Declare(
  imgui
  URL https://github.com/ocornut/imgui/archive/docking.zip
)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
endif()

find_package(SDL3 REQUIRED)
find_package(Freetype REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FONTCONFIG REQUIRED fontconfig)

find_package(OpenGL REQUIRED)

add_library(imgui_SDL3 STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp

  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  
  ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp 
)

target_compile_definitions(imgui_SDL3 PUBLIC IMGUI_ENABLE_FREETYPE)

target_link_libraries(imgui_SDL3 PUBLIC 
    SDL3::SDL3 
    Freetype::Freetype
    OpenGL::GL
)

target_include_directories(imgui_SDL3 PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
  ${imgui_SOURCE_DIR}/misc/freetype
  ${FONTCONFIG_INCLUDE_DIRS}
)

add_executable(leafedit src/main.cpp)

target_include_directories(leafedit PRIVATE 
    ${FONTCONFIG_INCLUDE_DIRS}
)

target_link_libraries(leafedit PRIVATE 
    imgui_SDL3
    SDL3::SDL3
    OpenGL::GL
    ${FONTCONFIG_LIBRARIES}
)