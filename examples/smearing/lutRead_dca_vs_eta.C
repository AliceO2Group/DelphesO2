#include "lutCovm.hh"

TGraph* lutRead_dca_vs_eta(const char* filename = "lutCovm.dat",
                          double pt = 1.,
                          double nch = 100.)
{

  // input file
  ifstream lutFile(filename, std::ofstream::binary);

  // read header
  lutHeader_t lutHeader;
  lutFile.read(reinterpret_cast<char*>(&lutHeader), sizeof(lutHeader));
  lutHeader.print();

  // entries
  const int nnch = lutHeader.nchmap.nbins;
  const int nrad = lutHeader.radmap.nbins;
  const int neta = lutHeader.etamap.nbins;
  const int npt = lutHeader.ptmap.nbins;

  const int nch_bin = lutHeader.nchmap.find(nch);
  const int rad_bin = lutHeader.radmap.find(0.);
  const int pt_bin = lutHeader.ptmap.find(pt);

  lutEntry_t lutTable;
  // create graph of pt resolution at eta = 0
  auto gpt = new TGraph();
  gpt->SetName(filename);
  gpt->SetTitle(filename);
  gpt->GetXaxis()->SetTitle("#it{#eta}");
  gpt->GetYaxis()->SetTitle("pointing resolution (#mum)");

  // read entries
  for (int inch = 0; inch < nnch; ++inch) {
    for (int irad = 0; irad < nrad; ++irad) {
      for (int ieta = 0; ieta < neta; ++ieta) {
        for (int ipt = 0; ipt < npt; ++ipt) {
          lutFile.read(reinterpret_cast<char*>(&lutTable), sizeof(lutEntry_t));
          // lutTable.print();
          if (inch != nch_bin || irad != rad_bin || ipt != pt_bin) {
            continue;
          }
          auto lutEntry = &lutTable;
          if (!lutEntry->valid) continue;
          auto val = sqrt(lutEntry->covm[0]) * 1.e4;
          gpt->SetPoint(gpt->GetN(), lutEntry->eta, val);
        }
      }
    }
  }

  lutFile.close();

  return gpt;
}
