cmake_minimum_required(VERSION 3.20)
project(ImpossibleWireframe VERSION 4.25 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(IW_BUILD_TESTS "Build geometry validation tests" ON)
# Off by default so a fresh toolchain (new Apple clang / MSVC) can't break a
# user's first build over an unrelated new warning. CI sets this ON to keep the
# strictness where the toolchain is controlled.
option(IW_WARNINGS_AS_ERRORS "Treat project warnings as errors" OFF)
function(iw_warnings target)
 if(MSVC)
  target_compile_options(${target} PRIVATE /W4 $<$<BOOL:${IW_WARNINGS_AS_ERRORS}>:/WX>)
 else()
  target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic $<$<BOOL:${IW_WARNINGS_AS_ERRORS}>:-Werror>)
 endif()
endfunction()
add_library(iw_geometry src/Geometry.cpp src/AdvancedGeometry.cpp src/Timeline.cpp src/Scene.cpp src/TextScroller.cpp src/Image.cpp src/Effects.cpp)
target_include_directories(iw_geometry PUBLIC include)
iw_warnings(iw_geometry)
find_package(OpenGL QUIET)
find_package(glfw3 QUIET)
find_package(PkgConfig QUIET)
# Some environments' find_package(glfw3) sets glfw3_FOUND but doesn't create the
# GLFW::GLFW3 imported target (e.g. a bare find_library-based setup). Fall back
# to a hand-rolled imported target so the link carries the found library path.
if(glfw3_FOUND AND NOT TARGET GLFW::GLFW3)
 find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h HINTS /opt/homebrew/include /opt/local/include /usr/local/include)
 find_library(GLFW_LIBRARY NAMES glfw3 glfw HINTS /opt/homebrew/lib /opt/local/lib /usr/local/lib)
 if(GLFW_INCLUDE_DIR AND GLFW_LIBRARY)
  add_library(GlfwFallback UNKNOWN IMPORTED)
  set_target_properties(GlfwFallback PROPERTIES IMPORTED_LOCATION "${GLFW_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${GLFW_INCLUDE_DIR}")
 endif()
endif()
# Resolve the GLFW link target: prefer the imported target (correct path),
# fall back to the bare name (linker default search) if no target was found.
if(TARGET GLFW::GLFW3)
 set(IW_GLFW_TARGET GLFW::GLFW3)
elseif(TARGET GlfwFallback)
 set(IW_GLFW_TARGET GlfwFallback)
else()
 set(IW_GLFW_TARGET glfw)
endif()
# SDL2: try CMake config, then pkg-config, then a raw find_path/find_library
# (Homebrew/CLT ship SDL2 with no CMake config and no pkg-config on a fresh
# Mac, so the raw fallback is what actually finds it in many cases).
find_package(SDL2 CONFIG QUIET)
if(TARGET SDL2::SDL2)
 set(IW_SDL_TARGET SDL2::SDL2)
elseif(PkgConfig_FOUND)
 pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
 if(TARGET PkgConfig::SDL2)
  set(IW_SDL_TARGET PkgConfig::SDL2)
 endif()
endif()
if(NOT IW_SDL_TARGET)
 find_path(SDL2_INCLUDE_DIR SDL.h HINTS /opt/homebrew/include /opt/local/include /usr/local/include)
 find_library(SDL2_LIBRARY NAMES SDL2 HINTS /opt/homebrew/lib /opt/local/lib /usr/local/lib)
 if(SDL2_INCLUDE_DIR AND SDL2_LIBRARY)
  add_library(iw_sdl2 UNKNOWN IMPORTED)
  set_target_properties(iw_sdl2 PROPERTIES IMPORTED_LOCATION "${SDL2_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}")
  set(IW_SDL_TARGET iw_sdl2)
 endif()
endif()
if(PkgConfig_FOUND)
 pkg_check_modules(OPENMPT QUIET IMPORTED_TARGET libopenmpt)
endif()
if(TARGET PkgConfig::OPENMPT)
 set(IW_OPENMPT_TARGET PkgConfig::OPENMPT)
else()
 find_path(OPENMPT_INCLUDE_DIR libopenmpt/libopenmpt.h HINTS /opt/homebrew/include /opt/local/include /usr/local/include)
 find_library(OPENMPT_LIBRARY NAMES openmpt HINTS /opt/homebrew/lib /opt/local/lib /usr/local/lib)
 if(OPENMPT_INCLUDE_DIR AND OPENMPT_LIBRARY)
  add_library(iw_openmpt UNKNOWN IMPORTED)
  set_target_properties(iw_openmpt PROPERTIES IMPORTED_LOCATION "${OPENMPT_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${OPENMPT_INCLUDE_DIR}")
  set(IW_OPENMPT_TARGET iw_openmpt)
 endif()
