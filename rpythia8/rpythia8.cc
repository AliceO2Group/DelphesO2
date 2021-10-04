#include <boost/program_options.hpp>
#include <string>

#include "Pythia8/Pythia.h"
#include "Pythia8/ParticleData.h"
#include "Pythia8Plugins/HepMC2.h"

int main(int argc, char** argv)
{

  int nevents, inject_nevents, seed;
  std::string config, output, inject_config;

  /** process arguments **/
  namespace po = boost::program_options;
  po::options_description desc("Options");
  try {
    desc.add_options()
      ("help", "Print help messages")
      ("nevents,n"      , po::value<int>(&nevents)->default_value(10), "Number of events")
      ("config,c"       , po::value<std::string>(&config), "Configuration file")
      ("output,o"       , po::value<std::string>(&output)->default_value("pythia8.hepmc"), "Output HepMC file")
      ("inject-config"  , po::value<std::string>(&inject_config), "Injected event configuration file")
      ("inject-nevents" , po::value<int>(&inject_nevents)->default_value(1), "Number of events to inject")
      ("seed"           , po::value<int>(&seed)->default_value(1), "initial seed");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 1;
    }
  } catch (std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::cout << desc << std::endl;
    return 1;
  }

  // pythia, config and init
  Pythia8::Pythia pythia;
  pythia.particleData.addParticle(9920443, "X(3872)", 3, 0, 0, 3.87196, 0.00012);

  Pythia8::Rndm rndm;
  if (!config.empty() && !pythia.readFile(config)) {
    std::cout << "Error: could not read config file \"" << config << "\"" << std::endl;
    return 1;
  }
  std::cout << "Random:seed =" + std::to_string(seed) << std::endl;
  pythia.readString("Random:setSeed = on");
  pythia.readString("Random:seed =" + std::to_string(seed));
  pythia.init();
  rndm.init();

  // Interface for conversion from Pythia8::Event to HepMC event.
  HepMC::Pythia8ToHepMC ToHepMC;
  HepMC::IO_GenEvent output_io(output, std::ios::out);

  // injection interface
  Pythia8::Pythia* pythia_inj = nullptr;
  if (!inject_config.empty()) {
    std::cout << "Injection: configure from " << inject_config << std::endl;
    pythia_inj = new Pythia8::Pythia;
    if (!pythia_inj->readFile(inject_config)) {
      std::cout << "Error: could not read config file \"" << inject_config << "\"" << std::endl;
      return 1;
    }
    pythia_inj->readString("Random:setSeed = on");
    pythia_inj->readString("Random:seed =" + std::to_string(seed));
    pythia_inj->init();
  }

  // event loop
  for (int iev = 0; iev < nevents; ++iev) {

    // generate event
    pythia.next();
    auto offset = pythia.event.size();

    // injection
    if (pythia_inj) {
      for (int iiev = 0; iiev < inject_nevents; ++iiev) {
        pythia_inj->next();
        pythia.event += pythia_inj->event;
      }
    }

    // convert to HepMC
    HepMC::GenEvent* hepmcevt = new HepMC::GenEvent();
    ToHepMC.fill_next_event(pythia, hepmcevt);

    output_io << hepmcevt;
    delete hepmcevt;

  } // end of event loop

  // print statistics
  pythia.stat();
  if (pythia_inj) {
    pythia_inj->stat();
    delete pythia_inj;
  }

  return 0;
}
