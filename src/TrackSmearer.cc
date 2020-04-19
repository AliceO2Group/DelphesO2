/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#include "TrackSmearer.hh"
#include "TrackUtils.hh"
#include "TRandom.h"
#include <iostream>
#include <fstream>

namespace o2
{
namespace delphes
{

/*****************************************************************/

void
TrackSmearer::loadTable(const char *filename)
{
  std::ifstream lutFile(filename, std::ifstream::binary);
  lutFile.read(reinterpret_cast<char *>(&mLUTHeader), sizeof(lutHeader_t));
  const int nnch = mLUTHeader.nchmap.nbins;
  const int nrad = mLUTHeader.radmap.nbins;
  const int neta = mLUTHeader.etamap.nbins;
  const int npt = mLUTHeader.ptmap.nbins;
  for (int inch = 0; inch < nnch; ++inch) {
    for (int irad = 0; irad < nrad; ++irad) {
      for (int ieta = 0; ieta < neta; ++ieta) {
	for (int ipt = 0; ipt < npt; ++ipt) {
	  lutFile.read(reinterpret_cast<char *>(&mLUTEntry[inch][irad][ieta][ipt]), sizeof(lutEntry_t));
	}}}}
  std::cout << " --- read covariance matrix table: " << filename << std::endl;
  mLUTHeader.print();
  lutFile.close();
}

/*****************************************************************/

lutEntry_t &
TrackSmearer::getLUTEntry(float nch, float radius, float eta, float pt)
{
  auto inch = mLUTHeader.nchmap.find(nch);
  auto irad = mLUTHeader.radmap.find(radius);
  auto ieta = mLUTHeader.etamap.find(eta);
  auto ipt  = mLUTHeader.ptmap.find(pt);
  return mLUTEntry[inch][irad][ieta][ipt];
};

/*****************************************************************/

void
TrackSmearer::smearTrack(O2Track &o2track, lutEntry_t &lutEntry)
{
  
  // transform params vector and smear
  double params_[5];
  for (int i = 0; i < 5; ++i) {
    double val = 0.;
    for (int j = 0; j < 5; ++j)
      val += lutEntry.eigvec[j][i] * o2track.getParam(j);
    params_[i] = gRandom->Gaus(val, sqrt(lutEntry.eigval[i]));
  }
  
  // transform back params vector
  for (int i = 0; i < 5; ++i) {
    double val = 0.;
    for (int j = 0; j < 5; ++j)
      val += lutEntry.eiginv[j][i] * params_[j];
    o2track.setParam(val, i);
  }

  // should make a sanity check that par[2] sin(phi) is in [-1, 1]
  
}  

/*****************************************************************/

void
TrackSmearer::smearTrack(Track &track, bool atDCA)
{
  O2Track o2track;
  TrackUtils::convertTrackToO2Track(track, o2track, atDCA);
  smearTrack(o2track, getLUTEntry(0., 0., track.Eta, track.PT));
  TrackUtils::convertO2TrackToTrack(o2track, track, atDCA);
}
  
/*****************************************************************/

  
} /** namespace delphes **/
} /** namespace o2 **/
