/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#include "TrackUtils.hh"

namespace o2
{
namespace delphes
{

/*****************************************************************/

void
TrackUtils::convertO2TrackToTrack(const O2Track& o2track, Track& track, bool atDCA)
{
  auto xyz = o2track.getXYZGlo();
  if (atDCA) {
    track.Xd = 10. * xyz.X();
    track.Yd = 10. * xyz.Y();
    track.Zd = 10. * xyz.Z(); 
  } else {
    track.X = 10. * xyz.X();
    track.Y = 10. * xyz.Y();
    track.Z = 10. * xyz.Z(); 
  }
  track.PT = o2track.getPt();
  track.Eta = o2track.getEta();
  track.Phi = o2track.getPhi();
  track.Charge = TMath::Nint( o2track.getSign() );
}

/*****************************************************************/

void
TrackUtils::convertTrackToO2Track(const Track& track, O2Track& o2track, bool atDCA)
{
  const float errYZ = 1e-2, errSlp = 1e-3, errQPT = 2e-2;
  std::array<float, 15> covm = {
    errYZ * errYZ,
    0., errYZ * errYZ,
    0, 0., errSlp * errSlp,
    0., 0., 0., errSlp * errSlp,
    0., 0., 0., 0., errQPT * errQPT};
  //
  float s, c, x;
  std::array<float, 5> params;
  o2::utils::sincosf(track.Phi, s, c);
  o2::utils::rotateZInv(0.1 * (atDCA ? track.Xd : track.X), 0.1 * (atDCA ? track.Yd : track.Y), x, params[0], s, c);
  params[1] = 0.1 * (atDCA ? track.Zd : track.Z);
  params[2] = 0.; // since alpha = phi
  auto theta = 2.*TMath::ATan( TMath::Exp(-track.Eta) );  
  params[3] = 1. / TMath::Tan(theta);
  params[4] = track.Charge / track.PT;
  
  covm[14] = errQPT * errQPT * params[4] * params[4];

  new (&o2track) O2Track(x, track.Phi, params, covm);

}
  
/*****************************************************************/

  
} /** namespace delphes **/
} /** namespace o2 **/
