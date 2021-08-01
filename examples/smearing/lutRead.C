enum EWhat {
  kEfficiency,
  kEfficiency2,
  kEfficiencyInnerTOF,
  kEfficiencyOuterTOF,
  kPtResolution,
  kRPhiResolution,
  kZResolution
};

enum EVs {
  kNch,
  kEta,
  kPt
};

TGraph *
lutRead(int pdg, const char *filename, int what, int vs, float nch = 0., float radius = 0., float eta = 0., float pt = 0.)
{
  o2::delphes::TrackSmearer smearer;
  smearer.loadTable(pdg, filename);
  auto lutHeader = smearer.getLUTHeader(pdg);
  map_t lutMap;
  if (vs == kNch) lutMap = lutHeader->nchmap;
  if (vs == kEta) lutMap = lutHeader->etamap;
  if (vs == kPt)  lutMap = lutHeader->ptmap;
  auto nbins = lutMap.nbins;
  auto g = new TGraph();

  bool canBeInvalid = true;
  for (int i = 0; i < nbins; ++i) {
    if (vs == kNch) nch = lutMap.eval(i);
    if (vs == kEta) eta = lutMap.eval(i);
    if (vs == kPt)  pt  = lutMap.eval(i);
    auto lutEntry = smearer.getLUTEntry(pdg, nch, radius, eta , pt);
    if (!lutEntry->valid || lutEntry->eff == 0.) {
      if (!canBeInvalid) std::cout << " --- warning: it cannot be invalid \n" << std::endl;
      continue;
    }
    canBeInvalid = false;
    
    double cen = 0.;
    if (vs == kNch) cen = lutEntry->nch;
    if (vs == kEta) cen = lutEntry->eta;
    if (vs == kPt)  cen = lutEntry->pt;
    double val = 0.;
    if (what == kEfficiency)         val = lutEntry->eff * 100.; // efficiency (%)
    if (what == kEfficiency2)        val = lutEntry->eff2 * 100.; // efficiency (%)
    if (what == kEfficiencyInnerTOF) val = lutEntry->itof * 100.; // efficiency (%)
    if (what == kEfficiencyOuterTOF) val = lutEntry->otof * 100.; // efficiency (%)
    if (what == kPtResolution)   val = sqrt(lutEntry->covm[14]) * lutEntry->pt * 100.; // pt resolution (%)
    if (what == kRPhiResolution) val = sqrt(lutEntry->covm[0]) * 1.e4; // rphi resolution (um)
    if (what == kZResolution)    val = sqrt(lutEntry->covm[1]) * 1.e4; // z resolution (um)
    if (val < 0.) continue;
    g->SetPoint(g->GetN(), cen, val);
  }
  
  return g;
  
}

