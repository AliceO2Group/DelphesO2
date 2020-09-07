### @author: Roberto Preghenella
### @email: preghenella@bo.infn.it

find_package(HepMC REQUIRED)
find_package(Pythia REQUIRED)
find_package(Boost COMPONENTS program_options REQUIRED)

include_directories(${Pythia_INCLUDE_DIR} ${HepMC_INCLUDE_DIR})

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

