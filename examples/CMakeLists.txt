### @author: Roberto Preghenella
### @email: preghenella@bo.infn.it

file(GLOB PYTHIA8
  pythia8/*.cfg
  )

file(GLOB PYTHIA8DECAYS
  pythia8/decays/*.cfg
  )

file(GLOB CARDS
  cards/*.tcl
  )

file(GLOB LUTS
  smearing/luts/*.dat
  )

file(GLOB SCRIPTS
  scripts/*.sh
  scripts/*.json
  )

file(GLOB SMEARING
  smearing/*.C
  )

file(GLOB AOD
  aod/*.C
  )

install(FILES ${PYTHIA8} DESTINATION examples/pythia8)
install(FILES ${PYTHIA8DECAYS} DESTINATION examples/pythia8/decays)
install(FILES ${CARDS} DESTINATION examples/cards)
install(FILES ${LUTS} DESTINATION examples/smearing/luts)
install(FILES ${SCRIPTS} DESTINATION examples/scripts)
install(FILES ${SMEARING} DESTINATION examples/smearing)
install(FILES ${AOD} DESTINATION examples/aod)

