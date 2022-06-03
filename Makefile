
#
# Copyright (C) 2013-2019 Christoph Sommer <sommer@ccs-labs.org>
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

.PHONY: all makefiles clean cleanall doxy formatting formatting-strict

# if out/config.py exists, we can also create command line scripts for running simulations
ADDL_TARGETS =
ifeq ($(wildcard out/config.py),)
else
    ADDL_TARGETS += bin/veins_run
endif

# default target
all: src/Makefile $(ADDL_TARGETS)

ifndef AIRMOBISIMHOME
	$(error AIRMOBISIMHOME is not set! Please run the following commands: 'export AIRMOBISIMHOME=/path/to/your/AirMobiSim' )
endif

ifneq (,$(wildcard subprojects/veins_libairmobisim/airmobisim.proto))
	@rm subprojects/veins_libairmobisim/airmobisim.proto
endif
	@ln $(AIRMOBISIMHOME)/proto/airmobisim.proto subprojects/veins_libairmobisim/airmobisim.proto
ifdef MODE
	@cd src && $(MAKE)
else
	@cd src && $(MAKE) MODE=release
	@cd src && $(MAKE) MODE=debug
endif
	@cd subprojects/veins_libairmobisim && $(MAKE)

# command line scripts
bin/veins_run: src/scripts/veins_run.in.py out/config.py
	@echo "Creating script \"$@\""
	@sed '/# v-- contents of out\/config.py go here/r out/config.py' "$<" > "$@"
	@chmod a+x "$@"

# legacy
makefiles:
	@echo
	@echo '====================================================================='
	@echo 'Warning: make makefiles has been deprecated in favor of ./configure'
	@echo '====================================================================='
	@echo
	./configure
	@echo
	@echo '====================================================================='
	@echo 'Warning: make makefiles has been deprecated in favor of ./configure'
	@echo '====================================================================='
	@echo

clean:
	@cd subprojects/veins_libairmobisim && $(MAKE) clean
	@rm -f subprojects/veins_libairmobisim/airmobisim.proto
ifdef MODE
	@cd src && $(MAKE) clean
else
	@cd src && $(MAKE) MODE=release clean
	@cd src && $(MAKE) MODE=debug clean
endif

cleanall: clean
	rm -f src/Makefile
	rm -f out/config.py
	rm -f bin/veins_run

src/Makefile:
	@echo
	@echo '====================================================================='
	@echo '$@ does not exist.'
	@echo 'Please run "./configure" or use the OMNeT++ IDE to generate it.'
	@echo '====================================================================='
	@echo
	@exit 1

out/config.py:
	@echo
	@echo '====================================================================='
	@echo '$@ does not exist.'
	@echo 'Please run "./configure" to generate it.'
	@echo '====================================================================='
	@echo
	@exit 1

# autogenerated documentation
doxy:
	VEINS_VERSION=$(shell doc/version.sh) doxygen doxy.cfg

doxyshow: doxy
	xdg-open doc/doxy/index.html

formatting:
	bin/veins_format_code src
	bin/veins_format_code subprojects

formatting-strict:
	bin/veins_format_code --strict src
	bin/veins_format_code --strict subprojects
