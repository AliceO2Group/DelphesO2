/// @Author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#ifndef _DelphesO2_PhotonConversion_h_
#define _DelphesO2_PhotonConversion_h_

#include "classes/DelphesClasses.h"

namespace o2
{
namespace delphes
{

class PhotonConversion
{

 public:
  PhotonConversion() = default;
  ~PhotonConversion() = default;

  void setup();
  bool hasPhotonConversion(const GenParticle& particle) const;
  bool makeSignal(const GenParticle& particle, TLorentzVector& pConv);

 protected:
  TLorentzVector smearPhotonP(const GenParticle& particle);

  float sigmaPt0 = 0.0284;  // parameter  sigma0 for momentum resolution
  float sigmaPt1 = 0.00577; // parameter sigma1 for momentum resolution
};

} // namespace delphes
} // namespace o2

#endif /** _DelphesO2_PhotonConversion_h_ **/
