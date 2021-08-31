### @author: Roberto Preghenella
### @email: preghenella@bo.infn.it

find_package(HepMC REQUIRED)
find_package(Pythia REQUIRED)
find_package(ROOT REQUIRED)
find_package(Boost COMPONENTS program_options REQUIRED)

find_path(ROOT_INCLUDE_DIR
  NAMES TMath.h
  PATH_SUFFIXES ROOT
  PATHS $ENV{ROOTSYS}/include)

set(ROOT_INCLUDE_DIR ${ROOT_INCLUDE_DIR})

include_directories(${Pythia_INCLUDE_DIR} ${HepMC_INCLUDE_DIR} ${ROOT_INCLUDE_DIR})

add_executable(rpythia8 rpythia8.cc)
target_link_libraries(rpythia8
  ${Pythia_LIBRARIES}
  ${HepMC_LIBRARIES} ${Boost_LIBRARIES})
install(TARGETS rpythia8 RUNTIME DESTINATION bin)

add_executable(rpythia8-gun rpythia8-gun.cc)
target_link_libraries(rpythia8-gun
  ${Pythia_LIBRARIES}
  ${HepMC_LIBRARIES} ${Boost_LIBRARIES})
install(TARGETS rpythia8-gun RUNTIME DESTINATION bin)

add_executable(rpythia8-box rpythia8-box.cc)
target_link_libraries(rpythia8-box
  ${Pythia_LIBRARIES}
  ${HepMC_LIBRARIES} ${Boost_LIBRARIES})
install(TARGETS rpythia8-box RUNTIME DESTINATION bin)

add_executable(rpythia8-bgbw rpythia8-bgbw.cc )
target_link_libraries(rpythia8-bgbw
  ${Pythia_LIBRARIES}
  ${HepMC_LIBRARIES}
  ${ROOT_LIBRARIES}
  ${Boost_LIBRARIES})
install(TARGETS rpythia8-bgbw RUNTIME DESTINATION bin)

