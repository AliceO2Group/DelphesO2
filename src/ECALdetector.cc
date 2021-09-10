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

void ECALdetector::setup(float resoEA, float resoEB, float resoEC, float resoPos)
{
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
bool ECALdetector::makeSignal(const GenParticle& particle, std::array<float, 3>& pos, float& energy) const
{
  TLorentzVector vec;
  const int pid = particle.PID;
  if (pid != 22) {
    return false;
  }
  pos[0] = 0.f;
  pos[1] = 0.f;
  pos[2] = 0.f;
  energy = 0.f;
  return true;
}

/*****************************************************************/

} // namespace delphes
} // namespace o2
