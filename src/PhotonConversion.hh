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

  float sigmaPt0 = 0.0314;  // parameter  sigma0 for momentum resolution
  float sigmaPt1 = 0.00406; // parameter sigma1 for momentum resolution

  float sigmaPF0 = 0.04082;  // parameter  sigma0 for momentum resolution ~30% worst than eta~0


};

} // namespace delphes
} // namespace o2

#endif /** _DelphesO2_PhotonConversion_h_ **/
