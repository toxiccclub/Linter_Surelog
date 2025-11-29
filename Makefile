# Use bash as the default shell
SHELL := /usr/bin/env bash

# Remove LC_ALL if defined
ifdef $(LC_ALL)
	undefine LC_ALL
endif

# Detect CPU cores
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
NPROC = $(shell nproc)
endif
ifeq ($(UNAME), Darwin)
NPROC = $(shell sysctl -n hw.physicalcpu)
endif

ifeq ($(CPU_CORES),)
	CPU_CORES := $(NPROC)
	ifeq ($(CPU_CORES),)
		CPU_CORES := 2
	endif
endif

PREFIX ?= /usr/local

ADDITIONAL_CMAKE_OPTIONS ?=

export CTEST_PARALLEL_LEVEL = $(CPU_CORES)


release: run-cmake-release
	cmake --build build -j $(CPU_CORES)

release-shared: run-cmake-release-shared
	cmake --build build -j $(CPU_CORES)

debug: run-cmake-debug
	cmake --build dbuild -j $(CPU_CORES)

quick: run-cmake-quick
	cmake --build dbuild -j $(CPU_CORES)

run-cmake-release:
	cmake -DCMAKE_BUILD_TYPE=Release \
	      -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
	      -DBUILD_SHARED_LIBS=OFF \
	      $(ADDITIONAL_CMAKE_OPTIONS) \
	      -S . -B build

run-cmake-release-shared:
	cmake -DCMAKE_BUILD_TYPE=Release \
	      -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
	      -DBUILD_SHARED_LIBS=ON \
	      $(ADDITIONAL_CMAKE_OPTIONS) \
	      -S . -B build

run-cmake-debug:
	cmake -DCMAKE_BUILD_TYPE=Debug \
	      -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
	      $(ADDITIONAL_CMAKE_OPTIONS) \
	      -S . -B dbuild

run-cmake-quick:
	cmake -DQUICK_COMP=1 \
	      -DCMAKE_BUILD_TYPE=Debug \
	      -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
	      $(ADDITIONAL_CMAKE_OPTIONS) \
	      -S . -B dbuild

run-cmake-coverage:
	cmake -DCMAKE_BUILD_TYPE=Debug \
	      -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
	      -DMY_CXX_WARNING_FLAGS="--coverage" \
	      $(ADDITIONAL_CMAKE_OPTIONS) \
	      -S . -B coverage-build

test:
	@echo "No tests in project."


install: release
	cmake --install build

install-shared: release-shared
	cmake --install build

clean:
	$(RM) -r build dbuild coverage-build dist

uninstall:
	$(RM) -r $(PREFIX)/bin/lint

