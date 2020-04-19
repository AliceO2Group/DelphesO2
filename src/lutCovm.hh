struct map_t {
  int nbins = 1;
  float min = 0.;
  float max = 1.e6;
  bool log = false;
  float eval(int bin) {
    float width = (max - min) / nbins;
    float val = min + (bin + 0.5) * width;
    if (log) return pow(10., val);
    return val;
  };
  int find(float val) {
    float width = (max - min) / nbins;
    int bin;
    if (log) bin = (int)((log10(val) - min) / width);
    else bin = (int)((val - min) / width);
    if (bin < 0) return 0;
    if (bin > nbins - 1) return nbins - 1;
    return bin;
  };
  void print() { printf("nbins = %d, min = %f, max = %f, log = %s \n", nbins, min, max, log ? "on" : "off"); };
};

struct lutHeader_t {
  int   pdg = 0.;
  float mass = 0.;
  float field = 0.;
  map_t nchmap;
  map_t radmap;
  map_t etamap;
  map_t ptmap;
  void print() {
    printf("    pdg: %d \n", pdg);
    printf("  field: %f \n", field);
    printf(" nchmap: "); nchmap.print();
    printf(" radmap: "); radmap.print();
    printf(" etamap: "); etamap.print();
    printf("  ptmap: "); ptmap.print();
  };
};

struct lutEntry_t {
  float eta;
  float pt;
  bool valid;
  float covm[15];
  float eigval[5];
  float eigvec[5][5];
  float eiginv[5][5];
  void print() {
    printf(" --- lutEntry: pt = %f, eta = %f (%s)\n", pt, eta, valid ? "valid" : "not valid");
    printf("     covMatix: ");
    int k = 0;
    for (int i = 0; i < 5; ++i) {
      for (int j = 0; j < i + 1; ++j)
	printf("% e ", covm[k++]);
      printf("\n               ");
    }
    printf("\n");
  }
};
  


