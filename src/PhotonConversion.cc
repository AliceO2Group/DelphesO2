/// @author: Ana Marin
/// @author: Nicolo' Jacazio <nicolo.jacazio@cern.ch>
/// @since 20/09/2021

#include <stdlib.h>
#include "PhotonConversion.hh"
#include "TDatabasePDG.h"
#include "TRandom.h"
#include "TLorentzVector.h"
#include "TDatime.h"
#include <iostream>
#include <fstream>

namespace o2
{
namespace delphes
{

/*****************************************************************/

void PhotonConversion::setup()
{

  TDatime t;
  gRandom->SetSeed(t.GetDate() + t.GetYear() * t.GetHour() * t.GetMinute() * t.GetSecond()); // NB: gRandom is a global pointer ?
}

/*****************************************************************/

bool PhotonConversion::hasPhotonConversion(const GenParticle& particle) const
{

  const int pid = particle.PID;
  TLorentzVector p4True = particle.P4();
  float convProb,eff;
  
  if (pid == 22) {
    if (TMath::Abs(particle.Eta) < 1.3) {
      convProb = 3.54334e-02 * TMath::Power(p4True.Pt(), 1.47512) / (1.56461e-02 + TMath::Power(p4True.Pt(), 1.43599));
      if (convProb > 0.04)
	convProb = 0.04;
      eff = 5.89182e-01 * TMath::Power(p4True.Pt(), 3.85834) / (2.96558e-03 + TMath::Power(p4True.Pt(), 3.72573));
      if (eff > 1.)
	eff = 1.;

    } else  if (TMath::Abs(particle.Eta) > 1.75 && TMath::Abs(particle.Eta) < 4.) {
      convProb = -8.24825e-03  *( TMath::Power(p4True.P(), -5.03182e-01 )-1.13113e+01*p4True.P()) / (2.23495e-01 + TMath::Power(p4True.P(), 1.08338e+00 ));
      eff = 5.89182e-01 * TMath::Power(p4True.P(), 3.85834) / (2.96558e-03 + TMath::Power(p4True.P(), 3.72573));
      if (eff > 1.)
	eff = 1.;
    }else{
      convProb = 0.;
      eff=0.;
    }
    return (gRandom->Uniform() < (convProb * eff));
  } else {
    const Float_t misConvProb = 0.0;
    return (gRandom->Uniform() < misConvProb);
  }
  return true;
}

/*****************************************************************/

bool PhotonConversion::makeSignal(const GenParticle& particle, TLorentzVector& photonConv)
{
  const int pid = particle.PID;
  if (pid != 22) {
    return false;
  }
  // Eta coverage of the central barrel. Region where the conv prob., rec effciency and momentum resolution have been parametrized.
  if (( TMath::Abs(particle.Eta) > 1.3  && TMath::Abs(particle.Eta) < 1.75) || TMath::Abs(particle.Eta) > 4 ) {
    return false;
  }
  TLorentzVector p4Smeared = smearPhotonP(particle);
  photonConv = p4Smeared;
  return true;
}

/*****************************************************************/

TLorentzVector PhotonConversion::smearPhotonP(const GenParticle& particle)
{
  // This function smears the photon 4-momentum from the true one via applying
  // parametrized pt and pz resolution

  TLorentzVector p4True = particle.P4();
  // std::cout<< "Particle px,py,pz,pt,p,m,Eta,phi::   "<< particle.Px << "  " << particle.Py << "  " << particle.Pz    << "  "<<
  //   particle.P << "  "<< particle.M1 << "  "<< particle.Eta << "  " << particle.Phi << "  "   << std::endl;

  // Get true energy from true 4-momentum and smear this energy
  double pTrue = p4True.P();
  double phi = p4True.Phi();
  double theta = p4True.Theta();

  double sigmaP;
  if( TMath::Abs(particle.Eta) < 1.3) {
    sigmaP = pTrue * TMath::Sqrt(sigmaPt0 * sigmaPt0 + (sigmaPt1 * pTrue) * (sigmaPt1 * pTrue));
  } else if ( TMath::Abs(particle.Eta) > 1.75   &&  TMath::Abs(particle.Eta) < 4 ) {
    sigmaP = pTrue * TMath::Sqrt(sigmaPF0 * sigmaPF0 );
  }
    
  double pSmearedMag = gRandom->Gaus(pTrue, sigmaP);
  if (pSmearedMag < 0)
    pSmearedMag = 0;

  // Calculate smeared components of 3-vector
  Double_t pxSmeared = pSmearedMag * TMath::Cos(phi) * TMath::Sin(theta);
  Double_t pySmeared = pSmearedMag * TMath::Sin(phi) * TMath::Sin(theta);
  Double_t pzSmeared = pSmearedMag * TMath::Cos(theta);

  // Construct new 4-momentum from smeared energy and 3-momentum
  TLorentzVector pSmeared;
  pSmeared.SetXYZM(pxSmeared, pySmeared, pzSmeared, 0.);
  return pSmeared;
}
/*****************************************************************/

} // namespace delphes
} // namespace o2
