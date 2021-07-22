#! /usr/bin/env bash

MODULES="TRK"
MODULES="CAVE PIPE ITS TPC"
NEVENTS=100
NWORKERS=4

## create the transport.C macro
cat <<EOF > transport.C
o2::data::Stack::TransportFcn
transport()
{
  return [](const TParticle& p, const std::vector<TParticle>& particles) -> bool {
           auto eta = p.Eta();
           if (std::fabs(eta) > 1.0) return false;
           auto pdg = std::abs(p.GetPdgCode());
           if (pdg == 11) return false;
           if (pdg == 13) return false;
           if (pdg == 22) return false;
           if (pdg == 211) return false;
           if (pdg == 321) return false;
           if (pdg == 2212) return false;
           return true;
	 };
}
EOF

## create the bkg_config.ini file with the required specs
cat <<EOF > config.ini
[Stack]
transportPrimary = external
transportPrimaryFileName = transport.C
transportPrimaryFuncName = transport()
transportPrimaryInvert = false
EOF

o2-sim -j ${NWORKERS} -n ${NEVENTS} -g pythia8hi -m ${MODULES} --configFile config.ini

