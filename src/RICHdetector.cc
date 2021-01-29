/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#include "RICHdetector.hh"
#include "TDatabasePDG.h"
#include "TRandom.h"

namespace o2
{
namespace delphes
{

/*****************************************************************/

void
RICHdetector::setup(float radius, float length)
{
  mRadius = radius;
  mLength = length;
}

/*****************************************************************/

bool
RICHdetector::hasRICH(const Track &track)
{
  auto x = track.XOuter * 0.1; // [cm]
  auto y = track.YOuter * 0.1; // [cm]
  auto z = track.ZOuter * 0.1; // [cm]
  /** check if hit **/
  bool ishit = (fabs(hypot(x, y) - mRadius) < 0.001 && fabs(z) < mLength);
  if (!ishit) return false;
  /** check if above threshold **/
  int pid = track.PID;
  double mass = TDatabasePDG::Instance()->GetParticle(pid)->Mass();
  auto thr = cherenkovThreshold(mass);
  if (track.P < thr) return false; // fixme: P should be the true P
  return true;
}

/*****************************************************************/

std::pair<float, float>
RICHdetector::getMeasuredAngle(const Track &track)
{
  if (!hasRICH(track)) return {0., 0.};
  int pid = track.PID;
  double mass = TDatabasePDG::Instance()->GetParticle(pid)->Mass();
  auto angle = cherenkovAngle(track.P, mass); // fixme: P should be the true P
  auto nph_av = numberOfPhotons(angle); // average number of photons
  auto nph = gRandom->Poisson(nph_av); // random number of photons
  if (nph <= 0) return {0., 0.};
  auto nph_el = 0; // number of photo-electrons
  for (int i = 0; i < nph; ++i) {
    if (gRandom->Uniform() < mEfficiency) nph_el++;
  }
  if (nph_el <= 0) return {0., 0.};
  auto sigma = mSigma / sqrt(nph_el);
  angle = gRandom->Gaus(angle, sigma);
  return {angle, sigma};
}

/*****************************************************************/

  
} /** namespace delphes **/
} /** namespace o2 **/
