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
      gRandom->SetSeed(t.GetDate()+t.GetYear()*t.GetHour()*t.GetMinute()*t.GetSecond()); // NB: gRandom is a global pointer ?


    }

    /*****************************************************************/

    bool PhotonConversion::hasPhotonConversion(const GenParticle& particle) const
    {
 

      const int pid = particle.PID;
      TLorentzVector p4True = particle.P4();
      if (pid == 22) {
	float convProb =  3.55541e-02* TMath::Power(p4True.Pt(),1.50281)/(1.39201e-02+TMath::Power(p4True.Pt(),1.45348));
	float eff =  5.72297e-01* TMath::Power(p4True.Pt(),3.35915)/(6.86633e-02 +TMath::Power(p4True.Pt(),3.08761));
	return (gRandom->Uniform() < (convProb*eff) );
      }
      else {
         const Float_t misConvProb = 0.0;
         return (gRandom->Uniform() < misConvProb);
      }
     // AM plug-in the conversion probability * photon reconstruction efficiency

      // auto x = track.XOuter * 0.1; // [cm]
      // auto y = track.YOuter * 0.1; // [cm]
      // auto z = track.ZOuter * 0.1; // [cm]
      // /** check if hit **/
      // bool ishit = false;
      // auto r = hypot(x, y);
      // ishit = (fabs(r - mRadius) < 0.001 && fabs(z) < mLength);
      // if (!ishit)
      // 	return false;
      // auto particle = (GenParticle*)track.Particle.GetObject();
      return true;
    }

    /*****************************************************************/
    // bool PhotonConversion::makeSignal(const GenParticle& particle, std::array<float, 3>& pos, float& energy) const
    bool PhotonConversion::makeSignal(const GenParticle& particle, TLorentzVector &photonConv)
    {
      const int pid = particle.PID;
      if (pid != 22) {
	return false;
      }

      // TLorentzVector p4True = particle.P4(); // true 4-momentum
      // Float_t vtX = particle.X; // particle vertex X
      // Float_t vtY = particle.Y; // particle vertex Y
      // Float_t vtZ = particle.Z; // particle vertex Z

      // posPhi = particle.Phi;                // azimuth angle of a photon hit
      // posZ   = mRadius * particle.CtgTheta; // z-coodrinate  of a photon hit

      //      Double_t ptSmeared = smearPhotonPt(particle); // smeared photon energy
      //      Double_t pzSmeared = smearPhotonPz(particle); // smeared photon energy
      // Double_t pSmeared = smearPhotonP(particle.E); // smeared photon energy
      TLorentzVector p4Smeared = smearPhotonP(particle);
      photonConv = p4Smeared;
      return true;
    }

    /*****************************************************************/
    //    TLorentzVector PhotonConversion::smearPhotonP4(const TLorentzVector pTrue)
    TLorentzVector PhotonConversion::smearPhotonP(const GenParticle&particle)
    {
      // This function smears the photon 4-momentum from the true one via applying
      // parametrized pt and pz resolution

      TLorentzVector p4True = particle.P4();      
      // Get true energy from true 4-momentum and smear this energy
      double pTrue    = p4True.P();
      double phi      = p4True.Phi();
      double theta    = p4True.Theta();
      //Fit of the Sigma (P,  could do  Pt & Pz)
      //0.0295012  0.00235785

      // double sigma0P = 0.0295012;
      // double sigma1P = 0.00235785;
      std::cout << sigmaPt0 << "  "<< sigmaPt1 << std::endl;
      Double_t sigmaP = pTrue *  TMath::Sqrt(sigmaPt0*sigmaPt0+(sigmaPt1*pTrue)*(sigmaPt1*pTrue));

      double pSmearedMag = gRandom->Gaus(pTrue,sigmaP);
      if (pSmearedMag < 0) pSmearedMag = 0;

 
      // Calculate smeared components of 3-vector
      Double_t pxSmeared = pSmearedMag*TMath::Cos(phi)*TMath::Sin(theta);
      Double_t pySmeared = pSmearedMag*TMath::Sin(phi)*TMath::Sin(theta);
      Double_t pzSmeared = pSmearedMag*TMath::Cos(theta);
      // Construct new 4-momentum from smeared energy and 3-momentum
      TLorentzVector pSmeared;
      pSmeared.SetXYZM(pxSmeared,pySmeared,pzSmeared,0.);
      return pSmeared;
    }
    /*****************************************************************/
    // Double_t PhotonConversion::sigmaX(const Double_t eTrue)
    // {
    //   // Calculate sigma of photon coordinate smearing [cm]
    //   // E is the photon energy
    //   Double_t dX = sqrt(mPositionResolutionA*mPositionResolutionA + mPositionResolutionB*mPositionResolutionB/eTrue);
    //   return dX;
    // }
    /*****************************************************************/
    // Double_t PhotonConversion::smearPhotonP(const Double_t eTrue)
    // {
    //   // Smear a photon energy eTrue according to a Gaussian distribution with energy resolution parameters
    //   // sigma of Gaussian smearing is calculated from parameters A,B,C and true energy

    //   Double_t sigmaE = eTrue * sqrt(mEnergyResolutionA*mEnergyResolutionA/eTrue/eTrue +
    // 				     mEnergyResolutionB*mEnergyResolutionB/eTrue +
    // 				     mEnergyResolutionC*mEnergyResolutionC);
    //   Double_t eSmeared = gRandom->Gaus(eTrue,sigmaE);
    //   if (eSmeared < 0) eSmeared = 0;
    //   return eSmeared;
    // }
    /*****************************************************************/

  } // namespace delphes
} // namespace o2
