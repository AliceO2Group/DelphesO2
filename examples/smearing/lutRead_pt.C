#include "lutCovm.hh"

TGraph* lutRead_pt(const char* filename = "lutCovm.dat",
                   double eta = 0.,
                   double nch = 100.)
{

  // input file
  ifstream lutFile(filename, std::ofstream::binary);

  // read header
  lutHeader_t lutHeader;
  lutFile.read(reinterpret_cast<char*>(&lutHeader), sizeof(lutHeader));
  lutHeader.print();
  cout << "header done" << endl;
  // entries
  const int nnch = lutHeader.nchmap.nbins;
  const int nrad = lutHeader.radmap.nbins;
  const int neta = lutHeader.etamap.nbins;
  const int npt = lutHeader.ptmap.nbins;

  auto nchbin = lutHeader.nchmap.find(0.);
  auto radbin = lutHeader.radmap.find(0.);
  auto etabin = lutHeader.etamap.find(eta);

  lutEntry_t lutTable[npt];

  // read entries
  for (int inch = 0; inch < nnch; ++inch) {
    for (int irad = 0; irad < nrad; ++irad) {
      for (int ieta = 0; ieta < neta; ++ieta) {
        for (int ipt = 0; ipt < npt; ++ipt) {
          if (inch == nchbin && irad == radbin && ieta == etabin) {
            lutFile.read(reinterpret_cast<char*>(&lutTable[ipt]), sizeof(lutEntry_t));
            //lutTable[ipt].print();
          } else {
            lutEntry_t dummy;
            lutFile.read(reinterpret_cast<char*>(&dummy), sizeof(lutEntry_t));
          }
        }
      }
    }
  }

  lutFile.close();
  // create graph of pt resolution at eta = 0
  auto gpt = new TGraph();
  gpt->SetName(filename);
  gpt->SetTitle(filename);
  gpt->GetXaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
  gpt->GetYaxis()->SetTitle("momentum resolution (%)");
  for (int ipt = 0; ipt < npt; ++ipt) {
    auto lutEntry = &lutTable[ipt];
    if (!lutEntry->valid)
      continue;
    auto cen = lutEntry->pt;
    auto val = sqrt(lutEntry->covm[14]) * lutEntry->pt * 100.;
    gpt->SetPoint(gpt->GetN(), cen, val);
  }

  return gpt;
}
