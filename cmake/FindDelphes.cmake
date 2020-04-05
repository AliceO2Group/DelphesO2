# @author: Roberto Preghenella
# @email: preghenella@bo.infn.it

include(FindPackageHandleStandardArgs)

find_path(Delphes_INCLUDE_DIR
  NAMES classes/DelphesClasses.h
  HINTS ENV DELPHES_ROOT
  PATH_SUFFIXES "include")

find_library(Delphes_LIBRARY
  NAMES libDelphes.so
  HINTS ${DELPHES_ROOT}/lib
  ENV LD_LIBRARY_PATH)

set(Delphes_INCLUDE_DIRS ${Delphes_INCLUDE_DIR})
set(Delphes_LIBRARIES ${Delphes_LIBRARY})

find_package_handle_standard_args(Delphes
  REQUIRED_VARS Delphes_INCLUDE_DIR Delphes_LIBRARY
  FAIL_MESSAGE "Delphes could not be found")

if(${Delphes_FOUND})
  mark_as_advanced(Delphes_INCLUDE_DIRS Delphes_LIBRARIES)

  # add targets
  if(NOT TARGET Delphes::Core)
    add_library(Delphes::Core INTERFACE IMPORTED)
    set_target_properties(Delphes::Core PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${Delphes_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${Delphes_LIBRARIES}"
      )
  endif()
  
endif()

