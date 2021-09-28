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
  if (pid == 22) {
    float convProb = 3.55541e-02 * TMath::Power(p4True.Pt(), 1.50281) / (1.39201e-02 + TMath::Power(p4True.Pt(), 1.45348));
    float eff = 5.72297e-01 * TMath::Power(p4True.Pt(), 3.35915) / (6.86633e-02 + TMath::Power(p4True.Pt(), 3.08761));
    // convProb =1;
    // eff=1;
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
  // Get true energy from true 4-momentum and smear this energy
  double pTrue = p4True.P();
  double phi = p4True.Phi();
  double theta = p4True.Theta();

  Double_t sigmaP = pTrue * TMath::Sqrt(sigmaPt0 * sigmaPt0 + (sigmaPt1 * pTrue) * (sigmaPt1 * pTrue));

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
