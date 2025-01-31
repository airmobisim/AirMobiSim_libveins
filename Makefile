
#
# Copyright (C) 2013-2025 Christoph Sommer <sommer@ccs-labs.org>
# Copyright (C) 2022-2025 Christoph Sommer <sommer@ccs-labs.org>
#
# Documentation for these modules is at http://veins.car2x.org/
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

.PHONY: all clean cleanall grpc conan_flags build_src

# Verzeichnisse und Dateien
BUILD_DIR = build
GENERATOR_DIR = $(BUILD_DIR)/Release/generators
TOOLCHAIN_FILE = $(GENERATOR_DIR)/conan_toolchain.cmake
CMAKE_BUILD_TYPE = Release

# Default-Ziel
all: conan_flags grpc build_src

# Conan Toolchain generieren
$(TOOLCHAIN_FILE):
	@echo "Running conan install to generate CMake files..."
	@mkdir -p $(BUILD_DIR)/Release
	@cd $(BUILD_DIR)/Release && conan install ../.. --build=missing
	@echo "Conan CMake files generated in $(GENERATOR_DIR)."

# Conan Flags generieren
conan_flags: $(TOOLCHAIN_FILE)
	@echo "Generating Conan flags..."
	@echo "-----------------------------------------------------"
	@echo "$(CONAN_RUNTIME_LIB_DIRS)"
	@mkdir -p $(BUILD_DIR)
	@echo "# Generated Conan flags" > $(BUILD_DIR)/conan_flags.mk
	@grep -E 'CONAN_.*_DIRS' $(GENERATOR_DIR)/*.cmake | \
	sed -e 's|set(CONAN_INCLUDE_DIRS_\(.*\) "\(.*\)")|CXXFLAGS+=-I\2|' \
	    -e 's|set(CONAN_LIB_DIRS_\(.*\) "\(.*\)")|LDFLAGS+=-L\2|' \
	    -e 's|set(CONAN_LIBS_\(.*\) "\(.*\)")|LDFLAGS+=-l\1|' \
	    >> $(BUILD_DIR)/conan_flags.mk
	@echo "Conan flags generated."

# gRPC-Build mit CMake
grpc: $(TOOLCHAIN_FILE)
	@echo "Building gRPC dependencies with CMake..."
	@mkdir -p $(BUILD_DIR)/Release
	@cd $(BUILD_DIR)/Release && cmake ../.. -DCMAKE_TOOLCHAIN_FILE=$(TOOLCHAIN_FILE) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -G "Unix Makefiles"
	@cd $(BUILD_DIR)/Release && $(MAKE)
	@echo "gRPC build completed."

# Build im src-Verzeichnis
build_src: grpc
ifndef AIRMOBISIMHOME
	$(error AIRMOBISIMHOME is not set! Please run the following commands: 'export AIRMOBISIMHOME=/path/to/your/AirMobiSim' )
endif
	@ln -sf $(AIRMOBISIMHOME)/proto/airmobisim.proto airmobisim.proto
	@echo "Building src/Makefile..."
	@cd src && $(MAKE) CXXFLAGS="$(CXXFLAGS)" LDFLAGS="$(LDFLAGS)"
ifdef MODE
	@cd src && $(MAKE) MODE=release CXXFLAGS="$(CXXFLAGS)" LDFLAGS="$(LDFLAGS)"
	@cd src && $(MAKE) MODE=debug CXXFLAGS="$(CXXFLAGS)" LDFLAGS="$(LDFLAGS)"
endif

# Cleanup
clean:
	@rm -rf $(BUILD_DIR)
	@rm -f airmobisim.proto
ifdef MODE
	@cd src && $(MAKE) clean
else
	@cd src && $(MAKE) MODE=release clean
	@cd src && $(MAKE) MODE=debug clean
endif

cleanall: clean
	rm -f src/Makefile
	rm -f out/config.py

