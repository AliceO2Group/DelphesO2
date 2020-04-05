# @author: Roberto Preghenella
# @email: preghenella@bo.infn.it

# Init
include(FindPackageHandleStandardArgs)

# find includes
find_path(AliceO2_INCLUDE_DIR
  NAMES Headers/DataHeader.h
  HINTS ENV O2_ROOT
  PATH_SUFFIXES "include"
)

set(AliceO2_INCLUDE_DIRS ${AliceO2_INCLUDE_DIR})

# find libraries
find_library(AliceO2_LIBRARY_DETECTORVERTEXING NAMES O2DetectorsVertexing HINTS ${O2_ROOT}/lib ENV LD_LIBRARY_PATH)
find_library(AliceO2_LIBRARY_RECONSTRUCTIONDATAFORMATS NAMES O2ReconstructionDataFormats HINTS ${O2_ROOT}/lib ENV LD_LIBRARY_PATH)
find_library(AliceO2_LIBRARY_GPUCOMMON NAMES O2GPUCommon HINTS ${O2_ROOT}/lib ENV LD_LIBRARY_PATH)

set(AliceO2_LIBRARIES
  ${AliceO2_LIBRARY_DETECTORVERTEXING}
  ${AliceO2_LIBRARY_RECONSTRUCTIONDATAFORMATS}
  ${AliceO2_LIBRARY_GPUCOMMON}
)

# handle the QUIETLY and REQUIRED arguments and set AliceO2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(AliceO2
  REQUIRED_VARS AliceO2_INCLUDE_DIR
  AliceO2_LIBRARY_DETECTORVERTEXING
  AliceO2_LIBRARY_RECONSTRUCTIONDATAFORMATS
  AliceO2_LIBRARY_GPUCOMMON
  FAIL_MESSAGE "AliceO2 could not be found."
)

if(${AliceO2_FOUND})
    message(STATUS "AliceO2 found, libraries: ${AliceO2_LIBRARIES}")

    mark_as_advanced(AliceO2_INCLUDE_DIRS AliceO2_LIBRARIES)

    # add targets
    if(NOT TARGET AliceO2::DetectorsVertexing)
        add_library(AliceO2::DetectorsVertexing INTERFACE IMPORTED)
        set_target_properties(AliceO2::DetectorsVertexing PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${AliceO2_INCLUDE_DIRS}"
          INTERFACE_LINK_LIBRARIES "${AliceO2_LIBRARY_DETECTORVERTEXING}"
        )
    endif()

    if(NOT TARGET AliceO2::ReconstructionDataFormats)
        add_library(AliceO2::ReconstructionDataFormats INTERFACE IMPORTED)
        set_target_properties(AliceO2::ReconstructionDataFormats PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${AliceO2_INCLUDE_DIRS}"
          INTERFACE_LINK_LIBRARIES "${AliceO2_LIBRARY_RECONSTRUCTIONDATAFORMATS}"
        )
    endif()

    if(NOT TARGET AliceO2::GPUCommon)
        add_library(AliceO2::GPUCommon INTERFACE IMPORTED)
        set_target_properties(AliceO2::GPUCommon PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${AliceO2_INCLUDE_DIRS}"
          INTERFACE_LINK_LIBRARIES "${AliceO2_LIBRARY_GPUCOMMON}"
        )
    endif()

endif()