endif()
if(OpenGL_FOUND AND glfw3_FOUND)
 add_executable(impossible_wireframe src/main.cpp src/Renderer.cpp src/Audio.cpp src/TextScroller.cpp src/Image.cpp)
  target_include_directories(impossible_wireframe PRIVATE include thirdparty)
target_include_directories(iw_geometry PUBLIC thirdparty)
    # Link GLFW via the imported target (carries the found library path, so a
    # Homebrew install in /opt/homebrew/lib resolves correctly) — never the bare
    # "glfw" string, which forces the linker to search default system paths.
    #
    # macOS OpenGL is tricky. Two failure modes hit:
    #  (a) find_package(OpenGL) resolves OpenGL::GL to a framework path under a
    #      specific SDK (.../MacOSX.sdk/.../OpenGL.framework); when that SDK path
    #      is absent the linker fails with "No rule to make target".
    #  (b) the CLT SDK ships OpenGL as a .tbd text stub whose arch tokens
    #      (e.g. "arm64e.x1-macos") some clang/ld versions can't parse:
    #      "tapi error: malformed file .../OpenGL.tbd ... unknown architecture".
    # Strategy: prefer a real dylib (Homebrew's libOpenGL.dylib) when present —
    # it has no .tbd stub and sidesteps both failures. Otherwise fall back to
    # -framework OpenGL (resolves at link time) and add OPENGL_INCLUDE_DIR so
    # <OpenGL/gl3.h> doesn't depend on toolchain-default SDK search.
    if(APPLE)
     find_path(IW_OPENGL_INCLUDE_DIR OpenGL/gl3.h
               HINTS /opt/homebrew/include /opt/local/include /usr/local/include
                     ${OPENGL_INCLUDE_DIR})
     find_library(IW_OPENGL_LIB OpenGL
                  HINTS /opt/homebrew/lib /opt/local/lib /usr/local/lib)
     if(IW_OPENGL_LIB)
      target_link_libraries(impossible_wireframe PRIVATE iw_geometry ${IW_GLFW_TARGET} ${IW_OPENGL_LIB})
     else()
      target_link_libraries(impossible_wireframe PRIVATE iw_geometry ${IW_GLFW_TARGET} "-framework OpenGL")
     endif()
     if(IW_OPENGL_INCLUDE_DIR)
      target_include_directories(impossible_wireframe PRIVATE "${IW_OPENGL_INCLUDE_DIR}")
     endif()
    else()
     target_link_libraries(impossible_wireframe PRIVATE iw_geometry ${IW_GLFW_TARGET} OpenGL::GL)
    endif()
  if(IW_SDL_TARGET AND IW_OPENMPT_TARGET)
   target_link_libraries(impossible_wireframe PRIVATE ${IW_SDL_TARGET} ${IW_OPENMPT_TARGET})
   target_compile_definitions(impossible_wireframe PRIVATE IW_HAS_AUDIO SDL_MAIN_HANDLED)
  elseif(NOT IW_SDL_TARGET)
   message(WARNING "SDL2 not found: renderer builds with BPM-only timing (macOS: brew install sdl2)")
  elseif(NOT IW_OPENMPT_TARGET)
   message(WARNING "libopenmpt not found: renderer builds with BPM-only timing (macOS: brew install libopenmpt)")
  endif()
 iw_warnings(impossible_wireframe)
 if(APPLE)
  target_compile_definitions(impossible_wireframe PRIVATE GL_SILENCE_DEPRECATION)
 endif()
  if(NOT EXISTS ${CMAKE_SOURCE_DIR}/shaders/wire.vert)
   message(FATAL_ERROR "shaders/ directory (wire.vert) missing; cannot build the renderer")
  endif()
  # Copy shaders next to the binary (the runtime resolves them there). rm -rf
  # first so a shader deleted from the source doesn't linger in the build dir.
  add_custom_command(TARGET impossible_wireframe POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E rm -rf $<TARGET_FILE_DIR:impossible_wireframe>/shaders
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:impossible_wireframe>/shaders)
else()
 message(WARNING "OpenGL/glfw3 not found: renderer target disabled; geometry/tests remain buildable")
endif()
if(IW_BUILD_TESTS)
 enable_testing()
 add_executable(geometry_tests tests/geometry_tests.cpp)
 target_link_libraries(geometry_tests PRIVATE iw_geometry)
 # Point the logo test at the repo's logo pack so it runs (and fails loud) on
 # every platform instead of silently skipping.
 target_compile_definitions(geometry_tests PRIVATE IW_TEST_LOGO_PACK="${CMAKE_SOURCE_DIR}/UBER_Fullscreen_Logo_Pack")
 iw_warnings(geometry_tests)
 add_test(NAME geometry_tests COMMAND geometry_tests)
endif()
