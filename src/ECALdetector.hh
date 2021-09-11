/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#ifndef _DelphesO2_ECALdetector_h_
#define _DelphesO2_ECALdetector_h_

#include "classes/DelphesClasses.h"

namespace o2
{
  namespace delphes
  {

    class ECALdetector
    {

    public:
      ECALdetector() = default;
      ~ECALdetector() = default;

      void setup(float resoEA, float resoEB, float resoEC, float resoPosA, float resoPosB);
      bool hasECAL(const Track& track) const;
      bool makeSignal(const GenParticle& particle, TLorentzVector &pECAL, float & posZ, float & posPhi);

    protected:
      Double_t       smearPhotonE (const Double_t eTrue);
      Double_t       sigmaX       (const Double_t eTrue);
      TLorentzVector smearPhotonP4(const TLorentzVector pTrue);

      float mRadius = 120.; // ECAL barrel inner radius [cm]
      float mLength = 200.; // ECAL half-length along beam axis [cm]

      float mEnergyResolutionA   = 0.002; // parameter A of energy resolution in GeV
      float mEnergyResolutionB   = 0.02;  // parameter B of energy resolution in GeV^{1/2}
      float mEnergyResolutionC   = 0.10;  // parameter C of energy resolution
      float mPositionResolutionA = 0.15;  // parameter A of coordinate resolution in cm
      float mPositionResolutionB = 0.30;  // parameter B of coordinate resolution in cm*GeV^{1/2}
    };

  } // namespace delphes
} // namespace o2

#endif /** _DelphesO2_ECALdetector_h_ **/
