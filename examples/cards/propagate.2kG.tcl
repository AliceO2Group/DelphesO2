set barrel_Bz 0.2
set barrel_Radius 100.e-2
set barrel_HalfLength 200.e-2
set barrel_TimeResolution 0.020e-9

set ExecutionPath {
    ParticlePropagator
    Merger
    TimeSmearing
    TreeWriter
}

# module Module Name
module ParticlePropagator ParticlePropagator {
    set InputArray Delphes/stableParticles
    set OutputArray stableParticles
    set ChargedHadronOutputArray chargedHadrons
    set ElectronOutputArray electrons
    set MuonOutputArray muons

    set Bz $barrel_Bz
    set Radius $barrel_Radius
    set HalfLength $barrel_HalfLength
}

module Merger Merger {
    add InputArray ParticlePropagator/chargedHadrons
    add InputArray ParticlePropagator/electrons
    add InputArray ParticlePropagator/muons
    set OutputArray tracks
}

module TimeSmearing TimeSmearing {
    add InputArray Merger/tracks
    add OutputArray tracks
    set TimeResolution $barrel_TimeResolution
}

module TreeWriter TreeWriter {
    # add Branch InputArray BranchName BranchClass
    add Branch Delphes/allParticles Particle GenParticle
    add Branch TimeSmearing/tracks Track Track
}

