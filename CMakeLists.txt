cmake_minimum_required(VERSION 3.14)

cmake_policy(SET CMP0135 NEW)

project(ImGuiSDL2WGPUExample)

include(FetchContent)

FetchContent_Declare(
  imgui
  URL https://github.com/ocornut/imgui/archive/docking.zip
)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
endif()

find_package(SDL2 REQUIRED)
find_package(Freetype REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FONTCONFIG REQUIRED fontconfig)

add_library(imgui_sdl2 STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp

  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
  
  ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp 
)

target_compile_definitions(imgui_sdl2 PUBLIC IMGUI_ENABLE_FREETYPE)
target_link_libraries(imgui_sdl2 PUBLIC SDL2::SDL2 Freetype::Freetype)

target_include_directories(imgui_sdl2 PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
  ${imgui_SOURCE_DIR}/misc/freetype
)

add_executable(leafedit src/main.cpp)

target_link_libraries(leafedit PRIVATE 
    imgui_sdl2
    SDL2::SDL2
    ${FONTCONFIG_LIBRARIES}
)