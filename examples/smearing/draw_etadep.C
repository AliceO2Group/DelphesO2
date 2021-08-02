#include "style.C"
#include "lutRead_allres_vseta.C"

void
draw_etadep()
{

  style();

  //std::vector<std::string> name = {"el", "pi", "ka", "pr"};
  //  std::vector<std::string> title = {"electron", "pion", "kaon", "proton"};
  //std::vector<std::string> title = {"e", "#pi", "K", "p"};

  float ptvals[] = {1, 5, 10, 20};
  Color_t colors[] = {kRed+1, kRed+1, kRed+1, kRed+1};
  Color_t linestyles[] = {1, 2, 3, 4};

  int nPtVals = 4;

  TLatex latex;
  latex.SetTextAlign(33);
  
  auto c1 = new TCanvas("c1", "c1: pt reso", 800, 600);
  c1->SetLogy();
  c1->DrawFrame(0, 1., 4, 200., ";#eta;momentum resolution (%)");

  TLegend *leg = new TLegend(0.18,0.6,0.3,0.85);
  leg->SetBorderSize(0);
  auto c2 = new TCanvas("c2", "c2: dca xy", 800, 600);
  c2->SetLogy();
  c2->DrawFrame(0, 1., 4, 200., ";#eta;dca_{xy} resolution (#mum)");

  auto c3 = new TCanvas("c3", "c3: dca z", 800, 600);
  c3->SetLogy();
  c3->DrawFrame(0, 1., 4, 200., ";#eta;dca_{z} resolution (#mum)");

  auto c4 = new TCanvas("c4", "c4: sin phi", 800, 600);
  c4->SetLogy();
  c4->DrawFrame(0, 1e-5, 4, 1e-2, ";#eta;sin(#phi) resolution");

  auto c5 = new TCanvas("c5", "c5: tan lambda", 800, 600);
  c5->SetLogy();
  c5->DrawFrame(0, 1e-5, 4, 1e-2, ";#eta;tan(#lambda) resolution");
  
  
  for (int i = 0; i < nPtVals; ++i) {
  
    /*
    auto c = new TCanvas((std::string("c") + name[i]).c_str(),
			 (std::string("c") + name[i]).c_str(),
			 800, 800);
    c->SetLogx();
    c->SetLogy();
    c->DrawFrame(1.e-2, 1., 100., 100., ";#it{p}_{T} (GeV/#it{c});momentum resolution (%)");
    latex.DrawLatexNDC(0.9, 0.9, title[i].c_str());
    */

    //auto g2a = lutRead_pt((std::string("lutCovm.") + name[i] + std::string(".2kG.20cm.default.dat")).c_str());
    auto ptres = new TGraph();
    auto dca_xy = new TGraph();
    auto dca_z = new TGraph();
    auto sinp = new TGraph();
    auto tanl = new TGraph();
    lutRead_allres_vseta("luts/lutCovm.v12.dat",dca_xy,dca_z,sinp,tanl,ptres,ptvals[i]);
    ptres->SetLineColor(colors[i]);
    ptres->SetLineStyle(i+1);
    ptres->SetLineWidth(3);
    leg->AddEntry(ptres,Form("p_{T} = %.1f GeV",ptvals[i]));
    c1->cd();
    ptres->Draw("samel");

    dca_xy->SetLineColor(colors[i]);
    dca_xy->SetLineStyle(i+1);
    dca_xy->SetLineWidth(3);
    c2->cd();
    dca_xy->Draw("samel");

    dca_z->SetLineColor(colors[i]);
    dca_z->SetLineStyle(i+1);
    dca_z->SetLineWidth(3);
    c3->cd();
    dca_z->Draw("samel");

    sinp->SetLineColor(colors[i]);
    sinp->SetLineStyle(i+1);
    sinp->SetLineWidth(3);
    c4->cd();
    sinp->Draw("samel");

    tanl->SetLineColor(colors[i]);
    tanl->SetLineStyle(i+1);
    tanl->SetLineWidth(3);
    c5->cd();
    tanl->Draw("samel");
  }
  c1->cd();
  leg->Draw();
  c1->Print("ptres_etadep.pdf");
  c2->cd();
  leg->Draw();
  c2->Print("dcaxyres_etadep.pdf");
  c3->cd();
  leg->Draw();
  c3->Print("dcazres_etadep.pdf");
  c4->cd();
  leg->Draw();
  c4->Print("sinphires_etadep.pdf");
  c5->cd();
  leg->Draw();
  c5->Print("tanlres_etadep.pdf");
}
