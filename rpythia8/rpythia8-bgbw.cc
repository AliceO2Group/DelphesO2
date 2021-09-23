#include <boost/program_options.hpp>
#include <stdlib.h>
#include <string>
#include <time.h>

#include "Pythia8/Pythia.h"
#include "Pythia8Plugins/HepMC2.h"

using namespace Pythia8;

#include "TMath.h"
#include "TRandom3.h"
#include "TF1.h"

const float maximum = 20;

Double_t IntegrandBG(const double* x, const double* p)
{
  // integrand for boltzman-gibbs blast wave
  // x[0] -> r (radius)
  // p[0] -> mass
  // p[1] -> pT (transverse momentum)
  // p[2] -> beta_max (surface velocity)
  // p[3] -> T (freezout temperature)
  // p[4] -> n (velocity profile)

  double x0 = x[0];

  double mass = p[0];
  double pT = p[1];
  double beta_max = p[2];
  double temp = p[3];
  Double_t n = p[4];

  // Keep beta within reasonable limits
  Double_t beta = beta_max * TMath::Power(x0, n);
  if (beta > 0.9999999999999999)
    beta = 0.9999999999999999;

  double mT = TMath::Sqrt(mass * mass + pT * pT);

  double rho0 = TMath::ATanH(beta);
  double arg00 = pT * TMath::SinH(rho0) / temp;
  if (arg00 > 700.)
    arg00 = 700.; // avoid FPE
  double arg01 = mT * TMath::CosH(rho0) / temp;
  double f0 = x0 * mT * TMath::BesselI0(arg00) * TMath::BesselK1(arg01);

  //  printf("r=%f, pt=%f, beta_max=%f, temp=%f, n=%f, mt=%f, beta=%f, rho=%f, argI0=%f, argK1=%f\n", x0, pT, beta_max, temp, n, mT, beta, rho0, arg00, arg01);

  return f0;
}

Double_t StaticBGdNdPt(const double* x, const double* p)
{

  // implementation of BGBW (1/pt dNdpt)

  double pT = x[0];
  ;

  double mass = p[0];
  double beta = p[1];
  double temp = p[2];
  double n = p[3];
  double norm = p[4];

  static TF1* fIntBG = 0;
  if (!fIntBG)
    fIntBG = new TF1("fIntBG", IntegrandBG, 0, 1, 5);

  fIntBG->SetParameters(mass, pT, beta, temp, n);
  double result = fIntBG->Integral(0, 1);
  //  printf ("[%4.4f], Int :%f\n", pT, result);
  return result * norm; //*1e30;;
}

Double_t StaticBGdNdPtTimesPt(const double* x, const double* p)
{
  // BGBW dNdpt implementation
  return x[0] * StaticBGdNdPt(x, p);
}

Double_t StaticBGdNdMtTimesMt(const double* x, const double* p)
{
  // BGBW dNdpt implementation
  // X0 is mt here
  Double_t pt = TMath::Sqrt(x[0] * x[0] - p[0] * p[0]);
  return pt * StaticBGdNdPt(&pt, p);
}

TF1* fLastFunc = nullptr;
float fLineWidth = 2;

// Times Pt funcs
// Boltzmann-Gibbs Blast Wave
TF1* GetBGBWdNdptTimesPt(Double_t mass, Double_t beta, Double_t temp, Double_t n,
                         Double_t norm, const char* name)
{

  // BGBW, dNdpt

  fLastFunc = new TF1(name, StaticBGdNdPtTimesPt, 0.0, maximum, 5);
  fLastFunc->SetParameters(mass, beta, temp, n, norm);
  fLastFunc->FixParameter(0, mass);
  fLastFunc->SetParNames("mass", "#beta", "temp", "n", "norm");
  fLastFunc->SetLineWidth(fLineWidth);
  return fLastFunc;
}

// Boltzmann-Gibbs Blast Wave
TF1* GetBGBWdNdptTimesMt(Double_t mass, Double_t beta, Double_t temp, Double_t n,
                         Double_t norm, const char* name)
{

  // BGBW, dNdpt
  // 1/Mt dN/dmt
  fLastFunc = new TF1(name, StaticBGdNdMtTimesMt, 0.0, maximum, 5);
  fLastFunc->SetParameters(mass, beta, temp, n, norm);
  fLastFunc->FixParameter(0, mass);
  fLastFunc->SetParNames("mass", "#beta", "temp", "n", "norm");
  fLastFunc->SetLineWidth(fLineWidth);
  return fLastFunc;
}

