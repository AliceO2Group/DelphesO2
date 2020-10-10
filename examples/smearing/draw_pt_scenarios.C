#include "style.C"
#include "lutRead_pt.C"

void
draw_pt_scenarios()
{

  style();

  std::vector<std::string> name = {"el", "pi", "ka", "pr"};
  //  std::vector<std::string> title = {"electron", "pion", "kaon", "proton"};
  std::vector<std::string> title = {"e", "#pi", "K", "p"};

  TLatex latex;
  latex.SetTextAlign(33);
  
  auto cc = new TCanvas("cc", "cc", 800, 800);
  cc->Divide(2, 2);
  
  for (int i = 0; i < 4; ++i) {
  
    auto c = new TCanvas((std::string("c") + name[i]).c_str(),
			 (std::string("c") + name[i]).c_str(),
			 800, 800);
    c->SetLogx();
    c->SetLogy();
    c->DrawFrame(1.e-2, 1., 100., 100., ";#it{p}_{T} (GeV/#it{c});momentum resolution (%)");
    latex.DrawLatexNDC(0.9, 0.9, title[i].c_str());

    auto g1 = lutRead_pt((std::string("lutCovm.") + name[i] + std::string(".5kG.20cm.scenario1.dat")).c_str());
    g1->SetLineColor(kRed+1);
    g1->SetLineStyle(kSolid);
    g1->SetLineWidth(3);
    g1->Draw("samel");
    
    auto g2 = lutRead_pt((std::string("lutCovm.") + name[i] + std::string(".5kG.20cm.scenario2.dat")).c_str());
    g2->SetLineColor(kRed+1);
    g2->SetLineStyle(kDashed);
    g2->SetLineWidth(3);
    g2->Draw("samel");

    auto g3 = lutRead_pt((std::string("lutCovm.") + name[i] + std::string(".5kG.20cm.scenario3.dat")).c_str());
    g3->SetLineColor(kAzure-3);
    g3->SetLineStyle(kSolid);
    g3->SetLineWidth(3);
    g3->Draw("samel");

    auto g4 = lutRead_pt((std::string("lutCovm.") + name[i] + std::string(".5kG.20cm.scenario4.dat")).c_str());
    g4->SetLineColor(kAzure-3);
    g4->SetLineStyle(kDashed);
    g4->SetLineWidth(3);
    g4->Draw("samel");

    c->SaveAs((std::string("draw_pt_scenarios.") + name[i] + std::string(".png")).c_str());
    
    cc->cd(i + 1);
    c->DrawClonePad();
  }

  cc->SaveAs("draw_pt_scenarios.png");

}
