/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#ifndef _DelphesO2_RICHdetector_h_
#define _DelphesO2_RICHdetector_h_

#include "classes/DelphesClasses.h"

namespace o2
{
namespace delphes
{

class RICHdetector {
  
public:
  RICHdetector() = default;
  ~RICHdetector() = default;
  
  void setup(float radius, float length);  
  bool hasRICH(const Track &track) const;

  void setIndex(float val) { mIndex = val; };
  void setRadiatorLength(float val) { mRadiatorLength = val; };
  void setEfficiency(float val) { mEfficiency = val; };
  void setSigma(float val) { mSigma = val; };

  void makePID(const Track &track, std::array<float, 5> &deltaangle, std::array<float, 5> &nsigma) const;
  std::pair<float, float> getMeasuredAngle(const Track &track) const;
  float getExpectedAngle(float p, float mass) const;
  
  double cherenkovAngle(double p, double m) const {
    return acos( sqrt( m * m + p * p ) / ( mIndex * p ) ); };
  double cherenkovThreshold(double m) const {
    return m / sqrt(mIndex * mIndex - 1.); };
  double numberOfPhotons(double angle) const {
    return 490. * sin(angle) * sin(angle) * mRadiatorLength; };
  double numberOfDetectedPhotons(double angle) const {
    return numberOfPhotons(angle) * mEfficiency; };
  double cherenkovAngleSigma(double p, double m) const {
    return mSigma / sqrt(numberOfDetectedPhotons(cherenkovAngle(p, m))); }
  
protected:
  
  float mRadius = 100.; // [cm]
  float mLength = 200.; // [cm]

  float mIndex = 1.03;
  float mRadiatorLength = 2.; // [cm]
  float mEfficiency = 0.4;
  float mSigma = 7.e-3; // [rad]
  float mMinPhotons = 3.;
  
};
  
} /** namespace delphes **/
} /** namespace o2 **/

#endif /** _DelphesO2_RICHdetector_h_ **/