TF1* GetBGBWdNdpt(Double_t mass, Double_t beta, Double_t temp,
                  Double_t n, Double_t norm, const char* name)
{

  // BGBW 1/pt dNdpt

  fLastFunc = new TF1(name, StaticBGdNdPt, 0.0, maximum, 5);
  fLastFunc->SetParameters(mass, beta, temp, n, norm);
  fLastFunc->FixParameter(0, mass);
  fLastFunc->SetParNames("mass", "#beta", "T", "n", "norm");
  fLastFunc->SetLineWidth(fLineWidth);
  return fLastFunc;
}

int main(int argc, char** argv)
{

  int nevents, pdg, seed;
  std::string config, output, background_config;
  double y_min, phi_min, pt_min;
  double y_max, phi_max, pt_max;
  int npart;
  double xProd, yProd, zProd;
  bool verbose, decay;
  //BGBW parameters
  double beta, temp, n;

  /** process arguments **/
  namespace po = boost::program_options;
  po::options_description desc("Options");
  try {
    desc.add_options()
      ("help", "Print help messages")
      ("nevents,n", po::value<int>(&nevents)->default_value(10), "Number of events to be generated")
      ("pdg,p", po::value<int>(&pdg)->required(), "PDG code of the particle")
      ("npart", po::value<int>(&npart)->default_value(1), "Number of particles per event in a box")
      ("ymin", po::value<double>(&y_min)->default_value(0.), "Minimum y")
      ("ymax", po::value<double>(&y_max)->default_value(0.), "Maximum y")
      ("phimin", po::value<double>(&phi_min)->default_value(0.), "Minimum phi")
      ("phimax", po::value<double>(&phi_max)->default_value(0.), "Maximum phi")
      ("ptmin", po::value<double>(&pt_min)->default_value(0.), "Minimum momentum")
      ("ptmax", po::value<double>(&pt_max)->default_value(20.), "Maximum momentum")
      ("xProd", po::value<double>(&xProd)->default_value(0.), "Production vertex in the x-direction")
      ("yProd", po::value<double>(&yProd)->default_value(0.), "Production vertex in the y-direction")
      ("zProd", po::value<double>(&zProd)->default_value(0.), "Production vertex in the z-direction")
      ("beta", po::value<double>(&beta)->default_value(0.57), "BGBW beta parameter")
      ("temp", po::value<double>(&temp)->default_value(0.100), "BGBW temperature parameter")
      ("n", po::value<double>(&n)->default_value(1.02), "BGBW n parameter")
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
      !pythia.particleData.isResonance(pdg)) {
    if (abs(pdg) < 1000000000) {
      std::cout << "Error: invalid PDG code \"" << pdg << "\"" << std::endl;
      return 1;
    } else {
      std::cout << "PDG code \"" << pdg << "\" stands for a nucleous" << std::endl;
    }
  }

  gRandom->SetSeed(seed);
  std::cout << "Random:seed =" + std::to_string(seed) << std::endl;
  pythia.readString("Random:setSeed = on");
  pythia.readString("Random:seed =" + std::to_string(seed));
  // init
  pythia.init();
  const double m = pythia.particleData.m0(pdg);

  // TF1* GetBGBWdNdptTimesPt(Double_t mass, Double_t beta, Double_t temp, Double_t n,
  //                          Double_t norm, const char* name)
  TF1* bgbw = GetBGBWdNdptTimesPt(m, beta * 1.5, temp, n, 1, Form("bgbw_for_pdg_%i", pdg));

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
  double y, phi, p, pt, pl;
  srand(time(NULL));
  for (int iev = 0; iev < nevents; ++iev) {

    // reset, add particle and decay
    pythia.event.reset();
    for (int ipart = 0; ipart < npart; ipart++) {
      pt = bgbw->GetRandom(pt_min, pt_max, gRandom);
      y = y_min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (y_max - y_min)));
      constexpr double eNep = TMath::E();
      pl = 0.5 * pow(eNep, (-y)) * (pow(eNep, y * 2) - 1) * sqrt(m * m + pt * pt);
      p = sqrt(pt * pt + pl * pl);
      phi = phi_min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (phi_max - phi_min)));
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
