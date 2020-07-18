/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#ifndef _DelphesO2_TOFLayer_h_
#define _DelphesO2_TOFLayer_h_

#include "classes/DelphesClasses.h"

namespace o2
{
namespace delphes
{

class TOFLayer {
  
public:
  TOFLayer() = default;
  ~TOFLayer() = default;
  
  void setup(float radius, float length, float sigmat);  
  bool hasTOF(const Track &track);
  float getBeta(const Track &track);
  void makePID(const Track &track, std::array<float, 5> &deltat, std::array<float, 5> &nsigma);
  
protected:

  float mRadius = 100.; // [cm]
  float mLength = 200.; // [cm]
  float mSigmaT = 0.02; // [ns]
  
};
  
} /** namespace delphes **/
} /** namespace o2 **/

#endif /** _DelphesO2_TOFLayer_h_ **/

