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
RICHdetector::hasRICH(const Track &track) const
{
  auto x = track.XOuter * 0.1; // [cm]
  auto y = track.YOuter * 0.1; // [cm]
  auto z = track.ZOuter * 0.1; // [cm]
  /** check if hit **/
  bool ishit = false;
  if (mType == kBarrel) {
    auto r = hypot(x, y);
    ishit = (fabs(r - mRadius) < 0.001 && fabs(z) < mLength);
  }
  if (mType == kForward) {
    auto r = hypot(x, y);
    ishit = (r > mRadiusIn) && (r < mRadius) && (fabs(fabs(z) - mLength) < 0.001);
  }
  if (!ishit) return false;
  /** check if above threshold **/
  auto particle = (GenParticle *)track.Particle.GetObject();
  int pid = particle->PID;
  double mass = TDatabasePDG::Instance()->GetParticle(pid)->Mass();
  auto thr = cherenkovThreshold(mass);
  if (particle->P < thr) return false;
  return true;
}

/*****************************************************************/

std::pair<float, float>
RICHdetector::getMeasuredAngle(const Track &track) const
{
  if (!hasRICH(track)) return {0., 0.};
  auto particle = (GenParticle *)track.Particle.GetObject();
  int pid = particle->PID;
  double mass = TDatabasePDG::Instance()->GetParticle(pid)->Mass();
  auto angle = cherenkovAngle(particle->P, mass);
  auto nph_av = numberOfPhotons(angle); // average number of photons
  auto nph = gRandom->Poisson(nph_av); // random number of photons
  if (nph < mMinPhotons) return {0., 0.};
  auto nph_el = 0; // number of photo-electrons
  for (int i = 0; i < nph; ++i) {
    if (gRandom->Uniform() < mEfficiency) nph_el++;
  }
  if (nph_el < mMinPhotons) return {0., 0.};
  auto sigma = mSigma / sqrt(nph_el);
  angle = gRandom->Gaus(angle, sigma);
  return {angle, sigma};
}

/*****************************************************************/

float
RICHdetector::getExpectedAngle(float p, float mass) const
{
  auto thr = cherenkovThreshold(mass);
  if (p < thr) return 0.;
  return cherenkovAngle(p, mass);
}

/*****************************************************************/

void
RICHdetector::makePID(const Track &track, std::array<float, 5> &deltaangle, std::array<float, 5> &nsigma) const
{
  double pmass[5] = {0.00051099891, 0.10565800, 0.13957000, 0.49367700, 0.93827200};
  
  /** get info **/
  auto measurement = getMeasuredAngle(track);
  auto angle = measurement.first;
  auto anglee = measurement.second;
  
  /** perform PID **/
  double p = track.P;
  double ep = p * track.ErrorP;
  double n = mIndex; 
  for (Int_t ipart = 0; ipart < 5; ++ipart) {
    auto m = pmass[ipart];
    auto exp_angle = getExpectedAngle(p, m);
    auto A = std::sqrt(n * n * p * p - m * m - p * p);
    auto B = std::sqrt(m * m + p * p);
    auto exp_sigma = m * m / p / A / B * ep;
    exp_sigma = sqrt(anglee * anglee + exp_sigma * exp_sigma);
    if (anglee <= 0. || exp_angle <= 0.) {
      deltaangle[ipart] = -1000.;
      nsigma[ipart] = 1000.;
      continue;
    }
    deltaangle[ipart] = angle - exp_angle;
    nsigma[ipart] = deltaangle[ipart] / exp_sigma; // should also consider the momentum resolution
  }

}

/*****************************************************************/

  
} /** namespace delphes **/
} /** namespace o2 **/
