# Copyright CERN and copyright holders of ALICE O2. This software is distributed
# under the terms of the GNU General Public License v3 (GPL Version 3), copied
# verbatim in the file "COPYING".
#
# See http://alice-o2.web.cern.ch/license for full licensing information.
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization or
# submit itself to any jurisdiction.

find_path(Pythia_INCLUDE_DIR
  NAMES Pythia.h
  PATH_SUFFIXES Pythia8
  PATHS $ENV{PYTHIA_ROOT}/include)

set(Pythia_INCLUDE_DIR ${Pythia_INCLUDE_DIR}/..)
	
find_library(Pythia_LIBRARIES
  NAMES libpythia8.so libpythia8.dylib
  PATHS $ENV{PYTHIA_ROOT}/lib)

find_package_handle_standard_args(Pythia
  REQUIRED_VARS Pythia_INCLUDE_DIR Pythia_LIBRARIES
  FAIL_MESSAGE "Pythia could not be found")

