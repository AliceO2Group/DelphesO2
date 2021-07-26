#include "lutCovm.hh"

TGraph* lutRead_dca(const char* filename = "lutCovm.dat",
                    double eta = 0.,
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
  lutEntry_t lutTable;

  const int nch_bin = lutHeader.nchmap.find(nch);
  const int rad_bin = lutHeader.radmap.find(0.);
  const int eta_bin = lutHeader.etamap.find(eta);

  // create graph of pt resolution at eta = 0
  auto gpt = new TGraph();
  gpt->SetName(filename);
  gpt->SetTitle(filename);
  gpt->GetXaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
  gpt->GetYaxis()->SetTitle("pointing resolution (#mum)");

  // read entries
  for (int inch = 0; inch < nnch; ++inch) {
    for (int irad = 0; irad < nrad; ++irad) {
      for (int ieta = 0; ieta < neta; ++ieta) {
        for (int ipt = 0; ipt < npt; ++ipt) {
          lutFile.read(reinterpret_cast<char*>(&lutTable), sizeof(lutEntry_t));
          // lutTable.print();
          auto lutEntry = &lutTable;
          if (!lutEntry->valid) {
            std::cout << " ipt = " << ipt << " is not valid " << std::endl;
            continue;
          }
          if (nch_bin != inch) {
            continue;
          }
          if (eta_bin != ieta) {
            continue;
          }
          if (rad_bin != irad) {
            continue;
          }
          auto cen = lutEntry->pt;
          auto val = sqrt(lutEntry->covm[0]) * 1.e4;
          gpt->SetPoint(gpt->GetN(), cen, val);
        }
      }
    }
  }

  lutFile.close();

  return gpt;
}
