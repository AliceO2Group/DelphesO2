set(CMAKE_VERBOSE_MAKEFILE ON)

set(SOURCES
  VertexFitter.cc
  TrackSmearer.cc
  TrackUtils.cc
  TOFLayer.cc
  RICHdetector.cc
  )

set(HEADERS
  VertexFitter.hh
  TrackSmearer.hh
  TrackUtils.hh
  RICHdetector.hh
  )

get_target_property(DELPHES_INCLUDE_DIRECTORIES
  Delphes::Core
  INTERFACE_INCLUDE_DIRECTORIES)

get_target_property(GPUCOMMON_INCLUDE_DIRECTORIES
  AliceO2::GPUCommon
  INTERFACE_INCLUDE_DIRECTORIES)

get_target_property(DETECTORSVERTEXING_INCLUDE_DIRECTORIES
  AliceO2::DetectorsVertexing
  INTERFACE_INCLUDE_DIRECTORIES)

get_target_property(RECONSTRUCTIONDATAFORMATS_INCLUDE_DIRECTORIES
  AliceO2::ReconstructionDataFormats
  INTERFACE_INCLUDE_DIRECTORIES)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
  ${DELPHES_INCLUDE_DIRECTORIES}
  ${DETECTORSVERTEXING_INCLUDE_DIRECTORIES}
  ${RECONSTRUCTIONDATAFORMATS_INCLUDE_DIRECTORIES}
  ${GPUCOMMON_INCLUDE_DIRECTORIES}/GPU)

add_library(DelphesO2 SHARED ${SOURCES} G__DelphesO2)
root_generate_dictionary(G__DelphesO2 ${HEADERS} LINKDEF DelphesO2LinkDef.h)

target_link_libraries(DelphesO2
  Delphes::Core
  ROOT::MathCore
  AliceO2::DetectorsVertexing
  AliceO2::ReconstructionDataFormats
  AliceO2::GPUCommon
  FairRoot::Tools
  )

install(TARGETS DelphesO2 DESTINATION lib)

install(FILES ${HEADERS} lutCovm.hh DESTINATION include)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/libDelphesO2_rdict.pcm
  ${CMAKE_CURRENT_BINARY_DIR}/libDelphesO2.rootmap
  DESTINATION lib)
