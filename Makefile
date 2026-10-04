# Convenience wrapper around the CMake build. The real build system is
# CMakeLists.txt; this Makefile just gives you `make`-style targets.
#
# Usage:
#   make            # configure + build (Release)
#   make test       # build + run ctest
#   make run        # build + launch (default music, 132 bpm)
#   make run-pulse  # launch with the CC0 Wireframe Pulse track
#   make clean      # remove the build/ directory
#   make reconfigure# force a fresh CMake configure (new build dir)
#
# Extra CMake flags can be passed as CMAKE_EXTRA, e.g.:
#   make CMAKE_EXTRA=-DIW_WARNINGS_AS_ERRORS=OFF
#
# To build audio in, the toolchain must expose SDL2 + libopenmpt via
# CMake/PkgConfig (brew install sdl2 libopenmpt on macOS; the CI does this).

BUILD_DIR   ?= build
TYPE        ?= Release
NPROC       ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
CMAKE       ?= cmake
CTEST       ?= ctest
BIN         := $(BUILD_DIR)/impossible_wireframe

.PHONY: all configure build test run run-pulse clean reconfigure help

all: build

# configure always wipes the build dir first. A stale CMake cache can hold an
# SDK path (CMAKE_OSX_SYSROOT / a baked OpenGL.framework / .tbd location) that
# no longer matches the installed Command Line Tools, which produces linker
# errors like "tapi error: malformed file .../libSystem.B.tbd ... unknown
# architecture" (this breaks even CMake's compiler check). A fresh configure
# re-resolves the SDK cleanly; CMakeLists pins CMAKE_OSX_ARCHITECTURES to the
# host and CMAKE_OSX_SYSROOT to xcrun's SDK. To pin a specific SDK:
#   make reconfigure CMAKE_EXTRA=-DIW_SYSROOT=$(xcrun --show-sdk-path)
configure:
	rm -rf $(BUILD_DIR)
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(TYPE) $(CMAKE_EXTRA)

build:
	@test -d $(BUILD_DIR) || $(MAKE) configure
	$(CMAKE) --build $(BUILD_DIR) --parallel $(NPROC)

test: build
	$(CTEST) --test-dir $(BUILD_DIR) --output-on-failure

run: build
	$(BIN) --bpm 132

run-pulse: build
	$(BIN) --bpm 128 --music assets/music/wireframe_pulse.wav

clean:
	rm -rf $(BUILD_DIR)

reconfigure: clean
	$(MAKE) configure

help:
	@echo "Targets: all build configure test run run-pulse clean reconfigure help"
	@echo "Vars:    BUILD_DIR TYPE NPROC CMAKE CTEST CMAKE_EXTRA"
