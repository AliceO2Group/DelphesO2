/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#include "TOFLayer.hh"
#include "TDatabasePDG.h"

namespace o2
{
namespace delphes
{

/*****************************************************************/

void
TOFLayer::setup(float radius, float length, float sigmat)
{
  mRadius = radius;
  mLength = length;
  mSigmaT = sigmat;
}

/*****************************************************************/

bool
TOFLayer::hasTOF(const Track &track)
{
  auto x = track.XOuter * 0.1; // [cm]
  auto y = track.YOuter * 0.1; // [cm]
  auto z = track.ZOuter * 0.1; // [cm]  
  if (mType == kBarrel) {
    auto r = hypot(x, y);
    return fabs(r - mRadius) < 0.001 && fabs(z) < mLength);
  }
  if (mType = kForward) {
    auto r = hypot(x, y)
      return (r > mRadiusIn) && (r < mRadius) && (fabs(fabs(z) - mLength) < 0.001);
  }
  return false;
}

/*****************************************************************/

float
TOFLayer::getBeta(const Track &track)
{
  double tof = track.TOuter * 1.e9 - mTime0; // [ns]
  double L = track.L * 0.1; // [cm]
  double c = 29.9792458; // [cm/ns]
  return (L / tof / c);
}

/*****************************************************************/

void
TOFLayer::makePID(const Track &track, std::array<float, 5> &deltat, std::array<float, 5> &nsigma)
{
  double pmass[5] = {0.00051099891, 0.10565800, 0.13957000, 0.49367700, 0.93827200};
  
  /** get info **/
  double tof = track.TOuter * 1.e9 - mTime0; // [ns]
  double L = track.L * 0.1; // [cm]
  double p = track.P;
  double p2 = p * p;
  double ep = p * track.ErrorP;
  double c = 29.9792458; // [cm/ns]
  double Lc = L / c;

  /** perform PID **/
  for (Int_t ipart = 0; ipart < 5; ++ipart) {
    double mass2 = pmass[ipart] * pmass[ipart];
    double texp = Lc / p * TMath::Sqrt(mass2 + p2);
    double etexp = Lc * mass2 / p2 / TMath::Sqrt(mass2 + p2) * ep;    
    double sigma = TMath::Sqrt(etexp * etexp + mSigmaT * mSigmaT + mSigma0 * mSigma0);
    deltat[ipart] = tof - texp;
    nsigma[ipart] = deltat[ipart] / sigma;
  }

}

/*****************************************************************/

bool
TOFLayer::eventTime(std::vector<Track *> &tracks, std::array<float, 2> &tzero)
{

  double sum  = 0.;
  double sumw = 0.;
  
  for (auto &track : tracks) {

    int pid       = track->PID;
    double mass   = TDatabasePDG::Instance()->GetParticle(pid)->Mass();
    double mass2  = mass * mass;   
    double tof    = track->TOuter * 1.e9; // [ns]
    double L      = track->L * 0.1;       // [cm]
    double p      = track->P;             // [GeV/c]
    double ep     = track->ErrorP;
    double p2     = p * p;
    double c      = 29.9792458;           // [cm/ns]
    double Lc     = L / c;
    double texp   = Lc / p * TMath::Sqrt(mass2 + p2);
    double etexp  = Lc * mass2 / p2 / TMath::Sqrt(mass2 + p2) * ep;    
    double sigma  = TMath::Sqrt(etexp * etexp + mSigmaT * mSigmaT);
    double deltat = tof - texp;

    double w = 1. / (sigma * sigma);

    sum += w * deltat;
    sumw += w;
  }

  if (sumw <= 0.) {
    mTime0 = 0.;
    mSigma0 = 1000.;
    return false;
  }

  mTime0 = tzero[0] = sum / sumw;
  mSigma0 = tzero[1] = sqrt(1. / sumw);

  return true;
}

/*****************************************************************/

  
} /** namespace delphes **/
} /** namespace o2 **/
