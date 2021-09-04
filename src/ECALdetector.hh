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

  void setup(float resoEA, float resoEB, float resoEC, float resoPos);
  bool hasECAL(const Track& track) const;
  bool makeSignal(const GenParticle& particle, std::array<float, 3>& pos, float& energy) const;

 protected:
  float mRadius = 120.; // [cm]
  float mLength = 200.; // [cm]

  float mEnergyResolutionA = 0;  // []
  float mEnergyResolutionB = 0;  // []
  float mEnergyResolutionC = 0;  // []
  float mPositionResolution = 0; // []
};

} // namespace delphes
} // namespace o2

#endif /** _DelphesO2_ECALdetector_h_ **/
