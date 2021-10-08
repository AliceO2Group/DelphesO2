/// @author: Yuri Kharlov
/// @author: Nicolo' Jacazio <nicolo.jacazio@cern.ch>
/// @since 04/09/2021

#include "ECALdetector.hh"
#include "TDatabasePDG.h"
#include "TRandom.h"
#include "TLorentzVector.h"

namespace o2
{
namespace delphes
{

/*****************************************************************/

void ECALdetector::setup(float resoEA, float resoEB, float resoEC, float resoXA, float resoXB)
{
  mEnergyResolutionA = resoEA;
  mEnergyResolutionB = resoEB;
  mEnergyResolutionC = resoEC;
  mPositionResolutionA = resoXA;
  mPositionResolutionB = resoXB;
}

/*****************************************************************/

bool ECALdetector::hasECAL(const Track& track) const
{
  auto x = track.XOuter * 0.1; // [cm]
  auto y = track.YOuter * 0.1; // [cm]
  auto z = track.ZOuter * 0.1; // [cm]
  /** check if hit **/
  bool ishit = false;
  auto r = hypot(x, y);
  ishit = (fabs(r - mRadius) < 0.001 && fabs(z) < mLength);
  if (!ishit)
    return false;
  auto particle = (GenParticle*)track.Particle.GetObject();
  return true;
}

/*****************************************************************/
bool ECALdetector::makeSignal(const GenParticle& particle,
                              TLorentzVector& p4ECAL,
                              float& posZ,
                              float& posPhi)
{
  // Simulate fast response of ECAL to photons:
  // take generated particle as input and calculate its smeared 4-momentum p4ECAL
  // and hit coordinates posZ, posPhi

  const int pid = particle.PID;
  if (pid != 22) { // only photons are treated so far. e+- and MIPs will be added later.
    return false;
  }

  TLorentzVector p4True = particle.P4(); // true 4-momentum
  if (TMath::Abs(p4True.Eta()) > 4) {    // ECAL acceptance is rougly limited by |eta|<4
    return false;
  }

  Float_t vtX = particle.X; // particle vertex X
  Float_t vtY = particle.Y; // particle vertex Y
  Float_t vtZ = particle.Z; // particle vertex Z

  posPhi = p4True.Phi(); // azimuth angle of a photon hit
  posZ = -1e6;
  Double_t tanTheta = TMath::Tan(p4True.Theta());
  if (tanTheta != 0.) {
    posZ = mRadius / tanTheta; // z-coodrinate  of a photon hit
  }

  p4ECAL = smearPhotonP4(p4True);

  return true;
}

/*****************************************************************/
TLorentzVector ECALdetector::smearPhotonP4(const TLorentzVector& pTrue)
{
  // This function smears the photon 4-momentum from the true one via applying
  // parametrized energy and coordinate resolution

  // Get true energy from true 4-momentum and smear this energy
  Double_t eTrue = pTrue.E();
  Double_t eSmeared = smearPhotonE(eTrue);
  // Smear direction of 3-vector
  Double_t phi = pTrue.Phi() + gRandom->Gaus(0., sigmaX(eTrue) / mRadius);
  Double_t theta = pTrue.Theta() + gRandom->Gaus(0., sigmaX(eTrue) / mRadius);
  // Calculate smeared components of 3-vector
  Double_t pxSmeared = eSmeared * TMath::Cos(phi) * TMath::Sin(theta);
  Double_t pySmeared = eSmeared * TMath::Sin(phi) * TMath::Sin(theta);
  Double_t pzSmeared = eSmeared * TMath::Cos(theta);
  // Construct new 4-momentum from smeared energy and 3-momentum
  TLorentzVector pSmeared(pxSmeared, pySmeared, pzSmeared, eSmeared);
  return pSmeared;
}
/*****************************************************************/
Double_t ECALdetector::sigmaX(const Double_t& eTrue)
{
  // Calculate sigma of photon coordinate smearing [cm]
  // E is the photon energy
  Double_t dX = sqrt(mPositionResolutionA * mPositionResolutionA + mPositionResolutionB * mPositionResolutionB / eTrue);
  return dX;
}
/*****************************************************************/
Double_t ECALdetector::smearPhotonE(const Double_t& eTrue)
{
  // Smear a photon energy eTrue according to a Gaussian distribution with energy resolution parameters
  // sigma of Gaussian smearing is calculated from parameters A,B,C and true energy

  const Double_t sigmaE = eTrue * sqrt(mEnergyResolutionA * mEnergyResolutionA / eTrue / eTrue +
                                       mEnergyResolutionB * mEnergyResolutionB / eTrue +
                                       mEnergyResolutionC * mEnergyResolutionC);
  Double_t eSmeared = gRandom->Gaus(eTrue, sigmaE);
  if (eSmeared < 0)
    eSmeared = 0;
  return eSmeared;
}
/*****************************************************************/

} // namespace delphes
} // namespace o2
