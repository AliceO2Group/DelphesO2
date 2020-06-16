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

bool
TrackSmearer::loadTable(int pdg, const char *filename)
{
  auto ipdg = getIndexPDG(pdg);
  mLUTHeader[ipdg] = new lutHeader_t;
  
  std::ifstream lutFile(filename, std::ifstream::binary);
  if (!lutFile.is_open()) {
    std::cout << " --- cannot open covariance matrix file for PDG " << pdg << ": " << filename << std::endl;
    return false;
  }
  lutFile.read(reinterpret_cast<char *>(mLUTHeader[ipdg]), sizeof(lutHeader_t));
  if (lutFile.gcount() != sizeof(lutHeader_t)) {
    std::cout << " --- troubles reading covariance matrix header for PDG " << pdg << ": " << filename << std::endl;
    return false;
  }
  const int nnch = mLUTHeader[ipdg]->nchmap.nbins;
  const int nrad = mLUTHeader[ipdg]->radmap.nbins;
  const int neta = mLUTHeader[ipdg]->etamap.nbins;
  const int npt = mLUTHeader[ipdg]->ptmap.nbins;
  for (int inch = 0; inch < nnch; ++inch) {
    for (int irad = 0; irad < nrad; ++irad) {
      for (int ieta = 0; ieta < neta; ++ieta) {
	for (int ipt = 0; ipt < npt; ++ipt) {
	  mLUTEntry[ipdg][inch][irad][ieta][ipt] = new lutEntry_t;
	  lutFile.read(reinterpret_cast<char *>(mLUTEntry[ipdg][inch][irad][ieta][ipt]), sizeof(lutEntry_t));
	  if (lutFile.gcount() != sizeof(lutEntry_t)) {
	    std::cout << " --- troubles reading covariance matrix entry for PDG " << pdg << ": " << filename << std::endl;
	    return false;
	  }
	}}}}
  std::cout << " --- read covariance matrix table for PDG " << pdg << ": " << filename << std::endl;
  mLUTHeader[ipdg]->print();
  lutFile.close();
  return true;
}

/*****************************************************************/

lutEntry_t *
TrackSmearer::getLUTEntry(int pdg, float nch, float radius, float eta, float pt)
{
  auto ipdg = getIndexPDG(pdg);
  if (!mLUTHeader[ipdg]) return nullptr;
  auto inch = mLUTHeader[ipdg]->nchmap.find(nch);
  auto irad = mLUTHeader[ipdg]->radmap.find(radius);
  auto ieta = mLUTHeader[ipdg]->etamap.find(eta);
  auto ipt  = mLUTHeader[ipdg]->ptmap.find(pt);
  return mLUTEntry[ipdg][inch][irad][ieta][ipt];
};

/*****************************************************************/

void
TrackSmearer::smearTrack(O2Track &o2track, lutEntry_t *lutEntry)
{
  // transform params vector and smear
  double params_[5];
  for (int i = 0; i < 5; ++i) {
    double val = 0.;
    for (int j = 0; j < 5; ++j)
      val += lutEntry->eigvec[j][i] * o2track.getParam(j);
    params_[i] = gRandom->Gaus(val, sqrt(lutEntry->eigval[i]));
  }  
  // transform back params vector
  for (int i = 0; i < 5; ++i) {
    double val = 0.;
    for (int j = 0; j < 5; ++j)
      val += lutEntry->eiginv[j][i] * params_[j];
    o2track.setParam(val, i);
  }
  // should make a sanity check that par[2] sin(phi) is in [-1, 1]
  if (fabs(o2track.getParam(2)) > 1.) {
    std::cout << " --- smearTrack failed sin(phi) sanity check: " << o2track.getParam(2) << std::endl;
  }
  // set covariance matrix
  for (int i = 0; i < 15; ++i) 
    o2track.setCov(lutEntry->covm[i], i);
}  

/*****************************************************************/

bool
TrackSmearer::smearTrack(O2Track &o2track, int pid)
{

  auto pt = o2track.getPt();
  auto eta = o2track.getEta();
  auto lutEntry = getLUTEntry(pid, 0., 0., eta, pt);
  if (!lutEntry || !lutEntry->valid) return false;
  
  smearTrack(o2track, lutEntry);
  return true;
}
  
/*****************************************************************/

bool
TrackSmearer::smearTrack(Track &track, bool atDCA)
{

  O2Track o2track;
  TrackUtils::convertTrackToO2Track(track, o2track, atDCA);
  if (!smearTrack(o2track, track.PID)) return false;
  TrackUtils::convertO2TrackToTrack(o2track, track, atDCA);
  return true;
  
#if 0
  auto lutEntry = getLUTEntry(track.PID, 0., 0., track.Eta, track.PT);
  if (!lutEntry) return;
  
  O2Track o2track;
  TrackUtils::convertTrackToO2Track(track, o2track, atDCA);
  smearTrack(o2track, lutEntry);
  TrackUtils::convertO2TrackToTrack(o2track, track, atDCA);
#endif

}
  
/*****************************************************************/

  
} /** namespace delphes **/
} /** namespace o2 **/
