# Copyright CERN and copyright holders of ALICE O2. This software is distributed
# under the terms of the GNU General Public License v3 (GPL Version 3), copied
# verbatim in the file "COPYING".
#
# See http://alice-o2.web.cern.ch/license for full licensing information.
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization or
# submit itself to any jurisdiction.

find_path(HepMC_INCLUDE_DIR
  NAMES IO_BaseClass.h
  PATH_SUFFIXES HepMC
  PATHS $ENV{HEPMC_ROOT}/include)

set(HepMC_INCLUDE_DIR ${HepMC_INCLUDE_DIR}/..)

find_library(HepMC_LIBRARIES
  NAMES libHepMC.so libHepMCfio.so libHepMC.dylib libHepMCfio.dylib
  PATHS $ENV{HEPMC_ROOT}/lib)

find_package_handle_standard_args(HepMC
  REQUIRED_VARS HepMC_INCLUDE_DIR HepMC_LIBRARIES
  FAIL_MESSAGE "HepMC could not be found")
