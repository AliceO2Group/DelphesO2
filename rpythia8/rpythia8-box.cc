#include <boost/program_options.hpp>
#include <stdlib.h>
#include <string>
#include <time.h>

#include "Pythia8/Pythia.h"
#include "Pythia8Plugins/HepMC2.h"

using namespace Pythia8;

int main(int argc, char** argv)
{

  int nevents, pdg, seed;
  std::string config, output, background_config;
  double eta_min, phi_min, p_min;
  double eta_max, phi_max, p_max;
  int npart;
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
      ("npart", po::value<int>(&npart)->default_value(1), "Number of particles per event in a box")
      ("etamin", po::value<double>(&eta_min)->default_value(0.), "Minimum eta")
      ("etamax", po::value<double>(&eta_max)->default_value(0.), "Maximum eta")
      ("phimin", po::value<double>(&phi_min)->default_value(0.), "Minimum phi")
      ("phimax", po::value<double>(&phi_max)->default_value(0.), "Maximum phi")
      ("pmin", po::value<double>(&p_min)->default_value(0.), "Minimum momentum")
      ("pmax", po::value<double>(&p_max)->default_value(0.), "Maximum momentum")
      ("xProd", po::value<double>(&xProd)->default_value(0.), "Production vertex in the x-direction")
      ("yProd", po::value<double>(&yProd)->default_value(0.), "Production vertex in the y-direction")
      ("zProd", po::value<double>(&zProd)->default_value(0.), "Production vertex in the z-direction")
      ("config,c", po::value<std::string>(&config), "Configuration file")
      ("background-config", po::value<std::string>(&background_config), "Background configuration file")
      ("output,o", po::value<std::string>(&output)->default_value("pythia-gun.hepmc"), "Output HepMC file")
      ("decay,D", po::bool_switch(&decay)->default_value(false), "Decay particle at production vertex")
      ("verbose,V", po::bool_switch(&verbose)->default_value(false), "Verbose event listing")
      ("seed", po::value<int>(&seed)->default_value(1), "initial seed");

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

  HepMC::Pythia8ToHepMC ToHepMC;
  HepMC::IO_GenEvent ascii_io(output, std::ios::out);

  // pythia
  Pythia pythia;

  // configure pythia
  pythia.readString("ProcessLevel:all = off");
  //  pythia.readString("SoftQCD:elastic on");
  if (!config.empty() && !pythia.readFile(config)) {
    std::cout << "Error: could not read config file \"" << config << "\"" << std::endl;
    return 1;
  }

  // check valid pdg code
  if (!pythia.particleData.isParticle(pdg)) {
    std::cout << "Error: invalid PDG code \"" << pdg << "\" is not in the particle list" << std::endl;
    return 1;
  }
  if (!pythia.particleData.isLepton(pdg) &&
      !pythia.particleData.isHadron(pdg) &&
      !pythia.particleData.isGamma(pdg) &&
      !pythia.particleData.isResonance(pdg)) {
    if (abs(pdg) < 1000000000) {
      std::cout << "Error: invalid PDG code \"" << pdg << "\"" << std::endl;
      return 1;
    } else {
      std::cout << "PDG code \"" << pdg << "\" stands for a nucleous" << std::endl;
    }
  }

  std::cout << "Random:seed =" + std::to_string(seed) << std::endl;
  pythia.readString("Random:setSeed = on");
  pythia.readString("Random:seed =" + std::to_string(seed));
  // init
  pythia.init();
  const double m = pythia.particleData.m0(pdg);

  // the particle
  Particle particle;
  particle.id(pdg);
  particle.status(11);
  particle.m(m);
  particle.xProd(xProd);
  particle.yProd(yProd);
  particle.zProd(zProd);

  // background interface
  Pythia8::Pythia* pythia_bkg = nullptr;
  if (!background_config.empty()) {
    std::cout << "Background: configure from " << background_config << std::endl;
    pythia_bkg = new Pythia8::Pythia;
    if (!pythia_bkg->readFile(background_config)) {
      std::cout << "Error: could not read config file \"" << background_config << "\"" << std::endl;
      return 1;
    }
    pythia_bkg->readString("Random:setSeed = on");
    pythia_bkg->readString("Random:seed =" + std::to_string(seed));
    pythia_bkg->init();
  }

  // event loop
  double eta, phi, p, pt, pl, e;
  srand(time(NULL));
  for (int iev = 0; iev < nevents; ++iev) {

    // reset, add particle and decay
    pythia.event.reset();
    for (int ipart = 0; ipart < npart; ipart++) {
      eta = eta_min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (eta_max - eta_min)));
      phi = phi_min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (phi_max - phi_min)));
      p = p_min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (p_max - p_min)));
      e = exp(2. * eta);
      pl = p * (e - 1.) / (1. + e);
      pt = sqrt(p * p - pl * pl);
      particle.e(sqrt(p * p + m * m));
      particle.px(pt * cos(phi));
      particle.py(pt * sin(phi));
      particle.pz(pl);
      pythia.event.append(particle);
    }
    if (decay)
      pythia.moreDecays();
    pythia.next();

    // print verbose
    if (verbose)
      pythia.event.list(1);

    // background
    if (pythia_bkg) {
      pythia_bkg->next();
      if (decay)
        pythia_bkg->moreDecays();
      pythia.event += pythia_bkg->event;
    }

    // write HepMC
    HepMC::GenEvent* hepmcevt = new HepMC::GenEvent();
    ToHepMC.fill_next_event(pythia, hepmcevt);
    ascii_io << hepmcevt;
    delete hepmcevt;
  }

  // print statistics
  pythia.stat();
  if (pythia_bkg) {
    pythia_bkg->stat();
    delete pythia_bkg;
  }

  return 0;
}
