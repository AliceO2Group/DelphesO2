#include <boost/program_options.hpp>
#include <string>

#include "Pythia8/Pythia.h"
#include "Pythia8Plugins/HepMC2.h"

using namespace Pythia8;

int main(int argc, char **argv)
{
  
  int nevents;
  std::string config, output;
  
  /** process arguments **/
  namespace po = boost::program_options;
  po::options_description desc("Options");
  try {
    desc.add_options()
      ("help", "Print help messages")
      ("nevents,n" , po::value<int>(&nevents)->default_value(10), "Number of events")
      ("config,c"  , po::value<std::string>(&config), "Configuration file")
      ("output,o"  , po::value<std::string>(&output)->default_value("pythia8.hepmc"), "Output HepMC file")
      ;
    
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    
    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 1;
    }
  }
  catch(std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::cout << desc << std::endl;
    return 1;
  }

  // Interface for conversion from Pythia8::Event to HepMC event.
  HepMC::Pythia8ToHepMC ToHepMC;
  HepMC::IO_GenEvent ascii_io(output, std::ios::out); 
  
  // pythia, config and init
  Pythia8::Pythia pythia;
  Pythia8::Rndm   rndm;
  if (!config.empty() && !pythia.readFile(config)) {
    std::cout << "Error: could not read config file \"" << config << "\"" << std::endl;
    return 1;
  }
  pythia.init();
  rndm.init();

  // event loop
  for (int iev = 0; iev < nevents; ++iev) {

    // generate event
    pythia.next();

    // photon conversions 
    
    // convert to HepMC
    HepMC::GenEvent *hepmcevt = new HepMC::GenEvent();
    ToHepMC.fill_next_event(pythia, hepmcevt);
    ascii_io << hepmcevt;
    delete hepmcevt;
    
  } // end of event loop
  
  // print statistics
  pythia.stat();
  
  return 0;
}
