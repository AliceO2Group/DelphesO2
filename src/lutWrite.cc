/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#include "lutCovm.hh"
#include "fwdRes/fwdRes.C"

DetectorK fat;
void diagonalise(lutEntry_t &lutEntry);

bool
fatSolve(float *eff, float *covm, float pt = 0.1, float eta = 0.0, float mass = 0.13957000)
{
  int q = 1;
  TrackSol tr(1, pt, eta, q, mass);
  bool retval = fat.SolveTrack(tr);
  if (!retval) return false;
  *eff = fat.GetEfficiency();
  AliExternalTrackParam trCopy = *((AliExternalTrackParam*)tr.fTrackCmb[0]);
  for (int i = 0; i < 15; ++i)
    covm[i] = trCopy.GetCovariance()[i];
  return true;
}

bool
fwdSolve(float *covm, float pt = 0.1, float eta = 0.0, float mass = 0.13957000)
{
  if (fwdRes(covm, pt, eta, mass) < 0) return false;
  return true;
}

void
lutWrite(const char *filename = "lutCovm.dat", int pdg = 211, float field = 0.2, float rmin = 20.)
{

  // output file
  ofstream lutFile(filename, std::ofstream::binary);

  // write header
  lutHeader_t lutHeader;
  // pid
  lutHeader.pdg = pdg;
  lutHeader.mass = TDatabasePDG::Instance()->GetParticle(pdg)->Mass();
  lutHeader.field = field;
  // nch
  lutHeader.nchmap.log   = true;
  lutHeader.nchmap.nbins = 1;
  lutHeader.nchmap.min   = 0.;
  lutHeader.nchmap.max   = 4.;
  // radius
  lutHeader.radmap.log   = false;
  lutHeader.radmap.nbins = 1;
  lutHeader.radmap.min   = 0.;
  lutHeader.radmap.max   = 100.;
  // eta
  lutHeader.etamap.log   = false;
  lutHeader.etamap.nbins = 40;
  lutHeader.etamap.min   = -2.;
  lutHeader.etamap.max   =  2.;
  // pt
  lutHeader.ptmap.log    = true;
  lutHeader.ptmap.nbins  = 200;
  lutHeader.ptmap.min    = -2;
  lutHeader.ptmap.max    = 2.;
  lutFile.write(reinterpret_cast<char *>(&lutHeader), sizeof(lutHeader));
  
  // entries
  const int nnch = lutHeader.nchmap.nbins;
  const int nrad = lutHeader.radmap.nbins;
  const int neta = lutHeader.etamap.nbins;
  const int npt = lutHeader.ptmap.nbins;
  lutEntry_t lutEntry;
  
  // write entries
  for (int inch = 0; inch < nnch; ++inch)
    for (int irad = 0; irad < nrad; ++irad)
      for (int ieta = 0; ieta < neta; ++ieta) {
	auto eta = lutHeader.etamap.eval(ieta);
	lutEntry.eta = lutHeader.etamap.eval(ieta);
	for (int ipt = 0; ipt < npt; ++ipt) {
	  lutEntry.pt = lutHeader.ptmap.eval(ipt);
	  lutEntry.valid = true;
	  if (fabs(eta) < 2.) {
	    printf(" --- fatSolve: pt = %f, eta = %f, mass = %f, field=%f \n", lutEntry.pt, lutEntry.eta, lutHeader.mass, lutHeader.field);
	    if (!fatSolve(&lutEntry.eff, lutEntry.covm, lutEntry.pt, lutEntry.eta, lutHeader.mass)) {
	      printf(" --- fatSolve: error \n");
	      lutEntry.valid = false;
	      for (int i = 0; i < 15; ++i)
		lutEntry.covm[i] = 0.;
	    }
	  }
	  else {
	    printf(" --- fwdSolve: pt = %f, eta = %f, mass = %f, field=%f \n", lutEntry.pt, lutEntry.eta, lutHeader.mass, lutHeader.field);
	    if (!fwdSolve(lutEntry.covm, lutEntry.pt, lutEntry.eta, lutHeader.mass)) {
	      printf(" --- fwdSolve: error \n");
	      lutEntry.valid = false;
	      for (int i = 0; i < 15; ++i)
		lutEntry.covm[i] = 0.;
	    }
	  }
	  diagonalise(lutEntry);
	  lutFile.write(reinterpret_cast<char *>(&lutEntry), sizeof(lutEntry_t));
	}}
  
	  
  lutFile.close();
}

void diagonalise(lutEntry_t &lutEntry)
{
  TMatrixDSym m(5);
  double fcovm[5][5];
  for (int i = 0, k = 0; i < 5; ++i)
    for (int j = 0; j < i + 1; ++j, ++k) {
      fcovm[i][j] = lutEntry.covm[k];
      fcovm[j][i] = lutEntry.covm[k];
    }  
  m.SetMatrixArray((double *)fcovm);
  TMatrixDSymEigen eigen(m);
  // eigenvalues vector
  TVectorD eigenVal = eigen.GetEigenValues();
  for (int i = 0; i < 5; ++i)
    lutEntry.eigval[i] = eigenVal[i];
  // eigenvectors matrix
  TMatrixD eigenVec = eigen.GetEigenVectors();
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 5; ++j)
      lutEntry.eigvec[i][j] = eigenVec[i][j];
  // inverse eigenvectors matrix
  eigenVec.Invert();
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 5; ++j)
      lutEntry.eiginv[i][j] = eigenVec[i][j];

}
