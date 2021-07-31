/// @author: Roberto Preghenella
/// @email: preghenella@bo.infn.it

#include "lutCovm.hh"
#include "fwdRes/fwdRes.C"

DetectorK fat;
void diagonalise(lutEntry_t &lutEntry);

bool
fatSolve(lutEntry_t &lutEntry, float pt = 0.1, float eta = 0.0, float mass = 0.13957000, int itof = 0, int otof = 0, int q = 1)
{
  lutEntry.valid = false;

  // solve track
  if (q > 1) mass = -mass;
  TrackSol tr(1, pt, eta, q, mass);
  if (!fat.SolveTrack(tr)) return false;
  AliExternalTrackParam *trPtr = (AliExternalTrackParam*)tr.fTrackCmb.At(0);
  if (!trPtr) return false;

  lutEntry.valid = true;
  lutEntry.eff = fat.GetGoodHitProb(0);
  lutEntry.itof = fat.GetGoodHitProb(itof);
  lutEntry.otof = fat.GetGoodHitProb(otof);
  for (int i = 0; i < 15; ++i) lutEntry.covm[i] = trPtr->GetCovariance()[i];
  
  return true;
}

bool
fwdSolve(float *covm, float pt = 0.1, float eta = 0.0, float mass = 0.13957000)
{
  if (fwdRes(covm, pt, eta, mass) < 0) return false;
  return true;
}

void
lutWrite(const char *filename = "lutCovm.dat", int pdg = 211, float field = 0.2, int itof = 0, int otof = 0)
{

  // output file
  ofstream lutFile(filename, std::ofstream::binary);
  if (!lutFile.is_open()) {
    Printf("Did not manage to open output file!!");
    return;
  }

  // write header
  lutHeader_t lutHeader;
  // pid
  lutHeader.pdg = pdg;
  lutHeader.mass = TDatabasePDG::Instance()->GetParticle(pdg)->Mass();
  const int q = std::abs(TDatabasePDG::Instance()->GetParticle(pdg)->Charge()) / 3;
  if (q <= 0) {
    Printf("Negative or null charge (%f) for pdg code %i. Fix the charge!", TDatabasePDG::Instance()->GetParticle(pdg)->Charge(), pdg);
    return;
  }
  lutHeader.field = field;
  // nch
  lutHeader.nchmap.log   = true;
  lutHeader.nchmap.nbins = 20;
  lutHeader.nchmap.min   = 0.5;
  lutHeader.nchmap.max   = 3.5;
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
  for (int inch = 0; inch < nnch; ++inch) {
    auto nch = lutHeader.nchmap.eval(inch);
    lutEntry.nch = nch;
    fat.SetdNdEtaCent(nch);
    std::cout << " --- setting FAT dN/deta: " << nch << std::endl;
    for (int irad = 0; irad < nrad; ++irad) {
      for (int ieta = 0; ieta < neta; ++ieta) {
	auto eta = lutHeader.etamap.eval(ieta);
	lutEntry.eta = lutHeader.etamap.eval(ieta);
	for (int ipt = 0; ipt < npt; ++ipt) {
	  lutEntry.pt = lutHeader.ptmap.eval(ipt);
	  lutEntry.valid = true;
	  if (fabs(eta) < 2.) {
            //            printf(" --- fatSolve: pt = %f, eta = %f, mass = %f, field=%f \n", lutEntry.pt, lutEntry.eta, lutHeader.mass, lutHeader.field);
	    if (!fatSolve(lutEntry, lutEntry.pt, lutEntry.eta, lutHeader.mass, itof, otof, q)) {
              //              printf(" --- [ERROR] fatSolve: pt = %f, eta = %f, mass = %f, field=%f \n", lutEntry.pt, lutEntry.eta, lutHeader.mass, lutHeader.field);
 	      lutEntry.valid = false;
              lutEntry.eff = 0.;
	      for (int i = 0; i < 15; ++i)
		lutEntry.covm[i] = 0.;
	    }
	  }
	  else {
	    //	    printf(" --- fwdSolve: pt = %f, eta = %f, mass = %f, field=%f \n", lutEntry.pt, lutEntry.eta, lutHeader.mass, lutHeader.field);
	    lutEntry.eff = 1.;
	    if (!fwdSolve(lutEntry.covm, lutEntry.pt, lutEntry.eta, lutHeader.mass)) {
	      //	      printf(" --- fwdSolve: error \n");
	      lutEntry.valid = false;
              lutEntry.eff = 0.;
	      for (int i = 0; i < 15; ++i)
		lutEntry.covm[i] = 0.;
	    }
	  }
	  diagonalise(lutEntry);
	  lutFile.write(reinterpret_cast<char *>(&lutEntry), sizeof(lutEntry_t));
	}}}}
  
	  
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
