#include <boost/program_options.hpp>
#include <string>

#include "Pythia8/Pythia.h"
#include "Pythia8Plugins/HepMC2.h"

using namespace Pythia8;

int main(int argc, char **argv)
{

  int nevents, pdg;
  std::string config, output;
  double px, py, pz;
  double xProd, yProd, zProd;
  bool verbose, decay;
  
  /** process arguments **/
  namespace po = boost::program_options;
  po::options_description desc("Options");
  try {
    desc.add_options()
      ("help", "Print help messages")
      ("nevents,n", po::value<int>(&nevents)->default_value(10), "Number of events to be generated")
      ("pdg,p", po::value<int>(&pdg)->required(), "PDG code of the particle")
      ("px", po::value<double>(&px)->default_value(0.), "Momentum in the x-direction")
      ("py", po::value<double>(&py)->default_value(0.), "Momentum in the y-direction")
      ("pz", po::value<double>(&pz)->default_value(0.), "Momentum in the z-direction")
      ("xProd", po::value<double>(&xProd)->default_value(0.), "Production vertex in the x-direction")
      ("yProd", po::value<double>(&yProd)->default_value(0.), "Production vertex in the y-direction")
      ("zProd", po::value<double>(&zProd)->default_value(0.), "Production vertex in the z-direction")
      ("config,c", po::value<std::string>(&config), "Configuration file")
      ("output,o", po::value<std::string>(&output)->default_value("pythia-gun.hepmc"), "Output HepMC file")
      ("decay,D", po::bool_switch(&decay)->default_value(false), "Decay particle at production vertex")
      ("verbose,V", po::bool_switch(&verbose)->default_value(false), "Verbose event listing")
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
  
  HepMC::Pythia8ToHepMC ToHepMC;
  HepMC::IO_GenEvent ascii_io(output, std::ios::out);
  
  // pythia
  Pythia pythia;

  // check valid pdg code
  if (!pythia.particleData.isLepton(pdg) &&
      !pythia.particleData.isHadron(pdg) &&
      !pythia.particleData.isResonance(pdg)) {
    std::cout << "Error: invalid PDG code \"" << pdg << "\"" << std::endl;
    return 1;
  }

  // config
  pythia.readString("ProcessLevel:all = off");
  //  pythia.readString("SoftQCD:elastic on");
  if (!config.empty() && !pythia.readFile(config)) {
    std::cout << "Error: could not read config file \"" << config << "\"" << std::endl;
    return 1;
  }

  // init
  pythia.init();
  double m = pythia.particleData.m0(pdg);
  double e = sqrt(px * px + py * py + pz * pz + m * m);

  // the particle
  Particle particle;
  particle.id(pdg);
  particle.status(11);
  particle.px(px);
  particle.py(py);
  particle.pz(pz);
  particle.e(e);
  particle.m(m);
  particle.xProd(xProd);
  particle.yProd(yProd);
  particle.zProd(zProd);
  
  // event loop
  for (int iev = 0; iev < nevents; ++iev) {

    // reset, add particle and decay
    pythia.event.reset();
    pythia.event.append(particle);
    if (decay) pythia.moreDecays();
    pythia.next();
    
    // print verbose
    if (verbose) pythia.event.list(1);

    // write HepMC
    HepMC::GenEvent *hepmcevt = new HepMC::GenEvent();
    ToHepMC.fill_next_event(pythia, hepmcevt);
    ascii_io << hepmcevt;
    delete hepmcevt;
  }
  
}
