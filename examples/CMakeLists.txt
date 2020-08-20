### @author: Roberto Preghenella
### @email: preghenella@bo.infn.it

file(GLOB PYTHIA8
  pythia8/*.cfg
  )

file(GLOB CARDS
  cards/*.tcl
  )

file(GLOB LUTS
  smearing/luts/*.dat
  )

file(GLOB AOD
  aod/*.C
  )

install(FILES ${PYTHIA8} DESTINATION examples/pythia8)
install(FILES ${CARDS} DESTINATION examples/cards)
install(FILES ${LUTS} DESTINATION examples/smearing/luts)
install(FILES ${AOD} DESTINATION examples/aod)

