/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#include "TrackUtils.hh"

namespace o2
{
namespace delphes
{

/*****************************************************************/

void
TrackUtils::makeO2Track(O2Track& o2track, std::array<float, 3> xyz, std::array<float, 3> ptetaphi, int charge)
{
  std::array<float, 5> params;
  std::array<float, 15> covm = {
				0.,
				0., 0.,
				0., 0., 0.,
				0., 0., 0., 0.,
				0., 0., 0., 0., 0.};
  float s, c, x;
  o2::utils::sincosf(ptetaphi[2], s, c);
  o2::utils::rotateZInv(xyz[0], xyz[1], x, params[0], s, c);
  params[1] = xyz[2];
  params[2] = 0.; // since alpha = phi
  auto theta = 2. * atan(exp(-ptetaphi[1]));  
  params[3] = 1. / tan(theta);
  params[4] = charge / ptetaphi[0];
  
  new (&o2track) O2Track(x, ptetaphi[2], params, covm);

}

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
  track.D0 = 10. * o2track.getY();
  track.ErrorD0 = 10. * std::sqrt( o2track.getSigmaY2() );
  track.DZ = 10. * o2track.getZ();
  track.ErrorDZ = 10. * std::sqrt( o2track.getSigmaZ2() );
  track.Charge = TMath::Nint( o2track.getSign() );
}

/*****************************************************************/

void
TrackUtils::convertTrackToO2Track(const Track& track, O2Track& o2track, bool atDCA)
{

  std::array<float, 3> xyz = {0.1f * static_cast<float>(atDCA ? track.Xd : track.X),
			      0.1f * static_cast<float>(atDCA ? track.Yd : track.Y),
			      0.1f * static_cast<float>(atDCA ? track.Zd : track.Z)};
  
  std::array<float, 3> ptetaphi = {static_cast<float>(track.PT), static_cast<float>(track.Eta), static_cast<float>(track.Phi)};
  int charge = track.Charge;
  makeO2Track(o2track, xyz, ptetaphi, charge);
  
#if 0
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
#endif
  
}
  
/*****************************************************************/

void
TrackUtils::convertGenParticleToO2Track(const GenParticle& particle, O2Track& o2track)
{

  std::array<float, 3> xyz = {0.1f * static_cast<float>(particle.X), 0.1f * static_cast<float>(particle.Y), 0.1f * static_cast<float>(particle.Z)};
  std::array<float, 3> ptetaphi = {static_cast<float>(particle.PT), static_cast<float>(particle.Eta), static_cast<float>(particle.Phi)};
  int charge = particle.Charge;
  makeO2Track(o2track, xyz, ptetaphi, charge);
  
#if 0
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
#endif
  
}
  
/*****************************************************************/

bool
TrackUtils::propagateToDCA(O2Track &o2track, std::array<float, 3> xyz, float Bz)
{
  float sn, cs, alp = o2track.getAlpha();
  o2::utils::sincosf(alp, sn, cs);
  float x = o2track.getX(), y = o2track.getY(), snp = o2track.getSnp(), csp = std::sqrt((1.f - snp) * (1.f + snp));
  float xv = xyz[0] * cs + xyz[1] * sn, yv = -xyz[0] * sn + xyz[1] * cs, zv = xyz[2];
  x -= xv;
  y -= yv;
  float crv = o2track.getCurvature(Bz);
  float tgfv = -(crv * x - snp) / (crv * y + csp);
  sn = tgfv / std::sqrt(1.f + tgfv * tgfv);
  cs = std::sqrt((1. - sn) * (1. + sn));
  cs = (std::abs(tgfv) > o2::constants::math::Almost0) ? sn / tgfv : o2::constants::math::Almost1;
  
  x = xv * cs + yv * sn;
  yv = -xv * sn + yv * cs;
  xv = x;
  
  o2::track::TrackParCov tmpT(o2track); // operate on the copy to recover after the failure
  alp += std::asin(sn);
  if (!tmpT.rotate(alp) || !tmpT.propagateTo(xv, Bz * 10.)) {
    LOG(ERROR) << "failed to propagate to alpha=" << alp << " X=" << xv << " for vertex "
	       << xyz[0] << ' ' << xyz[1] << ' ' << xyz[2] << " | Track is: ";
    tmpT.print();
    return false;
  }
  o2track = tmpT;
  return true;
  
}

/*****************************************************************/

  
} /** namespace delphes **/
} /** namespace o2 **/
