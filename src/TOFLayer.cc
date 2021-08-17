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
TOFLayer::setup(float radius, float length, float sigmat, float sigma0)
{
  mRadius = radius;
  mLength = length;
  mSigmaT = sigmat;
  mSigma0 = sigma0;
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
    return (fabs(r - mRadius) < 0.001 && fabs(z) < mLength);
  }
  if (mType == kForward) {
    auto r = hypot(x, y);
    return (r > mRadiusIn) && (r < mRadius) && (fabs(fabs(z) - mLength) < 0.001);
  }
  return false;
}

/*****************************************************************/

float
TOFLayer::getBeta(const Track &track)
{
  double tof = track.TOuter * 1.e9; // [ns]
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
  double tof = track.TOuter * 1.e9; // [ns]
  double etof = track.ErrorT * 1.e9; // [ns]
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
    double sigma = TMath::Sqrt(etexp * etexp + etof * etof);
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
    double etof   = track->ErrorT * 1.e9; // [ns]
    double L      = track->L * 0.1;       // [cm]
    double p      = track->P;             // [GeV/c]
    p *= abs(TDatabasePDG::Instance()->GetParticle(pid)->Charge()) / 3.; // [GeV/c]
    double ep     = track->ErrorP;
    double p2     = p * p;
    double c      = 29.9792458;           // [cm/ns]
    double Lc     = L / c;
    double texp   = Lc / p * TMath::Sqrt(mass2 + p2);
    double etexp  = Lc * mass2 / p2 / TMath::Sqrt(mass2 + p2) * ep;
    double sigma  = TMath::Sqrt(etexp * etexp + etof * etof);
    double deltat = tof - texp;

    double w = 1. / (sigma * sigma);

    sum += w * deltat;
    sumw += w;
  }

  if (sumw <= 0.) {
    tzero[0] = 0.;
    tzero[1] = mSigma0;
    return false;
  }

  tzero[0] = sum / sumw;
  tzero[1] = sqrt(1. / sumw);

  // if we have many tracks, we use the start-time computed with all tracks

  if (tracks.size() > 4) {
    for (auto &track : tracks) {
      track->TOuter -= tzero[0] * 1.e-9; // [s]
      track->ErrorT = std::hypot(track->ErrorT, tzero[1] * 1.e-9);
    }
    return true;
  }

  // if we have few tracks, we do the combinations excluding the track of interest

  std::vector<double> time0, sigma0;
  time0.reserve(tracks.size());
  sigma0.reserve(tracks.size());
  for (int itrack = 0; itrack < tracks.size(); ++itrack) {

    time0[itrack] = 0;
    sigma0[itrack] = mSigma0;

    sum = 0.;
    sumw = 0.;

    for (int jtrack = 0; jtrack < tracks.size(); ++jtrack) {
      if (itrack == jtrack) continue; // do not use self

      auto &track   = tracks[jtrack];
      int pid       = track->PID;
      double mass   = TDatabasePDG::Instance()->GetParticle(pid)->Mass();
      double mass2  = mass * mass;
      double tof    = track->TOuter * 1.e9; // [ns]
      double etof   = track->ErrorT * 1.e9; // [ns]
      double L      = track->L * 0.1;       // [cm]
      double p      = track->P;             // [GeV/c]
      p *= abs(TDatabasePDG::Instance()->GetParticle(pid)->Charge()) / 3.; // [GeV/c]
      double ep     = track->ErrorP;
      double p2     = p * p;
      double c      = 29.9792458;           // [cm/ns]
      double Lc     = L / c;
      double texp   = Lc / p * TMath::Sqrt(mass2 + p2);
      double etexp  = Lc * mass2 / p2 / TMath::Sqrt(mass2 + p2) * ep;
      double sigma  = TMath::Sqrt(etexp * etexp + etof * etof);
      double deltat = tof - texp;
      double w = 1. / (sigma * sigma);
      sum += w * deltat;
      sumw += w;
    }

    if (sumw <= 0.) continue;

    time0[itrack] = sum / sumw;
    sigma0[itrack] = std::sqrt(1. / sumw);

  }

  for (int itrack = 0; itrack < tracks.size(); ++itrack) {
    auto &track   = tracks[itrack];
    track->TOuter -= time0[itrack] * 1.e-9; // [s]
    track->ErrorT = std::hypot(track->ErrorT, sigma0[itrack] * 1.e-9);
  }

  return true;
}

/*****************************************************************/


} /** namespace delphes **/
} /** namespace o2 **/
